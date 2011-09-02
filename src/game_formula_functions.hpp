#ifndef GAME_FORMULA_FUNCTIONS_HPP_INCLUDED
#define GAME_FORMULA_FUNCTIONS_HPP_INCLUDED

#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_function.hpp"
#include "tile.hpp"
#include "tile_logic.hpp"
#include "variant.hpp"
#include "wml_node_fwd.hpp"

using game_logic::function_symbol_table;
function_symbol_table& get_game_formula_functions_symbol_table();
class client_play_game;

class game_command_callable : public game_logic::formula_callable {
public:
	virtual void execute(client_play_game* client=0) const = 0;
private:
	variant get_value(const std::string& key) const { return variant(); }
};

class location_object : public game_logic::formula_callable {
public:
	explicit location_object(const hex::location& loc);
	const hex::location& loc() const { return loc_; }
private:
	variant get_value(const std::string& key) const;
	hex::location loc_;
};

class tile_object : public game_logic::formula_callable {
public:
	explicit tile_object(const tile& t);
	const tile& get_tile() const { return tile_; }
private:
	variant get_value(const std::string& key) const;
	tile tile_;
};

#endif
