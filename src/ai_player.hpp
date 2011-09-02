#ifndef AI_PLAYER_HPP_INCLUDED
#define AI_PLAYER_HPP_INCLUDED

#include <vector>

#include "card.hpp"
#include "wml_node_fwd.hpp"

class game;

class ai_player : public card_selector {
public:
	static ai_player* create(game& g, int nplayer);
	ai_player(game& g, int nplayer);
	virtual ~ai_player();

	virtual std::vector<wml::node_ptr> play() = 0;
	int player_id() const { return nplayer_; }

	bool player_pay_cost(const std::vector<int>& cost);
protected:
	game& get_game() { return game_; }
	const game& get_game() const { return game_; }
private:
	game& game_;
	int nplayer_;
};

#endif
