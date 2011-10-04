#include "foreach.hpp"
#include "game.hpp"
#include "terrain.hpp"
#include "unit_utils.hpp"

unit_movement_cost_calculator::unit_movement_cost_calculator(const game* g, const_unit_ptr u)
  : game_(g), unit_(u)
{}

int unit_movement_cost_calculator::movement_cost(const hex::location& a, const hex::location& b) const
{
	const tile* t = game_->get_tile(b.x(), b.y());
	if(t == NULL) {
		std::cerr << "ILLEGAL MOVE: " << a << "\n";
		return -1;
	}

	const int res = unit_->move_type().movement_cost(t->terrain()->id());
	return res;
}

bool unit_movement_cost_calculator::allowed_to_move(const hex::location& a) const
{
	if(game_->get_unit_at(a).get() != NULL) {
		return false;
	}

	const tile* t = game_->get_tile(a.x(), a.y());
	if(t == NULL) {
		return false;
	}

	return unit_->move_type().movement_cost(t->terrain()->id()) >= 0;
}

bool unit_movement_cost_calculator::legal_move_endpoint(const hex::location& a) const
{
	if(game_->get_unit_at(a).get() != NULL) {
		return false;
	}

	return true;
}

int unit_protection(const game& g, const_unit_ptr u)
{
	int result = u->armor();
	const tile* t = g.get_tile(u->loc());
	if(t) {
		result += u->move_type().armor_modification(t->terrain()->id());
	}

	return result;
}
