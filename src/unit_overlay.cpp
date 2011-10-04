#include <boost/shared_ptr.hpp>
#include <map>

#include "filesystem.hpp"
#include "unit_animation.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"

namespace unit_overlay
{

namespace {
std::map<std::string, boost::shared_ptr<unit_animation> > cache;
}

void init(wml::const_node_ptr node)
{
	FOREACH_WML_CHILD(overlay_node, node, "overlay") {
		cache[overlay_node->attr("id").str()].reset(new unit_animation(overlay_node));
	}
}

const unit_animation* get(const std::string& key)
{
	if(cache.empty()) {
		init(wml::parse_wml_from_file("data/unit_overlays.xml"));
	}

	return cache[key].get();
}

}
