#include <boost/bind.hpp>

#include <map>

#include "asserts.hpp"
#include "card.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "game.hpp"
#include "raster.hpp"
#include "resource.hpp"
#include "string_utils.hpp"
#include "terrain.hpp"
#include "tile_logic.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"

const_card_ptr card::get(const std::string& id)
{
	static std::map<std::string, const_card_ptr> cache;
	if(cache.empty()) {
		wml::const_node_ptr cards_node = wml::parse_wml_from_file("data/cards.cfg");
		FOREACH_WML_CHILD(card_node, cards_node, "card") {
			const_card_ptr new_card(create(card_node));
			ASSERT_EQ(cache.count(new_card->id()), 0);
			cache[new_card->id()] = new_card;
		}
	}

	return cache[id];
}

card::card(wml::const_node_ptr node)
  : id_(node->attr("id")), name_(node->attr("name"))
{
	const std::string cost_str = node->attr("cost");
	for(int n = 0; n != resource::num_resources(); ++n) {
		cost_.push_back(std::count(cost_str.begin(), cost_str.end(), resource::resource_id(n)));
	}
}

card::~card()
{}

int card::cost(int nresource) const
{
	ASSERT_INDEX_INTO_VECTOR(nresource, cost_);
	return cost_[nresource];
}

namespace {

class land_card : public card {
public:
	explicit land_card(wml::const_node_ptr node);
	virtual ~land_card() {}
	virtual wml::node_ptr play_card(card_selector& client) const;
private:
	std::vector<std::string> land_;
};

land_card::land_card(wml::const_node_ptr node)
  : card(node), land_(util::split(node->attr("land")))
{
}

namespace {
bool is_void(const game* g, const hex::location loc) {
	const tile* t = g->get_tile(loc.x(), loc.y());
	return t && t->terrain()->id() == "void";
}
}

wml::node_ptr land_card::play_card(card_selector& client) const
{
	wml::node_ptr result(new wml::node("play"));
	result->set_attr("card", id());
	std::vector<hex::location> locs;
	foreach(const std::string& land, land_) {
		hex::location loc =
		  client.player_select_loc("Select where to place your land.",
		                           boost::bind(is_void, &client.get_game(), _1));
		if(loc.valid()) {
			wml::node_ptr land_node(hex::write_location("land", loc));
			land_node->set_attr("land", land);
			result->add_child(land_node);
		} else {
			return wml::node_ptr();
		}
	}

	return result;
}

}

const_card_ptr card::create(wml::const_node_ptr node)
{
	const std::string& type = node->attr("type");
	if(type == "land") {
		return const_card_ptr(new land_card(node));
	} else {
		ASSERT_LOG(false, "UNRECOGNIZED CARD TYPE '" << type << "'");
	}
}

std::vector<const_card_ptr> read_deck(const std::string& str)
{
	std::vector<const_card_ptr> result;
	std::vector<std::string> items = util::split(str);
	foreach(const std::string& item, items) {
		std::vector<std::string> v = util::split(item, ' ');
		ASSERT_LOG(v.size() == 1 || v.size() == 2, "ILLEGAL DECK FORMAT: " << str);
		int num_cards = 1;
		if(v.size() == 2) {
			num_cards = atoi(v[0].c_str());
		}

		const_card_ptr card_obj = card::get(v.back());
		for(int n = 0; n < num_cards; ++n) {
			result.push_back(card_obj);
		}
	}

	return result;
}

std::string write_deck(const std::vector<const_card_ptr>& cards)
{
	std::string result;
	for(int n = 0; n != cards.size(); ++n) {
		if(n != 0) {
			result += ",";
		}

		result += cards[n]->id();
	}

	return result;
}
