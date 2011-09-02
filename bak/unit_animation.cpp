#include "asserts.hpp"
#include "foreach.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "unit_animation.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace game {

namespace {
const int FrameWidth = 72;
const int FrameHeight = 72;
}

unit_animation_team_colored::unit_animation_team_colored(wml::const_node_ptr node, int team_color)
  : length_(0)
{
	ASSERT_LOG(node, "unit animation not found");

	graphics::texture t = graphics::texture::get_team_color(std::string("/units/") + node->attr("image").str(), team_color);
	const int row = wml::get_int(node, "row", 0);
	const int col = wml::get_int(node, "col", 0);
	const int nframes = wml::get_int(node, "frames", 1);
	std::vector<int> durations = vector_lexical_cast<int>(util::split(node->attr("duration")));
	if(durations.empty()) {
		durations.push_back(1);
	}
	
	for(int n = 0; n < nframes; ++n) {
		const int x = (col+n)*FrameWidth;
		const int y = row*FrameHeight;
		graphics::texture frame_texture = t.get_portion(x, y, x+FrameWidth, y+FrameHeight);
		Frame frame;
		frame.duration = n < durations.size() ? durations[n] : durations.back();
		frame.texture = frame_texture;
		frames_.push_back(frame);
		length_ += frame.duration;
	}
}

void unit_animation_team_colored::draw(int x, int y, int time, bool face_left) const
{
	const graphics::texture& texture = get_texture(time);

	GLfloat x1 = 0.0, x2 = 1.0;
	if(face_left) {
		x1 = 1.0;
		x2 = 0.0;
	}

	graphics::blit_texture(texture, x, y, texture.width(), texture.height(), 0.0, x1, 0.0, x2, 1.0);
}

const graphics::texture& unit_animation_team_colored::get_texture(int time) const
{
	if(frames_.empty()) {
		static graphics::texture empty_texture;
		return empty_texture;
	}

	int index = 0;
	while(index != frames_.size()-1 && time > 0) {
		time -= frames_[index].duration;
		++index;
	}

	return frames_[index].texture;
}

unit_animation::unit_animation(wml::const_node_ptr node)
  : definition_(node)
{}

const unit_animation_team_colored& unit_animation::team_animation(unsigned int team_color) const
{
	if(team_color >= team_anim_.size()) {
		team_anim_.resize(team_color+1);
	}

	if(!team_anim_[team_color]) {
		team_anim_[team_color] = boost::shared_ptr<unit_animation_team_colored>(new unit_animation_team_colored(definition_, team_color));
	}

	return *team_anim_[team_color];
}

}
