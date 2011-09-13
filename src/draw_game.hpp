#ifndef DRAW_GAME_HPP_INCLUDED
#define DRAW_GAME_HPP_INCLUDED

class client_play_game;
class game;

void draw_map(const client_play_game& info, const game& g, int xpos, int ypos);

void draw_underlays(const client_play_game& info, const game& g, const hex::location& loc);
void draw_overlays(const client_play_game& info, const game& g, const hex::location& loc);

#endif
