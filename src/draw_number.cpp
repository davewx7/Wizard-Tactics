#include <string>

#include "draw_number.hpp"
#include "raster.hpp"
#include "texture.hpp"

namespace {
void blit_digit(const graphics::texture& t, char digit, int color_index, int xpos, int ypos) {
	const int offset = (digit - '0') * 9;
	graphics::blit_texture(t, xpos, ypos, 16, 14, 0.0,
	                       GLfloat(offset)/GLfloat(t.width()), 0.2*color_index,
	                       GLfloat(offset + 8)/GLfloat(t.width()), 0.2*(color_index+1));
}
}

void draw_number(int number, int places, int color_index, int xpos, int ypos)
{
	static const std::string Texture = "numbers.png";
	graphics::texture t = graphics::texture::get(Texture);

	char buf[128];
	sprintf(buf, "%d", number);
	const int skip = places - strlen(buf);
	if(skip > 0) {
		xpos += skip*12;
	}

	for(int n = 0; buf[n]; ++n, xpos += 12) {
		blit_digit(t, buf[n], color_index, xpos, ypos);
	}
}
