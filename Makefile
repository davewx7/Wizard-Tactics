objects = ai_player.o card.o city.o debug_console_noop.o filesystem.o formula_callable_definition.o formula_constants.o formula_function.o formula.o formula_tokenizer.o formula_variable_storage.o game.o game_formula_functions.o game_utils.o geometry.o movement_type.o pathfind.o player_info.o preprocessor.o random.o resource.o server.o server_main.o simple_wml.o string_utils.o terrain.o thread.o tile.o tile_logic.o unit.o unit_ability.o unit_test.o unit_utils.o variant.o wml_formula_adapter.o wml_formula_callable.o wml_modify.o wml_node.o wml_parser.o wml_parser_test.o wml_preprocessor.o wml_schema.o wml_utils.o wml_writer.o xml_parser.o xml_writer.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o web_server.o
client_objects = ai_player.o button.o card.o city.o client.o client_network.o client_play_game.o color_utils.o debug_console.o dialog.o draw_card.o draw_game.o draw_number.o draw_utils.o filesystem.o font.o formula_callable_definition.o formula_constants.o formula_function.o formula.o formula_tokenizer.o formula_variable_storage.o framed_gui_element.o game.o game_formula_functions.o game_utils.o geometry.o grid_widget.o gui_section.o hex_geometry.o image_widget.o input.o iphone_controls.o key.o label.o movement_type.o pathfind.o preferences.o preprocessor.o random.o raster.o rectangle_rotator.o resource.o scrollbar_widget.o scrollable_widget.o simple_wml.o string_utils.o surface.o surface_cache.o surface_formula.o surface_palette.o surface_scaling.o terrain.o texture.o thread.o tile.o tile_logic.o tooltip.o translate.o unit.o unit_ability.o unit_animation.o unit_avatar.o unit_overlay.o unit_test.o unit_utils.o utils.o variant.o widget.o wml_formula_adapter.o wml_formula_callable.o wml_modify.o wml_node.o wml_parser.o wml_preprocessor.o wml_schema.o wml_utils.o wml_writer.o xml_parser.o xml_writer.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o IMG_savepng.o

%.o : src/%.cpp
	ccache g++ `sdl-config --cflags` -fno-inline-functions -g $(OPT) -DTIXML_USE_STL=1 -D_GNU_SOURCE=1 -D_REENTRANT -DIMPLEMENT_SAVE_PNG=1 -Wnon-virtual-dtor -Wreturn-type -fthreadsafe-statics -c $<

game: $(objects)
	ccache g++ `sdl-config --libs` -fno-inline-functions -g $(OPT) -L/sw/lib -L/usr/X11R6/lib -lX11 -DTIXML_USE_STL=1 -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -L/usr/lib `sdl-config --libs` -lSDLmain -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -lboost_iostreams-mt -fthreadsafe-statics $(objects) -o game


client: $(client_objects)
	g++ -DCLIENT=1 `sdl-config --libs` -fno-inline-functions -g $(OPT) -L/sw/lib -L/usr/X11R6/lib -lX11 -DTIXML_USE_STL=1 -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -L/usr/lib `sdl-config --libs` -lpng -lSDLmain -lSDL -lGL -lGLU -lGLEW -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -lboost_iostreams-mt -fthreadsafe-statics $(client_objects) -o client

clean:
	rm *.o game client
