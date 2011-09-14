#include <boost/bind.hpp>

#include <algorithm>
#include <numeric>

#include "SDL.h"

#include "asserts.hpp"
#include "client_network.hpp"
#include "client_play_game.hpp"
#include "debug_console.hpp"
#include "dialog.hpp"
#include "draw_card.hpp"
#include "draw_game.hpp"
#include "draw_utils.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "game_utils.hpp"
#include "grid_widget.hpp"
#include "hex_geometry.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "resource.hpp"
#include "tooltip.hpp"
#include "unit_animation.hpp"
#include "unit_avatar.hpp"
#include "unit_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

namespace {
class unit_avatar_widget : public gui::widget
{
public:
	unit_avatar_widget(const_unit_avatar_ptr a) : avatar_(a)
	{
		set_dim(100, 100);
	}

	void handle_draw() const {
		avatar_->draw(x() + 20, y());
	}

private:
	const_unit_avatar_ptr avatar_;
};

const int SideBarWidth = 200;

}

client_play_game::client_play_game(const std::string& player_name)
  : player_name_(player_name), game_(new game), game_context_(game_.get()),
    player_id_(-1), state_(STATE_WAITING),
  end_turn_button_(new gui::button(gui::widget_ptr(new gui::label("End Turn", graphics::color_white())), boost::bind(&client_play_game::end_turn, this))),
  cancel_button_(new gui::button(gui::widget_ptr(new gui::label("Cancel", graphics::color_white())), boost::bind(&client_play_game::cancel_action, this))),
  animation_time_(0),
  xscroll_(-SideBarWidth), yscroll_(0)
{
	end_turn_button_->set_loc(20, 640);
	cancel_button_->set_loc(20, 640);
	widgets_.insert(end_turn_button_);
}

client_play_game::~client_play_game()
{}

void client_play_game::play()
{
	for(;;) {
		wml::const_node_ptr msg;
		if(animation_time_ <= 0) {
			msg = network::receive();
		} else {
			--animation_time_;
		}

		foreach(unit_avatar_ptr a, unit_avatars_) {
			a->process();
		}

		if(msg) {
			if(msg->name() == "game") {
				game_ = boost::intrusive_ptr<game>(new game(msg));
				game_context_.set(game_.get());
				build_avatars();
				if(moving_unit_) {
					const int key = moving_unit_->key();
					moving_unit_.reset();
					foreach(unit_ptr u, game_->units()) {
						if(key == u->key()) {
							moving_unit_ = u;
							if(state_ == STATE_USE_ABILITIES) {
								moving_unit_->set_moved(false);
							}
						}
					}
					set_abilities_buttons();
				}
				player_id_ = -1;
				for(int n = 0; n != game_->players().size(); ++n) {
					if(game_->players()[n].name == player_name_) {
						player_id_ = n;
						break;
					}
				}
			} else if(msg->name() == "new_turn") {
				debug_console::add_message(formatter() << msg->attr("player").str() << "'s turn");
				if(msg->attr("player").str() == player_name_) {
					state_ = STATE_PLAY_CARDS;
					widgets_.insert(end_turn_button_);
				} else {
					state_ = STATE_WAITING;
					widgets_.erase(end_turn_button_);
				}
			} else if(msg->name() == "move_anim") {

				wml::const_node_ptr from_node = msg->get_child("from");
				wml::const_node_ptr to_node = msg->get_child("to");

				ASSERT_LOG(from_node.get() != NULL, "illegal move_anim format");
				ASSERT_LOG(to_node.get() != NULL, "illegal move_anim format");

				hex::location from_loc(wml::get_int(from_node, "x"), wml::get_int(from_node, "y"));
				hex::location to_loc(wml::get_int(to_node, "x"), wml::get_int(to_node, "y"));

				debug_console::add_message(formatter() << "Move unit from " << from_loc << " to " << to_loc);
				const unit_ptr u = game_->get_unit_at(from_loc);
				if(u.get() != NULL) {
					std::vector<hex::location> path;
					unit_movement_cost_calculator calc(game_.get(), u);
					hex::find_path(from_loc, to_loc, calc, &path);
					foreach(unit_avatar_ptr a, unit_avatars_) {
						if(a->get_unit() == u) {
							a->set_path(path);
							break;
						}
					}

					animation_time_ = (path.size()-1)*10;
				}
			} else if(msg->name() == "attack_anim") {
				wml::const_node_ptr from_node = msg->get_child("from");
				wml::const_node_ptr to_node = msg->get_child("to");

				ASSERT_LOG(from_node.get() != NULL, "illegal attack_anim format");
				ASSERT_LOG(to_node.get() != NULL, "illegal attack_anim format");

				hex::location from_loc(wml::get_int(from_node, "x"), wml::get_int(from_node, "y"));
				hex::location to_loc(wml::get_int(to_node, "x"), wml::get_int(to_node, "y"));
				debug_console::add_message(formatter() << "Attack from " << from_loc << " to " << to_loc);

				const unit_ptr u = game_->get_unit_at(from_loc);
				if(u.get() != NULL) {
					foreach(unit_avatar_ptr a, unit_avatars_) {
						if(a->get_unit() == u) {
							a->set_attack(to_loc);
							animation_time_ = 10;
							break;
						}
					}
				}
			}
		}

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			foreach(gui::widget_ptr w, widgets_) {
				if(w->process_event(event, false)) {
					break;
				}
			}

			switch(event.type) {
				case SDL_QUIT:
					return;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym == SDLK_ESCAPE) {
						return;
					}
					break;
				case SDL_MOUSEMOTION:
					handle_mouse_motion(event.motion);
					break;
				case SDL_MOUSEBUTTONDOWN:
					handle_mouse_button_down(event.button);
					break;
			}
		}

		if(state_ == STATE_SELECT_LOC && loc_result_.get() != NULL) {
			return;
		}

		const int ScrollSpeed = 12;

		if(key_[SDLK_UP]) {
			yscroll_ -= ScrollSpeed;
			if(yscroll_ < 0) {
				yscroll_ = 0;
			}
		}

		if(key_[SDLK_DOWN]) {
			yscroll_ += ScrollSpeed;
			const int MaxScroll = get_game().height()*HexHeight + 200 - graphics::screen_height();
			if(yscroll_ > MaxScroll) {
				yscroll_ = MaxScroll;
			}
		}

		if(key_[SDLK_LEFT]) {
			xscroll_ -= ScrollSpeed;
			if(xscroll_ < -SideBarWidth) {
				xscroll_ = -SideBarWidth;
			}
		}

		if(key_[SDLK_RIGHT]) {
			xscroll_ += ScrollSpeed;
			const int MaxScroll = get_game().width()*HexWidth + HexWidth/2 - graphics::screen_width();
			if(xscroll_ > MaxScroll) {
				xscroll_ = MaxScroll;
			}
		}

		graphics::prepare_raster();
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
		draw();
		SDL_GL_SwapBuffers();

		SDL_Delay(20);
	}
}

hex::location client_play_game::player_select_loc(const std::string& prompt,
                                                  boost::function<bool(hex::location)> valid_loc)
{
	widgets_.erase(end_turn_button_);
	widgets_.insert(cancel_button_);

	GAME_STATE old_state = state_;
	std::string old_prompt = prompt_;
	set_prompt(prompt);
	valid_loc_functor_ = valid_loc;
	state_ = STATE_SELECT_LOC;

	set_abilities_buttons();

	try {
		play();
	} catch(action_cancellation_exception& e) {
		loc_result_.reset();
	}

	valid_loc_functor_ = boost::function<bool(hex::location)>();

	set_prompt(old_prompt);
	state_ = old_state;

	widgets_.insert(end_turn_button_);
	widgets_.erase(cancel_button_);

	set_abilities_buttons();

	if(loc_result_.get() != NULL) {
		hex::location loc = *loc_result_;
		loc_result_.reset();
		return loc;
	} else {
		return hex::location();
	}
}

namespace {
bool loc_in_set(const hex::location& loc, const std::set<hex::location>& locs) {
	return locs.count(loc) != 0;
}
}

bool client_play_game::player_pay_cost(const std::vector<int>& cost_ref)
{
	const game::player& player = game_->players()[player_id_];
	for(int n = 0; n != cost_ref.size(); ++n) {
		if(n >= player.resources.size() || cost_ref[n] > player.resources[n]) {
			return false;
		}
	}

	return true;
}

void client_play_game::set_prompt(const std::string& prompt)
{
	prompt_ = prompt;
	prompt_texture_ = font::render_text(prompt, graphics::color_white(), 12);
}

namespace {
int cycle_num = 0;
}

void client_play_game::draw() const
{
	++cycle_num;

	glPushMatrix();
	glTranslatef(-xscroll_, -yscroll_, 0);

	draw_map(*this, *game_, 0, 0);
	if(player_id_ < 0) {
		glPopMatrix();
		return;
	}

	static const unit_animation_team_colored* const flag_anim[4] = {
	  unit_animation_team_colored::create_simple_anim("flag-1.png", 0),
	  unit_animation_team_colored::create_simple_anim("flag-1.png", 1),
	  unit_animation_team_colored::create_simple_anim("flag-1.png", 2),
	  unit_animation_team_colored::create_simple_anim("flag-1.png", 3),
	};

	hex::location selected_hex = selected_loc();
	if(current_routes_.get() != NULL && current_routes_->count(selected_hex)) {
		draw_arrow(current_routes_->find(selected_hex)->second.steps);
	}

	std::vector<unit_avatar_ptr> avatars = unit_avatars_;
	std::sort(avatars.begin(), avatars.end(), compare_unit_avatars_by_ypos);

	std::vector<unit_avatar_ptr>::const_iterator avatar_itor = avatars.begin();

	const std::set<hex::location>& towers = game_->tower_locs();
	for(int y = 0; y != game_->height(); ++y) {
		for(int x = 0; x != game_->height(); ++x) {
			hex::location loc(x, y);
			if(towers.count(loc) == 1) {
				draw_underlays(*this, *game_, loc);
			}
		}

		while(avatar_itor != avatars.end() && (*avatar_itor)->get_unit()->loc().y() == y) {
			(*avatar_itor)->draw();
			++avatar_itor;
		}

		for(int x = 0; x != game_->height(); ++x) {
			hex::location loc(x, y);
			if(towers.count(loc) == 1) {
				draw_overlays(*this, *game_, loc);
			}
		}
	}

	if(prompt_texture_.valid()) {
		graphics::blit_texture(prompt_texture_, 10, 10);
	}

	glPopMatrix();

	graphics::draw_rect(rect(0, 0, SideBarWidth, graphics::screen_height()), graphics::color(0, 0, 0, 255));

	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);

	boost::shared_ptr<std::string> card_tooltip;

	const int selected = selected_card();
	const game::player& player = game_->players()[player_id_];
	for(int n = 0; n != player.spells.size(); ++n) {
		const int x = SideBarWidth + 10 + n*100;
		const int y = 600 - (n == selected ? 20 : 0);

		if(player.spells[n].embargo) {
			glColor4f(0.5, 0.5, 0.5, 1.0);
		}
		const rect area = draw_card(player.spells[n].card, x, y);
		glColor4f(1.0, 1.0, 1.0, 1.0);

		if(mousex >= area.x() && mousex <= area.x2() &&
		   mousey >= area.y() && mousey <= area.y2()) {
			card_tooltip = player.spells[n].card->description();
		}
	}

	gui::set_tooltip(card_tooltip);

	int resourcey = 400;
	for(int n = 0; n != resource::num_resources(); ++n) {
		if(player.resource_gain[n] == 0) {
			continue;
		}

		char buf[128];
		sprintf(buf, "magic-icon-%c", resource::resource_id(n));

		const_gui_section_ptr section = gui_section::get(buf);

		for(int m = 0; m < player.resources[n]; ++m) {
			section->blit(20 + 16*(m%5), resourcey, 16, 16);
			if((m+1)%5 == 0 && m+1 < player.resources[n]) {
				resourcey += 24;
			}
		}

		resourcey += 24;
	}

	foreach(gui::widget_ptr w, widgets_) {
		w->draw();
	}

	debug_console::draw();
	gui::draw_tooltip();
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
	return pixel_pos_to_loc(xscroll_ + mousex, yscroll_ + mousey);
}

int client_play_game::selected_card() const
{
	if(player_id_ < 0) {
		return -1;
	}

	int x, y;
	SDL_GetMouseState(&x, &y);

	const game::player& player = game_->players()[player_id_];
	for(int n = 0; n != player.spells.size(); ++n) {
		const int xpos = SideBarWidth + 10 + n*100;
		const int ypos = 600;
		if(x >= xpos && x < xpos + 80 && y >= ypos && y < ypos + 100) {
			if(player.spells[n].embargo) {
				return -1;
			}

			return n;
		}
	}

	return -1;
}

unit_ptr client_play_game::display_unit() const
{
	if(mouse_over_unit_.get() != NULL) {
		return mouse_over_unit_;
	} else {
		return moving_unit_;
	}
}

unit_ptr client_play_game::unit_at_loc(const hex::location& loc) const {
	foreach(unit_ptr u, game_->units()) {
		if(u->loc() == loc) {
			return u;
		}
	}

	return unit_ptr();
}

void client_play_game::handle_mouse_motion(const SDL_MouseMotionEvent& event)
{
	const unit_ptr displayed = display_unit();
	const hex::location loc = selected_loc();
	if(loc.valid()) {
		mouse_over_unit_ = unit_at_loc(loc);
	}

	if(display_unit() != displayed) {
		set_abilities_buttons();
	}
}

void client_play_game::handle_mouse_button_down(const SDL_MouseButtonEvent& event)
{
	if(player_id_ < 0) {
		return;
	}

	if(event.button == SDL_BUTTON_RIGHT) {
		moving_unit_ = unit_ptr();
		current_routes_.reset();
		set_abilities_buttons();

		if(state_ == STATE_USE_ABILITIES) {
			network::send("[end_turn]\n[/end_turn]\n");
		}
		return;
	}

	const game::player& player = game_->players()[player_id_];
	hex::location loc = selected_loc();
	const int card = selected_card();
	switch(state_) {
		case STATE_PLAY_CARDS: {
			if(card != -1) {
				try {
					wml::node_ptr result = player.spells[card].card->use_card(*this);
					if(result.get() != NULL) {
						network::send(wml::output(result));
						network::send("[end_turn]\n[/end_turn]\n");
					}
				} catch(action_cancellation_exception& e) {
				}
			} else if(loc.valid()) {
				unit_ptr selected_unit = unit_at_loc(loc);

				if(selected_unit && selected_unit->side() == player_id_) {
					if(!selected_unit->has_moved()) {
						std::cerr << "FINDING POSSIBLE MOVES... " << selected_unit->move() << "\n";
						current_routes_.reset(new hex::route_map);
						find_possible_moves(selected_unit->loc(), selected_unit->move(), unit_movement_cost_calculator(game_.get(), selected_unit), *current_routes_);

						std::cerr << "FOUND MOVES: " << current_routes_->size() << "\n";

						moving_unit_ = selected_unit;
						set_abilities_buttons();
					} else {
						bool has_usable_abilities = false;
						foreach(unit_ability_ptr a, selected_unit->abilities()) {
							if(a->is_ability_usable(*selected_unit, *this)) {
								has_usable_abilities = true;
							}
						}

						if(has_usable_abilities) {
							current_routes_.reset();
							moving_unit_ = selected_unit;
							set_abilities_buttons();
						}
					}

				} else if(current_routes_.get() != NULL && current_routes_->count(loc)) {

					wml::node_ptr move_node(new wml::node("move"));
					move_node->add_child(hex::write_location("from", moving_unit_->loc()));
					move_node->add_child(hex::write_location("to", loc));
					network::send(wml::output(move_node));

					const hex::location src_loc = moving_unit_->loc();
					moving_unit_->set_loc(loc);

					bool has_usable_abilities = false;
					foreach(unit_ability_ptr a, moving_unit_->abilities()) {
						if(a->is_ability_usable(*moving_unit_, *this)) {
							has_usable_abilities = true;
						}
					}

					moving_unit_->set_loc(src_loc);

					std::cerr << "HAS USABLE: " << (has_usable_abilities ? " YES" : " NO") << "\n";

					if(has_usable_abilities) {
						state_ = STATE_USE_ABILITIES;
						if(moving_unit_->default_ability() &&
						   moving_unit_->default_ability()->is_ability_usable(*moving_unit_, *this)) {
							unit_ptr u = moving_unit_;
					// TODO: make default abilities work.
					//		activate_ability(moving_unit_, moving_unit_->default_ability());
							moving_unit_ = u;
							set_abilities_buttons();
						}
					} else {
						moving_unit_ = unit_ptr();
						current_routes_.reset();
						set_abilities_buttons();

						if(state_ == STATE_USE_ABILITIES) {
							network::send("[end_turn]\n[/end_turn]\n");
						}
					}

					current_routes_.reset();
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

void client_play_game::end_turn()
{
	const std::string end_turn = state_ == STATE_PLAY_CARDS ? "skip=\"yes\"\n" : "";
	set_prompt("");
	network::send("[end_turn]\n" + end_turn + "[/end_turn]\n");
}

void client_play_game::cancel_action()
{
	throw action_cancellation_exception();
}

void client_play_game::set_abilities_buttons()
{
	widgets_.erase(unit_profile_widget_);
	unit_profile_widget_ = gui::widget_ptr();

	unit_ptr displayed = display_unit();
	if(displayed == NULL) {
		return;
	}

	using namespace gui;

	gui::dialog_ptr main_grid(new gui::dialog(20, 5, 180, 380));
	main_grid->set_padding(8);

	main_grid->add_widget(widget_ptr(new label(displayed->name(), graphics::color_white(), 16)));

	main_grid->add_widget(widget_ptr(new unit_avatar_widget(unit_avatar_ptr(new unit_avatar(displayed)))));
	
	gui::grid_ptr ability_grid(new gui::grid(1));
	foreach(unit_ability_ptr a, displayed->abilities()) {
		const_card_ptr spell = card::get(a->spell());
		int ncost = 0;
		foreach(int n, spell->cost()) {
			ncost += n;
		}

		if(a->taps_caster()) {
			++ncost;
		}


		grid_ptr g(new grid(2 + ncost));
		g->set_hpad(6);
		g->add_col(widget_ptr(new image_widget("abilities/" + a->icon(), 30, 30)));
		g->add_col(widget_ptr(new label(a->name(), graphics::color_white())));

		for(int n = 0; n != spell->cost().size(); ++n) {
			for(int m = 0; m != spell->cost()[n]; ++m) {
				g->add_col(widget_ptr(new gui_section_widget(resource::resource_icon(n))));
			}
		}

		if(a->taps_caster()) {
			g->add_col(widget_ptr(new gui_section_widget("tired-icon")));
		}

		button* b = new button(g, boost::bind(&client_play_game::activate_ability, this, displayed, a));
		if(a->is_ability_usable(*displayed, *this) == false || state_ == STATE_SELECT_LOC) {
			b->set_disabled();
		}
		ability_grid->add_col(widget_ptr(b));
	}

	main_grid->add_widget(ability_grid);

	unit_profile_widget_ = main_grid;

	widgets_.insert(unit_profile_widget_);

/*
	foreach(boost::shared_ptr<gui::button> b, abilities_buttons_) {
		widgets_.erase(b);
	}

	abilities_buttons_.clear();

	if(moving_unit_.get() != NULL && state_ == STATE_PLAY_CARDS) {
		int ypos = 560;
		foreach(unit_ability_ptr a, moving_unit_->abilities()) {
			boost::shared_ptr<gui::button> b(new gui::button(gui::widget_ptr(new gui::label(a->name(), graphics::color_white())), boost::bind(&client_play_game::activate_ability, this, moving_unit_, a)));
			b->set_loc(880, ypos);
			abilities_buttons_.push_back(b);
			widgets_.insert(b);
			ypos -= 40;
		}
	}
	*/
}

void client_play_game::activate_ability(unit_ptr u, unit_ability_ptr a)
{
	try {
		wml::node_ptr result = a->use_ability(*u, *this);
		if(result.get() != NULL) {
			network::send(wml::output(result));
			network::send("[end_turn]\n[/end_turn]\n");

			moving_unit_.reset();
			current_routes_.reset();
		}
	} catch(action_cancellation_exception& e) {
	}

	set_abilities_buttons();
}

void client_play_game::build_avatars()
{
	std::vector<unit_avatar_ptr> other_avatars;
	std::map<int, unit_avatar_ptr> avatars_map;
	foreach(unit_avatar_ptr avatar, unit_avatars_) {
		if(avatar->get_unit()) {
			ASSERT_LOG(avatars_map.count(avatar->get_unit()->key()) == 0, "UNIT HAS MULTIPLE AVATARS: " << avatar->get_unit()->key());
			avatars_map[avatar->get_unit()->key()] = avatar;
		} else {
			other_avatars.push_back(avatar);
		}
	}

	unit_avatars_.clear();
	foreach(unit_ptr u, game_->units()) {
		unit_avatar_ptr avatar = avatars_map[u->key()];
		if(avatar) {
			avatar->set_unit(u);
			unit_avatars_.push_back(avatar);
		} else {
			unit_avatars_.push_back(unit_avatar_ptr(new unit_avatar(u)));
		}
	}

	unit_avatars_.insert(unit_avatars_.end(), other_avatars.begin(), other_avatars.end());
}

bool client_play_game::is_highlighted_loc(const hex::location& loc) const
{
	return current_routes_.get() && current_routes_->count(loc) ||
		   valid_loc_functor_ && valid_loc_functor_(loc);
}
