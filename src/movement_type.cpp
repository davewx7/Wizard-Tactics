#include "asserts.hpp"
#include "movement_type.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace {
std::map<std::string, const_movement_type_ptr> cache;
}

void movement_type::init(wml::const_node_ptr node)
{
	FOREACH_WML_CHILD(move_node, node, "move") {
		cache[move_node->attr("id")].reset(new movement_type(move_node));
	}
}

const_movement_type_ptr movement_type::get(const std::string& id)
{
	if(id == "") {
		static const std::string DefaultStr = "default";
		return get(DefaultStr);
	}

	ASSERT_LOG(cache.count(id) == 1, "Unrecognized movement type: '" << id << "'");
	return cache[id];
}

movement_type::movement_type(wml::const_node_ptr node)
  : id_(node->attr("id"))
{
	default_.cost = 10;
	default_.armor = 0;
	default_.attack = 0;

	FOREACH_WML_CHILD(terrain_node, node, "terrain") {
		Entry& e = entries_[terrain_node->attr("terrain")];
		e.cost = wml::get_int(terrain_node, "cost", 10);
		e.armor = wml::get_int(terrain_node, "armor");
		e.attack = wml::get_int(terrain_node, "attack");
	}
}

int movement_type::movement_cost(const std::string& terrain_id) const
{
	return get_entry(terrain_id).cost;
}

int movement_type::armor_modification(const std::string& terrain_id) const
{
	return get_entry(terrain_id).armor;
}

int movement_type::attack_modification(const std::string& terrain_id) const
{
	return get_entry(terrain_id).attack;
}

const movement_type::Entry& movement_type::get_entry(const std::string& terrain_id) const
{
	std::map<std::string, Entry>::const_iterator itor = entries_.find(terrain_id);
	if(itor != entries_.end()) {
		return itor->second;
	}

	return default_;
}
