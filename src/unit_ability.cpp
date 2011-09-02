#include <boost/bind.hpp>

#include <algorithm>

#include "card.hpp"
#include "formatter.hpp"
#include "game.hpp"
#include "game_formula_functions.hpp"
#include "unit.hpp"
#include "unit_ability.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

unit_ability::unit_ability(const std::string& unit_id, wml::const_node_ptr node)
  : name_(node->attr("name")), id_(node->attr("id")),
    card_(unit_id + "." + id_), description_(node->attr("description")),
	icon_(node->attr("icon")),
	ntargets_(wml::get_int(node, "targets")),
    range_(wml::get_int(node, "range", -1)),
	taps_caster_(wml::get_bool(node, "taps_caster", true))
{
	game_logic::function_symbol_table* symbols = &get_game_formula_functions_symbol_table();
	valid_targets_ = game_logic::formula::create_optional_formula(node->attr("valid_targets"), symbols);
}

wml::node_ptr unit_ability::write() const
{
	wml::node_ptr node(new wml::node("ability"));
	node->set_attr("name", name_);
	node->set_attr("id", id_);
	node->set_attr("description", description_);
	node->set_attr("targets", formatter() << ntargets_);
	node->set_attr("range", formatter() << range_);
	node->set_attr("taps_caster", taps_caster_ ? "yes" : "no");
	node->set_attr("icon", formatter() << icon_);

	if(valid_targets_.get() != NULL) {
		node->set_attr("valid_targets", valid_targets_->str());
	}

	return node;
}

namespace {
bool is_loc_in_vector(const hex::location loc, const std::vector<hex::location>& v) {
	return std::find(v.begin(), v.end(), loc) != v.end();
}
}

bool unit_ability::is_ability_usable(unit& u, card_selector& selector) const
{
	if(taps_caster_ && u.has_moved()) {
		return false;
	}

	const_card_ptr c = card::get(card_);
	if(c.get() != NULL && !selector.player_pay_cost(c->cost())) {
		return false;
	}

	if(ntargets_ > 0) {
		std::vector<hex::location> targets;
		calculate_valid_targets(u, targets);
		if(targets.size() < ntargets_) {
			return false;
		}
	}

	return true;
}

wml::node_ptr unit_ability::use_ability(unit& u, card_selector& client)
{
	wml::node_ptr result(new wml::node("play"));
	result->set_attr("spell", card_);
	result->set_attr("caster", formatter() << u.key());

	std::vector<hex::location> targets;
	calculate_valid_targets(u, targets);

	std::cerr << "NUM_TARGETS: " << ntargets_ << "\n";

	for(int n = 0; n < ntargets_; ++n) {
		std::cerr << "SELECTING TARGET FROM " << targets.size() << "...\n";
		const hex::location loc =
		    client.player_select_loc("Select target for ability",
			   boost::bind(is_loc_in_vector, _1, targets));
		std::cerr << "TARGET: " << loc << "\n";
		if(!loc.valid()) {
			return wml::node_ptr();
		}

		result->add_child(hex::write_location("target", loc));
	}

	return result;
}

void unit_ability::calculate_valid_targets(unit& u, std::vector<hex::location>& result) const
{
	const_card_ptr c = card::get(card_);
	if(c.get() != NULL) {
		c->calculate_valid_targets(&u, u.side(), result);
	}
}
