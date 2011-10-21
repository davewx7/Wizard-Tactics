#ifndef PLAYER_INFO_HPP_INCLUDED
#define PLAYER_INFO_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <set>
#include <string>
#include <vector>

#include "resource.hpp"
#include "wml_node_fwd.hpp"

class player_info {
public:
	player_info();
	explicit player_info(wml::const_node_ptr node);

	const std::string& id() const { return id_; }
	void set_id(const std::string& s) { id_ = s; }

	const std::vector<int>& resources() const { return resources_; }
	const std::vector<std::string>& deck() const { return deck_; }
	const std::vector<std::string>& collection() const { return collection_; }

	bool add_to_deck(const std::string& id);
	bool remove_from_deck(const std::string& id);

	bool modify_resources(char resource, int delta);

	void read(wml::const_node_ptr node);
	wml::node_ptr write() const;
private:
	std::string id_;
	std::vector<int> resources_;
	std::vector<std::string> deck_, collection_;
};

typedef boost::shared_ptr<player_info> player_info_ptr;
typedef boost::shared_ptr<const player_info> const_player_info_ptr;

#endif
