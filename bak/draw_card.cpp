#include "draw_card.hpp"
#include "font.hpp"
#include "raster.hpp"

void draw_card(const_card_ptr c, int x, int y)
{
	graphics::draw_rect(rect(x, y, 80, 100), graphics::color(64, 64, 64, 255));
	graphics::blit_texture(font::render_text(c->name(), graphics::color_white(), 14), x + 5, y + 5);
}
