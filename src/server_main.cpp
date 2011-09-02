#include <iostream>

#include "filesystem.hpp"
#include "movement_type.hpp"
#include "server.hpp"
#include "terrain.hpp"
#include "wml_parser.hpp"

int main()
{
	terrain::init(wml::parse_wml_from_file("data/terrain.cfg"));
	movement_type::init(wml::parse_wml_from_file("data/move.cfg"));

	boost::asio::io_service io_service;
	server s(io_service);
	io_service.run();
}
