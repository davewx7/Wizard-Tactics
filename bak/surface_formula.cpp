#include <iostream>
#include <map>
#include <SDL.h>
#ifndef SDL_VIDEO_OPENGL_ES
#include <GL/glew.h>
#endif

#include "asserts.hpp"
#include "concurrent_cache.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
//#include "formula.hpp"
//#include "formula_callable.hpp"
//#include "formula_function.hpp"
//#include "hi_res_timer.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "surface_formula.hpp"
#include "unit_test.hpp"

using namespace graphics;

namespace {

typedef std::pair<surface, std::string> cache_key;
typedef concurrent_cache<cache_key, surface> cache_map;
cache_map cache;

typedef std::pair<surface, int> team_color_cache_key;
typedef std::map<team_color_cache_key, surface> team_color_cache_map;
team_color_cache_map team_color_cache;
}

#if 0
class rgba_function : public function_expression {
public:
	explicit rgba_function(surface surf, const args_list& args)
	  : function_expression("rgba", args, 4), surf_(surf) {
	}

private:
	variant execute(const formula_callable& variables) const {
		return variant(SDL_MapRGBA(surf_->format,
		   Uint8(args()[0]->evaluate(variables).as_int()),
		   Uint8(args()[1]->evaluate(variables).as_int()),
		   Uint8(args()[2]->evaluate(variables).as_int()),
		   Uint8(args()[3]->evaluate(variables).as_int())));
	}
	surface surf_;
};

class surface_formula_symbol_table : public function_symbol_table
{
public:
	explicit surface_formula_symbol_table(surface surf) : surf_(surf)
	{}
	expression_ptr create_function(
	                  const std::string& fn,
	                  const std::vector<expression_ptr>& args,
					  const formula_callable_definition* callable_def) const;
private:
	surface surf_;
};

expression_ptr surface_formula_symbol_table::create_function(
                           const std::string& fn,
                           const std::vector<expression_ptr>& args,
						   const formula_callable_definition* callable_def) const
{
	if(fn == "rgba") {
		return expression_ptr(new rgba_function(surf_, args));
	} else {
		return function_symbol_table::create_function(fn, args, callable_def);
	}
}

class pixel_callable : public game_logic::formula_callable {
public:
	pixel_callable(const surface& surf, Uint32 pixel)
	{
		SDL_GetRGBA(pixel, surf->format, &r, &g, &b, &a);
		static const unsigned char AlphaPixel[] = {0x6f, 0x6d, 0x51};
		if(r == AlphaPixel[0] && g == AlphaPixel[1] && b == AlphaPixel[2]) {
			a = 0;
		}
	}

	bool is_alpha() const { return a == 0; }
private:
	variant get_value(const std::string& key) const {
		switch(*key.c_str()) {
		case 'r': return variant(r);
		case 'g': return variant(g);
		case 'b': return variant(b);
		case 'a': return variant(a);
		default: return variant();
		}
	}

	Uint8 r, g, b, a;
};

void run_formula(surface surf, const std::string& algo)
{
	const hi_res_timer timer("run_formula");

	const int ticks = SDL_GetTicks();
	surface_formula_symbol_table table(surf);
	game_logic::formula f(algo, &table);
	bool locked = false;
	if(SDL_MUSTLOCK(surf.get())) {
		const int res = SDL_LockSurface(surf.get());
		if(res == 0) {
			locked = true;
		}
	}

	std::map<Uint32, Uint32> pixel_map;

	Uint32* pixels = reinterpret_cast<Uint32*>(surf->pixels);
	Uint32* end_pixels = pixels + surf->w*surf->h;

	Uint32 AlphaPixel = SDL_MapRGBA(surf->format, 0x6f, 0x6d, 0x51, 0x0);

	int skip = 0;
	while(pixels != end_pixels) {
		if(((*pixels)&(~surf->format->Amask)) == AlphaPixel) {
			++pixels;
			continue;
		}
		std::map<Uint32, Uint32>::const_iterator itor = pixel_map.find(*pixels);
		if(itor == pixel_map.end()) {
			pixel_callable p(surf, *pixels);
			Uint32 result = f.execute(p).as_int();
			pixel_map[*pixels] = result;
			*pixels = result;
		} else {
			*pixels = itor->second;
		}
		++pixels;
	}

	if(locked) {
		SDL_UnlockSurface(surf.get());
	}
}

}

surface get_surface_formula(surface input, const std::string& algo)
{
	if(algo.empty()) {
		return input;
	}

	cache_key key(input, algo);
	surface surf = cache.get(key);
	if(surf.get() == NULL) {
		surf = input.clone();
		run_formula(surf, algo);
		cache.put(key, surf);
	}

	return surf;
}

#endif

surface get_surface_formula(surface input, const std::string& algo)
{
	return input;
}

namespace {
typedef std::map<std::pair<std::string, GLuint>, GLuint> shader_object_map;
shader_object_map shader_object_cache;

typedef std::map<std::pair<std::vector<std::string>,std::vector<std::string> >, GLuint> shader_map;
shader_map shader_cache;

void check_shader_errors(const std::string& fname, GLuint shader)
{
#ifndef SDL_VIDEO_OPENGL_ES
	GLint value = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &value);
	if(value == GL_FALSE) {
		char buf[1024*16];
		GLint len;
		glGetShaderInfoLog(shader, sizeof(buf), &len, buf);
		std::string errors(buf, buf + len);
		ASSERT_LOG(false, "COMPILE ERROR IN SHADER " << fname << ": " << errors);
	}
#endif
}

GLuint compile_shader(const std::string& shader_file, GLuint type)
{
	GLuint& id = shader_object_cache[std::make_pair(shader_file, type)];
	if(id) {
		return id;
	}
#ifndef SDL_VIDEO_OPENGL_ES
	id = glCreateShader(type);

	const std::string file_data = sys::read_file("data/shaders/" + shader_file);

	const char* file_str = file_data.c_str();
	glShaderSource(id, 1, &file_str, NULL);

	glCompileShader(id);
	check_shader_errors(shader_file, id);

	return id;
#else
	return 0;
#endif
}

}

GLuint get_gl_shader(const std::vector<std::string>& vertex_shader_file,
                     const std::vector<std::string>& fragment_shader_file)
{
#ifndef SDL_VIDEO_OPENGL_ES
	if(vertex_shader_file.empty() || fragment_shader_file.empty()) {
		return 0;
	}

	shader_map::iterator itor = shader_cache.find(std::make_pair(vertex_shader_file, fragment_shader_file));
	if(itor != shader_cache.end()) {
		return itor->second;
	}

	std::vector<GLuint> shader_objects;
	foreach(const std::string& shader_file, vertex_shader_file) {
		shader_objects.push_back(compile_shader(shader_file, GL_VERTEX_SHADER));
	}

	foreach(const std::string& shader_file, fragment_shader_file) {
		shader_objects.push_back(compile_shader(shader_file, GL_FRAGMENT_SHADER));
	}

	GLuint program_id = glCreateProgram();

	foreach(GLuint shader_id, shader_objects) {
		glAttachShader(program_id, shader_id);
	}

	glLinkProgram(program_id);

	GLint link_status = 0;
	glGetProgramiv(program_id, GL_LINK_STATUS, &link_status);
	if(link_status != GL_TRUE) {
		char buf[1024*16];
		GLint len;
		glGetProgramInfoLog(program_id, sizeof(buf), &len, buf);
		std::string errors(buf, buf + len);
		ASSERT_LOG(false, "LINK ERROR IN SHADER PROGRAM: " << errors);
	}

	shader_cache[std::make_pair(vertex_shader_file, fragment_shader_file)] = program_id;

	glUseProgram(0);

	return program_id;
#else
	return 0;
#endif
}

surface get_surface_team_color(surface input, int index)
{
	surface team_color_definition(surface_cache::get("team_color.png"));
	assert(team_color_definition.get());

	++index;
	assert(index < team_color_definition->w);

	team_color_cache_key key(input, index);
	surface& surf = team_color_cache[key];
	if(surf.get() != NULL) {
		return surf;
	}

	surf = input.clone();

	Uint32* src_range = reinterpret_cast<Uint32*>(team_color_definition->pixels);
	Uint32* end_src_range = src_range + team_color_definition->w;

	for(Uint32* src = src_range; src != end_src_range; ++src) {
		*src &= ~surf->format->Amask;
	}

	const Uint32* dst_range = src_range + team_color_definition->w*index;

	Uint32* pixels = reinterpret_cast<Uint32*>(surf->pixels);
	Uint32* end_pixels = pixels + surf->w*surf->h;
	while(pixels != end_pixels) {
		const Uint32* tc_pixel = std::find(src_range, end_src_range, *pixels & ~surf->format->Amask);
		if(tc_pixel != end_src_range) {
			const int i = tc_pixel - src_range;
			*pixels = (dst_range[i] & ~surf->format->Amask) + (*pixels & surf->format->Amask);
		}
		++pixels;
	}

	return surf;
}
