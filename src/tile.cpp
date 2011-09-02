#include <stdlib.h>

#include "asserts.hpp"
#include "string_utils.hpp"
#include "terrain.hpp"
#include "tile.hpp"

tile::tile()
{}

tile::tile(int x, int y, const std::string& data)
  : loc_(x, y)
{
	std::vector<std::string> v = util::split(data, ' ');
	ASSERT_GE(v.size(), 1);

	terrain_ = terrain::get(v.front());
	ASSERT_LOG(terrain_, "Could not find terrain " << v.front());
	if(v.size() > 1) {
		productivity_ = atoi(v[1].c_str());
	} else {
		productivity_ = terrain_->calculate_production_value();
	}
}

void tile::set_terrain(const std::string& id)
{
	terrain_ = terrain::get(id);
}

namespace {
int prandom(const hex::location& loc)
{
	const unsigned int a = (loc.x() + 92872873) ^ 918273;
	const unsigned int b = (loc.y() + 1672517) ^ 128123;
	return a*b + a + b;
}
}

const std::string& tile::texture() const
{
	return terrain_->texture();
}

const std::string& tile::overlay_texture() const
{
	return terrain_->overlay_texture();
}

const rect& tile::texture_area() const
{
	if(terrain_->texture_areas().empty()) {
		static const rect r(0,0,0,0);
		return r;
	}

	return terrain_->texture_areas()[prandom(loc_)%terrain_->texture_areas().size()];
}

const rect& tile::overlay_texture_area() const
{
	if(terrain_->overlay_texture_areas().empty()) {
		static const rect r(0,0,0,0);
		return r;
	}

	return terrain_->overlay_texture_areas()[prandom(loc_)%terrain_->overlay_texture_areas().size()];
}

void tile::write(std::string& str) const
{
	str += terrain_->id();
}
