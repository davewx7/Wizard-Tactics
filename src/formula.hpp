/* $Id: formula.hpp 24955 2008-03-21 23:11:31Z dragonking $ */
/*
   Copyright (C) 2007 - 2008 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef FORMULA_HPP_INCLUDED
#define FORMULA_HPP_INCLUDED

#include <map>
#include <string>

#include "formula_callable_definition_fwd.hpp"
#include "formula_fwd.hpp"
#include "formula_function.hpp"
#include "variant.hpp"
#include "wml_value.hpp"

namespace game_logic
{

class formula_callable;
class formula_expression;
class function_symbol_table;
typedef boost::shared_ptr<formula_expression> expression_ptr;

class formula {
public:
	static variant evaluate(const const_formula_ptr& f,
	                    const formula_callable& variables,
						variant default_res=variant(0)) {
		if(f) {
			return f->execute(variables);
		} else {
			return default_res;
		}
	}

	// function which will create a formula that is a single string literal, 'str'.
	// 'str' should not be enclosed in quotes.
	static formula_ptr create_string_formula(const std::string& str);
	static formula_ptr create_optional_formula(const wml::value& str, function_symbol_table* symbols=NULL, const formula_callable_definition* def=NULL);
	explicit formula(const wml::value& val, function_symbol_table* symbols=NULL, const formula_callable_definition* def=NULL);
	~formula();
	variant execute(const formula_callable& variables) const;
	variant execute() const;
	const std::string& str() const { return str_; }

	void output_debug_info() const;

private:
	formula() {}
	expression_ptr expr_;
	std::string str_;
	const std::string* filename_;
	int line_;
};

struct formula_error
{
	formula_error();
};

}

#endif
