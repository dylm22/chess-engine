#include "ai_player.hpp"
#include "move_gen.hpp"
#include "board.hpp"

#include <random>

board ai_player::b;

void ai_player::init(board _b, bool color) {
	this->b = _b;
	white = color;
}

void ai_player::handle_turn(move* move_ptr) {
	move_ptr->move_value = make_move().move_value;
}

move ai_player::make_move() {
	move_gen n;
	std::vector<move> legal_moves = n.generate_moves(b);

	move m = legal_moves[rand() % legal_moves.size()];

	return m;
}

