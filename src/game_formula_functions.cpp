#ifdef CLIENT
#include "client_play_game.hpp"
#endif

#include "debug_console.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "game.hpp"
#include "game_formula_functions.hpp"
#include "terrain.hpp"
#include "unit.hpp"
#include "wml_node.hpp"

using namespace game_logic;

#define EVAL_ARG(n) (args()[n]->evaluate(variables))

namespace {

class function_creator {
public:
	virtual ~function_creator() {}
	virtual function_expression* create(const function_expression::args_list& args) const = 0;
};

template<typename T>
class specific_function_creator : public function_creator {
public:
	virtual ~specific_function_creator() {}
	virtual function_expression* create(const function_expression::args_list& args) const {
		return new T(args);
	}
};

std::map<std::string, function_creator*>& function_creators() {
	static std::map<std::string, function_creator*> creators;
	return creators;
}

int register_function_creator(const std::string& id, function_creator* creator) {
	function_creators()[id] = creator;
	return function_creators().size();
}

std::vector<std::string>& function_helpstrings() {
	static std::vector<std::string> instance;
	return instance;
}

int register_function_helpstring(const std::string& str) {
	function_helpstrings().push_back(str);
	return function_helpstrings().size();
}

#define FUNCTION_DEF(name, min_args, max_args, helpstring) \
const int name##_dummy_help_var = register_function_helpstring(helpstring); \
class name##_function : public function_expression { \
public: \
	explicit name##_function(const args_list& args) \
	  : function_expression(#name, args, min_args, max_args) {} \
private: \
	variant execute(const formula_callable& variables) const {

#define END_FUNCTION_DEF(name) } }; const int name##_dummy_var = register_function_creator(#name, new specific_function_creator<name##_function>());

class debug_command : public game_command_callable
{
	std::string str_;
public:
	explicit debug_command(const std::string& str) : str_(str)
	{}

	void execute(client_play_game* client) const {
		std::cerr << "DEBUG: '" << str_ << "'\n";
		debug_console::add_message(str_);
		if(game::current()) {
			std::string str = str_;
			std::replace(str.begin(), str.end(), '"', '\'');
			std::ostringstream s;
			s << "<debug_message msg=\"" << str << "\"/>";
			game::current()->queue_message(s.str());
		}
	}
};

FUNCTION_DEF(debug, 1, -1, "debug(...): outputs arguments to console")
	std::string str;
	for(int n = 0; n != args().size(); ++n) {
		str += args()[n]->evaluate(variables).to_debug_string();
	}

	std::cerr << "EXECUTE DEBUG: " << str << "\n";
	return variant(new debug_command(str));
END_FUNCTION_DEF(debug)

FUNCTION_DEF(debug_fn, 2, -1, "debug_fn(expr, ...): evaluates to 'expr' while outputting other arguments")
	std::string str;
	for(int n = 1; n < args().size(); ++n) {
		str += args()[n]->evaluate(variables).to_debug_string();
	}

	std::cerr << "DEBUG_FN: '" << str << "'\n";

	return args()[0]->evaluate(variables);
END_FUNCTION_DEF(debug_fn)

FUNCTION_DEF(loc, 2, 2, "loc(x,  y): creates a location object")
	return variant(new location_object(
	    hex::location(args()[0]->evaluate(variables).as_int(),
	                  args()[1]->evaluate(variables).as_int())));
END_FUNCTION_DEF(loc)

FUNCTION_DEF(tower, 1, 1, "tower(loc): yields the owner of the tower at the given location, or -1 if there is no tower at that location")
	variant v = EVAL_ARG(0);
	const location_object* loc_obj = v.try_convert<const location_object>();
	if(loc_obj) {
		return variant(game::current()->tower_owner(loc_obj->loc()));
	}

	return variant(-1);
END_FUNCTION_DEF(tower)

FUNCTION_DEF(get_unit_at_loc, 1, 1, "get_unit_at_loc(loc): yields the unit at the given location")
	variant v = EVAL_ARG(0);
	const location_object* loc_obj = v.try_convert<const location_object>();
	if(loc_obj) {
		unit_ptr u = game::current()->get_unit_at(loc_obj->loc());
		if(u.get() != NULL) {
			return variant(u.get());
		}
	}

	return variant();
END_FUNCTION_DEF(get_unit_at_loc)

namespace {

//function to parse an argument to a location.
//if the argument is a unit, the location of the unit will be taken.
hex::location arg_to_loc(variant v) {
	unit_ptr u(v.try_convert<unit>());
	if(u == NULL) {
		const location_object* loc_obj = v.convert_to<const location_object>();
		return loc_obj->loc();
	} else {
		return u->loc();
	}
}
}

FUNCTION_DEF(get_adjacent_units, 1, 1, "get_adjacent_units(unit): yields all units adjacent to 'unit'")
	hex::location loc = arg_to_loc(EVAL_ARG(0));

	std::vector<variant> v;

	hex::location adj[6];
	get_adjacent_tiles(loc, adj);
	foreach(const hex::location& a, adj) {
		unit_ptr u = game::current()->get_unit_at(a);
		if(u) {
			v.push_back(variant(u.get()));
		}
	}

	return variant(&v);
END_FUNCTION_DEF(get_adjacent_units)

FUNCTION_DEF(distance_between, 2, 2, "distance_between(loc1, loc2): gives the number of tiles between the two locations")
	return variant(hex::distance_between(arg_to_loc(EVAL_ARG(0)), arg_to_loc(EVAL_ARG(1))));
END_FUNCTION_DEF(distance_between)

class free_attack_command : public game_command_callable
{
public:
	free_attack_command(unit_ptr attacker, unit_ptr target)
	  : attacker_(attacker), target_(target)
	{}

	virtual void execute(client_play_game* client) const {
		game::current()->unit_free_attack(attacker_, target_);
	}

private:
	unit_ptr attacker_, target_;
};

FUNCTION_DEF(free_attack, 2, 2, "free_attack(attacker, target): makes the attacker get a free attack on target")
	unit_ptr attacker(args()[0]->evaluate(variables).convert_to<unit>());
	unit_ptr target(args()[1]->evaluate(variables).convert_to<unit>());
	return variant(new free_attack_command(attacker, target));
END_FUNCTION_DEF(free_attack)

class unit_modification_command : public game_command_callable
{
	unit_ptr target_;
	unit::modification mod_;
public:
	unit_modification_command(unit_ptr target, const std::string& attr, int value, const std::string& expiration)
	  : target_(target)
	{
		wml::node_ptr node(new wml::node("mod"));
		node->set_attr(attr, formatter() << value);
		node->set_attr(expiration, "yes");
		mod_ = unit::modification(node);
	}

	virtual void execute(client_play_game* client) const {
		target_->add_modification(mod_);
	}
};

FUNCTION_DEF(modify_unit_until_end_of_turn, 3, 3, "modify_unit_until_end_of_turn(unit, attribute, modification): modifies the unit's attribute by 'modification' until the end of the turn")
	const unit_ptr target(args()[0]->evaluate(variables).convert_to<unit>());
	const std::string attr(args()[1]->evaluate(variables).as_string());
	const int value(args()[2]->evaluate(variables).as_int());

	return variant(new unit_modification_command(target, attr, value, "expires_end_of_turn"));
END_FUNCTION_DEF(modify_unit_until_end_of_turn)

FUNCTION_DEF(modify_unit_until_game_state_change, 3, 3, "modify_unit_until_game_state_change(unit, attribute, modification): modifies the unit's attribute by 'modification' until the next change of game state")
	const unit_ptr target(args()[0]->evaluate(variables).convert_to<unit>());
	const std::string attr(args()[1]->evaluate(variables).as_string());
	const int value(args()[2]->evaluate(variables).as_int());

	return variant(new unit_modification_command(target, attr, value, "expires_on_state_change"));
END_FUNCTION_DEF(modify_unit_until_game_state_change)

class set_command : public game_command_callable
{
public:
	set_command(variant target, const std::string& attr, variant val)
	  : target_(target), attr_(attr), val_(val)
	{}
	virtual void execute(client_play_game* client) const {
		target_.mutable_callable()->mutate_value(attr_, val_);
	}
private:
	variant target_;
	std::string attr_;
	variant val_;
};

class add_command : public game_command_callable
{
public:
	add_command(variant target, const std::string& attr, variant val)
	  : target_(target), attr_(attr), val_(val)
	{}
	virtual void execute(client_play_game* client) const {
		target_.mutable_callable()->mutate_value(attr_, target_.mutable_callable()->query_value(attr_) + val_);
	}
private:
	variant target_;
	std::string attr_;
	variant val_;
};

class set_function : public function_expression {
public:
	set_function(const args_list& args, const formula_callable_definition* callable_def)
	  : function_expression("set", args, 2, 3) {
		if(args.size() == 2) {
			variant literal = args[0]->is_literal();
			if(literal.is_string()) {
				key_ = literal.as_string();
			} else {
				args[0]->is_identifier(&key_);
			}
		}
	}
private:
	variant execute(const formula_callable& variables) const {
		if(!key_.empty()) {
			return variant(new set_command(variant(), key_, args()[1]->evaluate(variables)));
		}

		if(args().size() == 2) {
			try {
				std::string member;
				variant target = args()[0]->evaluate_with_member(variables, member);
				return variant(new set_command(
				  target, member, args()[1]->evaluate(variables)));
			} catch(game_logic::formula_error&) {
			}
		}

		variant target;
		if(args().size() == 3) {
			target = args()[0]->evaluate(variables);
		}
		const int begin_index = args().size() == 2 ? 0 : 1;
		return variant(new set_command(
		    target,
		    args()[begin_index]->evaluate(variables).as_string(),
			args()[begin_index + 1]->evaluate(variables)));
	}

	std::string key_;
};

class add_function : public function_expression {
public:
	add_function(const args_list& args, const formula_callable_definition* callable_def)
	  : function_expression("add", args, 2, 3) {
		if(args.size() == 2) {
			variant literal = args[0]->is_literal();
			if(literal.is_string()) {
				key_ = literal.as_string();
			} else {
				args[0]->is_identifier(&key_);
			}
		}
	}
private:
	variant execute(const formula_callable& variables) const {
		if(!key_.empty()) {
			return variant(new add_command(variant(), key_, args()[1]->evaluate(variables)));
		}

		if(args().size() == 2) {
			try {
				std::string member;
				variant target = args()[0]->evaluate_with_member(variables, member);
				return variant(new add_command(
				  target, member, args()[1]->evaluate(variables)));
			} catch(game_logic::formula_error&) {
			}
		}

		variant target;
		if(args().size() == 3) {
			target = args()[0]->evaluate(variables);
		}
		const int begin_index = args().size() == 2 ? 0 : 1;
		return variant(new add_command(
		    target,
		    args()[begin_index]->evaluate(variables).as_string(),
			args()[begin_index + 1]->evaluate(variables)));
	}

	std::string key_;
};

}  //end namespace

class game_formula_functions_symbol_table : public function_symbol_table
{
public:
	game_logic::expression_ptr create_function(
	                    const std::string& fn,
	                    const std::vector<expression_ptr>& args,
						const formula_callable_definition* callable_def) const;
};

game_logic::expression_ptr game_formula_functions_symbol_table::create_function(
                    const std::string& fn,
                    const std::vector<expression_ptr>& args,
					const formula_callable_definition* callable_def) const
{
	if(fn == "set") {
		return expression_ptr(new set_function(args, callable_def));
	} else if(fn == "add") {
		return expression_ptr(new add_function(args, callable_def));
	}

	std::map<std::string, function_creator*>::const_iterator i = function_creators().find(fn);
	if(i != function_creators().end()) {
		return expression_ptr(i->second->create(args));
	}

	return game_logic::function_symbol_table::create_function(fn, args, callable_def);
}


function_symbol_table& get_game_formula_functions_symbol_table()
{
	static game_formula_functions_symbol_table table;
	return table;
}

location_object::location_object(const hex::location& loc) : loc_(loc)
{}

variant location_object::get_value(const std::string& key) const
{
	if(key == "x") {
		return variant(loc_.x());
	} else if(key == "y") {
		return variant(loc_.y());
	} else {
		return variant();
	}
}

tile_object::tile_object(const tile& t) : tile_(t)
{}

variant tile_object::get_value(const std::string& key) const
{
	if(key == "loc") {
		return variant(new location_object(tile_.loc()));
	} else if(key == "terrain") {
		return variant(tile_.terrain()->id());
	} else {
		return variant();
	}
}
