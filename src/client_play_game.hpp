#ifndef CLIENT_PLAY_GAME_HPP_INCLUDED
#define CLIENT_PLAY_GAME_HPP_INCLUDED

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <SDL.h>

#include <set>

#include "button.hpp"
#include "card.hpp"
#include "game.hpp"
#include "key.hpp"
#include "pathfind.hpp"
#include "tile_logic.hpp"
#include "unit_avatar.hpp"

class client_play_game : public card_selector{
public:
	explicit client_play_game(const std::string& player_name);
	virtual ~client_play_game();
	void play();
	void draw() const;

	hex::location selected_loc() const;

	hex::location player_select_loc(const std::string& prompt,
	                                boost::function<bool(hex::location)> valid_loc);

	bool player_pay_cost(const std::vector<int>& cost);

	const game& get_game() const { return *game_; }
	game& get_game() { return *game_; }

	int player_id() const { return player_id_; }

	struct action_cancellation_exception {
	};

	bool is_highlighted_loc(const hex::location& loc) const;

private:
	void handle_mouse_button_down(const SDL_MouseButtonEvent& event);
	void handle_mouse_motion(const SDL_MouseMotionEvent& event);
	int selected_card() const;
	std::string player_name_;
	boost::intrusive_ptr<game> game_;
	game_context game_context_;
	int player_id_;

	enum GAME_STATE { STATE_WAITING, STATE_PLAY_CARDS, STATE_USE_ABILITIES, STATE_SELECT_LOC };
	GAME_STATE state_;

	void set_prompt(const std::string& prompt);
	graphics::texture prompt_texture_;
	std::string prompt_;
	boost::function<bool(hex::location)> valid_loc_functor_;
	boost::shared_ptr<hex::location> loc_result_;

	void end_turn();
	void cancel_action();

	boost::shared_ptr<gui::button> end_turn_button_, cancel_button_;

	gui::widget_ptr unit_profile_widget_;
	std::vector<boost::shared_ptr<gui::button> > abilities_buttons_;
	void set_abilities_buttons();

	void activate_ability(unit_ptr u, unit_ability_ptr a);

	std::set<gui::widget_ptr> widgets_;

	std::vector<unit_avatar_ptr> unit_avatars_;
	void build_avatars();

	int animation_time_;

	//function which returns the unit to display the stats of
	unit_ptr display_unit() const;
	unit_ptr unit_at_loc(const hex::location& loc) const;

	unit_ptr mouse_over_unit_;

	unit_ptr moving_unit_;
	boost::scoped_ptr<hex::route_map> current_routes_;

	int xscroll_, yscroll_;
	CKey key_;
};

#endif
