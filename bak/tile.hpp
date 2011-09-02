#ifndef TILE_HPP_INCLUDED
#define TILE_HPP_INCLUDED

#include <string>

#include "terrain_fwd.hpp"
#include "tile_logic.hpp"

class tile
{
public:
	tile();
	tile(int x, int y, const std::string& data);

	void set_terrain(const std::string& id);

	const std::string& texture() const;
	const std::string& overlay_texture() const;

	const_terrain_ptr terrain() const { return terrain_; }

	void write(std::string& str) const;
private:
	hex::location loc_;
	const_terrain_ptr terrain_;
	int productivity_;
};

#endif
