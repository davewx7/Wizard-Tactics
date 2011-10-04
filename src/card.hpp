#ifndef CARD_HPP_INCLUDED
#define CARD_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "formula_fwd.hpp"
#include "texture.hpp"
#include "tile_logic.hpp"
#include "unit.hpp"
#include "wml_node_fwd.hpp"

#include <map>
#include <set>
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
	virtual bool player_pay_cost(const std::vector<int>& cost) = 0;
	virtual int player_id() const = 0;
	virtual const game& get_game() const = 0;
	virtual game& get_game() = 0;
protected:
	~card_selector() {}
};

enum CARD_ACTIVATION_TYPE { CARD_ACTIVATION_PLAYER, CARD_ACTIVATION_EVENT };

class resolve_card_info : public game_logic::formula_callable
{
public:
	virtual unit_ptr caster() const = 0;
	virtual std::vector<hex::location> targets() const = 0;
	virtual CARD_ACTIVATION_TYPE activation_type() const { return CARD_ACTIVATION_PLAYER; }
protected:
	~resolve_card_info() {}
};

class card
{
public:
	static const_card_ptr create(wml::const_node_ptr node);
	static const_card_ptr get(const std::string& id);

	explicit card(wml::const_node_ptr node);
	virtual ~card();

	wml::node_ptr use_card(card_selector& client) const;
	wml::node_ptr use_card(unit* caster, int side, const std::vector<hex::location>& targets) const;

	virtual bool is_card_playable(unit* caster, int side, const std::vector<hex::location>& targets, std::vector<hex::location>& possible_targets) const = 0;

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }
	const boost::shared_ptr<std::string>& description() const { return description_; }
	int cost(int nresource) const;
	const std::vector<int>& cost() const { return cost_; }

	int speed() const { return speed_; }
	bool taps_caster() const { return taps_caster_; }

	virtual const std::string* monster_id() const { return NULL; }
	virtual const std::string* land_id() const { return NULL; }
	virtual const unit::modification* modification() const { return NULL; }

	virtual bool calculate_valid_targets(const unit* caster, int side, std::vector<hex::location>& result) const;

	virtual void resolve_card(const resolve_card_info* callable=NULL) const;

	virtual bool is_attack() const { return false; }

	//useful for displaying the card -- how much damage does it do?
	virtual int damage() const { return 0; }

protected:
	void handle_event(const std::string& name, const game_logic::formula_callable* callable=NULL) const;
private:
	virtual wml::node_ptr play_card(card_selector& client) const = 0;
	virtual wml::node_ptr play_card(unit* caster, int side, const std::vector<hex::location>& targets) const = 0;
	std::string id_, name_;
	boost::shared_ptr<std::string> description_;
	std::vector<int> cost_;
	int speed_;

	bool taps_caster_;

	typedef std::map<std::string, game_logic::const_formula_ptr> handlers_map;
	handlers_map handlers_;

	game_logic::const_formula_ptr valid_targets_;
};

struct held_card {
	const_card_ptr card;
	int embargo;
};

std::vector<held_card> read_deck(const std::string& str);
std::string write_deck(const std::vector<held_card>& cards);

bool is_valid_summoning_hex(const game* g, int player, const hex::location& loc, const_unit_ptr proto);

#endif
