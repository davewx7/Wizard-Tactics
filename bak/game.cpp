#include <algorithm>

#include "asserts.hpp"
#include "card.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "game.hpp"
#include "string_utils.hpp"
#include "terrain.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "wml_writer.hpp"

game::game()
  : started_(false), state_(STATE_SETUP), player_turn_(-1)
{
}

game::game(wml::const_node_ptr node)
  : started_(wml::get_bool(node, "started")),
    width_(wml::get_int(node, "width")),
    height_(wml::get_int(node, "height")),
	state_(STATE_SETUP), player_turn_(-1)
{
	std::vector<std::string> v = util::split(node->attr("tiles"));
	for(int n = 0; n != v.size(); ++n) {
		const int x = n%width_;
		const int y = n/width_;
		tiles_.push_back(tile(x, y, v[n]));
	}

	ASSERT_EQ(width_*height_, tiles_.size());

	FOREACH_WML_CHILD(city_node, node, "city") {
		cities_.push_back(city_ptr(new city(city_node)));
	}

	FOREACH_WML_CHILD(player_node, node, "player") {
		players_.push_back(player());
		players_.back().name = player_node->attr("name");
		players_.back().deck = read_deck(player_node->attr("deck"));
		players_.back().hand = read_deck(player_node->attr("hand"));
	}
}

wml::node_ptr game::write() const
{
	wml::node_ptr res(new wml::node("game"));
	res->set_attr("started", started_ ? "yes" : "no");
	res->set_attr("width", formatter() << width_);
	res->set_attr("height", formatter() << height_);
	std::string tiles;
	for(int n = 0; n != tiles_.size(); ++n) {
		if(n != 0) {
			tiles += ",";
		}

		tiles_[n].write(tiles);
	}

	res->set_attr("tiles", tiles);

	foreach(city_ptr c, cities_) {
		res->add_child(c->write());
	}

	foreach(const player& p, players_) {
		wml::node_ptr player_node(new wml::node("player"));
		player_node->set_attr("name", p.name);
		player_node->set_attr("deck", write_deck(p.deck));
		player_node->set_attr("hand", write_deck(p.hand));
		res->add_child(player_node);
	}

	return res;
}

void game::handle_message(int nplayer, const simple_wml::string_span& type, const simple_wml::node& msg)
{
	if(type == "setup") {
		setup_game();
		queue_message(wml::output(write()));

		ASSERT_GE(players_.size(), 1);
		queue_message(formatter() << "[new_turn]\nplayer=\"" << players_.front().name << "\"\n[/new_turn]\n");
		state_ = STATE_PLAYING;
		player_turn_ = 0;
	} else if(type == "deck") {
		ASSERT_INDEX_INTO_VECTOR(nplayer, players_);
		players_[nplayer].deck = read_deck(msg.attr("cards").to_string());
		std::random_shuffle(players_[nplayer].deck.begin(),
		                    players_[nplayer].deck.end());
		draw_hand(nplayer);
		queue_message(wml::output(write()));
	} else if(type == "play") {
		play_card(nplayer, msg);
	} else if(type == "end_turn") {
		ASSERT_INDEX_INTO_VECTOR(nplayer, players_);
		if(msg.attr("discard") == "yes") {
			players_[nplayer].hand.clear();
		}

		draw_hand(nplayer);
		++player_turn_;
		if(player_turn_ == players_.size()) {
			player_turn_ = 0;
		}

		queue_message(wml::output(write()));

		queue_message(formatter() << "[new_turn]\nplayer=\"" << players_[player_turn_].name << "\"\n[/new_turn]\n");
	}
}

void game::play_card(int nplayer, const simple_wml::node& msg)
{
	const std::string type = msg.attr("card").to_string();
	ASSERT_INDEX_INTO_VECTOR(nplayer, players_);
	player& p = players_[nplayer];
	const_card_ptr card;
	foreach(const_card_ptr& c, p.hand) {
		if(c->id() == type) {
			card = c;
			c.reset();
			break;
		}
	}

	p.hand.erase(std::remove(p.hand.begin(), p.hand.end(), const_card_ptr()), p.hand.end());

	ASSERT_LOG(card.get() != NULL, "CARD " << type << " NOT FOUND IN HAND OF PLAYER " << nplayer);

	foreach(const simple_wml::node* land, msg.children("land")) {
		hex::location loc(land->attr("x").to_int(), land->attr("y").to_int());
		tile* t = get_tile(loc.x(), loc.y());
		ASSERT_LOG(t, "ILLEGAL TILE LOCATION: " << loc);
		*t = tile(loc.x(), loc.y(), land->attr("land").to_string());
	}

	queue_message(wml::output(write()));
}

void game::draw_hand(int nplayer)
{
	ASSERT_INDEX_INTO_VECTOR(nplayer, players_);
	player& p = players_[nplayer];
	while(!p.deck.empty() && p.hand.size() < 5) {
		p.hand.push_back(p.deck.back());
		p.deck.pop_back();
	}
}

void game::swap_outgoing_messages(std::vector<message>& msg)
{
	outgoing_messages_.swap(msg);
	outgoing_messages_.clear();
}

void game::queue_message(const std::string& msg, int nplayer)
{
	outgoing_messages_.push_back(message());
	outgoing_messages_.back().contents = msg;
	if(nplayer >= 0) {
		outgoing_messages_.back().recipients.push_back(nplayer);
	}
}

void game::send_error(const std::string& msg, int nplayer)
{
	wml::node_ptr node(new wml::node("error"));
	node->set_attr("message", msg);
	queue_message(wml::output(node), nplayer);
}

bool game::add_city(city_ptr new_city)
{
	cities_.push_back(new_city);
	return true;
}

void game::setup_game()
{
	started_ = true;
	width_ = 15;
	height_ = 9;
	tiles_.clear();

	std::vector<std::pair<int, int> > tower_locs;
	for(int n = 0; n != 8; ++n) {
		const int x = rand()%width_;
		const int y = rand()%height_;

		tower_locs.push_back(std::pair<int, int>(x, y));
		tower_locs.push_back(std::pair<int, int>(width_ - x - 1, y));
	}

	const std::vector<std::string> terrains = terrain::all();
	for(int y = 0; y != height_; ++y) {
		for(int x = 0; x != width_; ++x) {
			const char* type = "void";
			if(std::find(tower_locs.begin(), tower_locs.end(), std::pair<int,int>(x,y)) != tower_locs.end()) {
				type = "tower";
			}
			tiles_.push_back(tile(x, y, type));

			if(y == 4 && (x == 1 || x == 13)) {
				tiles_.back() = tile(x, y, "castle");
			}
		}
	}
}

const tile* game::get_tile(int x, int y) const
{
	if(!started()) {
		return 0;
	}

	if(x < 0 || y < 0 || x >= width_ || y >= height_) {
		return 0;
	}

	ASSERT_EQ(width_*height_, tiles_.size());

	return &tiles_[y*width_ + x];
}

tile* game::get_tile(int x, int y)
{
	if(!started()) {
		return 0;
	}

	if(x < 0 || y < 0 || x >= width_ || y >= height_) {
		return 0;
	}

	ASSERT_EQ(width_*height_, tiles_.size());

	return &tiles_[y*width_ + x];
}

void game::add_player(const std::string& name)
{
	players_.push_back(player());
	players_.back().name = name;
}
