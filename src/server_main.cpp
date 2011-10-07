#include <iostream>

#include "filesystem.hpp"
#include "movement_type.hpp"
#include "server.hpp"
#include "terrain.hpp"
#include "web_server.hpp"
#include "wml_parser.hpp"

int main()
{
	terrain::init(wml::parse_wml_from_file("data/terrain.xml"));
	movement_type::init(wml::parse_wml_from_file("data/move.xml"));

	boost::asio::io_service io_service;
	server s(io_service);
	web_server ws(io_service, s);
	io_service.run();
}
