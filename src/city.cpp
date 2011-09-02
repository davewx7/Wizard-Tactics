#include "city.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

city::city(wml::const_node_ptr node)
  : loc_(wml::get_int(node, "x"), wml::get_int(node, "y"))
{
}

city::city(const hex::location& loc) : loc_(loc)
{
}

wml::node_ptr city::write() const
{
	wml::node_ptr result = hex::write_location("city", loc_);
	return result;
}
