#ifndef GAME_HPP_INCLUDED
#define GAME_HPP_INCLUDED

#include "card.hpp"
#include "city.hpp"
#include "simple_wml.hpp"
#include "tile.hpp"
#include "wml_node_fwd.hpp"

class game
{
public:
	game();
	explicit game(wml::const_node_ptr node);
	wml::node_ptr write() const;
	void handle_message(int nplayer, const simple_wml::string_span& type, const simple_wml::node& msg);

	struct message {
		std::vector<int> recipients;
		std::string contents;
	};

	void swap_outgoing_messages(std::vector<message>& msg);

	bool started() const { return started_; }

	const tile* get_tile(int x, int y) const;
	tile* get_tile(int x, int y);
	int width() const { return width_; }
	int height() const { return height_; }

	const std::vector<city_ptr>& cities() const { return cities_; }

	void add_player(const std::string& name);

	struct player {
		std::string name;
		std::vector<const_card_ptr> deck, hand;
	};

	const std::vector<player>& players() const { return players_; }
private:
	void play_card(int nplayer, const simple_wml::node& msg);
	bool add_city(city_ptr new_city);
	void setup_game();
	bool started_;
	std::vector<tile> tiles_;
	int width_, height_;

	std::vector<city_ptr> cities_;

	std::vector<player> players_;
	
	void draw_hand(int nplayer);
	void queue_message(const std::string& msg, int nplayer=-1);
	std::vector<message> outgoing_messages_;

	void send_error(const std::string& msg, int nplayer=-1);

	enum GAME_STATE { STATE_SETUP, STATE_PLAYING };
	GAME_STATE state_;
	int player_turn_;
};

#endif
