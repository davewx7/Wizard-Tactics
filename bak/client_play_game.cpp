#include <boost/bind.hpp>

#include "SDL.h"

#include "client_network.hpp"
#include "client_play_game.hpp"
#include "draw_card.hpp"
#include "draw_game.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "hex_geometry.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "wml_node.hpp"
#include "wml_writer.hpp"

client_play_game::client_play_game(const std::string& player_name)
  : player_name_(player_name), player_id_(-1), state_(STATE_WAITING),
  end_turn_button_(new gui::button(gui::widget_ptr(new gui::label("End Turn", graphics::color_white())), boost::bind(&client_play_game::end_turn, this, false))),
  discard_button_(new gui::button(gui::widget_ptr(new gui::label("Discard", graphics::color_white())), boost::bind(&client_play_game::end_turn, this, true)))
{
	discard_button_->set_loc(880, 600);
	end_turn_button_->set_loc(880, 640);
	widgets_.insert(discard_button_);
	widgets_.insert(end_turn_button_);
}

client_play_game::~client_play_game()
{}

void client_play_game::play()
{
	for(;;) {
		wml::const_node_ptr msg = network::receive();
		if(msg) {
			if(msg->name() == "game") {
				game_ = game(msg);
				player_id_ = -1;
				for(int n = 0; n != game_.players().size(); ++n) {
					if(game_.players()[n].name == player_name_) {
						player_id_ = n;
						break;
					}
				}
			} else if(msg->name() == "new_turn") {
				if(msg->attr("player").str() == player_name_) {
					state_ = STATE_PLAY_CARDS;
				} else {
					state_ = STATE_WAITING;
				}
			}
		}

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			foreach(gui::widget_ptr w, widgets_) {
				w->process_event(event, false);
			}

			switch(event.type) {
				case SDL_QUIT:
					return;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE) {
						return;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					handle_mouse_button_down(event.button);
					break;
			}
		}

		if(state_ == STATE_SELECT_LOC && loc_result_.get() != NULL) {
			return;
		}

		graphics::prepare_raster();
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
		draw();
		SDL_GL_SwapBuffers();
	}
}

hex::location client_play_game::player_select_loc(const std::string& prompt,
                                                  boost::function<bool(hex::location)> valid_loc)
{
	GAME_STATE old_state = state_;
	set_prompt(prompt);
	valid_loc_functor_ = valid_loc;
	state_ = STATE_SELECT_LOC;

	play();

	set_prompt("");
	state_ = old_state;

	if(loc_result_.get() != NULL) {
		hex::location loc = *loc_result_;
		loc_result_.reset();
		return loc;
	} else {
		return hex::location();
	}
}

void client_play_game::set_prompt(const std::string& prompt)
{
	prompt_ = prompt;
	prompt_texture_ = font::render_text(prompt, graphics::color_white(), 12);
}

void client_play_game::draw() const
{
	draw_map(*this, game_, 0, 0);
	if(player_id_ < 0) {
		return;
	}

	if(prompt_texture_.valid()) {
		graphics::blit_texture(prompt_texture_, 10, 10);
	}

	const int selected = selected_card();
	const game::player& player = game_.players()[player_id_];
	for(int n = 0; n != player.hand.size(); ++n) {
		draw_card(player.hand[n], 20 + n*100, 600 - (n == selected ? 20 : 0));
	}

	foreach(gui::widget_ptr w, widgets_) {
		w->draw();
	}
}

hex::location client_play_game::selected_loc() const
{
	switch(state_) {
		case STATE_WAITING:
			return hex::location();
		default:
			break;
	}

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);
	return pixel_pos_to_loc(mousex, mousey);
}

int client_play_game::selected_card() const
{
	if(player_id_ < 0) {
		return -1;
	}

	int x, y;
	SDL_GetMouseState(&x, &y);

	const game::player& player = game_.players()[player_id_];
	for(int n = 0; n != player.hand.size(); ++n) {
		const int xpos = 20 + n*100;
		const int ypos = 600;
		if(x >= xpos && x < xpos + 80 && y >= ypos && y < ypos + 100) {
			return n;
		}
	}

	return -1;
}

void client_play_game::handle_mouse_button_down(const SDL_MouseButtonEvent& event)
{
	if(player_id_ < 0) {
		return;
	}

	const game::player& player = game_.players()[player_id_];
	hex::location loc = selected_loc();
	const int card = selected_card();
	switch(state_) {
		case STATE_PLAY_CARDS: {
			if(card != -1) {
				wml::node_ptr result = player.hand[card]->play_card(*this);
				if(result.get() != NULL) {
					network::send(wml::output(result));
				}
			}
			break;
		}
		case STATE_SELECT_LOC: {
			if(loc.valid() && valid_loc_functor_(loc)) {
				loc_result_.reset(new hex::location(loc));
			}
			break;
		}
		case STATE_WAITING:
			break;
	};
}

void client_play_game::end_turn(bool discard)
{
	network::send(formatter() << "[end_turn]\n" << (discard ? "discard=\"yes\"\n" : "") << "[/end_turn]\n");
}
