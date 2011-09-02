#ifndef FORMULA_CONSTANTS_HPP_INCLUDED
#define FORMULA_CONSTANTS_HPP_INCLUDED

#include "variant.hpp"
#include "wml_node_fwd.hpp"

#include <string>

namespace game_logic
{

variant get_constant(const std::string& id);

class constants_loader
{
public:
	explicit constants_loader(wml::const_node_ptr node);
	~constants_loader();

	bool same_as_base() const { return same_as_base_; }

private:
	bool same_as_base_;
};

}

#endif
