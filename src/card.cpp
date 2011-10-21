#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>

#include <map>

#include "asserts.hpp"
#include "card.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "game.hpp"
#include "game_formula_functions.hpp"
#include "raster.hpp"
#include "resource.hpp"
#include "string_utils.hpp"
#include "terrain.hpp"
#include "tile_logic.hpp"
#include "unit_utils.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"
#include "xml_writer.hpp"

namespace {
std::map<std::string, const_card_ptr> cards_map;
std::vector<std::string> all_cards;

void load_cards_map()
{
	if(cards_map.empty()) {
		wml::const_node_ptr cards_node = wml::parse_wml_from_file("data/cards.xml");
		FOREACH_WML_CHILD(card_node, cards_node, "spell") {
			const_card_ptr new_card(card::create(card_node));
			ASSERT_LOG(cards_map.count(new_card->id()) == 0, "CARD REPEATED: " << new_card->id());
			cards_map[new_card->id()] = new_card;
			all_cards.push_back(new_card->id());
		}

		std::sort(all_cards.begin(), all_cards.end());
	}
}
}

const std::vector<std::string>& card::get_all_cards()
{
	load_cards_map();
	return all_cards;
}

const_card_ptr card::get(const std::string& id)
{
	static std::map<std::string, const_card_ptr> ability_cards_map;

	const_card_ptr card = cards_map[id];
	if(card.get() == NULL) {
		card = ability_cards_map[id];
	}

	if(card.get() == NULL) {
		std::string::const_iterator dot = std::find(id.begin(), id.end(), '.');
		if(dot != id.end()) {
			std::string unit_id(id.begin(), dot);
			std::string ability_id(dot+1, id.end());

			wml::const_node_ptr unit_node = wml::parse_wml_from_file("data/units/" + unit_id + ".xml");
			if(unit_node.get() != NULL) {
				FOREACH_WML_CHILD(card_node, unit_node, "ability") {
					const_card_ptr new_card(create(card_node));
					ability_cards_map[unit_id + "." + card_node->attr("id").str()] = new_card;
				}
			}

			card = ability_cards_map[id];
		}
	}

	ASSERT_LOG(card.get() != NULL, "UNKNOWN CARD: '" << id << "'");

	return card;
}

card::card(wml::const_node_ptr node)
  : id_(node->attr("id")), name_(node->attr("name")),
    speed_(wml::get_int(node, "speed")),
	taps_caster_(wml::get_bool(node, "taps_caster", true))
{
    description_.reset(new std::string(node->attr("description")));

	const std::string cost_str = node->attr("cost");
	for(int n = 0; n != resource::num_resources(); ++n) {
		cost_.push_back(std::count(cost_str.begin(), cost_str.end(), resource::resource_id(n)));
	}

	game_logic::function_symbol_table* symbols = &get_game_formula_functions_symbol_table();

	for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
		if(i->first.size() > 3 && std::equal(i->first.begin(), i->first.begin() + 3, "on_")) {
			const std::string event(i->first.begin() + 3, i->first.end());
			handlers_[event] = game_logic::formula::create_optional_formula(i->second, symbols);
		}
	}

	valid_targets_ = game_logic::formula::create_optional_formula(node->attr("valid_targets"), symbols);
}

card::~card()
{}

wml::node_ptr card::use_card(card_selector& client) const
{
	if(!client.player_pay_cost(cost_)) {
		return wml::node_ptr();
	}

	wml::node_ptr result = play_card(client);
	return result;
}

wml::node_ptr card::use_card(unit* caster, int side, const std::vector<hex::location>& targets) const
{
	wml::node_ptr result = play_card(caster, side, targets);
	return result;
}

int card::cost(int nresource) const
{
	ASSERT_INDEX_INTO_VECTOR(nresource, cost_);
	return cost_[nresource];
}

bool card::calculate_valid_targets(const unit* caster, int side, std::vector<hex::location>& result) const
{
	if(!valid_targets_) {
		return false;
	}
	
	boost::intrusive_ptr<game_logic::map_formula_callable> context(new game_logic::map_formula_callable);
	context->add("game", variant(game::current()));
	variant v = valid_targets_->execute(*context);
	for(int n = 0; n != v.num_elements(); ++n) {
		variant loc = v[n];
		const location_object* obj = loc.try_convert<location_object>();
		if(obj) {
			result.push_back(obj->loc());
		} else {
			const tile_object* t = loc.try_convert<tile_object>();
			if(t) {
				result.push_back(t->get_tile().loc());
			}
		}
	}

	return true;
}

void card::resolve_card(const resolve_card_info* callable) const
{
	variant holder(callable);
	handle_event("resolve", callable);
}

void card::handle_event(const std::string& name,
                        const game_logic::formula_callable* callable) const
{
	const handlers_map::const_iterator itor = handlers_.find(name);
	if(itor == handlers_.end()) {
		return;
	}

	boost::intrusive_ptr<game_logic::map_formula_callable> context(new game_logic::map_formula_callable(callable));
	context->add("game", variant(game::current()));

	const variant v = itor->second->execute(*context);
	game::current()->execute_command(v, NULL);
}

namespace {

class dummy_card : public card {
public:
	explicit dummy_card(wml::const_node_ptr node)
	  : card(node) {}

	wml::node_ptr play_card(card_selector& client) const
	{
		return wml::node_ptr();
	}

	virtual wml::node_ptr play_card(unit* caster, int side, const std::vector<hex::location>& targets) const {
		return wml::node_ptr();
	}

	virtual bool is_card_playable(unit* caster, int side, const std::vector<hex::location>& targets, std::vector<hex::location>& possible_targets) const {
		return false;
	}
};

namespace {
bool is_void(const game* g, const hex::location loc) {
	const tile* t = g->get_tile(loc.x(), loc.y());
	return t && t->terrain()->id() == "void";
}

bool is_loc_in_vector(const hex::location loc, const std::vector<hex::location>& v) {
	return std::find(v.begin(), v.end(), loc) != v.end();
}

bool is_loc_in_sorted_vector(const hex::location& loc, const std::vector<hex::location>& v) {
	return std::binary_search(v.begin(), v.end(), loc);
}
}

namespace {
bool true_loc(const hex::location& loc) { return true; }

}

class attack_card : public card {
public:
	explicit attack_card(wml::const_node_ptr node);
	virtual ~attack_card() {}
	virtual wml::node_ptr play_card(card_selector& client) const;
	virtual wml::node_ptr play_card(unit* caster, int side, const std::vector<hex::location>& targets) const;
	virtual bool is_card_playable(unit* caster, int side, const std::vector<hex::location>& targets, std::vector<hex::location>& possible_targets) const;
	virtual void resolve_card(const resolve_card_info* callable=NULL) const;
	virtual bool calculate_valid_targets(const unit* caster, int side, std::vector<hex::location>& result) const;
	int damage() const { return damage_; }

	bool is_attack() const { return true; }
private:
	int damage_, range_;

};

attack_card::attack_card(wml::const_node_ptr node)
  : card(node), damage_(wml::get_int(node, "damage")),
                range_(wml::get_int(node, "range"))
{
}

wml::node_ptr attack_card::play_card(card_selector& client) const
{
	wml::node_ptr result(new wml::node("play"));
	result->set_attr("spell", id());

	const hex::location loc =
	    client.player_select_loc("Select target for attack", true_loc);
	if(!loc.valid()) {
		return wml::node_ptr();
	}

	result->add_child(hex::write_location("target", loc));

	return result;
}

wml::node_ptr attack_card::play_card(unit* caster, int side, const std::vector<hex::location>& targets) const
{
	return wml::node_ptr();
}

bool attack_card::is_card_playable(unit* caster, int side, const std::vector<hex::location>& targets, std::vector<hex::location>& possible_targets) const
{
	if(caster == NULL || targets.size() > 1) {
		return false;
	}

	std::vector<hex::location> valid_targets;
	calculate_valid_targets(caster, side, valid_targets);

	if(targets.size() == 1) {
		return std::count(valid_targets.begin(), valid_targets.end(), targets.front()) != 0;
	}

	possible_targets = valid_targets;
	return false;
}

namespace {

class attack_card_callable : public game_logic::formula_callable
{
public:
	explicit attack_card_callable(int dmg, int def) : damage_(dmg), defense_(def)
	{}

	variant get_value(const std::string& key) const {
		if(key == "damage") {
			return variant(damage_);
		} else if(key == "defense") {
			return variant(defense_);
		} else {
			return variant();
		}
	}

	void set_value(const std::string& key, const variant& value) {
		if(key == "damage") {
			damage_ = value.as_int();
		} else if(key == "defense") {
			defense_ = value.as_int();
		}
	}

	int damage() const { return damage_; }
	int defense() const { return defense_; }

private:
	int damage_;
	int defense_;
};

}

void attack_card::resolve_card(const resolve_card_info* callable) const
{
	card::resolve_card(callable);

	if(callable) {
		using namespace game_logic;
		map_formula_callable_ptr map_callable(new map_formula_callable(callable));

		const std::vector<hex::location> targets = callable->targets();

		//if this is a regular attack (not a 'free' attack) we activate
		//the attacked event.
		if(callable->activation_type() == CARD_ACTIVATION_PLAYER) {
			foreach(const hex::location& target, targets) {
				unit_ptr u = game::current()->get_unit_at(target);
				if(u.get() != NULL) {
					u->handle_event("attacked", callable);
				}
			}
		}

		game::current()->do_state_based_actions();

		if(std::count(game::current()->units().begin(), game::current()->units().end(), callable->caster()) == 0) {
			//the attack is countered because the attacking unit is no longer
			//on the battlefield.
			return;
		}

		foreach(const hex::location& target, targets) {
			unit_ptr u = game::current()->get_unit_at(target);
			if(u.get() != NULL) {

				wml::node_ptr attack_anim_node(new wml::node("attack_anim"));
				attack_anim_node->add_child(hex::write_location("from", callable->caster()->loc()));
				attack_anim_node->add_child(hex::write_location("to", target));
				game::current()->queue_message(wml::output_xml(attack_anim_node));

				const int defense = unit_protection(*game::current(), u);

				attack_card_callable* attack_callable = new attack_card_callable(damage_, defense);
				variant attack_callable_var(attack_callable);
				map_callable->add("attack", attack_callable_var);

				handle_event("attack", map_callable.get());


				int final_damage = attack_callable->damage() - attack_callable->defense();
				if(final_damage <= 0) {
					if(attack_callable->damage() > 0) {
						final_damage = 1;
					} else {
						final_damage = 0;
					}
				}

				u->take_damage(final_damage);
			}
		}
	}
}

bool attack_card::calculate_valid_targets(const unit* caster, int side, std::vector<hex::location>& result) const
{
	if(caster != NULL && range_ >= 0) {
		std::vector<hex::location> v;
		get_tiles_in_radius(caster->loc(), range_, v);
		foreach(const hex::location& loc, v) {
			if(loc != caster->loc() && game::current()->get_unit_at(loc).get() != NULL) {
				result.push_back(loc);
			}
		}
		return true;
	}

	return false;
}

class modification_card : public card {
public:
	explicit modification_card(wml::const_node_ptr node);
	virtual ~modification_card() {}
	virtual wml::node_ptr play_card(card_selector& client) const;
	virtual wml::node_ptr play_card(unit* caster, int side, const std::vector<hex::location>& targets) const;
	virtual bool is_card_playable(unit* caster, int side, const std::vector<hex::location>& targets, std::vector<hex::location>& possible_targets) const;
	const unit::modification* modification() const { return &mod_; }
	bool calculate_valid_targets(const unit* caster, int side, std::vector<hex::location>& result) const;
private:
	int damage_;
	int ntargets_;
	int range_;
	unit::modification mod_;

};

modification_card::modification_card(wml::const_node_ptr node)
  : card(node),
    damage_(wml::get_int(node, "damage")),
    ntargets_(wml::get_int(node, "targets")),
	range_(wml::get_int(node, "range", -1)),
	mod_(node)
{
}

bool modification_card::calculate_valid_targets(const unit* caster, int side, std::vector<hex::location>& valid_targets) const
{
	if(range_ > 0) {
		foreach(const_unit_ptr u, game::current()->units()) {
			if(u->side() == side) {
				bool can_cast = true;
				for(int n = 0; n != resource::num_resources(); ++n) {
					if(cost(n) && !u->can_cast(resource::resource_id(n))) {
						can_cast = false;
						break;
					}
				}

				if(can_cast) {
					hex::get_tiles_in_radius(u->loc(), range_, valid_targets);
				}
			}
		}
	}

	return true;
}

wml::node_ptr modification_card::play_card(card_selector& client) const
{
	wml::node_ptr result(new wml::node("play"));
	result->set_attr("spell", id());

	boost::function<bool(const hex::location)> valid_target_fn = true_loc;

	std::vector<hex::location> valid_targets;
	if(range_ > 0) {
		foreach(const_unit_ptr u, client.get_game().units()) {
			if(u->side() == client.player_id()) {
				bool can_cast = true;
				for(int n = 0; n != resource::num_resources(); ++n) {
					if(cost(n) && !u->can_cast(resource::resource_id(n))) {
						can_cast = false;
						break;
					}
				}

				if(can_cast) {
					hex::get_tiles_in_radius(u->loc(), range_, valid_targets);
				}
			}
		}

		std::sort(valid_targets.begin(), valid_targets.end());
		valid_targets.erase(
		       std::unique(valid_targets.begin(),
		                   valid_targets.end()), valid_targets.end());

		valid_target_fn = boost::bind(is_loc_in_sorted_vector, _1, valid_targets);
	}


	for(int n = 0; n < ntargets_; ++n) {
		const hex::location loc =
		    client.player_select_loc("Select target for spell", valid_target_fn);
		if(!loc.valid()) {
			return wml::node_ptr();
		}

		result->add_child(hex::write_location("target", loc));
	}

	return result;
}

wml::node_ptr modification_card::play_card(unit* caster, int side, const std::vector<hex::location>& targets) const
{
	wml::node_ptr result(new wml::node("play"));
	result->set_attr("spell", id());
	
	foreach(const hex::location& target, targets) {
		result->add_child(hex::write_location("target", target));
	}

	return result;
}

bool modification_card::is_card_playable(unit* caster, int side, const std::vector<hex::location>& targets, std::vector<hex::location>& possible_targets) const
{
	std::vector<hex::location> valid_targets;
	calculate_valid_targets(caster, side, valid_targets);

	foreach(const hex::location& target, targets) {
		if(!std::count(valid_targets.begin(), valid_targets.end(), target)) {
			return false;
		}
	}

	if(targets.size() == ntargets_) {
		return true;
	}

	if(targets.size() > ntargets_) {
		return false;
	}

	possible_targets = valid_targets;
	return false;
}

class monster_card : public card {
public:
	explicit monster_card(wml::const_node_ptr node);
	virtual ~monster_card() {}
	virtual wml::node_ptr play_card(card_selector& client) const;
	virtual wml::node_ptr play_card(unit* caster, int side, const std::vector<hex::location>& targets) const;
	bool calculate_valid_targets(const unit* caster, int side, std::vector<hex::location>& result) const;
	virtual bool is_card_playable(unit* caster, int side, const std::vector<hex::location>& targets, std::vector<hex::location>& possible_targets) const;
	const std::string* monster_id() const { return &monster_; }
private:
	std::string monster_;
};

monster_card::monster_card(wml::const_node_ptr node)
  : card(node), monster_(node->attr("monster"))
{}

wml::node_ptr monster_card::play_card(card_selector& client) const
{
	std::vector<hex::location> targets;
	const bool targets_valid = calculate_valid_targets(NULL, client.player_id(), targets);

	boost::function<bool(const hex::location)> valid_target_fn;
	if(targets_valid) {
		valid_target_fn = boost::bind(is_loc_in_vector, _1, targets);
	} else {
		const_unit_ptr proto = unit::get_prototype(monster_);
		ASSERT_LOG(proto.get() != NULL, "INVALID MONSTER: " << monster_);
		valid_target_fn = boost::bind(is_valid_summoning_hex, &client.get_game(), client.player_id(), _1, proto);
	}

	hex::location loc =
	  client.player_select_loc("Select where to place your monster.",
	                           valid_target_fn);
	if(!loc.valid()) {
		return wml::node_ptr();
	}

	wml::node_ptr result(new wml::node("play"));
	result->set_attr("spell", id());
	wml::node_ptr monster_node(hex::write_location("monster", loc));
	monster_node->set_attr("type", monster_);
	result->add_child(monster_node);

	return result;
}

wml::node_ptr monster_card::play_card(unit* caster, int side, const std::vector<hex::location>& targets) const
{
	if(targets.size() != 1) {
		return wml::node_ptr();
	}

	wml::node_ptr result(new wml::node("play"));
	result->set_attr("spell", id());
	wml::node_ptr monster_node(hex::write_location("monster", targets.front()));
	monster_node->set_attr("type", monster_);
	result->add_child(monster_node);
	return result;
}

bool monster_card::is_card_playable(unit* caster, int side, const std::vector<hex::location>& targets, std::vector<hex::location>& possible_targets) const
{
	const int slots_available = get_player_unit_limit(side) - get_player_unit_limit_slots_used(side);

	const_unit_ptr proto = unit::get_prototype(monster_);
	if(proto->maintenance_cost() > slots_available) {
		return false;
	}

	std::vector<hex::location> valid_targets;
	const bool targets_valid = calculate_valid_targets(NULL, side, valid_targets);
	if(!targets_valid) {
		return false;
	}

	if(targets.empty()) {
		possible_targets = valid_targets;
		return false;
	} else if(targets.size() == 1) {
		return std::count(valid_targets.begin(), valid_targets.end(), targets.front()) != 0;
	} else {
		return false;
	}
}

bool monster_card::calculate_valid_targets(const unit* caster, int side, std::vector<hex::location>& result) const
{
	if(card::calculate_valid_targets(caster, side, result)) {
		return true;
	}

	const_unit_ptr proto = unit::get_prototype(monster_);
	for(int x = 0; x != game::current()->width(); ++x) {
		for(int y = 0; y != game::current()->height(); ++y) {
			hex::location loc(x, y);
			if(is_valid_summoning_hex(game::current(), side, loc, proto)) {
				result.push_back(loc);
			}
		}
	}

	return true;
}

}

const_card_ptr card::create(wml::const_node_ptr node)
{
	const std::string& type = node->attr("type");
	if(type == "monster") {
		return const_card_ptr(new monster_card(node));
	} else if(type == "modification") {
		return const_card_ptr(new modification_card(node));
	} else if(type == "attack") {
		return const_card_ptr(new attack_card(node));
	} else if(type == "dummy") {
		return const_card_ptr(new dummy_card(node));
	} else {
		ASSERT_LOG(false, "UNRECOGNIZED CARD TYPE '" << type << "'");
	}
}

std::vector<held_card> read_deck(const std::string& str)
{
	std::vector<held_card> result;
	std::vector<std::string> items = util::split(str);
	foreach(const std::string& item, items) {
		std::vector<std::string> v = util::split(item, ' ');
		ASSERT_LOG(v.size() == 1 || v.size() == 2, "ILLEGAL DECK FORMAT: " << str);
		int embargo = 0;
		if(v.size() > 1) {
			embargo = atoi(v[1].c_str());
		}

		const_card_ptr card_obj = card::get(v.front());
		ASSERT_LOG(card_obj.get() != NULL, "UNKNOWN CARD: '" << v.front() << "'");
		held_card res = { card_obj, embargo };
		result.push_back(res);
	}

	return result;
}

std::string write_deck(const std::vector<held_card>& cards)
{
	std::string result;
	for(int n = 0; n != cards.size(); ++n) {
		if(n != 0) {
			result += ",";
		}

		result += cards[n].card->id();
		if(cards[n].embargo) {
			result += (formatter() << " " << cards[n].embargo);
		}
	}

	return result;
}

bool is_valid_summoning_hex(const game* g, int player, const hex::location& loc, const_unit_ptr proto) {
	const tile* t = g->get_tile(loc.x(), loc.y());
	if(!t || t->terrain()->id() == "void" || g->get_unit_at(loc).get() != NULL) {
		return false;
	}

	if(t->terrain()->id() == "tower" && g->tower_owner(loc) != player) {
		return false;
	}

	foreach(const_unit_ptr u, g->units()) {
		if(u->side() == player && hex::tiles_adjacent(u->loc(), loc)) {
			bool can_summon = true;
			foreach(char resource_type, proto->upkeep()) {
				if(!u->can_summon(resource_type)) {
					can_summon = false;
				}
			}

			if(can_summon) {
				return true;
			}
		}
	}
	
	return false;
}
