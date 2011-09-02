#include <string>
#include <SDL.h>

#include "preferences.hpp"

namespace preferences {
	namespace {
		bool no_sound_ = false;
		bool show_debug_hitboxes_ = false;
		bool use_pretty_scaling_ = false;
		bool fullscreen_ = false;
		
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
		int virtual_screen_width_ = 960;
		int virtual_screen_height_ = 640;
		
		int actual_screen_width_ = 320;
		int actual_screen_height_ = 480;

		bool screen_rotated_ = true;
		
		const char *save_file_path_ = "../Documents/save.cfg";
#else
		int virtual_screen_width_ = 1024;
		int virtual_screen_height_ = 768;
		
		int actual_screen_width_ = 1024;
		int actual_screen_height_ = 768;
		
		bool screen_rotated_ = false;
		
		const char *save_file_path_ = "data/level/save.cfg";
#endif

		bool load_compiled_ = false;
		
		bool force_no_npot_textures_ = false;
	}

	int xypos_draw_mask = actual_screen_width_ < virtual_screen_width_ ? ~1 : ~0;
	bool compiling_tiles = false;

	namespace {
	void recalculate_draw_mask() {
		xypos_draw_mask = actual_screen_width_ < virtual_screen_width_ ? ~1 : ~0;
	}
	}
	
	bool no_sound() {
		return no_sound_;
	}
	
	const char *save_file_path() {
		return save_file_path_;
	}
	
	bool show_debug_hitboxes() {
		return show_debug_hitboxes_;
	}
	
	bool use_pretty_scaling() {
		return use_pretty_scaling_;
	}
	
	void set_use_pretty_scaling(bool value) {
		use_pretty_scaling_ = value;
	}
	
	bool fullscreen() {
		return fullscreen_;
	}
	
	void set_fullscreen(bool value) {
		fullscreen_ = value;
	}
	
	void set_widescreen()
	{
		virtual_screen_width_ = (virtual_screen_height_*16)/9;
		actual_screen_width_ = (actual_screen_height_*16)/9;
		recalculate_draw_mask();
	}
	
	int virtual_screen_width()
	{
		return virtual_screen_width_;
	}
	
	int virtual_screen_height()
	{
		return virtual_screen_height_;
	}
	
	int actual_screen_width()
	{
		return actual_screen_width_;
	}
	
	int actual_screen_height()
	{
		return actual_screen_height_;
	}
	
	void set_actual_screen_width(int width)
	{
		actual_screen_width_ = width;
		recalculate_draw_mask();
	}
	
	void set_actual_screen_height(int height)
	{
		actual_screen_height_ = height;
	}

	bool load_compiled()
	{
		return load_compiled_;
	}

	void set_load_compiled(bool value)
	{
		load_compiled_ = value;
	}
	
	bool force_no_npot_textures()
	{
		return force_no_npot_textures_;
	}
	
	bool screen_rotated()
	{
		return screen_rotated_;
	}
	
	bool parse_arg(const char* arg) {
		std::string s(arg);
		if(s == "--show_hitboxes") {
			show_debug_hitboxes_ = true;
		} else if(s == "--scale") {
			set_use_pretty_scaling(true);
		} else if(s == "--nosound") {
			no_sound_ = true;
		} else if(s == "--fullscreen") {
			fullscreen_ = true;
		} else if(s == "--widescreen") {
			set_widescreen();
		} else if(s == "--potonly") {
			force_no_npot_textures_ = true;
		} else {
			return false;
		}
		
		return true;
	}
}
