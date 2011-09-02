#ifndef WML_FORMULA_CALLABLE_HPP_INCLUDED
#define WML_FORMULA_CALLABLE_HPP_INCLUDED

#include <stdint.h>

#include <boost/intrusive_ptr.hpp>

#include "formula_callable.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class wml_serializable_formula_callable : public formula_callable
{
public:
	explicit wml_serializable_formula_callable(bool has_self=true) : formula_callable(has_self) {}

	virtual ~wml_serializable_formula_callable() {}

	wml::node_ptr write_to_wml() const {
		return serialize_to_wml();
	}
private:
	virtual wml::node_ptr serialize_to_wml() const = 0;

};

typedef boost::intrusive_ptr<wml_serializable_formula_callable> wml_serializable_formula_callable_ptr;
typedef boost::intrusive_ptr<const wml_serializable_formula_callable> const_wml_serializable_formula_callable_ptr;

class wml_formula_callable_serialization_scope
{
public:
	static void register_serialized_object(const_wml_serializable_formula_callable_ptr ptr, wml::node_ptr node);
	static std::string require_serialized_object(const_wml_serializable_formula_callable_ptr ptr);
	static bool is_active();

	wml_formula_callable_serialization_scope();
	~wml_formula_callable_serialization_scope();

	wml::node_ptr write_objects() const;

private:
};

class wml_formula_callable_read_scope
{
public:
	static void register_serialized_object(intptr_t addr, wml_serializable_formula_callable_ptr ptr);
	static wml_serializable_formula_callable_ptr get_serialized_object(intptr_t addr);
	wml_formula_callable_read_scope();
	~wml_formula_callable_read_scope();
private:
};

}

#endif
