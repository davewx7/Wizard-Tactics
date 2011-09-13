#include <algorithm>

#include "ai_player.hpp"
#include "asserts.hpp"
#include "card.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
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

namespace {
game* current_game = NULL;
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

	foreach(const player& p, players_) {
		wml::node_ptr player_node(new wml::node("player"));
		player_node->set_attr("name", p.name);
		player_node->set_attr("spells", write_deck(p.spells));

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
	}

	return res;
}

void game::handle_message(int nplayer, const simple_wml::string_span& type, simple_wml::node& msg)
{
	if(type == "setup") {
		setup_game();
		queue_message(wml::output(write()));

		ASSERT_GE(players_.size(), 1);
		queue_message(formatter() << "[new_turn]\nplayer=\"" << players_.front().name << "\"\n[/new_turn]\n");
		state_ = STATE_PLAYING;
		player_casting_ = player_turn_ = 0;
	} else if(type == "spells") {
		std::cerr << "READ DECK FOR " << nplayer << "\n";
		ASSERT_INDEX_INTO_VECTOR(nplayer, players_);
		players_[nplayer].spells = read_deck(msg.attr("spells").to_string());

		players_[nplayer].resource_gain.resize(resource::num_resources());
		int num_resources = resource::num_resources();
		util::split_into_ints(msg.attr("resource_gain").to_string().c_str(), &players_[nplayer].resource_gain[0], &num_resources);

		players_[nplayer].resources = players_[nplayer].resource_gain;

		draw_hand(nplayer);
		queue_message(wml::output(write()));
	} else if(type == "move") {
		const simple_wml::node* from = msg.child("from");
		const simple_wml::node* to = msg.child("to");
		ASSERT_LOG(from && to, "message format of move illegal");
		const hex::location from_loc(from->attr("x").to_int(), from->attr("y").to_int());
		const hex::location to_loc(to->attr("x").to_int(), to->attr("y").to_int());

		wml::node_ptr anim_node(new wml::node("move_anim"));
		anim_node->add_child(hex::write_location("from", from_loc));
		anim_node->add_child(hex::write_location("to", to_loc));
		queue_message(wml::output(anim_node));

		unit_ptr u = get_unit_at(from_loc);
		ASSERT_LOG(u.get(), "Could not find unit at from loc");
		u->set_loc(to_loc);
		u->set_moved();
		capture_tower(to_loc, u);
		queue_message(wml::output(write()));

	} else if(type == "play") {
		play_card(nplayer, msg);
		spell_casting_passes_ = 0;
	} else if(type == "end_turn") {
		const bool skipping = msg.attr("skip") == "yes";
		if(skipping) {
			++spell_casting_passes_;
		} else {
			spell_casting_passes_ = 0;
		}

		player_casting_ = player_casting_+1;
		if(player_casting_ == players_.size()) {
			player_casting_ = 0;
		}

		//make sure any units die that are meant to.
		resolve_combat();

		std::cerr << "SPELLS: " << spell_casting_passes_ << " / " << players_.size() << "\n";

		if(spell_casting_passes_ >= players_.size()) {
			spell_casting_passes_ = 0;

			int last_delay = -1;

			//make sure any units die that are meant to.
			resolve_combat();

			ASSERT_INDEX_INTO_VECTOR(nplayer, players_);

			resolve_combat();

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
		
		queue_message(wml::output(write()));
		queue_message(formatter() << "[new_turn]\nplayer=\"" << players_[player_casting_].name << "\"\n[/new_turn]\n");

		foreach(boost::shared_ptr<ai_player> ai, ai_) {
			std::vector<wml::node_ptr> nodes = ai->play();

			while(nodes.empty() == false) {
				foreach(wml::node_ptr node, nodes) {
					std::string msg(wml::output(node));
					std::cerr << "AI MESSAGE: " << msg << "\n";
					simple_wml::document doc(msg.c_str(), simple_wml::INIT_STATIC);
					handle_message(ai->player_id(), doc.root().first_child(), *doc.root().first_child_node());
				}

				nodes = ai->play();
			}
		}
	}
}

void game::play_card(int nplayer, simple_wml::node& msg, int speed)
{
	const std::string type = msg.attr("spell").to_string();

	const_card_ptr card = card::get(type);
	ASSERT_LOG(card.get() != NULL, "CARD " << type << " NOT FOUND " << nplayer);

	ASSERT_INDEX_INTO_VECTOR(nplayer, players_);
	player& p = players_[nplayer];

	if(msg.has_attr("caster")) {
		const int caster_key = msg.attr("caster").to_int();
		unit_ptr caster;
		foreach(unit_ptr u, units_) {
			if(u->key() == caster_key) {
				caster = u;
			}
		}

		ASSERT_LOG(caster.get() != NULL, "COULD NOT FIND SPELL CASTER: " << caster_key);
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
	resolve_combat();

	queue_message(wml::output(write()));
}

namespace {
class card_info_callable : public resolve_card_info
{
	game& g_;
	const simple_wml::node& msg_;

	variant get_value(const std::string& key) const {
		if(key == "caster") {
			unit_ptr u = caster();
			if(u.get() != NULL) {
				return variant(u.get());
			} else {
				return variant();
			}
		} else if(key == "target") {
			const simple_wml::node* t = msg_.child("target");
			if(t) {
				return variant(new location_object(hex::location(t->attr("x").to_int(), t->attr("y").to_int())));
			}
		} else if(key == "targets") {
			std::vector<variant> v;
			foreach(const simple_wml::node* t, msg_.children("target")) {
				v.push_back(variant(new location_object(hex::location(t->attr("x").to_int(), t->attr("y").to_int()))));
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
		foreach(const simple_wml::node* t, msg_.children("target")) {
			v.push_back(hex::location(t->attr("x").to_int(), t->attr("y").to_int()));
		}

		return v;
	}

	unit_ptr caster() const
	{
		const int caster_key = msg_.attr("caster").to_int();
		foreach(unit_ptr u, g_.units()) {
			if(u->key() == caster_key) {
				return u;
			}
		}

		return unit_ptr();
	}

public:
	card_info_callable(game& g, const simple_wml::node& msg) : g_(g), msg_(msg)
	{}
};

}

void game::resolve_card(int nplayer, const_card_ptr card, simple_wml::node& msg)
{
	card_info_callable* callable_context = new card_info_callable(*this, msg);
	const variant context_holder(callable_context);
	card->resolve_card(callable_context);

	std::cerr << "RESOLVING CARD...\n";
	foreach(const simple_wml::node* land, msg.children("land")) {
		hex::location loc(land->attr("x").to_int(), land->attr("y").to_int());
		tile* t = get_tile(loc.x(), loc.y());
		ASSERT_LOG(t, "ILLEGAL TILE LOCATION: " << loc);
		*t = tile(loc.x(), loc.y(), land->attr("land").to_string());
		assign_tower_owners();
	}

	foreach(const simple_wml::node* monster, msg.children("monster")) {
		hex::location loc(monster->attr("x").to_int(), monster->attr("y").to_int());
		units_.push_back(unit::build_from_prototype(monster->attr("type").to_string(), nplayer, loc));
		units_.back()->set_moved();

	}

	foreach(const simple_wml::node* target, msg.children("target")) {
		hex::location loc(target->attr("x").to_int(), target->attr("y").to_int());
		unit_ptr u = get_unit_at(loc);
		if(u.get() != NULL) {
			if(card->modification()) {
				u->add_modification(*card->modification());
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
	queue_message(wml::output(node), nplayer);
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
	units_.push_back(unit::build_from_prototype("wizard", 1, hex::location(12, 8)));

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

void game::resolve_combat()
{
	foreach(unit_ptr& u, units_) {
		if(u->damage_taken() >= u->life()) {
			u.reset();
		}
	}

	units_.erase(std::remove(units_.begin(), units_.end(), unit_ptr()), units_.end());
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
