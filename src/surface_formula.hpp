#ifndef SURFACE_FORMULA_HPP_INCLUDED
#define SURFACE_FORMULA_HPP_INCLUDED

#include <string>
#include <vector>

#include <GL/gl.h>

#include "surface.hpp"

graphics::surface get_surface_formula(graphics::surface input, const std::string& algo);

GLuint get_gl_shader(const std::vector<std::string>& vertex_shader,
                     const std::vector<std::string>& fragment_shader);

graphics::surface get_surface_team_color(graphics::surface input, int color_index);

#endif
