#ifndef CITY_HPP_INCLUDED
#define CITY_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"

class city
{
public:
	explicit city(wml::const_node_ptr node);
	explicit city(const hex::location& loc);
	wml::node_ptr write() const;
	const hex::location& loc() const { return loc_; }
private:
	hex::location loc_;
};

typedef boost::shared_ptr<city> city_ptr;
typedef boost::shared_ptr<const city> const_city_ptr;

#endif
