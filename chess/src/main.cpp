#include <iostream>


#include "def.hpp"
#include "game.hpp"
#include "board.hpp"


int main() {
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