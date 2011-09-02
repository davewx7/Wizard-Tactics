#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include <set>

#include "ai_player.hpp"
#include "card.hpp"
#include "city.hpp"
#include "formula_callable.hpp"
#include "simple_wml.hpp"
#include "tile.hpp"
#include "tile_logic.hpp"
#include "unit.hpp"
#include "variant.hpp"
#include "wml_node_fwd.hpp"

class game : public game_logic::formula_callable
{
public:
	static game* current();

	game();
	explicit game(wml::const_node_ptr node);
	wml::node_ptr write() const;
	void handle_message(int nplayer, const simple_wml::string_span& type, simple_wml::node& msg);

	struct message {
		std::vector<int> recipients;
		std::string contents;
	};

	void swap_outgoing_messages(std::vector<message>& msg);

	bool started() const { return started_; }

	const tile* get_tile(const hex::location& loc) const { return get_tile(loc.x(), loc.y()); }
	const tile* get_tile(int x, int y) const;
	tile* get_tile(int x, int y);
	const std::vector<tile>& tiles() const { return tiles_; }
	int width() const { return width_; }
	int height() const { return height_; }

	const std::vector<city_ptr>& cities() const { return cities_; }

	void add_player(const std::string& name);
	void add_ai_player(const std::string& name);

	struct player {
		hex::location castle;
		std::string name;
		std::vector<held_card> spells;
		std::map<hex::location, char> towers; //maps towers to resources
		std::vector<int> resources;
		std::vector<int> resource_gain;
	};

	const std::vector<player>& players() const { return players_; }

	std::vector<unit_ptr>& units() { return units_; }
	const std::vector<unit_ptr>& units() const { return units_; }

	unit_ptr get_unit_at(const hex::location& loc);
	const_unit_ptr get_unit_at(const hex::location& loc) const;

	void resolve_combat();

	void capture_tower(const hex::location& loc, unit_ptr u);

	void queue_message(const std::string& msg, int nplayer=-1);

	int player_turn() const { return player_turn_; }
	int player_casting() const { return player_casting_; }

	int tower_owner(const hex::location& loc, char* resource=NULL) const;

	void execute_command(variant v, class client_play_game* client);
private:
	variant get_value(const std::string& key) const;

	void play_card(int nplayer, simple_wml::node& msg, int speed=-1);
	void resolve_card(int nplayer, const_card_ptr card, simple_wml::node& msg);
	bool add_city(city_ptr new_city);
	void setup_game();
	bool started_;
	std::vector<tile> tiles_;
	int width_, height_;

	std::vector<city_ptr> cities_;

	std::vector<player> players_;

	std::vector<unit_ptr> units_;
	
	void draw_hand(int nplayer, int ncards=5);
	std::vector<message> outgoing_messages_;

	void send_error(const std::string& msg, int nplayer=-1);

	enum GAME_STATE { STATE_SETUP, STATE_PLAYING };
	GAME_STATE state_;
	int player_turn_, player_casting_;

	std::set<hex::location> neutral_towers_;

	int spell_casting_passes_;

	bool done_main_phase_;

	void assign_tower_owners();

	std::vector<boost::shared_ptr<ai_player> > ai_;
};

class game_context {
	game* old_game_;
public:
	explicit game_context(game* g);
	void set(game* g);
	~game_context();
};

#endif
