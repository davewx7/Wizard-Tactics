#include <GL/gl.h>

#include <boost/array.hpp>

#include <iostream>
#include <math.h>
#include <vector>

#include "draw_utils.hpp"
#include "foreach.hpp"
#include "hex_geometry.hpp"

namespace {

typedef boost::array<GLfloat, 2> Point;

void curve_through_hex(const hex::location& prev, const hex::location& current, const hex::location& next, std::vector<Point>& points)
{
	Point p0, p1, p2;
	p0[0] = (tile_center_x(prev) + tile_center_x(current))/2;
	p0[1] = (tile_center_y(prev) + tile_center_y(current))/2;
	p1[0] = tile_center_x(current);
	p1[1] = tile_center_y(current);
	p2[0] = (tile_center_x(next) + tile_center_x(current))/2;
	p2[1] = (tile_center_y(next) + tile_center_y(current))/2;

	for(GLfloat t = 0.0; t < 0.98; t += 0.02) {
		Point p;
		for(int n = 0; n != 2; ++n) {
			//formula for a bezier curve.
			p[n] = (1-t)*(1-t)*p0[n] + 2*(1-t)*t*p1[n] + t*t*p2[n];
		}
		points.push_back(p);
	}
}

void render_arrow(const std::vector<Point>& path, GLfloat alpha=1.0)
{
	if(path.empty()) {
		return;
	}

	const GLfloat PathLength = path.size()-1;

	std::vector<Point> left_path, right_path;
	for(int n = 0; n < path.size()-1; ++n) {
		const Point& p = path[n];
		const Point& next = path[n+1];

		Point direction;
		for(int m = 0; m != 2; ++m) {
			direction[m] = next[m] - p[m];
		}

		const GLfloat vector_length = sqrt(direction[0]*direction[0] + direction[1]*direction[1]);
		if(vector_length == 0.0) {
			continue;
		}

		Point unit_direction;
		for(int m = 0; m != 2; ++m) {
			unit_direction[m] = direction[m]/vector_length;
		}
		
		Point normal_direction_left, normal_direction_right;
		normal_direction_left[0] = -unit_direction[1];
		normal_direction_left[1] = unit_direction[0];
		normal_direction_right[0] = unit_direction[1];
		normal_direction_right[1] = -unit_direction[0];

		const GLfloat ratio = n/PathLength;

		GLfloat arrow_width = 12.0 - 7.0*ratio;

		const int ArrowHeadLength = 30;

		const int time_until_end = path.size()-2 - n;
		if(time_until_end < ArrowHeadLength) {
			arrow_width = time_until_end/2;
		}

		Point left, right;
		for(int m = 0; m != 2; ++m) {
			left[m] = p[m] + normal_direction_left[m]*arrow_width;
			right[m] = p[m] + normal_direction_right[m]*arrow_width;
		}

		left_path.push_back(left);
		right_path.push_back(right);
	}

	glBegin(GL_TRIANGLE_STRIP);
	for(int n = 0; n != left_path.size(); ++n) {
		if(n < 50) {
			glColor4f(0.8, 0.0, 0.0, alpha*(n/50.0));
		}
		const Point& p0 = left_path[n];
		const Point& p1 = right_path[n];
		glVertex3f(p0[0], p0[1], 0);
		glVertex3f(p1[0], p1[1], 0);
	}
	glEnd();

	glColor4f(0.0, 0.0, 0.0, alpha);
	glLineWidth(2);
	glBegin(GL_LINE_STRIP);
	foreach(const Point& p, right_path) {
		glVertex3f(p[0], p[1], 0);
	}
	glEnd();
	glBegin(GL_LINE_STRIP);
	foreach(const Point& p, left_path) {
		glVertex3f(p[0], p[1], 0);
	}
	glEnd();
}

}

void draw_arrow(const std::vector<hex::location>& hex_path)
{
	std::vector<hex::location> path = hex_path;
	if(path.size() < 2) {
		return;
	}

	//duplicate the end points so that we can draw an arrow into the starting and ending hexes.
	path.insert(path.begin(), path.front());
	path.push_back(path.back());
	
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	std::vector<Point> points;
	for(int n = 1; n < path.size() - 1; ++n) {
		curve_through_hex(path[n-1], path[n], path[n+1], points);
	}

	if(points.size() >= 50) {
		//cut the first and last 25 points off
		points.erase(points.begin(), points.begin() + 25);
		points.erase(points.end() - 25, points.end());
	}

	int ncount = 0;
	glDisable(GL_TEXTURE_2D);

	render_arrow(points);

	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POLYGON_SMOOTH);
}

void draw_arrow(int xfrom, int yfrom, int xto, int yto)
{
	std::vector<Point> v;
	Point p0, p1, p2;
	p0[0] = xfrom;
	p0[1] = yfrom;
	p2[0] = xto;
	p2[1] = yto;
	p1[0] = (p0[0] + p2[0])/2;
	p1[1] = p0[1];

	for(GLfloat t = 0.0; t < 0.98; t += 0.002) {
		Point p;
		for(int n = 0; n != 2; ++n) {
			//formula for a bezier curve.
			p[n] = (1-t)*(1-t)*p0[n] + 2*(1-t)*t*p1[n] + t*t*p2[n];
		}
		v.push_back(p);
	}

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glDisable(GL_TEXTURE_2D);

	render_arrow(v, 0.5);

	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POLYGON_SMOOTH);
}
