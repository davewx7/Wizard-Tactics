#include <numeric>
#include <string.h>

#include "card.hpp"
#include "filesystem.hpp"
#include "formatter.hpp"
#include "player_info.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "xml_parser.hpp"

bool is_dummy_card(const std::string& card) {
	return strstr(card.c_str(), "dummy") != NULL;
}

player_info::player_info()
{
	read(wml::parse_xml(sys::read_file("deck.xml")));
	collection_ = card::get_all_cards();
	collection_.erase(std::remove_if(collection_.begin(), collection_.end(), is_dummy_card), collection_.end());
}

player_info::player_info(wml::const_node_ptr node)
{
	read(node);
	collection_ = card::get_all_cards(); //temporarily make everyone get all cards.
	collection_.erase(std::remove_if(collection_.begin(), collection_.end(), is_dummy_card), collection_.end());
}

bool player_info::add_to_deck(const std::string& id)
{
	//we can't add it if our deck is too big, if the spell is already in there
	//or if we don't own the spell in our collection.
	if(deck_.size() >= 10 || std::count(deck_.begin(), deck_.end(), id) != 0 ||
	   std::count(collection_.begin(), collection_.end(), id) == 0) {
		return false;
	}

	deck_.push_back(id);
	return true;
}

bool player_info::remove_from_deck(const std::string& id)
{
	deck_.erase(std::remove(deck_.begin(), deck_.end(), id), deck_.end());
	return true;
}

bool player_info::modify_resources(char resource, int delta)
{
	const int index = resource::resource_index(resource);
	if(index < 0 || index >= resources_.size()) {
		return false;
	}

	const int total_resources = std::accumulate(resources_.begin(), resources_.end(), 0);
	if(total_resources + delta > 10) {
		return false;
	}

	if(resources_[index] + delta < 0) {
		return false;
	}

	resources_[index] += delta;

	return true;
}

void player_info::read(wml::const_node_ptr node)
{
	id_ = node->attr("id");
	resources_.resize(resource::num_resources());
	int nresources = resources_.size();
	util::split_into_ints(node->attr("resource_gain").str().c_str(), &resources_[0], &nresources);

	deck_ = util::split(node->attr("spells").str());
	collection_ = util::split(node->attr("collection").str());
}

wml::node_ptr player_info::write() const
{
	wml::node_ptr node(new wml::node("player_info"));
	node->set_attr("id", id_);
	node->set_attr("resource_gain", util::join_ints(&resources_[0], resources_.size()));
	node->set_attr("spells", util::join(deck_));
	node->set_attr("collection", util::join(collection_));

	return node;
}
