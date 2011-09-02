#include <sstream>
#include <string>
#include <vector>
#include <GL/gl.h>

#include "color_utils.hpp"
#include "string_utils.hpp"
#include "unit_test.hpp"
#include "utils.hpp"

SDL_Color string_to_color(const std::string& str)
{
	SDL_Color res = {0,0,0,0};
	if(str.size() == 6) {
		unsigned int num = strtol(str.c_str(), NULL, 16);
		res.r = (num >> 16)&0xFF;
		res.g = (num >> 8)&0xFF;
		res.b = num&0xFF;
	}
	return res;
}

namespace graphics {

color::color( int r, int g, int b, int a)
{
	c_.rgba[0] = truncate_to_char(r);
	c_.rgba[1] = truncate_to_char(g);
	c_.rgba[2] = truncate_to_char(b);
	c_.rgba[3] = truncate_to_char(a);
}

color::color( uint32_t rgba)
{
	c_.value = rgba;
	c_ = convert_pixel_byte_order(c_);
}

color::color( const std::string& str)
{
	std::vector<std::string> components;
	components = util::split(str);
	
	if( components.size() == 4){
		int i=0;
		for( std::vector<std::string>::iterator itor = components.begin();
			itor != components.end(); ++itor)
		{
			c_.rgba[i] = atoi(itor->c_str());
			++i;
		}
	} else if ( components.size() == 3){ //assume that the fourth value, alpha, is 255
		int i=0;
		for( std::vector<std::string>::iterator itor = components.begin();
			itor != components.end(); ++itor)
		{
				c_.rgba[i] = atoi(itor->c_str());
				++i;
		}
		c_.rgba[3] = 255; //no need to read the string element, b/c there isn't one
	} else {
		c_.value = 0;
	}
}

void color::set_as_current_color() const
{
	glColor4ub(c_.rgba[0], c_.rgba[1], c_.rgba[2], c_.rgba[3]);
}

void color::add_to_vector(std::vector<GLfloat>* v) const
{
	v->push_back(r()/255.0);
	v->push_back(g()/255.0);
	v->push_back(b()/255.0);
	v->push_back(a()/255.0);
}

color_transform::color_transform(const color& c)
{
	rgba_[0] = c.r();
	rgba_[1] = c.g();
	rgba_[2] = c.b();
	rgba_[3] = c.a();
}

color_transform::color_transform(uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
	rgba_[0] = r;
	rgba_[1] = g;
	rgba_[2] = b;
	rgba_[3] = a;
}

color_transform::color_transform(const std::string& str)
{
	std::fill(rgba_, rgba_ + 4, 255);
	std::vector<std::string> components = util::split(str);
	for(int n = 0; n != components.size(); ++n) {
		if(n < 4) {
			rgba_[n] = atoi(components[n].c_str());
		}
	}
}

std::string color_transform::to_string() const
{
	std::ostringstream s;
	for(int n = 0; n != 4; ++n) {
		if(n) {
			s << ",";
		}

		s << rgba_[n];
	}

	return s.str();
}

namespace {
uint16_t clip(uint16_t val) {
	return val > 255 ? 255 : val;
	if(val > 255) {
		val = 255;
	}

	return val;
}
}

color color_transform::to_color() const
{
	return color(clip(r()),clip(g()),clip(b()),clip(a()));
}

bool color_transform::fits_in_color() const
{
	for(int n = 0; n != 4; ++n) {
		if(rgba_[n] > 255) {
			return false;
		}
	}

	return true;
}

color_transform operator+(const color_transform& a, const color_transform& b)
{
	return color_transform(a.r() + b.r(), a.g() + b.g(), a.b() + b.b(), std::max(a.a(), b.a()));
}

color_transform operator-(const color_transform& a, const color_transform& b)
{
	color_transform result = a;
	for(int n = 0; n != 3; ++n) {
		if(result.buf()[n] > b.buf()[n]) {
			result.buf()[n] -= b.buf()[n];
		} else {
			result.buf()[n] = 0;
		}
	}

	if(result.buf()[3] > 255) {
		result.buf()[3] = 255;
	}

	return result;
}

UNIT_TEST(color)
{
	//check that a value plugged into the color is returned when we
	//ask for it.
	CHECK_EQ(color(758492).rgba(), 758492);

	//check that an rgba value is equal to an expected int value.
	CHECK_EQ(color(255,255,0,255).rgba(), color(0xFFFF00FF).rgba());
	
	//check that parsing from a string works properly
	CHECK_EQ(color("255,128,255,128").rgba(), color(255,128,255,128).rgba());

	//it would be nice to be able to specify a string with three instead of
	//four values, and have it just assume 255 for the alpha.
	CHECK_EQ(color("255,128,255").rgba(), color(255,128,255,255).rgba());
}

}
