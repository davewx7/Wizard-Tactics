#include <iostream>

#include "filesystem.hpp"
#include "server.hpp"
#include "terrain.hpp"
#include "wml_parser.hpp"

int main()
{
	terrain::init(wml::parse_wml_from_file("data/terrain.cfg"));

	boost::asio::io_service io_service;
	server s(io_service);
	try {
		io_service.run();
	} catch(...) {
		std::cerr << "ERROR CAUGHT. EXITING\n";
	}
}
