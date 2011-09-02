#include "asserts.hpp"
#include "foreach.hpp"
#include "raster.hpp"
#include "string_utils.hpp"
#include "unit_animation.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
const int FrameWidth = 36;
const int FrameHeight = 36;
}

unit_animation_team_colored::unit_animation_team_colored(wml::const_node_ptr node, int team_color)
  : length_(0),
    frame_width_(wml::get_int(node, "width", FrameWidth)),
    frame_height_(wml::get_int(node, "height", FrameHeight))
{
	ASSERT_LOG(node, "unit animation not found");

	const int xbase = wml::get_int(node, "x");
	const int ybase = wml::get_int(node, "y");

	graphics::texture t;
	
	if(team_color >= 0) {
		t = graphics::texture::get_team_color(node->attr("image").str(), team_color);
	} else {
		t = graphics::texture::get(node->attr("image").str());
	}

	const int row = wml::get_int(node, "row", 0);
	const int col = wml::get_int(node, "col", 0);
	const int nframes = wml::get_int(node, "frames", 1);
	std::vector<int> durations = vector_lexical_cast<int>(util::split(node->attr("duration")));
	if(durations.empty()) {
		durations.push_back(1);
	}
	
	for(int n = 0; n < nframes; ++n) {
		const int x = xbase + (col+n)*frame_width_;
		const int y = ybase + row*frame_height_;
		graphics::texture frame_texture = t.get_portion(x, y, x+frame_width_, y+frame_height_);
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

	x -= frame_width_;
	y -= frame_height_;

	GLfloat x1 = 0.0, x2 = 1.0;
	if(face_left) {
		x1 = 1.0;
		x2 = 0.0;
	}

	graphics::blit_texture(texture, x, y, texture.width()*2, texture.height()*2, 0.0, x1, 0.0, x2, 1.0);
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

const unit_animation_team_colored* unit_animation_team_colored::create_simple_anim(const std::string& img, int side)
{
	wml::node_ptr node(new wml::node("anim"));
	node->set_attr("image", img);
	return new unit_animation_team_colored(node, side);
}
