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

const std::string& tile::texture() const
{
	return terrain_->texture();
}

const std::string& tile::overlay_texture() const
{
	return terrain_->overlay_texture();
}

void tile::write(std::string& str) const
{
	str += terrain_->id();
}
