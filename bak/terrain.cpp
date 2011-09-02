#include <iostream>

#include "terrain.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
std::map<std::string, terrain_ptr> terrains;
int next_overlay_order = 1;
}

std::vector<std::string> terrain::all()
{
	std::vector<std::string> res;
	for(std::map<std::string, terrain_ptr>::const_iterator i = terrains.begin(); i != terrains.end(); ++i) {
		res.push_back(i->first);
	}

	return res;
}

void terrain::init(wml::const_node_ptr node)
{
	FOREACH_WML_CHILD(terrain_node, node, "terrain") {
		terrain_ptr t(new terrain(terrain_node));
		terrains[t->id()] = t;
	}
}

const_terrain_ptr terrain::get(const std::string& key)
{
	std::map<std::string, terrain_ptr>::const_iterator itor = terrains.find(key);
	if(itor != terrains.end()) {
		return itor->second;
	} else {
		return const_terrain_ptr();
	}
}

terrain::terrain(wml::const_node_ptr node)
  : id_(node->attr("id")),
    texture_(node->attr("image")),
    overlay_texture_(node->attr("overlay_image")),
    resource_(node->attr("resource")),
	production_difficulty_(wml::get_int(node, "difficulty")),
	overlay_order_(next_overlay_order++)
{
	std::cerr << "LOAD TERRAIN: " << id_ << "\n";
}

int terrain::calculate_production_value() const
{
	return production_difficulty_ + rand()%6 + 1;
}
