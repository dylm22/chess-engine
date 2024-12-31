#include <iostream>


#include "def.hpp"
#include "game.hpp"
#include "board.hpp"
#include "pre_computed_data.hpp"

int main() {
	precomputed_data::precompute_data();

	game* p_g = new game;
	board* p_b = new board;
	p_b->init();
	p_g->b = *p_b;

	p_g->create_board();

	bool continue_game = true;

	while (continue_game) {
		continue_game = p_g->game_ended();
		p_g->draw_board();
		p_g->handle_play();
	}
	return 0;
}