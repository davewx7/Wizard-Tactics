#include "debug_console.hpp"
#include "game.hpp"
#include "wml_node.hpp"
#include "xml_writer.hpp"

namespace debug_console
{
void add_message(const std::string& msg)
{
	wml::node_ptr node(new wml::node("debug_msg"));
	node->set_attr("msg", msg);
	game::current()->queue_message(wml::output_xml(node));
}

void draw()
{
}

}
