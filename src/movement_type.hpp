#ifndef MOVEMENT_TYPE_HPP_INCLUDED
#define MOVEMENT_TYPE_HPP_INCLUDED

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "wml_node_fwd.hpp"

class movement_type;

typedef boost::shared_ptr<const movement_type> const_movement_type_ptr;

class movement_type
{
public:
	static void init(wml::const_node_ptr node);
	static const_movement_type_ptr get(const std::string& id);

	const std::string& id() const { return id_; }
	int movement_cost(const std::string& terrain_id) const;
	int armor_modification(const std::string& terrain_id) const;
	int attack_modification(const std::string& terrain_id) const;
private:

	explicit movement_type(wml::const_node_ptr node);
	std::string id_;

	struct Entry {
		int cost;
		int armor;
		int attack;
	};

	const Entry& get_entry(const std::string& terrain_id) const;

	Entry default_;
	std::map<std::string, Entry> entries_;
};

#endif
