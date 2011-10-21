var board_canvas = document.getElementById('board');

function get_board_canvas() {
	if(!board_canvas) {
		board_canvas = document.getElementById('board');
	}

	return board_canvas;
}

var ajax_script = '/cgi-bin/dave/wizard_ajax.pl';
var image_store = [];
var images_cache = {};
var user_id = '';
var player_side = -1;
var players_signed_up_for_game = 0;
var game = null;
var data_url = 'http://' + server_hostname + '/dave/';

var unit_move_info = null;
var choose_ability_info = null;

//the spell we're currently casting and selecting targets for.
var spell_casting = null;
var spell_targets = null;
var legal_targets = null;

var unit_casting = null;
var ability_using = null;

//an array which will contain info about each tile we draw
//on the map.
var draw_map_info = [];

//an array which contains info about each tile which needs
//to be drawn every frame and can't be composited into
//the background.
var draw_map_info_nobg = [];

var terrain_index = {};

var gui_sections_index = {};

var unit_animations = {};
var unit_prototypes = {};
var overlay_map = {};

var unit_avatars = {};

var HexHeight = 36;
var HexWidth = 58;

var mouseover_loc = null;

var bg_canvas = null;

var frame_number = 0;

var spells_index = {};
var current_spells_table = null;
var current_abilities_table = null;

//an object representing what info we are currently displaying
//in the info label.
var current_info_displayed = null;

//number of frames we delay processing messages to allow
//animations to play.
var animation_time = 0;
var responses_waiting_for_processing = [];

function set_animation_time(t) {
	if(t > animation_time) {
		animation_time = t;
	}
}

var ResourceIndex = {
	g: 0,
	f: 1,
	b: 2,
	s: 3,
	h: 4,
	z: 5,
};

var ResourceID = "gfbshz".split('');

var NumResources = 6;

function Loc(x, y) {
	this.x = x;
	this.y = y;
}

function locs_equal(a, b) {
	return a.x == b.x && a.y == b.y;
}

function pixel_pos_to_loc(x, y) {
	var ypos = Math.floor(y/HexHeight);
	var xpos = Math.floor((x - ((ypos%2 == 1) ? HexWidth/2 : 0))/HexWidth);
	return new Loc(xpos, ypos);
}

function tile_pixel_x(loc) {
	return loc.x*HexWidth + ((loc.y%2 == 1) ? HexWidth/2 : 0);
}

function tile_pixel_y(loc) {
	return loc.y*HexHeight;
}

function tile_center_x(loc) {
	return tile_pixel_x(loc) + HexWidth/2;
}

function tile_center_y(loc) {
	return tile_pixel_y(loc) + HexHeight/2;
}

//position of unit after adjusting for terrain height in a loc.
function tile_unit_y(loc) {
	var result = tile_center_y(loc);
	if(!game) {
		return result;
	}

	var index = loc.y*game.width + loc.x;
	if(index < draw_map_info.length) {
		result += draw_map_info[index].unit_y_offset;
	}

	return result;
}

function prandom(loc) {
	var a = (loc.x + 92872873) ^ 918273;
	var b = (loc.y + 1672517) ^ 128123;
	return a*b + a + b;
}

function build_draw_map_info() {
	if(game == null) {
		return;
	}

	draw_map_info = [];
	draw_map_info_nobg = [];

	for(var n = 0; n != game.tiles.length; ++n) {
		var terrain = terrain_index[game.tiles[n]];
		if(terrain == null) {
			return;
		}

		var loc = new Loc(n%game.width, Math.floor(n/game.width));

		var area = terrain.image_area[prandom(loc)%terrain.image_area.length];

		var is_tower = game.tiles[n] == 'tower';
		var tower_area = null;
		if(is_tower) {
			//calculate tower_area -- the area within towers.png
			//that we draw the tower surface from.
			tower_area = [1, 2, 38, 40];
			var tower_info = game.tower_info[n];
			if(tower_info) {
				switch(tower_info.resource) {
				case 0: tower_area = [115, 2, 38, 40]; break;
				case 1: tower_area = [191, 2, 38, 40]; break;
				case 2: tower_area = [153, 2, 38, 40]; break;
				case 3: tower_area = [115, 46, 38, 40]; break;
				case 4: tower_area = [153, 46, 38, 40]; break;
				case 5: tower_area = [191, 46, 38, 40]; break;
				}
			}
		}

		var draw_info = {
			xpos: tile_pixel_x(loc),
			ypos: tile_pixel_y(loc) + HexHeight - area.h*2,
			unit_y_offset: terrain.unit_y_offset,
			image: terrain.image,
			image_area: area,
			loc: loc,
			background: terrain.background,
			highlight: false,
			is_tower: is_tower,
			tower_area: tower_area,
			avatar: null,
		};

		for(var key in unit_avatars) {
			if(locs_equal(unit_avatars[key].unit, loc)) {
				draw_info.avatar = unit_avatars[key];
				break;
			}
		}

		draw_map_info.push(draw_info);

		if(terrain.background == false) {
			draw_map_info_nobg.push(draw_info);
		}
	}
}

function draw_game() {
	if(game == null) {
		return;
	}

	if(draw_map_info.length == 0) {
		build_draw_map_info();
	}

	if(draw_map_info.length == 0) {
		return;
	}

	if(bg_canvas == null) {
		var bg = document.createElement('canvas');
		bg.width = HexWidth*game.width;
		bg.height = HexHeight*game.height;

		var context = bg.getContext('2d');

		for(var n = 0; n != draw_map_info.length; ++n) {
			var info = draw_map_info[n];
			var img = images_cache[info.image];
			if(!img) {
				return;
			}

			context.drawImage(img, info.image_area.x, info.image_area.y, info.image_area.w, info.image_area.h, info.xpos, info.ypos, info.image_area.w*2, info.image_area.h*2);
		}

		bg_canvas = bg;
	}

	var highlight_alpha = 0.6 + 0.4*Math.sin(frame_number/20);

	var canvas = document.getElementById('board').getContext('2d');

	canvas.drawImage(bg_canvas, 0, 0);

	for(var n = 0; n != draw_map_info.length; ++n) {
		var info = draw_map_info[n];
		var img = images_cache[info.image];

		if(!info.background) {
			canvas.drawImage(img, info.image_area.x, info.image_area.y, info.image_area.w, info.image_area.h, info.xpos, info.ypos, info.image_area.w*2, info.image_area.h*2);
		}

		if(draw_map_info[n].highlight) {
			canvas.globalAlpha = highlight_alpha;
			canvas.globalCompositeOperation = 'lighter';
			canvas.drawImage(img, info.image_area.x, info.image_area.y, info.image_area.w, info.image_area.h, info.xpos, info.ypos, info.image_area.w*2, info.image_area.h*2);
			canvas.globalCompositeOperation = 'source-over';
			canvas.globalAlpha = 1.0;
		}

		if(mouseover_loc != null && info.loc.x == mouseover_loc.x && info.loc.y == mouseover_loc.y) {
			canvas.globalCompositeOperation = 'lighter';
			canvas.drawImage(img, info.image_area.x, info.image_area.y, info.image_area.w, info.image_area.h, info.xpos, info.ypos, info.image_area.w*2, info.image_area.h*2);
			canvas.globalCompositeOperation = 'source-over';
		}
	}

	var towers_image = images_cache['towers.png'];
	if(!towers_image) {
		return;
	}

	for(var n = 0; n != draw_map_info.length; ++n) {
		var info = draw_map_info[n];
		if(info.is_tower) {
			var area = [39, 2, 38, 40];

			canvas.drawImage(towers_image, area[0], area[1], area[2], area[3], info.xpos - 6, info.ypos - 56, area[2]*2, area[3]*2);
		}

		if(draw_map_info[n].avatar) {
			draw_unit(canvas, draw_map_info[n].avatar);
		}

		if(info.is_tower) {
			var area = info.tower_area;
			canvas.drawImage(towers_image, area[0], area[1], area[2], area[3], info.xpos - 6, info.ypos - 56, area[2]*2, area[3]*2);
		}
	}
}

function load_cached_image(key) {
	if(images_cache[key] != null) {
		return;
	}

	var img = new Image();
	img.key = key;
	img.src = data_url + 'wizard-images/' + img.key;
	img.onload = function() {
		images_cache[img.key] = img;
	};

	image_store.push(img);
}

function init_wizard() {
	    if (!Function.prototype.bind) {  
      
      Function.prototype.bind = function (oThis) {  
      
        if (typeof this !== "function") // closest thing possible to the ECMAScript 5 internal IsCallable function  
          throw new TypeError("Function.prototype.bind - what is trying to be fBound is not callable");  
      
        var aArgs = Array.prototype.slice.call(arguments, 1),   
            fToBind = this,   
            fNOP = function () {},  
            fBound = function () {  
              return fToBind.apply(this instanceof fNOP ? this : oThis || window, aArgs.concat(Array.prototype.slice.call(arguments)));      
            };  
      
        fNOP.prototype = this.prototype;  
        fBound.prototype = new fNOP();  
      
        return fBound;  
      
      };  
    }  
}

function set_user_id(id) {
	user_id = id;
}

function Rect(s) {
	var dim = s.split(',');
	this.x = parseInt(dim[0]);
	this.y = parseInt(dim[1]);
	this.w = (parseInt(dim[2]) - this.x) + 1;
	this.h = (parseInt(dim[3]) - this.y) + 1;
}

function Route(element) {
	this.x = parseInt(element.getAttribute('x'));
	this.y = parseInt(element.getAttribute('y'));

	this.steps = [];
	var steps = element.getAttribute('steps').split(',');
	while(steps.length >= 2) {
		var loc = new Object();
		loc.x = parseInt(steps.shift());
		loc.y = parseInt(steps.shift());
		this.steps.push(loc);
	}
}

function UnitMoveInfo(element) {
	this.x = parseInt(element.getAttribute('x'));
	this.y = parseInt(element.getAttribute('y'));
	this.routes = [];

	var route_elements = element.getElementsByTagName('route');
	for(var n = 0; n != route_elements.length; ++n) {
		this.routes.push(new Route(route_elements[n]));
	}
}

function parse_overlays(element) {
	var overlay_elements = element.getElementsByTagName('overlay');
	for(var n = 0; n != overlay_elements.length; ++n) {
		var overlay = new UnitAnimation(overlay_elements[n]);
		overlay.id = overlay_elements[n].getAttribute('id');
		overlay_map[overlay.id] = overlay;
	}
}

function create_ability_table(unit) {
	var unit_td = document.getElementById('unit_td');
	var abilities_table = document.createElement('table');

	for(var n = 0; n != unit.abilities.length; ++n) {
		var row = document.createElement('tr');
		var cell = document.createElement('td');

		var canvas = document.createElement('canvas');
		canvas.width = 108;
		canvas.height = 120;

		unit.abilities[n].canvas = canvas;

		draw_ability(canvas, unit.abilities[n]);

		var obj = {
			ability: unit.abilities[n],
			caster: unit,
		};

		canvas.onmousemove = function(event) {
			if(current_info_displayed == this) {
				return;
			}

			current_info_displayed = this;

			var info_label = document.getElementById('info_label');
			info_label.innerHTML = this.ability.description;
		}.bind(obj);

		canvas.onmousedown = function(event) {
			spell_casting = null;
			unit_casting = this.caster;
			ability_using = this.ability;
			send_xml('<play spell="' + this.caster.id + '.' + this.ability.id + '" caster="' + this.caster.key + '"/>');
				}.bind(obj);

		cell.appendChild(canvas);
		row.appendChild(cell);
		abilities_table.appendChild(row);
	}

	unit_td.appendChild(abilities_table);

	if(current_abilities_table) {
		unit_td.removeChild(current_abilities_table);
	}

	current_abilities_table = abilities_table;
}

function ChooseAbilityInfo(element) {
	var unit_key = parseInt(element.getAttribute('unit'));
	for(var n = 0; n != game.units.length; ++n) {
		if(game.units[n].key == unit_key) {
			this.unit = game.units[n];
		}
	}

	var abilities_map = {};
	var abilities = element.getAttribute('abilities').split(',');
	for(var n = 0; n != abilities.length; ++n) {
		abilities_map[abilities[n]] = 1;
	}
	
	for(var n = 0; n != this.unit.abilities.length; ++n) {
		if(abilities_map[this.unit.abilities[n].id]) {
			this.unit.abilities[n].usable = true;
		} else {
			this.unit.abilities[n].usable = false;
		}
	}
}

function set_unit_move_info(info) {
	unit_move_info = info;
	for(var n = 0; n != draw_map_info.length; ++n) {
		for(var m = 0; m != unit_move_info.routes.length; ++m) {
			if(locs_equal(unit_move_info.routes[m], draw_map_info[n].loc)) {
				draw_map_info[n].highlight = true;
			}
		}
	}
}

function clear_unit_move_info() {
	unit_move_info = null;
	for(var n = 0; n != draw_map_info.length; ++n) {
		draw_map_info[n].highlight = false;
	}
}

function set_choose_ability_info(info) {
	clear_choose_ability_info();
	choose_ability_info = info;
}

function clear_choose_ability_info() {
	if(choose_ability_info) {
		for(var n = 0; n != choose_ability_info.unit.abilities.length; ++n) {
			choose_ability_info.unit.abilities[n].usable = false;
			choose_ability_info.unit.abilities[n].canvas = null;
		}

		choose_ability_info = null;
	}

	if(current_abilities_table) {
		var unit_td = document.getElementById('unit_td');
		unit_td.removeChild(current_abilities_table);
		current_abilities_table = null;
	}
}

function UnitAnimation(element) {
	this.image_name = element.getAttribute('image');
	load_cached_image(this.image_name);
	this.image = images_cache[this.image_name];
	this.x = parseInt(element.getAttribute('x'));
	this.y = parseInt(element.getAttribute('y'));

	this.w = 36;
	this.h = 36;

	if(element.getAttribute('width')) {
		this.w = parseInt(element.getAttribute('width'));
	}

	if(element.getAttribute('height')) {
		this.h = parseInt(element.getAttribute('height'));
	}
}

function draw_player(canvas, player) {
	if(!player.resources) {
		return;
	}

	var context = canvas.getContext('2d');

	context.fillStyle = '#ffffff';
	context.fillRect(0, 0, canvas.width, canvas.height);

	var xpos = 5, ypos = 5;

	for(var n = 0; n != player.resources.length; ++n) {
		if(player.resources[n] > 0) {
			var icon = 'magic-icon-' + ResourceID[n];
			var gui_section = gui_sections_index[icon];
			if(gui_section) {
				for(var m = 0; m != player.resources[n]; ++m) {
					if(m != 0 && m%5 == 0) {
						ypos += 20;
					}
					draw_gui_section(gui_section, context, xpos + (m%5)*10, ypos);
				}

				ypos += 20;
			}
		}
	}

	ypos += 20;

	context.fillStyle = '#000000';
	context.fillText('Units: ' + player.unit_limit, 5, ypos);
}

function draw_ability(canvas, ability) {
	var img = images_cache['card.png'];
	if(!img) {
		return false;
	}

	var context = canvas.getContext('2d');

	context.drawImage(img, 0, 0);

	context.font = "14px sans-serif";
	context.fillText(ability.name, 8, 18, 96);

	var icon = images_cache[ability.icon];
	if(!icon) {
		return false;
	}

	context.drawImage(icon, 25, 46);

	return true;
}

function draw_spell(spell) {
	if(!spell.canvas) {
		return;
	}

	var img = images_cache['card.png'];
	if(!img) {
		return;
	}

	var context = spell.canvas.getContext('2d');

	context.fillRect(img, 0, 0, spell.canvas.width, spell.canvas.height);

	if(!spell.castable) {
		context.globalAlpha = 0.5;
	}

	context.drawImage(img, 0, 0);

	if(!spell.castable) {
		context.globalAlpha = 1.0;
	}

	if(!spell.spell) {
		spell.spell = spells_index[spell.id];
		if(!spell.spell) {
			window.setTimeout(function(element) {draw_spell(this);}.bind(spell), 50);
			return;
		}
	}

	context.font = "14px sans-serif";
	context.fillText(spell.spell.name, 8, 18, 96);

	var resource_pos = 68;
	for(var n = 0; n != NumResources; ++n) {
		var cost = spell.spell.cost[n];
		if(cost > 0) {
			var icon = 'magic-icon-' + ResourceID[n];
			var gui_section = gui_sections_index[icon];
			if(gui_section) {
				draw_gui_section(gui_section, context, resource_pos, 22);
				context.fillText('x' + cost, resource_pos + 16, 34);
				resource_pos -= 30;
			}
		}
	}

	if(spell.spell.monster) {
		var anim = unit_animations[spell.spell.monster];
		if(anim) {
			var res = draw_unit_animation(context, anim, 20, 38);
			if(!res) {
				window.setTimeout(function(element) {draw_spell(this);}.bind(spell), 50);
			}
		}
	}
}

function draw_unit_animation(canvas, anim, x, y) {
	if(!anim.image) {
		anim.image = images_cache[anim.image_name];
		if(!anim.image) {
			return false;
		}
	}

	canvas.drawImage(anim.image, anim.x, anim.y, anim.w, anim.h, x, y, anim.w*2, anim.h*2);
	return true;
}

function draw_unit_hitpoints(canvas, unit, x, y) {
	var BarWidth = 5;
	canvas.strokeStyle = '#000000';
	canvas.fillStyle = '#000000';

	canvas.fillRect(x-1, y, 1, unit.life*5);
	canvas.fillRect(x+1, y-2, BarWidth, 1);
	canvas.fillRect(x+1, y+unit.life*5+1, BarWidth, 1);
	canvas.fillRect(x+1, y, BarWidth, (unit.life-unit.damage_taken)*5);

	canvas.strokeStyle = '#ffc20e';
	canvas.fillStyle = '#ffc20e';
	canvas.fillRect(x, y, 1, unit.life*5);
	canvas.fillRect(x+BarWidth+1, y, 1, unit.life*5);
	canvas.fillRect(x, y-1, BarWidth+2, 1);
	canvas.fillRect(x, y+unit.life*5, BarWidth+2, 1);

	canvas.strokeStyle = '#4c1711';
	canvas.fillStyle = '#4c1711';
	canvas.fillRect(x+BarWidth+2, y, 1, unit.life*5);

	if(unit.side == 0) {
		canvas.strokeStyle = '#ef343e';
		canvas.fillStyle = '#ef343e';
	} else {
		canvas.strokeStyle = '#3e34ef';
		canvas.fillStyle = '#3e34ef';
	}

	canvas.fillRect(x+1, y + unit.damage_taken*5, BarWidth, (unit.life - unit.damage_taken)*5);

	canvas.strokeStyle = '#000000';
	canvas.fillStyle = '#000000';
	canvas.globalAlpha = 0.5;
	for(var n = 1; n < unit.life; ++n) {
		canvas.fillRect(x+1, y + n*5 - 1, BarWidth, 2);
	}

	canvas.globalAlpha = 1.0;
	canvas.strokeStyle = '#ffffff';
	canvas.fillStyle = '#ffffff';
}

function download_xml_file(url, onget) {
	var request = new XMLHttpRequest();
	var info = {
		url: url,
	};
	request.open("GET", url);
	request.onreadystatechange = function() {
		var done = 4, ok = 200;
		if(request.readyState == done && request.status == ok) {
			if(request.responseXML) {
  		  		var element = request.responseXML.documentElement;
				onget(element);
				on_file_completed_download(this.url);
			}
		}
	}.bind(info);

	request.send(null);
}

function load_unit_prototype(unit_id) {
	download_xml_file(data_url + 'wizard-data/units/' + unit_id + '.xml',
	                  function(element) {
		var stand_elements = element.getElementsByTagName('stand');
		for(var n = 0; n != stand_elements.length; ++n) {
		  unit_animations[unit_id] = new UnitAnimation(stand_elements[n]);
		}

		unit_prototypes[unit_id] = new Unit(element);
	});
}

function UnitAbility(element) {
	this.id = element.getAttribute('id');
	this.name = element.getAttribute('name');
	this.taps_caster = element.getAttribute('taps_caster') == 'yes';
	this.usable = false;
	this.icon = 'abilities/' + element.getAttribute('icon');
	this.description = element.getAttribute('description');
	load_cached_image(this.icon);
}

function Unit(element) {
	this.id = element.getAttribute('id');

	if(!unit_animations[this.id]) {
		load_unit_prototype(this.id);
	}

	this.x = parseInt(element.getAttribute('x'));
	this.y = parseInt(element.getAttribute('y'));

	this.side = parseInt(element.getAttribute('side'));

	this.name = element.getAttribute('name');
	this.key = parseInt(element.getAttribute('key'));
	this.life = parseInt(element.getAttribute('life'));
	this.effective_life = parseInt(element.getAttribute('effective_life'));
	if(!this.effective_life) {
		this.effective_life = life;
	}
	this.damage_taken = parseInt(element.getAttribute('damage_taken'));
	this.armor = parseInt(element.getAttribute('armor'));
	this.effective_armor = parseInt(element.getAttribute('effective_armor'));
	if(!this.effective_armor) {
		this.effective_armor = armor;
	}
	this.move = parseInt(element.getAttribute('move'));
	this.has_moved = element.getAttribute('has_moved') == 'yes';
	this.abilities = [];

	var ability_elements = element.getElementsByTagName('ability');
	for(var n = 0; n != ability_elements.length; ++n) {
		this.abilities.push(new UnitAbility(ability_elements[n]));
	}

	this.overlays = [];
	this.underlays = [];

	if(element.getAttribute('overlays')) {
		var overlays = element.getAttribute('overlays').split(',');
		for(var n = 0; n != overlays.length; ++n) {
			var ov = overlay_map[overlays[n]];
			if(ov) {
				this.overlays.push(ov);
			}
		}
	}

	if(element.getAttribute('underlays')) {
		var underlays = element.getAttribute('underlays').split(',');
		for(var n = 0; n != underlays.length; ++n) {
			var ov = overlay_map[underlays[n]];
			if(ov) {
				this.underlays.push(ov);
			}
		}
	}
}

function describe_unit(unit) {
	var desc = '';
	desc += '<p><h3>' + unit.name + '</h3></p><p>Life: ' + unit.life;
	if(unit.damage_taken > 0) {
		desc += '<font color="red"> (' + unit.damage_taken + ')</font>';
	}

	desc += '<br>Move: ' + unit.move;
	if(unit.defense) {
		desc += '<br>Defense: ' + unit.defense;
	}

	if(unit.abilities.length > 0) {
		desc += '<ul>';
		for(var n = 0; n != unit.abilities.length; ++n) {
			var ability = unit.abilities[n];
			desc += ability.name + ': ' + ability.description;
		}
		desc += '</ul>';
	}

	desc += '</p>';

	return desc;
}

function describe_spell(spell) {
	var desc = '';
	desc += '<p><h3>' + spell.name + '</h3></p><p>' + spell.description + '</p>';
	return desc;
}

function Terrain(element) {
	this.id = element.getAttribute('id');
	this.image = element.getAttribute('image');
	load_cached_image(this.image);

	//whether this type of terrain can be drawn purely
	//in the background.
	this.background = true;

	if(this.id == 'tower') {
		this.background = false;
	}

	this.image_area = element.getAttribute('image_area').split(':');
	for(var n = 0; n < this.image_area.length; ++n) {
		this.image_area[n] = new Rect(this.image_area[n]);

		if(this.image_area[n].h > 22) {
			this.background = false;
		}
	}

	var unit_y_offset = element.getAttribute('unit_y_offset');
	if(unit_y_offset != null) {
		this.unit_y_offset = parseInt(unit_y_offset);
	} else {
		this.unit_y_offset = 0;
	}
}

function load_terrain() {
  var request =  new XMLHttpRequest();
  request.open("GET", data_url + 'wizard-data/terrain.xml');
  request.onreadystatechange = function() {
    var done = 4, ok = 200;
    if (request.readyState == done && request.status == ok) {
      if (request.responseXML) {
	    var element = request.responseXML.documentElement;
		var terrain_elements = element.getElementsByTagName('terrain');
		for(var n = 0; n != terrain_elements.length; ++n) {
			var t = new Terrain(terrain_elements[n]);
			terrain_index[t.id] = t;
		}
	  }
	}
  }

  request.send(null);
}

function load_game_data() {
	load_terrain();
}

function UnitAvatar(u) {
	this.unit = u;
	this.anim = unit_animations[u.id];
	this.anim_time = 0;
}

function update_avatar_position(avatar) {
	if(avatar.dying) {
		avatar.anim_time++;
	} else if(avatar.attack_loc){
		avatar.anim_time++;
		if(avatar.anim_time >= 20) {
			avatar.anim_time = 0;
			delete avatar.attack_loc;
			delete avatar.position_override;
		} else {
				
			var x1 = tile_center_x(avatar.unit);
			var y1 = tile_unit_y(avatar.unit);
			var x2 = tile_center_x(avatar.attack_loc);
			var y2 = tile_unit_y(avatar.attack_loc);

			var anim_time = avatar.anim_time;
			if(anim_time > 10) {
				anim_time = 20 - anim_time;
			}

			var x = (x2*anim_time + x1*(10-anim_time))/10;
			var y = (y2*anim_time + y1*(10-anim_time))/10;

			avatar.position_override = {
				x: x,
				y: y,
			};
		}
	} else if(avatar.steps) {
		avatar.anim_time++;
		if(avatar.anim_time >= 10) {
			avatar.steps.shift();
			avatar.anim_time = 0;
			if(avatar.steps.length <= 1) {
				delete avatar.steps;
				delete avatar.position_override;
			}

			avatar.anim_time = 0;
		}
			
		if(avatar.steps && avatar.steps.length >= 2) {
			var x1 = tile_center_x(avatar.steps[0]);
			var y1 = tile_unit_y(avatar.steps[0]);
			var x2 = tile_center_x(avatar.steps[1]);
			var y2 = tile_unit_y(avatar.steps[1]);

			var x = (x2*avatar.anim_time + x1*(10-avatar.anim_time))/10;
			var y = (y2*avatar.anim_time + y1*(10-avatar.anim_time))/10;

			avatar.position_override = {
				x: x,
				y: y,
			};
		}
	} else if(avatar.position_override) {
		delete avatar.position_override;
	}
}

function draw_unit(canvas, avatar) {
	if(!avatar.anim) {
		avatar.anim = unit_animations[avatar.unit.id];
		if(!avatar.anim) {
			return;
		}
	}

	var xpos, ypos;

	if(avatar.position_override) {
		xpos = avatar.position_override.x;
		ypos = avatar.position_override.y;
	} else {
		xpos = tile_center_x(avatar.unit);
		ypos = tile_unit_y(avatar.unit);
	}

	var orig_global_alpha;
	if(avatar.dying) {
		if(avatar.anim_time >= 10) {
			return;
		}
		orig_global_alpha = canvas.globalAlpha;
		canvas.globalAlpha *= (10 - avatar.anim_time)/10;
	}

	var x = xpos - avatar.anim.w;
	var y = ypos - avatar.anim.h*2 + 10;

	if(avatar.unit.side&1) {
		canvas.save();
		canvas.translate(avatar.anim.w*2, 0);
		canvas.scale(-1, 1);
		x = -x;
	}

	for(var n = 0; n != avatar.unit.underlays.length; ++n) {
		draw_unit_animation(canvas, avatar.unit.underlays[n], x, y);
	}
	
	draw_unit_animation(canvas, avatar.anim, x, y);

	for(var n = 0; n != avatar.unit.overlays.length; ++n) {
		draw_unit_animation(canvas, avatar.unit.overlays[n], x, y);
	}

	draw_unit_hitpoints(canvas, avatar.unit, x+14, y+14);

	if(avatar.unit.has_moved) {
		var img = images_cache['tired.png'];
		if(img) {
			canvas.drawImage(img, x - 6, y, img.width*2, img.height*2);
		}
	}

	if(avatar.unit.armor != 0) {

		var gui_section = gui_sections_index[avatar.unit.armor < 0 ? 'vulnerable-icon' : 'defense_icon'];
		var num = avatar.unit.armor > 0 ? avatar.unit.armor : -avatar.unit.armor;
		for(var n = 0; n != num; ++n) {
			draw_gui_section(gui_section, canvas, x - 10, y + 12 + n*6);
		}
	}

	if(avatar.unit.side&1) {
		canvas.restore();
	}

	if(avatar.dying) {
		canvas.globalAlpha = orig_global_alpha;
	}
}

function Player(element) {
	this.name = element.getAttribute('name');
	this.resource_gain = [];
	this.unit_limit = element.getAttribute('unit_limit');

	if(!element.getAttribute('resources')) {
		return;
	}

	var resource_gain = element.getAttribute('resource_gain').split(',');
	for(var n = 0; n != resource_gain.length; ++n) {
		this.resource_gain.push(parseInt(resource_gain[n]));
	}

	this.resources = [];

	var resources = element.getAttribute('resources').split(',');
	for(var n = 0; n != resources.length; ++n) {
		this.resources.push(parseInt(resources[n]));
	}

	this.spells = [];
	var spells = element.getAttribute('spells').split(',');
	for(var n = 0; n != spells.length; ++n) {
		var items = spells[n].split(' ');
		var id = items[0];
		var embargo = 0;
		var castable = false;
		if(items.length > 1) {
			embargo = parseInt(items[1]);
		}

		if(items.length > 2 && parseInt(items[2]) == 1) {
			castable = true;
		}

		var spell = {
			spell: spells_index[spells[n]],
			id: id,
			embargo: embargo,
			castable: castable,
		};
		this.spells.push(spell);
	}
}

function Game(element) {
	this.height = parseInt(element.getAttribute('height'));
	this.width = parseInt(element.getAttribute('width'));
	this.tiles = element.getAttribute('tiles').split(',');

	this.getTile = function(x, y) {
		if(x < 0 || x >= this.width || y < 0 || y >= this.height) {
			return null;
		}

		return this.tiles[y*this.width + x];
	};

	this.players = [];

	this.tower_info = {};

	var player_elements = element.getElementsByTagName('player');
	for(var n = 0; n != player_elements.length; ++n) {
		var player = new Player(player_elements[n]);
		if(player.name == user_id) {
			player_side = n;
		}

		var tower_elements = player_elements[n].getElementsByTagName('tower');
		for(var m = 0; m != tower_elements.length; ++m) {
			var tower_element = tower_elements[m];

			var info = {
				x: parseInt(tower_element.getAttribute('x')),
				y: parseInt(tower_element.getAttribute('y')),
				resource: ResourceIndex[tower_element.getAttribute('resource')],
				owner: n,
			};

			this.tower_info[info.y*this.width + info.x] = info;
		}

		this.players.push(player);
	}

	this.units = [];

	var seen_unit_keys = {};

	var unit_elements = element.getElementsByTagName('unit');
	for(var n = 0; n != unit_elements.length; ++n) {
		var u = new Unit(unit_elements[n]);
		if(unit_avatars[u.key]) {
			unit_avatars[u.key].unit = u;
		} else {
			unit_avatars[u.key] = new UnitAvatar(u);
		}
		this.units.push(u);

		seen_unit_keys[u.key] = true;
	}

	//get rid of unit avatars of units that no longer exist.
	var delete_keys = [];
	for(key in unit_avatars) {
		if(!seen_unit_keys[key]) {
			//TODO: find if we can mutate the associative array while
			//iterating over it.
			delete_keys.push(key);
		}
	}

	for(var n = 0; n != delete_keys.length; ++n) {
		delete unit_avatars[delete_keys[n]];
	}

	for(var n = 0; n != draw_map_info.length; ++n) {
		draw_map_info[n].avatar = null;
	}

	for(key in unit_avatars) {
		var avatar = unit_avatars[key];
		var index = this.width*avatar.unit.y + avatar.unit.x;
		if(index < draw_map_info.length) {
			draw_map_info[index].avatar = avatar;
		}
	}
}

var current_games_table = null;

function handle_lobby(element) {

	if(game != null) {
		return;
	}

	var lobby_para = document.getElementById('lobby_para');
	lobby_para.style.display = 'block';

	if(current_games_table != null) {
		lobby_para.removeChild(current_games_table);
	}

	var games_table = document.createElement('table');

	var games = element.getElementsByTagName('game');
	for(var n = 0; n != games.length; ++n) {
		var g = games[n];
		var row = document.createElement('tr');
		var cell = document.createElement('td');
		var text = document.createTextNode(g.getAttribute('clients'));
		cell.appendChild(text);
		row.appendChild(cell);

		console.log('game started: "' + g.getAttribute('started') + '"');

		if(g.getAttribute('started') == 'no') {
			cell = document.createElement('td');

			var button = document.createElement('input');
			button.setAttribute('type', 'button');
			button.setAttribute('value', 'Join Game');
			button.onclick = function() {
				send_xml('<commands><join_game/><spells resource_gain="0,0,10,0,0,0" spells="dark_adept,skeleton,vampire,vampire_bat,terror,flesh_wound,fireball"/></commands>');
			};

			cell.appendChild(button);
			row.appendChild(cell);
		}

		games_table.appendChild(row);
	}

	lobby_para.appendChild(games_table);

	current_games_table = games_table;

}

var current_deck_table = null;
var current_collection_table = null;

function handle_player_info(element) {
	if(game != null) {
		return;
	}

	var editor_para = document.getElementById('editor_para');
	editor_para.style.display = 'block';

	var deck_div = document.getElementById('deck_div');
	var collection_div = document.getElementById('collection_div');

	var deck_table = document.createElement('table');
	var collection_table = document.createElement('table');

	var spells = element.getAttribute('spells').split(',');

	var deck_rows = {};
	var collection_rows = {};

	for(var n = 0; n != spells.length; ++n) {
		var spell = spells_index[spells[n]];

		var row = document.createElement('tr');
		var cell = document.createElement('td');
		var canvas = document.createElement('canvas');
		canvas.width = 108;
		canvas.height = 120;
		cell.appendChild(canvas);

		var spell_info = {
			id: spells[n],
			canvas: canvas,
			castable: 1,
		};

		canvas.onmousedown = function(event) {
			send_xml('<modify_deck remove="' + this.spell.id + '"/>');
		}.bind(spell_info);

		draw_spell(spell_info);

		row.appendChild(cell);
		deck_table.appendChild(row);

		deck_rows[spells[n]] = row;
	}

	var collection = element.getAttribute('collection').split(',');
	for(var n = 0; n != collection.length; ++n) {
		var spell = spells_index[collection[n]];

		var row = document.createElement('tr');
		var cell = document.createElement('td');
		var canvas = document.createElement('canvas');
		canvas.width = 108;
		canvas.height = 120;
		cell.appendChild(canvas);
		row.appendChild(cell);

		var spell_info = {
			id: collection[n],
			canvas: canvas,
			castable: 1,
		};

		canvas.onmousedown = function(event) {
			send_xml('<modify_deck add="' + this.spell.id + '"/>');
		}.bind(spell_info);

		draw_spell(spell_info);

		collection_table.appendChild(row);

		collection_rows[collection[n]] = row;
	}

	for(var key in collection_rows) {
		if(deck_rows[key]) {
			collection_rows[key].style.display = 'none'
		} else {
			collection_rows[key].style.display = 'block'
		}
	}

	deck_div.appendChild(deck_table);
	collection_div.appendChild(collection_table);

	if(current_deck_table != null) {
		deck_div.removeChild(current_deck_table);
	}

	if(current_collection_table != null) {
		collection_div.removeChild(current_collection_table);
	}

	current_deck_table = deck_table;
	current_collection_table = collection_table;

	var resource_gain = element.getAttribute('resource_gain').split(',');
	var total_num_resources = 0;
	for(var n = 0; n != resource_gain.length; ++n) {
		total_num_resources += parseInt(resource_gain[n]);
	}

	for(var n = 0; n != ResourceID.length; ++n) {
		var resource = ResourceID[n];
		var num_resources = 0;
		if(n < resource_gain.length) {
			num_resources = parseInt(resource_gain[n]);
		}

		var canvas = document.getElementById('resource_canvas_' + resource);
		if(!canvas) {
			continue;
		}
		canvas.width = canvas.width; //clear the canvas
		var context = canvas.getContext('2d');
		
		var icon = 'magic-icon-' + resource;
		var gui_section = gui_sections_index[icon];
		draw_gui_section(gui_section, context, 0, 4);

		for(var m = 0; m < num_resources; ++m) {
			draw_gui_section(gui_section, context, 24 + m*8 + (m >= 5 ? 8 : 0), 4);
		}

		var add_button = document.getElementById('add_resource_button_' + resource);
		var remove_button = document.getElementById('del_resource_button_' + resource);
		add_button.style.display = total_num_resources < 10 ? 'block' : 'none';
		remove_button.style.display = num_resources > 0 ? 'block' : 'none';
	}
}

function modify_resources(resource, delta) {
	send_xml('<modify_resources resource="' + resource + '" delta="' + delta + '"/>');
}

function process_response(response) {
	if(animation_time > 0) {
		responses_waiting_for_processing.push(response);
		return;
	}

	var element = response.documentElement;
	
	if(element.tagName == 'debug_message') {
		console.log('SERVER DEBUG MSG: ' + element.getAttribute('msg'));
	} else if(element.tagName == 'lobby') {
		handle_lobby(element);
	} else if(element.tagName == 'player_info') {
		handle_player_info(element);
	} else if(element.tagName == 'game') {
		spell_casting = null;
		spell_targets = null;
		legal_targets = null;
		ability_using = null;
		unit_casting = null;
		clear_choose_ability_info();

		//make sure we are showing the main game table.
		document.getElementById('game_para').style.display = 'block';
		document.getElementById('lobby_para').style.display = 'none';
		document.getElementById('editor_para').style.display = 'none';

		game = new Game(element);
		draw_map_info = []; //clear draw map info so it's rebuilt.

		if(player_side >= 0 && player_side < game.players.length) {
			var player = game.players[player_side];
			draw_player(document.getElementById('player_info'), player);

			if(player.spells) {
				var spells_para = document.getElementById('spells_para');
				var spells_table = document.createElement('table');

				var row = document.createElement('tr');
				
				for(var n = 0; n != player.spells.length; ++n) {
					var spell = player.spells[n];

					var cell = document.createElement('td');
					var canvas = document.createElement('canvas');
					canvas.width = 108;
					canvas.height = 120;
					canvas.onmousedown = function(event) {
						spell_casting = this.spell;
						send_xml('<play spell="' + this.spell.id + '"/>');
					}.bind(spell);

					canvas.onmousemove = function(event) {
						if(current_info_displayed == this) {
							return;
						}

						current_info_displayed = this;

						var spell = spells_index[this.spell.id];
						var info_label = document.getElementById('info_label');

						if(spell.monster && unit_prototypes[spell.monster]) {
							info_label.innerHTML += describe_unit(unit_prototypes[spell.monster]);
						} else {
							info_label.innerHTML = describe_spell(spell);
						}
						
					}.bind(spell);

					spell.canvas = canvas;

					cell.appendChild(canvas);

					row.appendChild(cell);

					draw_spell(player.spells[n]);
				}

				spells_table.appendChild(row);
				spells_para.appendChild(spells_table);

				if(current_spells_table != null) {
					spells_para.removeChild(current_spells_table);
				}

				current_spells_table = spells_table;
			}
		}
	} else if(element.tagName == 'game_created' || element.tagName == 'join_game') {
		if(element.tagName == 'join_game') {
			players_signed_up_for_game++;
			console.log('player joined game');
		}

		if(players_signed_up_for_game >= 1) {
			send_xml('<commands><setup/><spells resource_gain="0,0,10,0,0,0" spells="dark_adept,skeleton,vampire,vampire_bat,terror,flesh_wound,fireball"/></commands>');
		}
	} else if(element.tagName == 'select_unit_move') {
		set_unit_move_info(new UnitMoveInfo(element));
	} else if(element.tagName == 'choose_ability') {
		set_choose_ability_info(new ChooseAbilityInfo(element));
		create_ability_table(choose_ability_info.unit);
	} else if(element.tagName == 'move_anim') {

		var from_elements = element.getElementsByTagName('from');
		var to_elements = element.getElementsByTagName('to');

		if(from_elements.length < 1 || to_elements.length < 1) {
			return;
		}

		var from_loc = {
			x: parseInt(from_elements[0].getAttribute('x')),
			y: parseInt(from_elements[0].getAttribute('y')),
		};

		var to_loc = {
			x: parseInt(to_elements[0].getAttribute('x')),
			y: parseInt(to_elements[0].getAttribute('y')),
		};
				
		var steps_array = element.getAttribute('steps').split(',');
		var steps = [];
		while(steps_array.length >= 2) {
			var loc = new Object();
			loc.x = parseInt(steps_array.shift());
			loc.y = parseInt(steps_array.shift());
			steps.push(loc);
		}

		set_animation_time(steps.length*10);

		for(var key in unit_avatars) {
			if(locs_equal(from_loc, unit_avatars[key].unit)) {
				unit_avatars[key].unit.x = to_loc.x;
				unit_avatars[key].unit.y = to_loc.y;

				unit_avatars[key].steps = steps;
				unit_avatars[key].anim_time = 0;
				
				break;
			}
		}

	} else if(element.tagName == 'attack_anim') {
		clear_choose_ability_info();

		var from_elements = element.getElementsByTagName('from');
		var to_elements = element.getElementsByTagName('to');

		if(from_elements.length < 1 || to_elements.length < 1) {
			return;
		}

		var from_loc = {
			x: parseInt(from_elements[0].getAttribute('x')),
			y: parseInt(from_elements[0].getAttribute('y')),
		};

		var to_loc = {
			x: parseInt(to_elements[0].getAttribute('x')),
			y: parseInt(to_elements[0].getAttribute('y')),
		};

		set_animation_time(20);

		for(var key in unit_avatars) {
			var avatar = unit_avatars[key];
			if(locs_equal(from_loc, avatar.unit)) {
				avatar.attack_loc = to_loc;
				avatar.anim_time = 0;
				
				break;
			}
		}
			
	} else if(element.tagName == 'death_anim') {
		var loc = {
			x: parseInt(element.getAttribute('x')),
			y: parseInt(element.getAttribute('y')),
		};

		for(var key in unit_avatars) {
			var avatar = unit_avatars[key];
			if(locs_equal(loc, avatar.unit)) {
				avatar.anim_time = 0;
				avatar.dying = true;
				animation_time = 10;
				break;
			}
		}


	} else if(element.tagName == 'illegal_cast') {
		var array = element.getAttribute('legal_targets').split(',');
		if(array.length >= 2) {
			legal_targets = [];
		} else {
			spell_casting = null;
			spell_targets = null;
			legal_targets = null;
			ability_using = null;
			unit_casting = null;
			return;
		}

		while(array.length >= 2) {
			var x = parseInt(array.shift());
			var y = parseInt(array.shift());
			legal_targets.push({ x: x, y: y, });
		}

		for(var n = 0; n != draw_map_info.length; ++n) {
			var target = false;
			for(var m = 0; m != legal_targets.length; ++m) {
				if(locs_equal(legal_targets[m], draw_map_info[n].loc)) {
					target = true;
					break;
				}
			}

			draw_map_info[n].highlight = target;
		}
	}
}

var xml_in_flight = 0;
function send_xml(xml_data) {
  var payload = user_id + '\n' + xml_data;
  var request =  new XMLHttpRequest();
  var url = ajax_script;

  request.open("POST", url, true);
  request.setRequestHeader("Content-Type",
                           "application/x-www-form-urlencoded");
  request.setRequestHeader("Content-length", payload.length);
  request.setRequestHeader("Connection", "close");
 
  request.onreadystatechange = function() {
    var done = 4, ok = 200;
    if (request.readyState == done && request.status == ok) {
      if (request.responseXML) {
		--xml_in_flight;
        process_response(request.responseXML);
		if(xml_in_flight == 0) {
			send_xml('<request_updates/>');
		}
      }
    }
  };

  ++xml_in_flight;
  request.send(payload);
}

var mouse_move_count = 0;

function mouse_move(e) {
	var canvas = get_board_canvas();
	var xpos = e.pageX - canvas.offsetLeft;
	var ypos = e.pageY - canvas.offsetTop;

	if(mouseover_loc != null && locs_equal(mouseover_loc, pixel_pos_to_loc(xpos, ypos))) {
		return;
	}

	mouseover_loc = pixel_pos_to_loc(xpos, ypos);

	var loc_label = document.getElementById('loc_label');
	loc_label.innerHTML = '' + mouseover_loc.x + ', ' + mouseover_loc.y;

	var info_label = document.getElementById('info_label');
	info_label.innerHTML = '';

	for(var n = 0; n != game.units.length; ++n) {
		if(locs_equal(game.units[n], mouseover_loc)) {
			current_info_displayed = null;

			var unit = game.units[n];
			info_label.innerHTML = describe_unit(game.units[n]);
			break;
		}
	}
}

function mouse_down(e) {
	var canvas = get_board_canvas();
	var xpos = e.pageX - canvas.offsetLeft;
	var ypos = e.pageY - canvas.offsetTop;
	var loc = pixel_pos_to_loc(xpos, ypos);

	if(game == null) {
		return;
	}

	if(legal_targets != null) {
		for(var n = 0; n != draw_map_info.length; ++n) {
			draw_map_info[n].highlight = false;
		}

		for(var n = 0; n != legal_targets.length; ++n) {
			if(locs_equal(legal_targets[n], loc)) {
				if(spell_targets == null) {
					spell_targets = [];
				}

				spell_targets.push(loc);

				var msg;
				
				if(spell_casting) {
					msg = '<play spell="' + spell_casting.id + '">';
				} else if(unit_casting) {
					msg = '<play spell="' + unit_casting.id + '.' + ability_using.id + '" caster="' + unit_casting.key + '">';
				}

				for(var n = 0; n != spell_targets.length; ++n) {
					msg += '<target x="' + spell_targets[n].x + '" y="' + spell_targets[n].y + '"/>';
				}

				msg += '</play>';

				legal_targets = null;
				send_xml(msg);
				return;
			}
		}

		legal_targets = null;
		spell_casting = null;
		spell_targets = null;
		ability_using = null;
		unit_casting = null;
	}

	if(unit_move_info != null) {
		for(var n = 0; n != unit_move_info.routes.length; ++n) {
			if(locs_equal(unit_move_info.routes[n], loc)) {
				send_xml('<move><query_abilities/><from x="' + unit_move_info.x + '" y="' + unit_move_info.y + '"/><to x="' + unit_move_info.routes[n].x + '" y="' + unit_move_info.routes[n].y + '"/></move>');
			}
		}

		clear_unit_move_info();
		return;
	}

	for(var n = 0; n != game.units.length; ++n) {
		if(locs_equal(game.units[n], loc)) {
			send_xml('<select_unit x="' + loc.x + '" y="' + loc.y + '"/>');
			return;
		}
	}
}

function end_turn() {
	send_xml('<end_turn skip="yes"/>');
}

function resign_game() {
	player_side = -1;
	game = null;
	spell_casting = null;
	unit_casting = null;
	draw_map_info = [];
	draw_map_info_nobg = [];
	document.getElementById('game_para').style.display = 'none';
	send_xml('<enter_lobby/>');
}

function create_game_with_bots() {
	players_signed_up_for_game = 1;
	send_xml('<create_game bots="1"/>');
}

function create_game() {
	players_signed_up_for_game = 0;
	send_xml('<create_game/>');
}

function enter_lobby() {
	send_xml('<enter_lobby/>');
}

function anim_loop() {
	++frame_number;

	if(animation_time > 0) {
		--animation_time;
		while(animation_time == 0 && responses_waiting_for_processing.length > 0) {
			var response = responses_waiting_for_processing.shift();
			process_response(response);
		}
	}

	for(var key in unit_avatars) {
		update_avatar_position(unit_avatars[key]);
	}

	draw_game();
	requestAnimFrame(anim_loop, null);
}

function Spell(element) {
	this.id = element.getAttribute('id');
	this.name = element.getAttribute('name');
	this.description = element.getAttribute('description');

	this.cost = [0,0,0,0,0,0];

	if(element.getAttribute('cost')) {
		//calculate the cost as an array of integers.
		var cost = element.getAttribute('cost').split('');
		for(var n = 0; n != cost.length; ++n) {
			this.cost[ResourceIndex[cost[n]]]++;
		}
	}

	this.monster = element.getAttribute('monster');
	if(this.monster && !unit_animations[this.monster]) {
		load_unit_prototype(this.monster);
	}
}

function parse_spells(element) {
	var spell_elements = element.getElementsByTagName('spell');
	for(var n = 0; n != spell_elements.length; ++n) {
		var spell = new Spell(spell_elements[n]);
		spells_index[spell.id] = spell;
	}
}

function GuiSection(element) {
	this.id = element.getAttribute('id');
	this.image_name = element.getAttribute('image');
	this.rect = new Rect(element.getAttribute('rect'));
	load_cached_image(this.image_name);
}

function draw_gui_section(section, canvas, x, y) {
	if(!section) {
		return;
	}

	if(!section.image) {
		section.image = images_cache[section.image_name];
		if(!section.image) {
			return;
		}
	}

	canvas.drawImage(section.image, section.rect.x, section.rect.y, section.rect.w, section.rect.h, x, y, section.rect.w, section.rect.h);
}

function parse_gui(element) {
	var section_elements = element.getElementsByTagName('section');
	for(var n = 0; n != section_elements.length; ++n) {
		var section = new GuiSection(section_elements[n]);
		gui_sections_index[section.id] = section;
	}
}

var files_needed_to_start_game = new Array();

function on_file_completed_download(fname) {
	var has_keys = 0;
	for(var key in files_needed_to_start_game) { has_keys++; }
	console.log('completed download: ' + fname + ' -> ' + has_keys);
	if(has_keys != 0) {
		delete files_needed_to_start_game[fname];
		has_keys = 0;
		for(var key in files_needed_to_start_game) { has_keys = 1; }
		if(has_keys == 0) {
			enter_lobby();
			anim_loop();
		}
	}
}

function load_cached_images() {
	load_cached_image('tired.png');
	load_cached_image('card.png');
	load_cached_image('towers.png');

	var url_prefix = data_url + 'wizard-data/';

	files_needed_to_start_game[url_prefix + 'cards.xml'] = 1;
	files_needed_to_start_game[url_prefix + 'gui.xml'] = 1;
	files_needed_to_start_game[url_prefix + 'unit_overlays.xml'] = 1;

	download_xml_file(url_prefix + 'cards.xml', parse_spells);
	download_xml_file(url_prefix + 'gui.xml', parse_gui);
	download_xml_file(url_prefix + 'unit_overlays.xml', parse_overlays);

	window.requestAnimFrame = (function(){
      return  window.requestAnimationFrame       || 
              window.webkitRequestAnimationFrame || 
              window.mozRequestAnimationFrame    || 
              window.oRequestAnimationFrame      || 
              window.msRequestAnimationFrame     || 
              function(/* function */ callback, /* DOMElement */ element){
                window.setTimeout(callback, 1000 / 60);
              };
    })();
}
