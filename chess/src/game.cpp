#include "def.hpp"
#include "game.hpp"
#include <iostream>

board game::b;


void game::load_position_from_fen(std::string fen) {
	int file = 0; int rank = 7;

	int n = fen.find(" ");

	for (int i = 0; i < n; i++) {
		if (fen[i] == '/') {
			file = 0;
			rank--;
		}
		else {
			if (fen[i] < 58) {
				file += ((int)fen[i] - 48); //58 is the number directly after nine in ascii, 48 is 0.
			}
			else {
				b.squares[(rank * 8) + file] = fen[i];
				file++;
			}
		}
	}
}

void game::create_board() {
	load_position_from_fen(STARTING_POSITION);
}
void game::draw_board() {
	std::cout << std::endl << std::endl;
	for (int y = 7; y >=0 ; y--) {
		for (int x = 0; x < 8; x++) {
			std::cout << (char)b.squares[y*8 + x] << ' ';
		}
		std::cout << "  " << y+1 << std::endl;
	}
	std::cout << std::endl << "a b c d e f g h" << std::endl << std::endl;
}

void game::handle_input() {
	std::cout << std::endl << "enter move in the format [ e2->e4 ] : ";
	std::string _move;
	std::getline(std::cin, _move);
	
	b.make_move(move(notation_to_number(_move.substr(0, 2)), notation_to_number(_move.substr(4, 2))), false);
}

int game::notation_to_number(std::string notation) {
	return ((((int)notation[0]) - 96)-1) + ((((int)notation[1] - 48)-1) * 8);
}



//std::vector<move> game::gen_moves() {
	 //
//}