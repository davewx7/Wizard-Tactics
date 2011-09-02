#ifndef UNIT_ANIMATION_HPP_INCLUDED
#define UNIT_ANIMATION_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <vector>

#include "texture.hpp"
#include "wml_node_fwd.hpp"

namespace game {

class unit_animation_team_colored {
public:
	unit_animation_team_colored(wml::const_node_ptr node, int team_color);
	void draw(int x, int y, int time=0, bool face_left=false) const;
	const graphics::texture& get_texture(int time=0) const;
	int length() const { return length_; }
private:
	struct Frame {
		int duration;
		graphics::texture texture;
	};

	std::vector<Frame> frames_;
	int length_;
};

class unit_animation {
public:
	explicit unit_animation(wml::const_node_ptr node);
	const unit_animation_team_colored& team_animation(unsigned int team_color) const;
private:
	mutable std::vector<boost::shared_ptr<unit_animation_team_colored> > team_anim_;
	wml::const_node_ptr definition_;
};

}

#endif
