#include <algorithm>
#include <numeric>

#include "ai_player.hpp"
#include "foreach.hpp"
#include "game.hpp"
#include "game_utils.hpp"
#include "pathfind.hpp"
#include "resource.hpp"
#include "terrain.hpp"
#include "unit.hpp"
#include "unit_utils.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_writer.hpp"

namespace {

class default_ai_player : public ai_player
{
public:
	default_ai_player(game& g, int nplayer)
	  : ai_player(g, nplayer), set_deck_(false)
	{}
	std::vector<wml::node_ptr> play();

	hex::location player_select_loc(const std::string& prompt,
	                          boost::function<bool(hex::location)> valid_loc);

private:
	mutable bool set_deck_;
};

hex::location default_ai_player::player_select_loc(const std::string& prompt,
	                          boost::function<bool(hex::location)> valid_loc)
{
	foreach(const tile& t, get_game().tiles()) {
		if(!valid_loc(t.loc())) {
			continue;
		}

		unit_ptr target = get_game().get_unit_at(t.loc());
		if(target->side() != player_id()) {
			return t.loc();
		}
	}

	return hex::location();
}

std::vector<wml::node_ptr> default_ai_player::play()
{
	std::vector<wml::node_ptr> result;
	if(!set_deck_) {
		set_deck_ = true;
		result.push_back(wml::parse_wml_from_file("deck.cfg"));
		return result;
	}

	std::cerr << "AI PLAYING: " << get_game().player_casting() << " VS " << player_id() << "\n";

	if(get_game().player_casting() != player_id()) {
		return result;
	}

	const game::player& player = get_game().players()[player_id()];
	if(result.empty()) {
		foreach(unit_ptr u, get_game().units()) {

			if(u->side() == player_id()) {
				if(u->has_moved() == false) {
					unit_movement_cost_calculator calc(&get_game(), u);
					hex::route_map routes;
					hex::find_possible_moves(u->loc(), u->move(), calc, routes);

					hex::location move_to;
					int target_distance = -1;

					for(hex::route_map::const_iterator i = routes.begin();
					    i != routes.end(); ++i) {
						const tile* t = get_game().get_tile(i->first);
						if(!t) {
							continue;
						}

						if(t->terrain()->id() == "tower" && !player.towers.count(i->first)) {
							move_to = i->first;
							break;
						}

						int closest_enemy = -1;
						foreach(const_unit_ptr enemy, get_game().units()) {
							if(enemy->side() != player_id()) {
								const int distance = hex::distance_between(i->first, enemy->loc());
								if(closest_enemy == -1 || distance < closest_enemy) {
									closest_enemy = distance;
								}
							}
						}

						if(move_to.valid() == false || closest_enemy < target_distance) {
							move_to = i->first;
							target_distance = closest_enemy;
						}
					}

					std::cerr << "TARGET DISTANCE: " << target_distance << "\n";

					if(move_to.valid() && move_to != u->loc()) {
						const hex::location move_from = u->loc();

						wml::node_ptr move_node(new wml::node("move"));
						move_node->add_child(hex::write_location("from", move_from));
						move_node->add_child(hex::write_location("to", move_to));
						result.push_back(move_node);

						u->set_loc(move_to);
						foreach(unit_ability_ptr a, u->abilities()) {
							wml::node_ptr attack_node = a->use_ability(*u, *this);
							if(attack_node.get() != NULL) {
								result.push_back(attack_node);
							}
						}

						u->set_loc(move_from);

						break;
					}
				}
			}
		}
	}

	std::cerr << "AI PLAYER...HAS " << player.spells.size() << " CARDS\n";
	if(result.empty()) {
		foreach(const held_card& held, player.spells) {
			if(held.embargo) {
				continue;
			}

			const_card_ptr card = held.card;

			if(!player_pay_cost(card->cost())) {
				continue;
			}

			if(card->monster_id()) {

				hex::location summoning_hex;
				foreach(unit_ptr u, get_game().units()) {
					if(u->side() == player_id() && u->can_summon()) {
						hex::location adj[6];
						hex::get_adjacent_tiles(u->loc(), adj);
						foreach(const hex::location a, adj) {
							if(get_game().get_tile(a) != NULL && get_game().get_unit_at(a).get() == NULL) {
								summoning_hex = a;
								break;
							}
						}
					}
				}

				if(summoning_hex.valid()) {
					wml::node_ptr play_node(new wml::node("play"));
					play_node->set_attr("spell", card->id());
					wml::node_ptr monster_node(hex::write_location("monster", summoning_hex));
					monster_node->set_attr("type", *card->monster_id());
					play_node->add_child(monster_node);

					result.push_back(play_node);
					break;
				}
			}
		}
	}

	wml::node_ptr end_turn_node(new wml::node("end_turn"));
	if(result.empty()) {
		end_turn_node->set_attr("discard", "yes");
		end_turn_node->set_attr("skip", "yes");
	}

	result.push_back(end_turn_node);
	return result;
}

}

ai_player* ai_player::create(game& g, int nplayer)
{
	return new default_ai_player(g, nplayer);
}

ai_player::ai_player(game& g, int nplayer)
  : game_(g), nplayer_(nplayer)
{}

ai_player::~ai_player()
{}

bool ai_player::player_pay_cost(const std::vector<int>& cost)
{
	const game::player& player = get_game().players()[player_id()];
	for(int n = 0; n != cost.size(); ++n) {
		if(n >= player.resources.size() || cost[n] > player.resources[n]) {
			return false;
		}
	}

	return true;
}
