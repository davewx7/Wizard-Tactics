#ifndef UNIT_OVERLAY_HPP_INCLUDED
#define UNIT_OVERLAY_HPP_INCLUDED

#include <string>

#include "wml_node_fwd.hpp"

class unit_animation;

namespace unit_overlay
{

void init(wml::const_node_ptr node);

const unit_animation* get(const std::string& key);

};

#endif
