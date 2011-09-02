/* $Id: formula_function.cpp 25895 2008-04-17 18:57:13Z mordante $ */
/*
   Copyright (C) 2008 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2
   or at your option any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include <iostream>
#include <math.h>

#include "foreach.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "formula_callable_utils.hpp"
#include "formula_function.hpp"

#include "SDL.h"

namespace game_logic {

variant formula_expression::execute_member(const formula_callable& variables, std::string& id) const
{
	throw formula_error();
}

namespace {

class dir_function : public function_expression {
public:
	explicit dir_function(const args_list& args)
	     : function_expression("dir", args, 1, 1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		variant var = args()[0]->evaluate(variables);
		const formula_callable* callable = var.as_callable();
		std::vector<formula_input> inputs = callable->inputs();
		std::vector<variant> res;
		for(size_t i=0; i<inputs.size(); ++i) {
			const formula_input& input = inputs[i];
			res.push_back(variant(input.name));
		}

		return variant(&res);
	}
};

class if_function : public function_expression {
public:
	explicit if_function(const args_list& args)
	     : function_expression("if", args, 2, 3)
	{}

	expression_ptr optimize() const {
		variant v;
		if(args()[0]->can_reduce_to_variant(v)) {
			if(v.as_bool()) {
				return args()[1];
			} else {
				if(args().size() == 3) {
					return args()[2];
				} else {
					return expression_ptr(new variant_expression(variant()));
				}
			}
		}

		return expression_ptr();
	}

private:
	variant execute(const formula_callable& variables) const {
		const int i = args()[0]->evaluate(variables).as_bool() ? 1 : 2;
		if(i >= args().size()) {
			return variant();
		}
		return args()[i]->evaluate(variables);
	}
};

class switch_function : public function_expression {
public:
	explicit switch_function(const args_list& args)
	    : function_expression("switch", args, 3, -1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		variant var = args()[0]->evaluate(variables);
		for(size_t n = 1; n < args().size()-1; n += 2) {
			variant val = args()[n]->evaluate(variables);
			if(val == var) {
				return args()[n+1]->evaluate(variables);
			}
		}

		if((args().size()%2) == 0) {
			return args().back()->evaluate(variables);
		} else {
			return variant();
		}
	}
};

class query_function : public function_expression {
public:
	explicit query_function(const args_list& args)
	  : function_expression("query", args, 2, 2)
	{}
	
private:
	variant execute(const formula_callable& variables) const {
		variant callable = args()[0]->evaluate(variables);
		variant str = args()[1]->evaluate(variables);
		return callable.as_callable()->query_value(str.as_string());
	}
};

class rgb_function : public function_expression {
public:
	explicit rgb_function(const args_list& args)
	     : function_expression("rgb", args, 3, 3)
	{}

private:
	variant execute(const formula_callable& variables) const {
		return variant(10000*
		 std::min<int>(99,std::max<int>(0,args()[0]->evaluate(variables).as_int())) +
		 std::min<int>(99,std::max<int>(0,args()[1]->evaluate(variables).as_int()))*100+
		 std::min<int>(99,std::max<int>(0,args()[2]->evaluate(variables).as_int())));
	}
};

namespace {
int transition(int begin, int val1, int end, int val2, int value) {
	if(value < begin || value > end) {
		return 0;
	}

	if(value == begin) {
		return val1;
	} else if(value == end) {
		return val2;
	}

	const int comp1 = val1*(end - value);
	const int comp2 = val2*(value - begin);
	return (comp1 + comp2)/(end - begin);
}
}

class transition_function : public function_expression {
public:
	explicit transition_function(const args_list& args)
			: function_expression("transition", args, 5, 5)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const int value = args()[0]->evaluate(variables).as_int();
		const int begin = args()[1]->evaluate(variables).as_int();
		const int end = args()[3]->evaluate(variables).as_int();
		if(value < begin || value > end) {
			return variant(0);
		}
		const int val1 = args()[2]->evaluate(variables).as_int();
		const int val2 = args()[4]->evaluate(variables).as_int();
		return variant(transition(begin, val1, end, val2, value));
	}
};

class color_transition_function : public function_expression {
public:
	explicit color_transition_function(const args_list& args)
			: function_expression("color_transition", args, 5)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const int value = args()[0]->evaluate(variables).as_int();
		int begin = args()[1]->evaluate(variables).as_int();
		int end = -1;
		size_t n = 3;
		while(n < args().size()) {
			end = args()[n]->evaluate(variables).as_int();
			if(value >= begin && value <= end) {
				break;
			}

			begin = end;
			n += 2;
		}

		if(value < begin || value > end) {
			return variant(0);
		}
		const int val1 = args()[n-1]->evaluate(variables).as_int();
		const int val2 = args()[n+1 < args().size() ? n+1 : n]->
		                               evaluate(variables).as_int();
		const int r1 = (val1/10000)%100;
		const int g1 = (val1/100)%100;
		const int b1 = (val1)%100;
		const int r2 = (val2/10000)%100;
		const int g2 = (val2/100)%100;
		const int b2 = (val2)%100;

		const int r = transition(begin,r1,end,r2,value);
		const int g = transition(begin,g1,end,g2,value);
		const int b = transition(begin,b1,end,b2,value);
		return variant(
		       std::min<int>(99,std::max<int>(0,r))*100*100 +
		       std::min<int>(99,std::max<int>(0,g))*100+
		       std::min<int>(99,std::max<int>(0,b)));
	}
};


class abs_function : public function_expression {
public:
	explicit abs_function(const args_list& args)
	     : function_expression("abs", args, 1, 1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const int n = args()[0]->evaluate(variables).as_int();
		return variant(n >= 0 ? n : -n);
	}
};

class min_function : public function_expression {
public:
	explicit min_function(const args_list& args)
	     : function_expression("min", args, 1, -1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		bool found = false;
		int res = 0;
		for(size_t n = 0; n != args().size(); ++n) {
			const variant v = args()[n]->evaluate(variables);
			if(v.is_list()) {
				for(size_t m = 0; m != v.num_elements(); ++m) {
					if(!found || v[m].as_int() < res) {
						res = v[m].as_int();
						found = true;
					}
				}
			} else if(v.is_int()) {
				if(!found || v.as_int() < res) {
					res = v.as_int();
					found = true;
				}
			}
		}

		return variant(res);
	}
};

class max_function : public function_expression {
public:
	explicit max_function(const args_list& args)
	     : function_expression("max", args, 1, -1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		bool found = false;
		int res = 0;
		for(size_t n = 0; n != args().size(); ++n) {
			const variant v = args()[n]->evaluate(variables);
			if(v.is_list()) {
				for(size_t m = 0; m != v.num_elements(); ++m) {
					if(!found || v[m].as_int() > res) {
						res = v[m].as_int();
						found = true;
					}
				}
			} else if(v.is_int()) {
				if(!found || v.as_int() > res) {
					res = v.as_int();
					found = true;
				}
			}
		}

		return variant(res);
	}
};

class keys_function : public function_expression {
public:
	explicit keys_function(const args_list& args)
	     : function_expression("keys", args, 1, 1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const variant map = args()[0]->evaluate(variables);
		return map.get_keys();
	}
};

class values_function : public function_expression {
public:
	explicit values_function(const args_list& args)
	     : function_expression("values", args, 1, 1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const variant map = args()[0]->evaluate(variables);
		return map.get_values();
	}
};

class choose_function : public function_expression {
public:
	explicit choose_function(const args_list& args)
	     : function_expression("choose", args, 1, 2)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const variant items = args()[0]->evaluate(variables);
		int max_index = -1;
		variant max_value;
		for(size_t n = 0; n != items.num_elements(); ++n) {
			variant val;
			
			if(args().size() >= 2) {
				formula_callable_ptr callable(new formula_variant_callable_with_backup(items[n], variables));
				val = args()[1]->evaluate(*callable);
			} else {
				val = variant(rand());
			}

			if(max_index == -1 || val > max_value) {
				max_index = n;
				max_value = val;
			}
		}

		if(max_index == -1) {
			return variant();
		} else {
			return items[max_index];
		}
	}
};

class wave_function : public function_expression {
public:
	explicit wave_function(const args_list& args)
	     : function_expression("wave", args, 1, 1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const int value = args()[0]->evaluate(variables).as_int()%1000;
		const double angle = 2.0*3.141592653589*(static_cast<double>(value)/1000.0);
		return variant(static_cast<int>(sin(angle)*1000.0));
	}
};

namespace {
class variant_comparator : public formula_callable {
	expression_ptr expr_;
	const formula_callable* fallback_;
	mutable variant a_, b_;
	variant get_value(const std::string& key) const {
		if(key == "a") {
			return a_;
		} else if(key == "b") {
			return b_;
		} else {
			return fallback_->query_value(key);
		}
	}

	void get_inputs(std::vector<formula_input>* inputs) const {
		fallback_->get_inputs(inputs);
	}
public:
	variant_comparator(const expression_ptr& expr, const formula_callable& fallback) : formula_callable(false), expr_(expr), fallback_(&fallback)
	{}

	bool operator()(const variant& a, const variant& b) const {
		a_ = a;
		b_ = b;
		return expr_->evaluate(*this).as_bool();
	}
};
}

class sort_function : public function_expression {
public:
	explicit sort_function(const args_list& args)
	     : function_expression("sort", args, 1, 2)
	{}

private:
	variant execute(const formula_callable& variables) const {
		variant list = args()[0]->evaluate(variables);
		std::vector<variant> vars;
		vars.reserve(list.num_elements());
		for(size_t n = 0; n != list.num_elements(); ++n) {
			vars.push_back(list[n]);
		}

		if(args().size() == 1) {
			std::sort(vars.begin(), vars.end());
		} else {
			std::sort(vars.begin(), vars.end(), variant_comparator(args()[1], variables));
		}

		return variant(&vars);
	}
};

class filter_function : public function_expression {
public:
	explicit filter_function(const args_list& args)
	    : function_expression("filter", args, 2, 3)
	{}
private:
	variant execute(const formula_callable& variables) const {
		std::vector<variant> vars;
		const variant items = args()[0]->evaluate(variables);
		if(args().size() == 2) {
			for(size_t n = 0; n != items.num_elements(); ++n) {
				formula_callable_ptr callable(new formula_variant_callable_with_backup(items[n], variables));
				const variant val = args()[1]->evaluate(*callable);
				if(val.as_bool()) {
					vars.push_back(items[n]);
				}
			}
		} else {
			map_formula_callable* self_callable = new map_formula_callable;
			formula_callable_ptr callable(self_callable);
			self_callable->add("context", variant(&variables));
			const std::string self = args()[1]->evaluate(variables).as_string();
			for(size_t n = 0; n != items.num_elements(); ++n) {
				self_callable->add(self, items[n]);
				formula_callable_ptr callable_with_backup(new formula_variant_callable_with_backup(items[n], variables));
				formula_callable_ptr callable_ptr(new formula_callable_with_backup(*self_callable, *callable_with_backup));
				const variant val = args()[2]->evaluate(*callable_ptr);
				if(val.as_bool()) {
					vars.push_back(items[n]);
				}
			}
		}

		return variant(&vars);
	}
};

class mapping_function : public function_expression {
public:
	explicit mapping_function(const args_list& args)
	  : function_expression("mapping", args, -1, -1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		map_formula_callable* callable = new map_formula_callable;
		for(size_t n = 0; n < args().size()-1; n += 2) {
			callable->add(args()[n]->evaluate(variables).as_string(),
			              args()[n+1]->evaluate(variables));
		}

		return variant(callable);
	}
};

class find_function : public function_expression {
public:
	explicit find_function(const args_list& args)
	    : function_expression("find", args, 2, 3)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const variant items = args()[0]->evaluate(variables);

		if(args().size() == 2) {
			for(size_t n = 0; n != items.num_elements(); ++n) {
				formula_callable_ptr callable(new formula_variant_callable_with_backup(items[n], variables));
				const variant val = args()[1]->evaluate(*callable);
				if(val.as_bool()) {
					return items[n];
				}
			}
		} else {
			map_formula_callable* self_callable = new map_formula_callable;
			formula_callable_ptr callable(self_callable);
			const std::string self = args()[1]->evaluate(variables).as_string();
			for(size_t n = 0; n != items.num_elements(); ++n) {
				self_callable->add(self, items[n]);

				boost::intrusive_ptr<formula_variant_callable_with_backup> callable_backup(new formula_variant_callable_with_backup(items[n], variables));

				formula_callable_ptr callable(new formula_callable_with_backup(*self_callable, *callable_backup));
				const variant val = args().back()->evaluate(*callable);
				if(val.as_bool()) {
					return items[n];
				}
			}
		}

		return variant();
	}
};

class transform_callable : public formula_callable {
public:
	explicit transform_callable(const formula_callable& backup)
	  : backup_(backup)
	{}

	void set(const variant& v, const variant& i)
	{
		value_ = v;
		index_ = i;
	}
private:
	variant get_value(const std::string& key) const {
		if(key == "v") {
			return value_;
		} else if(key == "i") {
			return index_;
		} else {
			return backup_.query_value(key);
		}
	}

	variant get_value_by_slot(int slot) const {
		return backup_.query_value_by_slot(slot);
	}

	const formula_callable& backup_;
	variant value_, index_;
};

class transform_function : public function_expression {
public:
	explicit transform_function(const args_list& args)
	    : function_expression("transform", args, 2, 2)
	{}
private:
	variant execute(const formula_callable& variables) const {
		std::vector<variant> vars;
		const variant items = args()[0]->evaluate(variables);

		vars.reserve(items.num_elements());

		transform_callable* callable = new transform_callable(variables);
		variant v(callable);

		const int nitems = items.num_elements();
		for(size_t n = 0; n != nitems; ++n) {
			callable->set(items[n], variant(n));
			const variant val = args().back()->evaluate(*callable);
			vars.push_back(val);
		}

		return variant(&vars);
	}
};

class map_function : public function_expression {
public:
	explicit map_function(const args_list& args)
	    : function_expression("map", args, 2, 3)
	{}
private:
	variant execute(const formula_callable& variables) const {
		std::vector<variant> vars;
		const variant items = args()[0]->evaluate(variables);

		vars.reserve(items.num_elements());

		if(args().size() == 2) {
			for(size_t n = 0; n != items.num_elements(); ++n) {
				formula_callable_ptr callable(new formula_variant_callable_with_backup(items[n], variables));
				const variant val = args().back()->evaluate(*callable);
				vars.push_back(val);
			}
		} else {
			static const std::string index_str = "index";
			map_formula_callable* self_callable = new map_formula_callable;
			formula_callable_ptr callable_ref(self_callable);
			self_callable->add("context", variant(&variables));
			const std::string self = args()[1]->evaluate(variables).as_string();
			for(size_t n = 0; n != items.num_elements(); ++n) {
				self_callable->add(self, items[n]);
				self_callable->add(index_str, variant(n));

				formula_callable_ptr callable_backup(new formula_callable_with_backup(*self_callable, variables));
				const variant val = args().back()->evaluate(*callable_backup);
				vars.push_back(val);
			}
		}

		return variant(&vars);
	}
};

class sum_function : public function_expression {
public:
	explicit sum_function(const args_list& args)
	    : function_expression("sum", args, 1, 2)
	{}
private:
	variant execute(const formula_callable& variables) const {
		variant res(0);
		const variant items = args()[0]->evaluate(variables);
		if(args().size() >= 2) {
			res = args()[1]->evaluate(variables);
		}
		for(size_t n = 0; n != items.num_elements(); ++n) {
			res = res + items[n];
		}

		return res;
	}
};

class range_function : public function_expression {
public:
	explicit range_function(const args_list& args)
	  : function_expression("range", args, 1, 1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const int nelem = args()[0]->evaluate(variables).as_int();
		std::vector<variant> v;

		if(nelem > 0) {
			v.reserve(nelem);

			for(int n = 0; n < nelem; ++n) {
				v.push_back(variant(n));
			}
		}

		return variant(&v);
	}
};

class head_function : public function_expression {
public:
	explicit head_function(const args_list& args)
	    : function_expression("head", args, 1, 1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const variant items = args()[0]->evaluate(variables);
		return items[0];
	}
};

class size_function : public function_expression {
public:
	explicit size_function(const args_list& args)
	    : function_expression("size", args, 1, 1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const variant items = args()[0]->evaluate(variables);
		return variant(static_cast<int>(items.num_elements()));
	}
};

class slice_function : public function_expression {
public:
	explicit slice_function(const args_list& args)
	    : function_expression("slice", args, 3, 3)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const variant list = args()[0]->evaluate(variables);
		if(list.num_elements() == 0) {
			return variant();
		}
		int begin_index = args()[1]->evaluate(variables).as_int()%list.num_elements();
		int end_index = args()[2]->evaluate(variables).as_int()%list.num_elements();
		if(end_index >= begin_index) {
			std::vector<variant> result;
			result.reserve(end_index - begin_index);
			while(begin_index != end_index) {
				result.push_back(list[begin_index++]);
			}

			return variant(&result);
		} else {
			return variant();
		}
	}
};

class str_function : public function_expression {
public:
	explicit str_function(const args_list& args)
	    : function_expression("str", args, 1, 1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const variant item = args()[0]->evaluate(variables);
		std::string str;
		item.serialize_to_string(str);
		return variant(str);
	}
};

class strstr_function : public function_expression {
public:
	explicit strstr_function(const args_list& args)
	  : function_expression("strstr", args, 2, 2)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const std::string& haystack = args()[0]->evaluate(variables).as_string();
		const std::string& needle = args()[1]->evaluate(variables).as_string();
		return variant(strstr(haystack.c_str(), needle.c_str()) != NULL);
	}
};

class null_function : public function_expression {
public:
	explicit null_function(const args_list& args)
	    : function_expression("null", args, 0, 0)
	{}
private:
	variant execute(const formula_callable& /*variables*/) const {
		return variant();
	}
};

class refcount_function : public function_expression {
public:
	explicit refcount_function(const args_list& args)
	    : function_expression("refcount", args, 1, 1)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(args()[0]->evaluate(variables).refcount());
	}
};

class deserialize_function : public function_expression {
public:
	explicit deserialize_function(const args_list& args)
	  : function_expression("deserialize", args, 1, 1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const intptr_t id = strtoll(args()[0]->evaluate(variables).as_string().c_str(), NULL, 16);
		return variant::create_variant_under_construction(id);
	}
};

}

formula_function_expression::formula_function_expression(const std::string& name, const args_list& args, const_formula_ptr formula, const_formula_ptr precondition, const std::vector<std::string>& arg_names)
  : function_expression(name, args, arg_names.size(), arg_names.size()),
    formula_(formula), precondition_(precondition), arg_names_(arg_names), star_arg_(-1)
{
	for(size_t n = 0; n != arg_names_.size(); ++n) {
		if(arg_names_.empty() == false && arg_names_[n][arg_names_[n].size()-1] == '*') {
			arg_names_[n].resize(arg_names_[n].size()-1);
			star_arg_ = n;
			break;
		}
	}
}

variant formula_function_expression::execute(const formula_callable& variables) const
{
	if(!callable_ || callable_->refcount() != 1) {
		callable_ = boost::intrusive_ptr<slot_formula_callable>(new slot_formula_callable);
	}

	callable_->set_names(&arg_names_);

	//we reset callable_ to NULL during any calls so that recursive calls
	//will work properly.
	boost::intrusive_ptr<slot_formula_callable> tmp_callable(callable_);
	callable_.reset(NULL);

	for(size_t n = 0; n != arg_names_.size(); ++n) {
		variant var = args()[n]->evaluate(variables);
		tmp_callable->add(var);
		if(static_cast<int>(n) == star_arg_) {
			tmp_callable->set_fallback(var.as_callable());
		}
	}

	if(precondition_) {
		if(!precondition_->execute(*tmp_callable).as_bool()) {
			std::cerr << "FAILED function precondition for function '" << formula_->str() << "' with arguments: ";
			for(size_t n = 0; n != arg_names_.size(); ++n) {
				std::cerr << "  arg " << (n+1) << ": " << args()[n]->evaluate(variables).to_debug_string() << "\n";
			}
		}
	}

	variant res = formula_->execute(*tmp_callable);

	callable_ = tmp_callable;
	callable_->clear();

	return res;
}

formula_function_expression_ptr formula_function::generate_function_expression(const std::vector<expression_ptr>& args) const
{
	return formula_function_expression_ptr(new formula_function_expression(name_, args, formula_, precondition_, args_));
}

void function_symbol_table::add_formula_function(const std::string& name, const_formula_ptr formula, const_formula_ptr precondition, const std::vector<std::string>& args)
{
	custom_formulas_[name] = formula_function(name, formula, precondition, args);
}

expression_ptr function_symbol_table::create_function(const std::string& fn, const std::vector<expression_ptr>& args, const formula_callable_definition* callable_def) const
{
	const std::map<std::string, formula_function>::const_iterator i = custom_formulas_.find(fn);
	if(i != custom_formulas_.end()) {
		return i->second.generate_function_expression(args);
	}

	if(backup_) {
		return backup_->create_function(fn, args, callable_def);
	}

	return expression_ptr();
}

std::vector<std::string> function_symbol_table::get_function_names() const
{
	std::vector<std::string> res;
	for(std::map<std::string, formula_function>::const_iterator iter = custom_formulas_.begin(); iter != custom_formulas_.end(); iter++ ) {
		res.push_back((*iter).first);
	}
	return res;
}

recursive_function_symbol_table::recursive_function_symbol_table(const std::string& fn, const std::vector<std::string>& args, function_symbol_table* backup)
  : name_(fn), stub_(fn, const_formula_ptr(), const_formula_ptr(), args), backup_(backup)
{
}

expression_ptr recursive_function_symbol_table::create_function(
                 const std::string& fn,
	             const std::vector<expression_ptr>& args,
				 const formula_callable_definition* callable_def) const
{
	if(fn == name_) {
		formula_function_expression_ptr expr = stub_.generate_function_expression(args);
		expr_.push_back(expr);
		return expr;
	} else if(backup_) {
		return backup_->create_function(fn, args, callable_def);
	}

	return expression_ptr();
}

void recursive_function_symbol_table::resolve_recursive_calls(const_formula_ptr f)
{
	foreach(formula_function_expression_ptr& fn, expr_) {
		fn->set_formula(f);
	}
}

namespace {

class base_function_creator {
public:
	virtual expression_ptr create_function(const std::vector<expression_ptr>& args) const = 0;
	virtual ~base_function_creator() {}
};

template<typename T>
class function_creator : public base_function_creator {
public:
	virtual expression_ptr create_function(const std::vector<expression_ptr>& args) const {
		return expression_ptr(new T(args));
	}
	virtual ~function_creator() {}
};

typedef std::map<std::string, base_function_creator*> functions_map;

functions_map& get_functions_map() {

	static functions_map functions_table;

	if(functions_table.empty()) {
#define FUNCTION(name) functions_table[#name] = new function_creator<name##_function>();
		FUNCTION(dir);
		FUNCTION(if);
		FUNCTION(switch);
		FUNCTION(query);
		FUNCTION(abs);
		FUNCTION(min);
		FUNCTION(max);
		FUNCTION(choose);
		FUNCTION(wave);
		FUNCTION(sort);
		FUNCTION(filter);
		FUNCTION(mapping);
		FUNCTION(find);
		FUNCTION(transform);
		FUNCTION(map);
		FUNCTION(sum);
		FUNCTION(range);
		FUNCTION(head);
		FUNCTION(rgb);
		FUNCTION(transition);
		FUNCTION(color_transition);
		FUNCTION(size);
		FUNCTION(slice);
		FUNCTION(str);
		FUNCTION(strstr);
		FUNCTION(null);
		FUNCTION(refcount);
		FUNCTION(keys);
		FUNCTION(values);
		FUNCTION(deserialize);
#undef FUNCTION
	}

	return functions_table;
}

}

expression_ptr create_function(const std::string& fn,
                               const std::vector<expression_ptr>& args,
							   const function_symbol_table* symbols,
							   const formula_callable_definition* callable_def)
{
	if(symbols) {
		expression_ptr res(symbols->create_function(fn, args, callable_def));
		if(res) {
			return res;
		}
	}

	functions_map::const_iterator i = get_functions_map().find(fn);
	if(i == get_functions_map().end()) {
		return expression_ptr();
	}

	return i->second->create_function(args);
}

bool optimize_function_arguments(const std::string& fn,
                                 const function_symbol_table* symbols)
{
	static const std::string disable_optimization_functions[] = {
	    "choose", "filter", "find", "map",
	};

	for(int n = 0; n < sizeof(disable_optimization_functions)/sizeof(*disable_optimization_functions); ++n) {
		if(disable_optimization_functions[n] == fn) {
			return false;
		}
	}

	return true;
}

std::vector<std::string> builtin_function_names()
{
	std::vector<std::string> res;
	const functions_map& m = get_functions_map();
	for(functions_map::const_iterator i = m.begin(); i != m.end(); ++i) {
		res.push_back(i->first);
	}

	return res;
}

function_expression::function_expression(
                    const std::string& name,
                    const args_list& args,
                    int min_args, int max_args)
    : name_(name), args_(args)
{
	set_name(name.c_str());
	if(min_args >= 0 && args_.size() < static_cast<size_t>(min_args)) {
		std::cerr << "ERROR: incorrect number of arguments to function '" << name << "': expected [" << min_args << "," << max_args << "], found " << args_.size() << "\n";
		throw formula_error();
	}

	if(max_args >= 0 && args_.size() > static_cast<size_t>(max_args)) {
		std::cerr << "ERROR: incorrect number of arguments to function '" << name << "': expected [" << min_args << "," << max_args << "], found " << args_.size() << "\n";
		throw formula_error();
	}
}


}
