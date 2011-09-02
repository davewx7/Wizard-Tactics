#ifndef TERRAIN_FWD_HPP_INCLUDED
#define TERRAIN_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

class terrain;

typedef boost::shared_ptr<terrain> terrain_ptr;
typedef boost::shared_ptr<const terrain> const_terrain_ptr;

#endif
