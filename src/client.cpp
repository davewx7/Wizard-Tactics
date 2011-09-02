#include <SDL.h>
#ifndef SDL_VIDEO_OPENGL_ES
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#if defined(__APPLE__) && !(TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
#include <OpenGL/OpenGL.h>
#endif

#include "asserts.hpp"
#include "client_network.hpp"
#include "client_play_game.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "framed_gui_element.hpp"
#include "gui_section.hpp"
#include "movement_type.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "terrain.hpp"
#include "texture.hpp"
#include "wml_parser.hpp"
#include "wml_writer.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <stdio.h>

int main(int argc, char** argv)
{
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	if (SDL_SetVideoMode(preferences::actual_screen_width(),preferences::actual_screen_height(),0,SDL_OPENGL|(preferences::fullscreen() ? SDL_FULLSCREEN : 0)) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}

	std::string nick = "david";
	std::string deck = "deck.cfg";

	bool is_host = false;
	bool is_joining = false;
	for(int n = 0; n != argc; ++n) {
		std::string arg(argv[n]);
		if(arg == "--host") {
			is_host = true;
			nick = "host";
		} else if(arg == "--join") {
			is_joining = true;
		} else if(arg == "--deck" && n+1 != argc) {
			++n;
			deck = argv[n];
		}
	}

	SDL_WM_SetCaption("Wizard", "Wizard");

	graphics::texture::manager texture_manager;
	terrain::init(wml::parse_wml_from_file("data/terrain.cfg"));
	font::manager font_manager;
	gui_section::init(wml::parse_wml_from_file("data/gui.cfg"));
	framed_gui_element::init(wml::parse_wml_from_file("data/gui.cfg"));
	movement_type::init(wml::parse_wml_from_file("data/move.cfg"));

	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef SDL_VIDEO_OPENGL_ES
	GLenum glew_status = glewInit();
	ASSERT_EQ(glew_status, GLEW_OK);
#endif

	network::manager network_manager;

	network::connect("localhost", "17000");

	network::send("[login]\nname=\"" + nick +"\"\n[/login]\n");

	if(is_joining) {
		network::send("[join_game]\n[/join_game]\n");
	} else if(is_host) {
		network::send("[create_game]\n[/create_game]\n");
		for(;;) {
			wml::const_node_ptr msg = network::receive();
			if(msg && msg->name() == "join_game") {
				break;
			}
			SDL_Delay(1000);
			std::cerr << "waiting...\n";
		}
		network::send("[setup]\n[/setup]\n");
	} else {
		network::send("[create_game]\nbots=\"1\"\n[/create_game]\n");
		network::send("[setup]\n[/setup]\n");
	}

	network::send(sys::read_file(deck));

	client_play_game(nick).play();
}
