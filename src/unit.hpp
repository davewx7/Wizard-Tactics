#ifndef UNIT_HPP_INCLUDED
#define UNIT_HPP_INCLUDED

#include <vector>

#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_variable_storage.hpp"
#include "movement_type.hpp"
#include "tile_logic.hpp"
#include "unit_ability.hpp"
#include "variant.hpp"
#include "wml_node_fwd.hpp"

class game;
class unit;

typedef boost::intrusive_ptr<unit> unit_ptr;
typedef boost::intrusive_ptr<const unit> const_unit_ptr;

class unit : public game_logic::formula_callable
{
public:
	struct modification {
		modification();
		explicit modification(wml::const_node_ptr node);
		wml::node_ptr write() const;
		int id;
		int life, armor, move;
		bool expires_end_of_turn;
		bool expires_on_state_change;
	};

	static const_unit_ptr get_prototype(const std::string& id);
	static unit_ptr build_from_prototype(const std::string& id, int side, const hex::location& loc);

	explicit unit(wml::const_node_ptr node);

	wml::node_ptr write() const;

	typedef boost::shared_ptr<modification> mod_ptr;

	int key() const { return key_; }
	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }

	const std::string& upkeep() const { return upkeep_; }

	int resource_type() const;

	const std::vector<std::string>& underlays() const { return underlays_; }
	const std::vector<std::string>& overlays() const { return overlays_; }

	const hex::location& loc() const;
	void set_loc(const hex::location& loc);

	int side() const;
	int life() const;
	int armor() const;
	int move() const;

	bool has_moved() const { return has_moved_; }
	void set_moved(bool value=true) { has_moved_ = value; }

	void new_turn();

	void game_state_changed();

	int damage_taken() const { return damage_taken_; }
	void take_damage(int amount) { damage_taken_ += amount; }
	void heal();

	int maintenance_cost() const { return maintenance_cost_; }

	bool scout() const { return scout_; }

	const std::vector<hex::location>& movement_path() const { return movement_path_; }

	const movement_type& move_type() const { return *move_type_; }

	bool can_cast(char resource_type) const;
	bool can_summon(char resource_type) const;
	bool can_produce() const { return can_produce_; }

	const std::vector<unit_ability_ptr>& abilities() const { return abilities_; }
	unit_ability_ptr default_ability() const;

	void add_modification(const modification& mod);

	void handle_event(const std::string& name, const game_logic::formula_callable* callable=NULL) const;

private:
	void assign_new_unit_key();
	variant get_value(const std::string& key) const;
	void set_value(const std::string& key, const variant& val);

	int key_;

	std::string id_, name_;

	std::string upkeep_;

	std::vector<std::string> overlays_, underlays_;

	hex::location loc_;
	int side_;
	int life_, armor_, move_;

	int damage_taken_;

	//the cost of the unit toward the unit limit. Default to 1.
	int maintenance_cost_;

	bool has_moved_, scout_;
	std::vector<hex::location> movement_path_;

	std::vector<mod_ptr> mods_;
	int mod_id_;

	const_movement_type_ptr move_type_;

	std::string can_summon_, can_cast_;
	bool can_produce_;

	std::vector<unit_ability_ptr> abilities_;

	game_logic::formula_variable_storage_ptr vars_, vars_turn_;

	typedef std::map<std::string, game_logic::const_formula_ptr> handlers_map;
	handlers_map handlers_;
};

#endif
