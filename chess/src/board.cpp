#include "def.hpp"
#include "board.hpp"
#include<iostream>

int board::squares[64];
bool board::white_turn;
piece_list* board::knights;
piece_list* board::bishops;
piece_list* board::pawns;
piece_list* board::queens;
piece_list* board::rooks;
piece_list* board::all_lists;


void board::make_move(move move, bool in_search) {
	int move_from = move.get_start_square();
	int move_to = move.get_target_square();
	int move_piece = squares[move_from];

	std::cout << move_from << "->" << move_to;

	squares[move_to] = move_piece;
	squares[move_from] = NONE;

	white_turn = !white_turn;
}

void board::init() {
	for (int i = 0; i < 64; i++) {
		squares[i] = '.';
	}

	white_turn = true;

	knights = new piece_list[2]{ piece_list(10),  piece_list(10) };
	pawns = new piece_list[2]{  piece_list(8),  piece_list(8) };
	rooks = new piece_list[2]{  piece_list(10),  piece_list(10) };
	bishops = new piece_list[2]{  piece_list(10),  piece_list(10) };
	queens = new piece_list[2]{  piece_list(9),  piece_list(9) };

	piece_list emptyList = piece_list(0);
	all_lists = new piece_list[16]{
		emptyList,
		emptyList,
		pawns[WHITE_INDEX],
		knights[WHITE_INDEX],
		emptyList,
		bishops[WHITE_INDEX],
		rooks[WHITE_INDEX],
		queens[WHITE_INDEX],
		emptyList,
		emptyList,
		pawns[BLACK_INDEX],
		knights[BLACK_INDEX],
		emptyList,
		bishops[BLACK_INDEX],
		rooks[BLACK_INDEX],
		queens[BLACK_INDEX],
	};
	
}