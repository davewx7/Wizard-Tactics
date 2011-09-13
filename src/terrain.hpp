#ifndef TERRAIN_HPP_INCLUDED
#define TERRAIN_HPP_INCLUDED

#include <string>
#include <vector>

#include "geometry.hpp"
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
	const std::vector<rect>& texture_areas() const { return texture_areas_; }
	const std::vector<rect>& overlay_texture_areas() const { return overlay_texture_areas_; }
	const std::string& resource() const { return resource_; }
	int resource_id() const { return resource_id_; }
	int calculate_production_value() const;

	int unit_y_offset() const { return unit_y_offset_; }

private:
	std::string id_, resource_;

	std::string texture_, overlay_texture_;

	std::vector<rect> texture_areas_, overlay_texture_areas_;

	int resource_id_;

	int production_difficulty_;
	int overlay_order_;

	int unit_y_offset_;
};

#endif
