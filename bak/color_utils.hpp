#ifndef COLOR_UTILS_HPP_INCLUDED
#define COLOR_UTILS_HPP_INCLUDED

#include "SDL.h"
#include "GL/gl.h"

#include <algorithm>
#include <string>
#include <vector>

SDL_Color string_to_color(const std::string& str);


namespace graphics {
class color
{
	public:
	uint8_t r() const {return c_.rgba[0]; }
	uint8_t g() const {return c_.rgba[1]; }
	uint8_t b() const {return c_.rgba[2]; }
	uint8_t a() const {return c_.rgba[3]; }
	uint32_t rgba() const {return convert_pixel_byte_order(c_).value; }
	//TODO: color to return string version

	uint32_t value() const { return c_.value; }
	
	color( int r, int g, int b, int a);
	explicit color( uint32_t rgba = 0);
	explicit color( const std::string& str);
	
	void set_as_current_color() const;
	void add_to_vector(std::vector<GLfloat>* v) const;

	union PixelUnion {
		uint32_t value;
		uint8_t rgba[4];
	};

		
	static PixelUnion convert_pixel_byte_order(PixelUnion p) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		std::reverse(p.rgba, p.rgba + sizeof(p.rgba));
#endif
		return p;
	}

	private:
		PixelUnion c_;
};

class color_transform
{
public:
	color_transform(const color& c);
	color_transform(uint16_t r, uint16_t g, uint16_t b, uint16_t a);
	explicit color_transform(const std::string& str);

	std::string to_string() const;

	uint16_t r() const { return rgba_[0]; }
	uint16_t g() const { return rgba_[1]; }
	uint16_t b() const { return rgba_[2]; }
	uint16_t a() const { return rgba_[3]; }

	uint16_t* buf() { return rgba_; }
	const uint16_t* buf() const { return rgba_; }
	
	color to_color() const;

	//whether this fits in a color object.
	bool fits_in_color() const;
private:
	uint16_t rgba_[4];
};

color_transform operator+(const color_transform& a, const color_transform& b);
color_transform operator-(const color_transform& a, const color_transform& b);

}
#endif
