#include <algorithm>
#include <string>

#include "ai_player.hpp"
#include "asserts.hpp"
#include "card.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula_callable.hpp"
#include "game.hpp"
#include "game_formula_functions.hpp"
#include "game_utils.hpp"
#include "pathfind.hpp"
#include "resource.hpp"
#include "string_utils.hpp"
#include "terrain.hpp"
#include "unit_utils.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"
#include "xml_writer.hpp"

namespace {
game* current_game = NULL;

hex::location parse_loc_from_xml(const TiXmlElement& el)
{
	int x = 0, y = 0;
	el.QueryIntAttribute("x", &x);
	el.QueryIntAttribute("y", &y);
	return hex::location(x, y);
}

int xml_int(const TiXmlElement& el, const char* s)
{
	int res = 0;
	el.QueryIntAttribute(s, &res);
	return res;
}

std::string xml_str(const TiXmlElement& el, const char* s)
{
	const char* attr = el.Attribute(s);
	if(!attr) {
		return "";
	}

	return attr;
}
}

game_context::game_context(game* g) : old_game_(current_game)
{
	current_game = g;
}

game_context::~game_context()
{
	current_game = old_game_;
}

void game_context::set(game* g)
{
	current_game = g;
}

game* game::current()
{
	return current_game;
}

game::game()
  : started_(false), width_(0), height_(0), state_(STATE_SETUP), player_turn_(-1), player_casting_(-1), spell_casting_passes_(0), done_main_phase_(false)
{
}

game::game(wml::const_node_ptr node)
  : started_(wml::get_bool(node, "started")),
    width_(wml::get_int(node, "width")),
    height_(wml::get_int(node, "height")),
	state_(STATE_SETUP),
	player_turn_(wml::get_int(node, "player_turn", -1)),
	player_casting_(wml::get_int(node, "player_casting", -1)),
	spell_casting_passes_(0), done_main_phase_(false)
{
	std::cerr << "GAME: " << wml::output(node) << "\n";
	std::vector<std::string> v = util::split(node->attr("tiles"));
	for(int n = 0; n != v.size(); ++n) {
		const int x = n%width_;
		const int y = n/width_;
		tiles_.push_back(tile(x, y, v[n]));
	}

	ASSERT_EQ(width_*height_, tiles_.size());

	FOREACH_WML_CHILD(city_node, node, "city") {
		cities_.push_back(city_ptr(new city(city_node)));
	}

	FOREACH_WML_CHILD(player_node, node, "player") {
		players_.push_back(player());
		players_.back().name = player_node->attr("name");
		players_.back().spells = read_deck(player_node->attr("spells"));

		players_.back().resources.resize(resource::num_resources());
		int num_resources = resource::num_resources();
		util::split_into_ints(player_node->attr("resources").str().c_str(), &players_.back().resources[0], &num_resources);

		num_resources = resource::num_resources();
		players_.back().resource_gain.resize(resource::num_resources());
		util::split_into_ints(player_node->attr("resource_gain").str().c_str(), &players_.back().resource_gain[0], &num_resources);


		FOREACH_WML_CHILD(tower_node, player_node, "tower") {
			std::string resource = tower_node->attr("resource");
			ASSERT_LOG(resource.size() == 1, "BAD RESOURCE FOR TOWER: " << resource);
			players_.back().towers[hex::location(wml::get_int(tower_node, "x"), wml::get_int(tower_node, "y"))] = resource[0];
		}
	}

	FOREACH_WML_CHILD(neutral_node, node, "neutral") {
		neutral_towers_.insert(hex::location(wml::get_int(neutral_node, "x"), wml::get_int(neutral_node, "y")));
	}

	FOREACH_WML_CHILD(unit_node, node, "unit") {
		units_.push_back(unit_ptr(new unit(unit_node)));
	}
}

wml::node_ptr game::write() const
{
	wml::node_ptr res(new wml::node("game"));
	res->set_attr("started", started_ ? "yes" : "no");
	res->set_attr("width", formatter() << width_);
	res->set_attr("height", formatter() << height_);
	res->set_attr("player_turn", formatter() << player_turn_);
	res->set_attr("player_casting", formatter() << player_casting_);
	std::string tiles;
	for(int n = 0; n != tiles_.size(); ++n) {
		if(n != 0) {
			tiles += ",";
		}

		tiles_[n].write(tiles);
	}

	res->set_attr("tiles", tiles);

	foreach(city_ptr c, cities_) {
		res->add_child(c->write());
	}

	foreach(const hex::location& loc, neutral_towers_) {
		res->add_child(hex::write_location("neutral", loc));
	}

	foreach(unit_ptr u, units_) {
		res->add_child(u->write());
	}

	int nplayer = 0;
	foreach(const player& p, players_) {
		wml::node_ptr player_node(new wml::node("player"));
		player_node->set_attr("name", p.name);
		player_node->set_attr("unit_limit", formatter() << get_player_unit_limit_slots_used(nplayer) << "/" << get_player_unit_limit(nplayer));


		std::ostringstream stream;
		foreach(const held_card& c, p.spells) {
			bool usable = player_casting_ == nplayer && c.embargo == 0;
			if(usable) {
				std::vector<hex::location> targets, possible_targets;
				const bool playable = c.card->is_card_playable(NULL, nplayer, targets, possible_targets);
				usable = playable || !possible_targets.empty();
			}
			stream << c.card->id() << " " << c.embargo << " " << (usable ? "1" : "0") << ",";
		}

		std::string spells_str = stream.str();
		if(spells_str.empty() == false) {
			//cut off the last comma
			spells_str.resize(spells_str.size()-1);
		}

		player_node->set_attr("spells", spells_str);

		if(!p.resources.empty()) {
			player_node->set_attr("resources", util::join_ints(&p.resources[0], p.resources.size()));
			player_node->set_attr("resource_gain", util::join_ints(&p.resource_gain[0], p.resource_gain.size()));
		}

		for(std::map<hex::location, char>::const_iterator i = p.towers.begin();
		    i != p.towers.end(); ++i) {
			wml::node_ptr node = hex::write_location("tower", i->first);
			node->set_attr("resource", formatter() << i->second);
			player_node->add_child(node);
		}

		res->add_child(player_node);
		++nplayer;
	}

	return res;
}

namespace {

std::string write_steps(const hex::route& rt)
{
	std::ostringstream s;
	bool first = true;
	foreach(const hex::location& step, rt.steps) {
		if(!first) {
			s << ",";
		}

		first = false;
		s << step.x() << "," << step.y();
	}

	return s.str();
}

void write_route_map(const hex::route_map& m, wml::node_ptr result) {
	for(std::map<hex::location, hex::route>::const_iterator i = m.begin();
	    i != m.end(); ++i) {
		wml::node_ptr node = hex::write_location("route", i->first);

		node->set_attr("steps", write_steps(i->second));

		result->add_child(node);
	}
}
}

void game::handle_message(int nplayer, const TiXmlElement& msg)
{
	const std::string type = msg.Value();
	if(type == "commands") {
		for(const TiXmlElement* t = msg.FirstChildElement(); t != NULL; t = t->NextSiblingElement()) {
			handle_message(nplayer, *t);
		}
	} else if(type == "setup") {
		setup_game();
		queue_message(wml::output_xml(write()));

		ASSERT_GE(players_.size(), 1);
		queue_message(formatter() << "<new_turn player=\"" << players_.front().name << "\"></new_turn>\n");
		state_ = STATE_PLAYING;
		player_casting_ = player_turn_ = 0;
	} else if(type == "spells") {
		std::cerr << "READ DECK FOR " << nplayer << "\n";
		ASSERT_INDEX_INTO_VECTOR(nplayer, players_);
		players_[nplayer].spells = read_deck(msg.Attribute("spells"));

		players_[nplayer].resource_gain.resize(resource::num_resources());
		int num_resources = resource::num_resources();
		util::split_into_ints(msg.Attribute("resource_gain"), &players_[nplayer].resource_gain[0], &num_resources);

		players_[nplayer].resources = players_[nplayer].resource_gain;

		draw_hand(nplayer);
		queue_message(wml::output_xml(write()));
	} else if(type == "select_unit") {
		const hex::location loc(parse_loc_from_xml(msg));
		unit_ptr u = get_unit_at(loc);
		if(u && u->side() == nplayer && player_casting_ == nplayer) {
			if(u->has_moved() == false) {
				hex::route_map routes;
				hex::find_possible_moves(u->loc(), u->move(), unit_movement_cost_calculator(this, u), routes);

				wml::node_ptr node(hex::write_location("select_unit_move", loc));
				write_route_map(routes, node);
				queue_message(wml::output_xml(node), nplayer);
			}
		}

	} else if(type == "move") {
		const TiXmlElement* from = msg.FirstChildElement("from");
		const TiXmlElement* to = msg.FirstChildElement("to");
		const TiXmlElement* query_abilities = msg.FirstChildElement("query_abilities");
		ASSERT_LOG(from && to, "message format of move illegal");
		const hex::location from_loc(parse_loc_from_xml(*from));
		const hex::location to_loc(parse_loc_from_xml(*to));

		unit_ptr u = get_unit_at(from_loc);
		ASSERT_LOG(u.get(), "Could not find unit at from loc");

		hex::route_map routes;
		hex::find_possible_moves(u->loc(), u->move(), unit_movement_cost_calculator(this, u), routes);

		if(routes.count(to_loc) == 0) {
			return;
		}

		wml::node_ptr anim_node(new wml::node("move_anim"));
		anim_node->set_attr("steps", write_steps(routes[to_loc]));
		anim_node->add_child(hex::write_location("from", from_loc));
		anim_node->add_child(hex::write_location("to", to_loc));
		queue_message(wml::output_xml(anim_node));

		u->set_loc(to_loc);
		u->set_moved();

		if(!u->scout()) {
			capture_tower(to_loc, u);
		}

		do_state_based_actions();

		//every other unit gets a message telling it about this unit moving.
		{
		game_logic::map_formula_callable* callable = new game_logic::map_formula_callable;
		variant holder(callable);
		callable->add("move_from", variant(new location_object(from_loc)));
		callable->add("move_to", variant(new location_object(to_loc)));
		callable->add("moving_unit", variant(u.get()));
		foreach(unit_ptr other_unit, units_) {
			if(other_unit != u) {
				other_unit->handle_event("other_unit_move", callable);
			}
		}
		}


		std::vector<unit_ability_ptr> playable_abilities;
		foreach(unit_ability_ptr ability, u->abilities()) {
			card_ptr spell(card::get(ability->spell()));
			if(!spell) {
				continue;
			}

			std::vector<hex::location> targets, possible_targets;
			const bool playable = spell->is_card_playable(u.get(), u->side(), targets, possible_targets) || possible_targets.empty() == false;
			if(playable) {
				playable_abilities.push_back(ability);
			}
		}

		//if the attribute has hold_turn it means the client will just
		//continue to send instructions. Otherwise it expects the server to
		//take appropriate action.
		if(!msg.Attribute("hold_turn")) {
			if(playable_abilities.empty()) {
				end_turn(u->side(), false);
			} else {
				queue_message(wml::output_xml(write()));
	
				if(query_abilities != NULL) {
					std::ostringstream s;
					s << "<choose_ability unit=\"" << u->key() << "\"  abilities=\"";
					for(int n = 0; n != playable_abilities.size(); ++n) {
						if(n != 0) {
							s << ",";
						}

						s << playable_abilities[n]->id();
					}

					s << "\"/>";
					queue_message(s.str(), u->side());
				}
			}
		}

	} else if(type == "play") {
		if(play_card(nplayer, msg)) {
			spell_casting_passes_ = 0;
			end_turn(nplayer, false);
		}
	} else if(type == "end_turn") {
		end_turn(nplayer, xml_str(msg, "skip") == "yes");
	}
}

void game::end_turn(int nplayer, bool skipping)
{
	if(skipping) {
		++spell_casting_passes_;
	} else {
		spell_casting_passes_ = 0;
	}

	ASSERT_EQ(nplayer, player_casting_);

	player_casting_ = player_casting_+1;
	if(player_casting_ == players_.size()) {
		player_casting_ = 0;
	}

	//make sure any units die that are meant to.
	do_state_based_actions();

	std::cerr << "SPELLS: " << spell_casting_passes_ << " / " << players_.size() << "\n";

	if(spell_casting_passes_ >= players_.size()) {
		spell_casting_passes_ = 0;

		int last_delay = -1;

		//make sure any units die that are meant to.
		do_state_based_actions();

		ASSERT_INDEX_INTO_VECTOR(nplayer, players_);

		do_state_based_actions();

		for(int n = 0; n != players_.size(); ++n) {
			draw_hand(n);

			foreach(player& p, players_) {
				foreach(held_card& spell, p.spells) {
					if(spell.embargo > 0) {
						--spell.embargo;
					}
				}
			}

			std::vector<int>& resources = players_[n].resources;
			resources.resize(resource::num_resources());
			players_[n].resource_gain.resize(resource::num_resources());

			ASSERT_EQ(resources.size(), players_[n].resource_gain.size());

			std::vector<int> upkeep(resources.size());
			foreach(unit_ptr u, units_) {
				if(u->side() != n) {
					continue;
				}

				foreach(char c, u->upkeep()) {
					upkeep[resource::resource_index(c)]++;
				}
			}

			for(std::map<hex::location, char>::const_iterator i = players_[n].towers.begin(); i != players_[n].towers.end(); ++i) {
				const int r = resource::resource_index(i->second);
				if(upkeep[r] > 0) {
					--upkeep[r];
				}
			}

			for(int m = 0; m != resource::num_resources(); ++m) {
				resources[m] += players_[n].resource_gain[m] - upkeep[m];
				if(resources[m] < 0) {
					resources[m] = 0;
				}
				std::cerr << "PLAYER " << n << " RESOURCE " << m << ": " << resources[m] << "\n";
			}
		}

		++player_turn_;
		if(player_turn_ == players_.size()) {
			player_turn_ = 0;
		}

		player_casting_ = player_turn_;

		foreach(unit_ptr u, units_) {
			u->new_turn();
		}

		spell_casting_passes_ = 0;
	}
	
	queue_message(wml::output_xml(write()));
	queue_message(formatter() << "<new_turn player=\"" << players_[player_casting_].name << "\"></new_turn>\n");

	foreach(boost::shared_ptr<ai_player> ai, ai_) {
		std::vector<wml::node_ptr> nodes = ai->play();

		while(nodes.empty() == false) {
			foreach(wml::node_ptr node, nodes) {
				std::string msg(wml::output_xml(node));
				std::cerr << "AI MESSAGE: " << msg << "\n";

				TiXmlDocument xml_doc;
				xml_doc.Parse(msg.c_str());

				TiXmlElement* doc = xml_doc.RootElement();

				handle_message(ai->player_id(), *doc);
			}

			nodes = ai->play();
		}
	}
}

bool game::play_card(int nplayer, const TiXmlElement& msg, int speed)
{
	const std::string type = xml_str(msg, "spell");

	const_card_ptr card = card::get(type);
	ASSERT_LOG(card.get() != NULL, "CARD " << type << " NOT FOUND " << nplayer);

	ASSERT_INDEX_INTO_VECTOR(nplayer, players_);
	player& p = players_[nplayer];

	unit_ptr caster;
	if(msg.Attribute("caster")) {
		const int caster_key = xml_int(msg, "caster");
		foreach(unit_ptr u, units_) {
			if(u->key() == caster_key) {
				caster = u;
			}
		}

		if(!caster) {
			return false;
		}
	}

	std::vector<hex::location> targets, possible_targets;
	for(const TiXmlElement* target = msg.FirstChildElement("target"); target != NULL; target = target->NextSiblingElement("target")) {
		hex::location loc(parse_loc_from_xml(*target));
		targets.push_back(loc);
	}

	std::cerr << "CHECKING CARD PLAYABILITY...\n";

	if(!card->is_card_playable(caster.get(), nplayer, targets, possible_targets)) {
		std::cerr << "NOT PLAYABLE..\n";
		std::ostringstream msg;
		msg << "<illegal_cast legal_targets=\"";
		for(int n = 0; n != possible_targets.size(); ++n) {
			if(n != 0) {
				msg << ",";
			}

			msg << possible_targets[n].x() << "," << possible_targets[n].y();
		}

		msg << "\"/>";

		queue_message(msg.str(), nplayer);
		return false;
	}

	std::cerr << "PLAYABLE..\n";

	if(caster) {
		if(card->taps_caster()) {
			caster->set_moved();
		}
	} else {
		foreach(held_card& spell, p.spells) {
			if(spell.card->id() == type) {
				spell.embargo = 1 + rand()%3;
			}
		}
	}

	const std::vector<int>& resource_cost = card->cost();
	if(resource_cost.size() == resource::num_resources()) {
		for(int n = 0; n != resource::num_resources(); ++n) {
			if(p.resources.size() > n) {
				p.resources[n] -= resource_cost[n];
			}
		}
	}

	resolve_card(nplayer, card, msg);
	do_state_based_actions();

	queue_message(wml::output_xml(write()));
	return true;
}

namespace {
class card_info_callable : public resolve_card_info
{
	game& g_;
	const TiXmlElement& msg_;

	variant get_value(const std::string& key) const {
		if(key == "caster") {
			unit_ptr u = caster();
			if(u.get() != NULL) {
				return variant(u.get());
			} else {
				return variant();
			}
		} else if(key == "target") {
			const TiXmlElement* t = msg_.FirstChildElement("target");
			if(t) {
				return variant(new location_object(parse_loc_from_xml(*t)));
			}
		} else if(key == "targets") {
			std::vector<variant> v;
			for(const TiXmlElement* t = msg_.FirstChildElement("target"); t != NULL; t = t->NextSiblingElement("target")) {
				v.push_back(variant(new location_object(parse_loc_from_xml(*t))));
			}

			return variant(&v);
		} else if(key == "game") {
			return variant(&g_);
		}

		return variant();
	}

	std::vector<hex::location> targets() const
	{
		std::vector<hex::location> v;
		for(const TiXmlElement* t = msg_.FirstChildElement("target"); t != NULL; t = t->NextSiblingElement("target")) {
			v.push_back(parse_loc_from_xml(*t));
		}

		return v;
	}

	unit_ptr caster() const
	{
		const int caster_key = xml_int(msg_, "caster");
		foreach(unit_ptr u, g_.units()) {
			if(u->key() == caster_key) {
				return u;
			}
		}

		return unit_ptr();
	}

public:
	card_info_callable(game& g, const TiXmlElement& msg) : g_(g), msg_(msg)
	{}
};

}

void game::resolve_card(int nplayer, const_card_ptr card, const TiXmlElement& msg)
{
	card_info_callable* callable_context = new card_info_callable(*this, msg);
	const variant context_holder(callable_context);
	card->resolve_card(callable_context);

	std::cerr << "RESOLVING CARD...\n";

	for(const TiXmlElement* monster = msg.FirstChildElement("monster"); monster != NULL; monster = monster->NextSiblingElement("monster")) {
		hex::location loc(parse_loc_from_xml(*monster));

		units_.push_back(unit::build_from_prototype(monster->Attribute("type"), nplayer, loc));
		units_.back()->set_moved();
		units_.back()->handle_event("summoned");
	}

	for(const TiXmlElement* target = msg.FirstChildElement("target"); target != NULL; target = target->NextSiblingElement("target")) {
		hex::location loc(parse_loc_from_xml(*target));

		if(card->monster_id()) {
			units_.push_back(unit::build_from_prototype(*card->monster_id(), nplayer, loc));
			units_.back()->set_moved();
			units_.back()->handle_event("summoned");
		} else {
			unit_ptr u = get_unit_at(loc);
			if(u.get() != NULL) {
				if(card->modification()) {
					u->add_modification(*card->modification());
				}
			}
		}
	}
}

namespace {
class free_attack_info_callable : public resolve_card_info
{
	unit_ptr attacker_;
	hex::location target_;	

	variant get_value(const std::string& key) const {
		if(key == "caster") {
			return variant(attacker_.get());
		} else if(key == "target") {
			return variant(new location_object(target_));
		} else if(key == "targets") {
			std::vector<variant> v;
			variant value(new location_object(target_));
			v.push_back(value);
			return variant(&v);
		} else if(key == "game") {
			return variant(game::current());
		} else {
			return variant();
		}
	}

	std::vector<hex::location> targets() const {
		std::vector<hex::location> res;
		res.push_back(target_);
		return res;
	}

	unit_ptr caster() const {
		return attacker_;
	}

	CARD_ACTIVATION_TYPE activation_type() const { return CARD_ACTIVATION_EVENT; }

public:
	free_attack_info_callable(unit_ptr a, const hex::location& target)
	  : attacker_(a), target_(target)
	{}
};

}

void game::unit_free_attack(unit_ptr a, unit_ptr b)
{
	foreach(unit_ability_ptr ability, a->abilities()) {
		const_card_ptr spell = card::get(ability->spell());
		if(spell->is_attack()) {
			std::vector<hex::location> valid_targets;
			spell->calculate_valid_targets(a.get(), a->side(), valid_targets);
			if(std::count(valid_targets.begin(), valid_targets.end(), b->loc())) {
				free_attack_info_callable* callable_context = new free_attack_info_callable(a, b->loc());
				const variant context_holder(callable_context);
				spell->resolve_card(callable_context);
				return;
			}
		}
	}
}

void game::draw_hand(int nplayer, int ncards)
{
}

void game::capture_tower(const hex::location& loc, unit_ptr u)
{
	const int nplayer = u->side();

	bool is_tower = neutral_towers_.count(loc) != 0;
	if(is_tower) {
		neutral_towers_.erase(loc);
	} else {
		for(int n = 0; n != players_.size(); ++n) {
			if(n != nplayer) {
				if(players_[n].towers.count(loc)) {
					players_[n].towers.erase(loc);
					is_tower = true;
					break;
				}
			}
		}
	}

	if(!is_tower) {
		return;
	}

	if(nplayer >= 0 && nplayer < players_.size()) {
		players_[nplayer].towers[loc] = resource::resource_id(u->resource_type());
	}
}

void game::swap_outgoing_messages(std::vector<message>& msg)
{
	outgoing_messages_.swap(msg);
	outgoing_messages_.clear();
}

void game::queue_message(const std::string& msg, int nplayer)
{
	outgoing_messages_.push_back(message());
	outgoing_messages_.back().contents = msg;
	if(nplayer >= 0) {
		outgoing_messages_.back().recipients.push_back(nplayer);
	}
}

void game::send_error(const std::string& msg, int nplayer)
{
	wml::node_ptr node(new wml::node("error"));
	node->set_attr("message", msg);
	queue_message(wml::output_xml(node), nplayer);
}

bool game::add_city(city_ptr new_city)
{
	cities_.push_back(new_city);
	return true;
}

void game::setup_game()
{
	started_ = true;
	width_ = 16;
	height_ = 16;
	tiles_.clear();

	const std::vector<std::string> terrains = terrain::all();
	for(int y = 0; y != height_; ++y) {
		for(int x = 0; x != width_; ++x) {
			const hex::location loc(x, y);

			static const char* TerrainTypes[] = { "grassland" };

			const char* type = TerrainTypes[rand()%(sizeof(TerrainTypes)/sizeof(*TerrainTypes))];

			if(loc == hex::location(2, 8) || loc == hex::location(12, 8)) {
				//pass.
			} else if(rand()%24 == 0) {
				type = "tower";
				neutral_towers_.insert(loc);
			} else if(rand()%16 == 0) {
				type = "rock";
			}
			tiles_.push_back(tile(x, y, type));
		}
	}

	units_.push_back(unit::build_from_prototype("wizard", 0, hex::location(2, 8)));
	units_.push_back(unit::build_from_prototype("diamond_sorceress", 1, hex::location(11, 8)));

	if(players_.size() > 0) {
		players_[0].castle = hex::location(1, 14);
	}

	if(players_.size() > 1) {
		players_[1].castle = hex::location(13, 14);
	}

	assign_tower_owners();
}

const tile* game::get_tile(int x, int y) const
{
	if(!started()) {
		return 0;
	}

	if(x < 0 || y < 0 || x >= width_ || y >= height_) {
		return 0;
	}

	ASSERT_EQ(width_*height_, tiles_.size());

	return &tiles_[y*width_ + x];
}

tile* game::get_tile(int x, int y)
{
	if(!started()) {
		return 0;
	}

	if(x < 0 || y < 0 || x >= width_ || y >= height_) {
		return 0;
	}

	ASSERT_EQ(width_*height_, tiles_.size());

	return &tiles_[y*width_ + x];
}

void game::add_player(const std::string& name)
{
	players_.push_back(player());
	players_.back().name = name;
}

void game::add_ai_player(const std::string& name)
{
	players_.push_back(player());
	players_.back().name = name;
	ai_.push_back(boost::shared_ptr<ai_player>(ai_player::create(*this, players_.size()-1)));
}

namespace {
using namespace hex;
class reachable_cost_calculator : public hex::path_cost_calculator
{
	const game* game_;
public:
	reachable_cost_calculator(const game& g) : game_(&g)
	{}

	int estimated_cost(const location& a, const location& b) const {
		return allowed_to_move(b) ? 1 : 0;
	}

	bool allowed_to_move(const location& a) const {
		const tile* t = game_->get_tile(a.x(), a.y());
		return t && t->terrain()->id() != "void";
	}

	bool legal_move_endpoint(const location& a) const {
		return allowed_to_move(a);
	}
};
}

void game::assign_tower_owners()
{
}

int game::tower_owner(const hex::location& loc, char* resource) const
{
	int index = 0;
	foreach(const player& p, players_) {
		if(p.towers.count(loc)) {
			if(resource) {
				*resource = p.towers.find(loc)->second;
			}
			return index;
		}

		++index;
	}

	return -1;
}

std::set<hex::location> game::tower_locs() const
{
	std::set<hex::location> res = neutral_towers_;
	foreach(const player& p, players_) {
		for(std::map<hex::location, char>::const_iterator i = p.towers.begin();
		    i != p.towers.end(); ++i) {
			res.insert(i->first);
		}
	}

	return res;
}

unit_ptr game::get_unit_at(const hex::location& loc)
{
	foreach(const unit_ptr& u, units_) {
		if(u->loc() == loc) {
			return u;
		}
	}

	return unit_ptr();
}

const_unit_ptr game::get_unit_at(const hex::location& loc) const
{
	foreach(const const_unit_ptr& u, units_) {
		if(u->loc() == loc) {
			return u;
		}
	}

	return const_unit_ptr();
}

bool game::do_state_based_actions()
{
	//visit all units and let them know of the game state
	//change so they can update themselves.
	foreach(unit_ptr& u, units_) {
		u->game_state_changed();
	}

	//visit all units and kill any that are now dead.
	bool actions = false;
	foreach(unit_ptr& u, units_) {
		if(u->damage_taken() >= u->life()) {
			const wml::const_node_ptr death_anim_node(hex::write_location("death_anim", u->loc()));
			queue_message(wml::output_xml(death_anim_node));

			actions = true;
			u.reset();
		}
	}

	units_.erase(std::remove(units_.begin(), units_.end(), unit_ptr()), units_.end());
	if(actions) {
		do_state_based_actions();
	}

	return actions;
}

namespace {
using namespace game_logic;
class player_callable : public formula_callable
{
	game::player& player_;
public:
	explicit player_callable(game::player& p) : player_(p)
	{}

	variant get_value(const std::string& key) const {
		for(int n = 0; n != resource::num_resources(); ++n) {
			if(key == resource::resource_name(n)) {
				return variant(player_.resources[n]);
			}
		}

		return variant();
	}

	void set_value(const std::string& key, const variant& v) {
		for(int n = 0; n != resource::num_resources(); ++n) {
			if(key == resource::resource_name(n)) {
				player_.resources[n] = v.as_int();
				return;
			}
		}
	}
};
}

variant game::get_value(const std::string& key) const
{
	if(key == "players") {
		std::vector<variant> v;
		for(int n = 0; n != players_.size(); ++n) {
			v.push_back(variant(new player_callable(const_cast<player&>(players_[n]))));
		}

		return variant(&v);
	} else if(key == "tiles") {
		std::vector<variant> v;
		foreach(const tile& t, tiles_) {
			v.push_back(variant(new tile_object(t)));
		}

		return variant(&v);
	} else if(key == "units") {
		std::vector<variant> v;
		foreach(const unit_ptr u, units_) {
			v.push_back(variant(u.get()));
		}

		return variant(&v);
	}

	return variant();
}

void game::execute_command(variant v, client_play_game* client)
{
	if(v.is_list()) {
		for(int n = 0; n != v.num_elements(); ++n) {
			execute_command(v[n], client);
		}
	}

	const game_command_callable* cmd = v.try_convert<const game_command_callable>();
	if(cmd) {
		cmd->execute(client);
	}
}

bool can_player_pay_cost(int side, const std::vector<int>& v)
{
	if(side < 0 || side >= game::current()->players().size()) {
		return false;
	}

	const std::vector<int>& resources = game::current()->players()[side].resources;;
	for(int n = 0; n < resources.size() && n < v.size(); ++n) {
		if(v[n] > resources[n]) {
			return false;
		}
	}

	return true;
}

int get_player_unit_limit_slots_used(int side)
{
	if(side < 0 || side >= game::current()->players().size()) {
		return 0;
	}

	int slots = 0;
	const game& g = *game::current();
	foreach(unit_ptr u, g.units()) {
		if(u->side() == side) {
			slots += u->maintenance_cost();
		}
	}

	return slots;
}

int get_player_unit_limit(int side)
{
	if(side < 0 || side >= game::current()->players().size()) {
		return 0;
	}

	const game::player& p = game::current()->players()[side];
	return p.towers.size() + 2;
}
