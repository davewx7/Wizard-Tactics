#ifndef CLIENT_NETWORK_HPP_INCLUDED
#define CLIENT_NETWORK_HPP_INCLUDED

#include "wml_node_fwd.hpp"

namespace network
{
struct manager {
	manager();
	~manager();
};

struct network_error {};
void connect(const std::string& hostname, const std::string& port);
void send(const std::string& msg);
void send(wml::const_node_ptr node);
wml::const_node_ptr receive();

}

#endif
