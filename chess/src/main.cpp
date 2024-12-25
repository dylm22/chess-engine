#include <iostream>


#include "def.hpp"
#include "game.hpp"
#include "board.hpp"
#include "pre_computed_data.hpp"

int main() {
	precomputed_data::precompute_edge_dist();

	game *p_g = new game;
	board* p_b = new board;
	p_b->init();
	p_g->b = *p_b;

	p_g->create_board();

	while (true) {
		p_g->draw_board();
		p_g->handle_input();
	}
	return 0;
}