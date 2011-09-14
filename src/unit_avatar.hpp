#ifndef UNIT_AVATAR_HPP_INCLUDED
#define UNIT_AVATAR_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "geometry.hpp"
#include "unit.hpp"
#include "wml_node_fwd.hpp"

class unit_animation_set;

class unit_avatar
{
public:
	explicit unit_avatar(unit_ptr u);
	void draw(int time=0) const;
	void draw(int x, int y, int time=0) const;
	unit_ptr get_unit() { return unit_; }
	const_unit_ptr get_unit() const { return unit_; }
	void set_unit(unit_ptr u) { unit_ = u; }
	void set_path(const std::vector<hex::location>& path);

	void set_attack(const hex::location& attack_target);

	void process();
private:
	point get_position_between_tiles(const hex::location& a, const hex::location& b, int percent) const;

	unit_ptr unit_;
	const unit_animation_set* anim_;
	std::vector<hex::location> path_;
	std::vector<hex::location> arrow_;
	hex::location attack_target_;
	int time_in_path_;
};

typedef boost::shared_ptr<unit_avatar> unit_avatar_ptr;
typedef boost::shared_ptr<const unit_avatar> const_unit_avatar_ptr;

bool compare_unit_avatars_by_ypos(const_unit_avatar_ptr a, const_unit_avatar_ptr b);

#endif
