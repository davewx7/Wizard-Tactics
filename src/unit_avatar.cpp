#include <map>

#include "color_utils.hpp"
#include "debug_console.hpp"
#include "draw_number.hpp"
#include "draw_utils.hpp"
#include "game.hpp"
#include "gui_section.hpp"
#include "hex_geometry.hpp"
#include "raster.hpp"
#include "unit_animation.hpp"
#include "unit_avatar.hpp"
#include "unit_overlay.hpp"
#include "unit_utils.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"

struct unit_animation_set
{
	unit_animation_set(wml::const_node_ptr node, int color)
	  : standing(new unit_animation_team_colored(node->get_child("stand"), color))
	{}
	boost::shared_ptr<unit_animation_team_colored> standing;
};

namespace {
typedef boost::shared_ptr<unit_animation_set> anim_set_ptr;
typedef boost::shared_ptr<const unit_animation_set> const_anim_set_ptr;

std::map<std::pair<std::string, int>, const_anim_set_ptr> anim_set_cache;

const_anim_set_ptr get_animations_for_unit(const std::string& id, int color)
{
	const_anim_set_ptr& a = anim_set_cache[std::make_pair(id, color)];
	if(a.get() == NULL) {
		a.reset(new unit_animation_set(wml::parse_wml_from_file("data/units/" + id + ".cfg"), color));
	}

	return a;
}

void draw_hitpoints(int x, int y, int hp, int max_hp)
{
	const graphics::color border_color(0xff, 0xc2, 0x0e, 0xff);

	const int BarWidth = 5;
	
	graphics::draw_rect(rect(x-1, y, 1, max_hp*5), graphics::color(0,0,0,255));
	graphics::draw_rect(rect(x, y, 1, max_hp*5), border_color);
	graphics::draw_rect(rect(x+BarWidth+1, y, 1, max_hp*5), border_color);
	graphics::draw_rect(rect(x+BarWidth+2, y, 1, max_hp*5), graphics::color(0x4c, 0x17, 0x11, 0xff));

	graphics::draw_rect(rect(x+1, y-2, BarWidth, 1), graphics::color(0,0,0,255));
	graphics::draw_rect(rect(x, y-1, BarWidth+2, 1), border_color);
	graphics::draw_rect(rect(x, y+max_hp*5, BarWidth+2, 1), border_color);
	graphics::draw_rect(rect(x+1, y+max_hp*5+1, BarWidth, 1), graphics::color(0,0,0,255));

	graphics::draw_rect(rect(x+1, y, BarWidth, (max_hp-hp)*5), graphics::color(0, 0, 0, 255));
	graphics::draw_rect(rect(x+1, y + (max_hp-hp)*5, BarWidth, hp*5), graphics::color(0xef, 0x34, 0x3e, 0xff));

	for(int n = 1; n < max_hp; ++n) {
		graphics::draw_rect(rect(x+1, y + n*5 - 1, BarWidth, 2), graphics::color(0, 0, 0, 128));
	}
}

}

unit_avatar::unit_avatar(unit_ptr u)
 : unit_(u),
   anim_(get_animations_for_unit(u->id(), u->side()).get()),
   dead_(false), time_in_path_(0)
{}

void unit_avatar::draw(int time) const
{
	int x = tile_center_x(unit_->loc());
	int y = tile_center_y(unit_->loc()) + game::current()->get_tile(unit_->loc())->unit_y_offset();
	if(path_.empty() == false) {
		draw_arrow(arrow_);
		const hex::location a = path_.back();
		const hex::location b = path_.size() > 1 ? path_[path_.size()-2] : path_.back();

		const point pos = get_position_between_tiles(a, b, time_in_path_*10);
		x = pos.x;
		y = pos.y;
	} else if(attack_target_.valid()) {
		const int percent = time_in_path_ < 10 ? time_in_path_*5 : (20 - time_in_path_)*5;
		const point pos = get_position_between_tiles(unit_->loc(), attack_target_, percent);
		x = pos.x;
		y = pos.y;
	}

	y -= 18;

	if(dead_) {
		glColor4f(1.0, 1.0, 1.0, 1.0 - time_in_path_/10.0);
	}

	draw(x, y, time);

	if(dead_) {
		glColor4f(1.0, 1.0, 1.0, 1.0);
	}
}

point unit_avatar::get_position_between_tiles(const hex::location& a, const hex::location& b, int percent) const
{
	point res;
	res.x = (tile_center_x(a)*(100 - percent) + tile_center_x(b)*percent)/100;
	res.y = ((tile_center_y(a) + game::current()->get_tile(a)->unit_y_offset())*(100 - percent) +
		    (tile_center_y(b) + game::current()->get_tile(b)->unit_y_offset())*percent)/100;
	
	return res;
}

void unit_avatar::draw(int x, int y, int time) const
{
	const bool face_left = unit_->side()&1;
	foreach(const std::string& overlay, unit_->underlays()) {
		const unit_animation* anim = unit_overlay::get(overlay);
		if(anim != NULL) {
			anim->team_animation(0).draw(x, y, 0, face_left);
		}
	}

	anim_->standing->draw(x, y, anim_->standing->length() == 0 ? 0 : (time%anim_->standing->length()), face_left);

	foreach(const std::string& overlay, unit_->overlays()) {
		const unit_animation* anim = unit_overlay::get(overlay);
		if(anim != NULL) {
			anim->team_animation(0).draw(x, y, 0, face_left);
		}
	}

	static const graphics::texture tired = graphics::texture::get("tired.png");

	if(unit_->has_moved()) {
		graphics::blit_texture(tired, x - (face_left ? 20 : 50), y - 36, tired.width()*2, tired.height()*2);
	}

	draw_hitpoints(x + (face_left ? 22 : -30), y - 20, unit_->life() - unit_->damage_taken(), unit_->life());

	int armor = unit_protection(*game::current(), unit_);
	if(armor != 0) {
		const_gui_section_ptr section = gui_section::get(armor > 0 ? "defense-icon" : "vulnerable-icon");
		if(armor < 0) {
			armor = -armor;
		}

		for(int n = 0; n != armor; ++n) {
			section->blit(x - 24, y - 10 + n*6, section->width()/2, section->height()/2);
		}
	}
}

void unit_avatar::set_path(const std::vector<hex::location>& path)
{
	path_ = path;
	arrow_ = path;
	std::reverse(arrow_.begin(), arrow_.end());
	time_in_path_ = 0;
}

void unit_avatar::set_attack(const hex::location& attack_target)
{
	attack_target_ = attack_target;
	time_in_path_ = 0;
}

void unit_avatar::set_dead()
{
	dead_ = true;
	path_.clear();
	attack_target_ = hex::location();
	time_in_path_ = 0;
}

void unit_avatar::process()
{
	if(path_.empty() == false) {
		++time_in_path_;
		if(time_in_path_ >= 10) {
			time_in_path_ = 0;
			path_.pop_back();
		}
	}
	
	if(attack_target_.valid()) {
		++time_in_path_;
		if(time_in_path_ >= 20) {
			time_in_path_ = 0;
			attack_target_ = hex::location();
		}
	}

	if(dead_) {
		++time_in_path_;
	}
}

bool compare_unit_avatars_by_ypos(const_unit_avatar_ptr a, const_unit_avatar_ptr b)
{
	return a->get_unit()->loc().y() < b->get_unit()->loc().y();
}
