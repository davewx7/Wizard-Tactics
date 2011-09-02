#include "draw_card.hpp"
#include "font.hpp"
#include "formatter.hpp"
#include "gui_section.hpp"
#include "raster.hpp"
#include "resource.hpp"
#include "terrain.hpp"
#include "unit.hpp"
#include "unit_avatar.hpp"

namespace {
const_unit_avatar_ptr get_avatar(const std::string& id)
{
	static std::map<std::string, const_unit_avatar_ptr> cache;
	const_unit_avatar_ptr& p = cache[id];
	if(p.get() != NULL) {
		return p;
	}

	p.reset(new unit_avatar(unit::build_from_prototype(id, 0, hex::location())));
	return p;
}
}

rect draw_card(const_card_ptr c, int x, int y)
{
	const rect area(x, y, 108, 120);
	graphics::blit_texture(graphics::texture::get("card.png"), x, y);

	const std::string* monster = c->monster_id();
	if(monster) {
		const_unit_avatar_ptr a = get_avatar(*monster);
		if(a) {
			a->draw(x + 40, y + 70);
		}
	}

	const std::string* land = c->land_id();
	if(land) {
		const_terrain_ptr t = terrain::get(*land);
		if(t) {
			graphics::texture tex(graphics::texture::get(t->texture()));
			graphics::blit_texture(tex, x + 40 - tex.width()/2, y + 65 - tex.height()/2);
			if(!t->overlay_texture().empty()) {
				graphics::blit_texture(graphics::texture::get(t->overlay_texture()), x - 45, y - 35);
			}
		}
	}


	int cost_pos = x + 82;
	int cost_pos_y = y + 8;
	for(int n = resource::num_resources()-1; n != -1; --n) {
		char buf[128];
		sprintf(buf, "magic-icon-%c", resource::resource_id(n));

		const_gui_section_ptr section = gui_section::get(buf);

		for(int m = 0; m != c->cost(n); ++m) {
			section->blit(cost_pos, cost_pos_y, 16, 16);
			cost_pos -= 16;
			if(cost_pos < x) {
				cost_pos = x + 72;
				cost_pos_y += 16;
			}
		}
	}

	//graphics::blit_texture(font::render_text(c->name(), graphics::color_white(), 14), x + 3, cost_pos_y + 18);

	return area;
}
