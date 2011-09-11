#include "client_play_game.hpp"
#include "draw_game.hpp"
#include "foreach.hpp"
#include "game.hpp"
#include "gui_section.hpp"
#include "hex_geometry.hpp"
#include "raster.hpp"
#include "texture.hpp"
#include "tile.hpp"
#include "tile.hpp"

namespace {

void draw_tile(const client_play_game& info, const game& g, int xpos, int ypos, const hex::location& loc, const std::string& texture_id, const rect& area, bool selected)
{
	if(texture_id.empty() || area.w() == 0) {
		return;
	}

	const graphics::texture texture = graphics::texture::get(texture_id);

	const GLfloat TileEpsilon = 0.01;
	const GLfloat x1 = GLfloat(area.x())/GLfloat(texture.width());
	const GLfloat x2 = GLfloat(area.x() + area.w() - TileEpsilon)/GLfloat(texture.width());
	const GLfloat y1 = GLfloat(area.y())/GLfloat(texture.height());
	const GLfloat y2 = GLfloat(area.y() + area.h() - TileEpsilon)/GLfloat(texture.height());

	graphics::blit_texture(texture,
	   xpos + tile_pixel_x(loc) + HexWidth/2 - area.w(),
	   ypos + tile_pixel_y(loc) + HexHeight - area.h()*2,
	   area.w()*2, area.h()*2, 0, x1, y1, x2, y2);

	if(selected) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glColor4f(1.0, 1.0, 1.0, 0.7 + sin(SDL_GetTicks()/250.0)*0.3);
		graphics::blit_texture(texture,
		   xpos + tile_pixel_x(loc) + HexWidth/2 - area.w(),
		   ypos + tile_pixel_y(loc) + HexHeight - area.h()*2,
		   area.w()*2, area.h()*2, 0, x1, y1, x2, y2);

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

	for(int y = 0; y != g.height(); ++y) {
		for(int x = 0; x != g.width(); ++x) {
			const tile* t = g.get_tile(x, y);
			if(!t) {
				continue;
			}

			const hex::location loc(x, y);

			draw_tile(info, g, xpos, ypos, loc, t->texture(), t->texture_area(), loc == selected || info.is_highlighted_loc(loc));

			char tower_resource = 0;
			const int tower_owner = g.tower_owner(loc, &tower_resource);
			if(tower_owner != -1) {
				std::cerr << "TOWER_OWNER: " << tower_owner << " " << tower_resource << "\n";
				rect area;
				switch(tower_owner) {
				case 0: area = rect(46, 7, 31, 20); break;
				case 1: area = rect(78, 7, 31, 20); break;
				}

				draw_tile(info, g, xpos, ypos, loc, "terrain/towers.png", area, loc == selected || info.is_highlighted_loc(loc));

				char buf[128];
				sprintf(buf, "magic-icon-%c", tower_resource);

				const const_gui_section_ptr section = gui_section::get(buf);
				section->blit(xpos + tile_pixel_x(loc) + 24, ypos + tile_pixel_y(loc) + 10);
			}
		}
	}

	for(int y = 0; y != g.height(); ++y) {
		for(int x = 0; x != g.width(); ++x) {
			const tile* t = g.get_tile(x, y);
			if(!t) {
				continue;
			}

			const hex::location loc(x, y);
			draw_tile(info, g, xpos, ypos, loc, t->overlay_texture(), t->overlay_texture_area(), loc == selected);
		}
	}
}
