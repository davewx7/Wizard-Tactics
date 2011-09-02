#ifndef CLIENT_PLAY_GAME_HPP_INCLUDED
#define CLIENT_PLAY_GAME_HPP_INCLUDED

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <SDL.h>

#include <set>

#include "button.hpp"
#include "card.hpp"
#include "game.hpp"
#include "tile_logic.hpp"

class client_play_game : public card_selector{
public:
	explicit client_play_game(const std::string& player_name);
	virtual ~client_play_game();
	void play();
	void draw() const;

	hex::location selected_loc() const;

	hex::location player_select_loc(const std::string& prompt,
	                                boost::function<bool(hex::location)> valid_loc);

	const game& get_game() const { return game_; }

private:
	void handle_mouse_button_down(const SDL_MouseButtonEvent& event);
	int selected_card() const;
	std::string player_name_;
	game game_;
	int player_id_;

	enum GAME_STATE { STATE_WAITING, STATE_PLAY_CARDS, STATE_SELECT_LOC };
	GAME_STATE state_;

	void set_prompt(const std::string& prompt);
	graphics::texture prompt_texture_;
	std::string prompt_;
	boost::function<bool(hex::location)> valid_loc_functor_;
	boost::shared_ptr<hex::location> loc_result_;

	void end_turn(bool discard);

	boost::shared_ptr<gui::button> end_turn_button_, discard_button_;

	std::set<gui::widget_ptr> widgets_;
};

#endif
