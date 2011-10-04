#ifndef UNIT_ABILITY_HPP_INCLUDED
#define UNIT_ABILITY_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <string>

#include "formula_fwd.hpp"
#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"

class card_selector;
class unit;

class unit_ability
{
public:
	explicit unit_ability(const std::string& unit_id, wml::const_node_ptr node);

	wml::node_ptr write() const;

	bool is_ability_usable(unit& u, card_selector& selector) const;
	wml::node_ptr use_ability(unit& u, card_selector& selector);

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }
	const std::string& icon() const { return icon_; }

	const std::string& spell() const { return card_; }
	bool taps_caster() const { return taps_caster_; }
private:
	void calculate_valid_targets(unit& u, std::vector<hex::location>& result) const;
	std::string name_, id_, card_, description_, icon_;

	int ntargets_;
	int range_;
	game_logic::const_formula_ptr valid_targets_;

	bool taps_caster_;
};

typedef boost::shared_ptr<unit_ability> unit_ability_ptr;
typedef boost::shared_ptr<const unit_ability> const_unit_ability_ptr;

#endif
