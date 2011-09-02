#include "client_play_game.hpp"
#include "draw_game.hpp"
#include "foreach.hpp"
#include "game.hpp"
#include "hex_geometry.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "tile.hpp"
#include "tile.hpp"

namespace {

void draw_tile(const client_play_game& info, const game& g, int xpos, int ypos, const hex::location& loc, const std::string& texture_id, bool selected)
{
	if(texture_id.empty()) {
		return;
	}

	const graphics::texture texture = graphics::texture::get(texture_id);
	graphics::blit_texture(texture, xpos + tile_pixel_x(loc) + HexWidth/2 - texture.width()/2, ypos + tile_pixel_y(loc) + HexHeight/2 - texture.height()/2);

	if(selected) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColor4f(1.0, 1.0, 1.0, 0.7 + sin(SDL_GetTicks()/250.0)*0.3);
		graphics::blit_texture(texture, xpos + tile_pixel_x(loc) + HexWidth/2 - texture.width()/2, ypos + tile_pixel_y(loc) + HexHeight/2 - texture.height()/2);

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

}

void draw_map(const client_play_game& info, const game& g, int xpos, int ypos)
{
	if(!g.started()) {
		return;
	}

	const hex::location selected = info.selected_loc();

	for(int x = 0; x != g.width(); ++x) {
		for(int y = 0; y != g.height(); ++y) {
			const tile* t = g.get_tile(x, y);
			if(!t) {
				continue;
			}

			const hex::location loc(x, y);

			draw_tile(info, g, xpos, ypos, loc, t->texture(), loc == selected);
		}
	}

	for(int x = 0; x != g.width(); ++x) {
		for(int y = 0; y != g.height(); ++y) {
			const tile* t = g.get_tile(x, y);
			if(!t) {
				continue;
			}

			const hex::location loc(x, y);
			draw_tile(info, g, xpos, ypos, loc, t->overlay_texture(), loc == selected);
		}
	}

	foreach(city_ptr c, g.cities()) {
		draw_tile(info, g, xpos, ypos, c->loc(), "terrain/house1.png", c->loc() == selected);
	}
}
