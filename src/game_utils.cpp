#include "foreach.hpp"
#include "game.hpp"
#include "game_utils.hpp"
#include "terrain.hpp"
#include "tile.hpp"
#include "tile_logic.hpp"

int terrain_area(const game& g, const hex::location& loc, std::set<hex::location>* locs)
{
	std::set<hex::location> locs_buf;
	if(!locs) {
		locs = &locs_buf;
	}

	const tile* t = g.get_tile(loc.x(), loc.y());
	if(!t) {
		return 0;
	}

	locs->insert(loc);

	const_terrain_ptr ter = t->terrain();
	hex::location adj[6];
	get_adjacent_tiles(loc, &adj[0]);
	foreach(const hex::location& a, adj) {
		if(locs->count(a)) {
			continue;
		}

		const tile* t2 = g.get_tile(a.x(), a.y());
		if(t2 && t2->terrain() == ter) {
			terrain_area(g, a, locs);
		}
	}

	return locs->size();
}

int tower_resource_production(const game& g, const hex::location& tower, int resource, std::set<hex::location>* locs)
{
	const tile* t = g.get_tile(tower.x(), tower.y());
	if(!t || t->terrain()->resource_id() != resource) {
		return 0;
	}

	const int amount = terrain_area(g, tower, locs);
	return 1 + amount/2;
}

