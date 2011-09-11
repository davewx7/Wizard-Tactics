#include "asserts.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "game.hpp"
#include "resource.hpp"
#include "string_utils.hpp"
#include "unit.hpp"
#include "unit_utils.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"

void unit::assign_new_unit_key()
{
	static int unit_key = 1;
	key_ = unit_key++;
}

unit::modification::modification()
  : id(0), life(0), move(0), expires_end_of_turn(false)
{}

unit::modification::modification(wml::const_node_ptr node)
  : id(wml::get_int(node, "id")),
    life(wml::get_int(node, "life")),
    armor(wml::get_int(node, "armor")),
    move(wml::get_int(node, "move")),
    expires_end_of_turn(wml::get_bool(node, "expires_end_of_turn"))
{}

wml::node_ptr unit::modification::write() const
{
	wml::node_ptr node(new wml::node("mod"));
	node->set_attr("id", formatter() << id);
	node->set_attr("life", formatter() << life);
	node->set_attr("armor", formatter() << armor);
	node->set_attr("move", formatter() << move);
	node->set_attr("expires_end_of_turn", expires_end_of_turn ? "yes" : "no");
	return node;
}

int unit::resource_type() const
{
	if(upkeep_.empty() == false) {
		return resource::resource_index(upkeep_[0]);
	}

	const game::player& player = game::current()->players()[side_];
	for(int n = 0; n != player.resource_gain.size(); ++n) {
		if(player.resource_gain[n]) {
			return n;
		}
	}

	return 0;
}

const_unit_ptr unit::get_prototype(const std::string& id)
{
	static std::map<std::string, const_unit_ptr> cache;
	const_unit_ptr& u = cache[id];
	if(!u.get()) {
		u.reset(new unit(wml::parse_wml_from_file("data/units/" + id + ".cfg")));
	}

	return u;
}

unit_ptr unit::build_from_prototype(const std::string& id, int side, const hex::location& loc)
{
	const_unit_ptr u = get_prototype(id);
	unit_ptr res(new unit(*u));
	res->side_ = side;
	res->loc_ = loc;
	res->assign_new_unit_key();
	return res;

}

unit::unit(wml::const_node_ptr node)
  : key_(wml::get_int(node, "key", -1)),
    id_(node->attr("id")), name_(node->attr("name")),
	upkeep_(node->attr("upkeep")),
	overlays_(util::split(node->attr("overlays").str())),
	underlays_(util::split(node->attr("underlays").str())),
    loc_(wml::get_int(node, "x"), wml::get_int(node, "y")),
	side_(wml::get_int(node, "side")),
    life_(wml::get_int(node, "life")),
    armor_(wml::get_int(node, "armor")),
	move_(wml::get_int(node, "move")),
	damage_taken_(wml::get_int(node, "damage_taken")),
	has_moved_(wml::get_bool(node, "has_moved", false)),
	mod_id_(wml::get_int(node, "mod_id")),
	move_type_(movement_type::get(node->attr("movement_type"))),
	can_summon_(node->attr("can_summon")),
	can_cast_(node->attr("can_cast")),
	can_produce_(wml::get_bool(node, "can_produce", false))
{
	if(key_ == -1) {
		assign_new_unit_key();
	}

	ASSERT_LOG(move_type_.get() != NULL, "Unknown movement type for " << id_ << ": " << node->attr("movement_type"));

	ASSERT_LOG(move_type_.get() != NULL, "Could not load movement type");

	FOREACH_WML_CHILD(mod_node, node, "mod") {
		mods_.push_back(mod_ptr(new modification(mod_node)));
	}

	FOREACH_WML_CHILD(ability_node, node, "ability") {
		abilities_.push_back(unit_ability_ptr(new unit_ability(id_, ability_node)));
	}
}

wml::node_ptr unit::write() const
{
	wml::node_ptr node(new wml::node("unit"));
	node->set_attr("movement_type", move_type_->id());
	node->set_attr("key", formatter() << key_);
	node->set_attr("id", id_);
	node->set_attr("overlays", util::join(overlays_));
	node->set_attr("underlays", util::join(underlays_));
	node->set_attr("name", name_);
	node->set_attr("upkeep", upkeep_);
	node->set_attr("side", formatter() << side_);
	node->set_attr("x", formatter() << loc_.x());
	node->set_attr("y", formatter() << loc_.y());
	node->set_attr("life", formatter() << life_);
	node->set_attr("armor", formatter() << armor_);
	node->set_attr("move", formatter() << move_);

	if(damage_taken_) {
		node->set_attr("damage_taken", formatter() << damage_taken_);
	}

	if(has_moved_) {
		node->set_attr("has_moved", "yes");
	}

	node->set_attr("mod_id", formatter() << mod_id_);

	foreach(mod_ptr m, mods_) {
		node->add_child(m->write());
	}

	node->set_attr("can_summon", can_summon_);
	node->set_attr("can_cast", can_cast_);
	node->set_attr("can_produce", can_produce_ ? "yes" : "no");

	foreach(unit_ability_ptr a, abilities_) {
		node->add_child(a->write());
	}

	return node;
}

const hex::location& unit::loc() const
{
	return loc_;
}

void unit::set_loc(const hex::location& loc)
{
	loc_ = loc;
}

int unit::side() const
{
	return side_;
}

int unit::life() const
{
	int result = life_;
	foreach(const mod_ptr& mod, mods_) {
		result += mod->life;
	}

	return result;
}

int unit::armor() const
{
	int result = armor_;
	foreach(const mod_ptr& mod, mods_) {
		result += mod->armor;
	}

	return result;
}

int unit::move() const
{
	int result = move_;
	foreach(const mod_ptr& mod, mods_) {
		result += mod->move;
	}

	return result;
}

void unit::new_turn()
{
	has_moved_ = false;
	movement_path_.clear();
}

void unit::heal()
{
	damage_taken_ = 0;
	std::vector<mod_ptr>::iterator i = mods_.begin();
	while(i != mods_.end()) {
		if((*i)->expires_end_of_turn) {
			mods_.erase(i++);
		} else {
			++i;
		}
	}
}

bool unit::can_summon(char resource_type) const
{
	return std::find(can_summon_.begin(), can_summon_.end(), resource_type) != can_summon_.end();
}

bool unit::can_cast(char resource_type) const
{
	return std::find(can_cast_.begin(), can_cast_.end(), resource_type) != can_cast_.end();
}

unit_ability_ptr unit::default_ability() const
{
	if(abilities_.empty()) {
		return unit_ability_ptr();
	}

	return abilities_.front();
}

void unit::add_modification(const modification& mod)
{
	mods_.push_back(mod_ptr(new modification(mod)));
}

variant unit::get_value(const std::string& key) const {
	if(key == "side") {
		return variant(side_);
	} else if(key == "life") {
		return variant(life());
	} else if(key == "damage") {
		return variant(damage_taken_);
	} else if(key == "has_moved") {
		return variant(has_moved_);
	} else {
		return variant();
	}
}

void unit::set_value(const std::string& key, const variant& value)
{
	if(key == "damage") {
		damage_taken_ = value.as_int();
	} else if(key == "has_moved") {
		has_moved_ = value.as_bool();
	} else if(key == "life") {
		life_ = value.as_int();
	}
}
