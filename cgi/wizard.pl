#!/usr/bin/perl

use strict;
use CGI qw/:standard/;

print "Content-type: text/html\n\n";

my $id = param('id');
my $hostname = $ENV{'SERVER_NAME'};

print qq~
<html>
<head>
	<script type="application/javascript">
	var server_hostname = '$hostname';
	</script>
	<script type="application/x-javascript" src="/dave/wizard.js">
	</script>
</head>
~;

if($id) {
	print qq~
	<body onload="init_wizard(); set_user_id('$id'); load_game_data(); load_cached_images(); create_game_with_bots(); anim_loop();">
	<p>
	<table cellpadding=0 cellspacing=0>
	<tr><td>
		<table cellpadding=0 cellspacing=0>
		<tr><td>
		<canvas id="board" width=800 height=600 onmousemove="mouse_move(event);" onmousedown="mouse_down(event);">
		Canvas not supported. :-(
		</canvas>
		</td>
		<td>
			<table>
			<tr><td>
			<input id="end_turn_button" type="submit" value="End Turn" onclick="end_turn()"/>
			</td></tr>
			<tr><td><label id="loc_label"></label></td></tr>
			<tr><td><canvas id="player_info" width=100 height=100></canvas></td></tr>
			<tr><td id='unit_td'></td></tr>
			</table>
		</td>
		</tr>
		</table>
	</td></tr>

	<tr>
	<td id='spells_para'>
	</td>
	</tr>
	</table>
	</p>

	</body>
	</html>
~;
} else {
	print qq~
	<body onload="send_create_request();">
		<p>
			<label id='status'>Loading...</label>
		</p>
	</body>
	</html>
~;
}
