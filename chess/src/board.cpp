#include "def.hpp"
#include "board.hpp"
#include "utils.hpp"
#include <iostream>

int board::squares[64];
bool board::white_turn;
int board::king_squares[2];
piece_list* board::knights;
piece_list* board::bishops;
piece_list* board::pawns;
piece_list* board::queens;
piece_list* board::rooks;
piece_list* board::all_lists[16];

piece_list *board::get_piece_list(int color_index, int piece){
	return all_lists[color_index * 8 + (piece % 10)];
}
void board::make_move(move move, bool in_search) {
	int move_from = move.get_start_square();
	int move_to = move.get_target_square();
	int move_piece = squares[move_from];
	
	int piece_type = utils::piece_type(move_piece);
	int color = white_turn ? 0 : 1;

	if (squares[move_to] != NONE) { //capturing
		get_piece_list(1 - color, utils::piece_type(squares[move_to]))->remove_piece_at_square(move_to);
	}

	if (piece_type == KING) {
		king_squares[color] = move_to;
		//castling bs
	}
	else {
		get_piece_list(color, piece_type)->move_piece(move_from, move_to);
	}

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
	
	
	all_lists[0] = &emptyList;
	all_lists[1] = &bishops[WHITE_INDEX];
	all_lists[2] = &emptyList;
	all_lists[3] = &knights[WHITE_INDEX];
	all_lists[4] = &emptyList;
	all_lists[5] = &pawns[WHITE_INDEX];
	all_lists[6] = &queens[WHITE_INDEX];
	all_lists[7] = &rooks[WHITE_INDEX];
	all_lists[8] = &emptyList;
	all_lists[9] = &bishops[BLACK_INDEX];
	all_lists[10] = &emptyList;
	all_lists[11] = &knights[BLACK_INDEX];
	all_lists[12] = &emptyList;
	all_lists[13] = &pawns[BLACK_INDEX];
	all_lists[14] = &queens[BLACK_INDEX];
	all_lists[15] = &rooks[BLACK_INDEX];

	
	
	
}


/*all_lists = new piece_list[16]{
		emptyList,
		bishops[WHITE_INDEX],
		emptyList,
		knights[WHITE_INDEX],
		emptyList,
		pawns[WHITE_INDEX],
		queens[WHITE_INDEX],
		rooks[WHITE_INDEX],
		emptyList,
		bishops[BLACK_INDEX],
		emptyList,
		knights[BLACK_INDEX],
		emptyList,
		pawns[BLACK_INDEX],
		queens[BLACK_INDEX],
		rooks[BLACK_INDEX]
	};*/