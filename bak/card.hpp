#ifndef CARD_HPP_INCLUDED
#define CARD_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "texture.hpp"
#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"

#include <string>
#include <vector>

class card;
class client_play_game;
class game;
typedef boost::shared_ptr<const card> card_ptr;
typedef boost::shared_ptr<const card> const_card_ptr;

class card_selector
{
public:
	virtual hex::location player_select_loc(const std::string& prompt,
	                          boost::function<bool(hex::location)> valid_loc) = 0;
	virtual const game& get_game() const = 0;
protected:
	~card_selector() {}
};

class card
{
public:
	static const_card_ptr create(wml::const_node_ptr node);
	static const_card_ptr get(const std::string& id);

	explicit card(wml::const_node_ptr node);
	virtual ~card();

	virtual wml::node_ptr play_card(card_selector& client) const = 0;

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }
	int cost(int nresource) const;
private:
	std::string id_, name_;
	std::vector<int> cost_;
};

std::vector<const_card_ptr> read_deck(const std::string& str);
std::string write_deck(const std::vector<const_card_ptr>& cards);

#endif
