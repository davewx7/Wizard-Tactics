#ifndef TERRAIN_HPP_INCLUDED
#define TERRAIN_HPP_INCLUDED

#include <string>
#include <vector>

#include "terrain_fwd.hpp"
#include "wml_node_fwd.hpp"

class terrain
{
public:
	static void init(wml::const_node_ptr node);
	static const_terrain_ptr get(const std::string& id);
	static std::vector<std::string> all();

	explicit terrain(wml::const_node_ptr node);

	const std::string& id() const { return id_; }
	const std::string& texture() const { return texture_; }
	const std::string& overlay_texture() const { return overlay_texture_; }
	const std::string& resource() const { return resource_; }
	int calculate_production_value() const;
private:
	std::string id_, texture_, overlay_texture_, resource_;

	int production_difficulty_;
	int overlay_order_;
};

#endif
