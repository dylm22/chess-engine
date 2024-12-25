#include "def.hpp"
#include "game.hpp"
#include <iostream>
#include "utils.hpp"
#include "move_gen.hpp"

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
	//load_position_from_fen(STARTING_POSITION);

	b.squares[22] = WHITE + ROOK;

	b.squares[60] = WHITE + ROOK;

	b.squares[20] = WHITE + BISHOP;

	b.squares[15] = BLACK + QUEEN;

	b.squares[34] = BLACK + ROOK;

	for (int i = 0; i < 64; i++) {
		int piece = b.squares[i];
		if (piece != NONE) {
			int piece_color = utils::is_black(piece) ? 1 : 0;

			if (piece - WHITE == QUEEN || piece - BLACK == QUEEN) {
				b.queens[piece_color].add_piece_at_square(i);
			} else if (piece - WHITE == ROOK || piece - BLACK == ROOK) {
				b.rooks[piece_color].add_piece_at_square(i);
			} else if (piece - WHITE == BISHOP || piece - BLACK == BISHOP) {
				b.bishops[piece_color].add_piece_at_square(i);
			} else if (piece - WHITE == KNIGHT || piece - BLACK == KNIGHT) {
				b.knights[piece_color].add_piece_at_square(i);
			} else if (piece - WHITE == PAWN || piece - BLACK == PAWN) {
				b.pawns[piece_color].add_piece_at_square(i);
			} else if (piece - WHITE == KING || piece - BLACK == KING) {
				b.king_squares[piece_color] = i;
			}
		}
	}
	
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
	std::cout << std::endl << "enter move in the format [ e2e4 ] : ";
	std::string _move;
	std::getline(std::cin, _move);
	
	int start_index = notation_to_number(_move.substr(0, 2));
	int target_index = notation_to_number(_move.substr(2, 4));

	bool is_legal = false;
	move_gen n;

	std::vector<move> legal_moves = n.generate_moves(b);

	for (int i = 0; i < legal_moves.size(); i++) {
		move legal_move = legal_moves[i];
		if (b.squares[legal_moves[i].get_target_square()] == NONE) {
			b.squares[legal_moves[i].get_target_square()] = 120;
		}
		
		if (legal_move.get_start_square() == start_index && legal_move.get_target_square() == target_index) {
			is_legal = true;
			break;
		}
	}
	draw_board();
	for (int i = 0; i < legal_moves.size(); i++) {
		if (b.squares[legal_moves[i].get_target_square()] == 120) {
			b.squares[legal_moves[i].get_target_square()] = NONE;
		}
		
	}

	if (is_legal)
		b.make_move(move(start_index, target_index), false);
	else
		std::cout << "move was illegal";
}

int game::notation_to_number(std::string notation) {
	return ((((int)notation[0]) - 96)-1) + ((((int)notation[1] - 48)-1) * 8);
}



//std::vector<move> game::gen_moves() {
	 //
//}


/*b.all_lists[2] = b.pawns[WHITE_INDEX];
	b.all_lists[3] = b.knights[WHITE_INDEX];
	b.all_lists[5] = b.bishops[WHITE_INDEX];
	b.all_lists[6] = b.rooks[WHITE_INDEX];
	b.all_lists[7] = b.queens[WHITE_INDEX];
	b.all_lists[10] =   b.pawns[BLACK_INDEX];
	b.all_lists[11] = b.knights[BLACK_INDEX];
	b.all_lists[13] = b.bishops[BLACK_INDEX];
	b.all_lists[14] =   b.rooks[BLACK_INDEX];
	b.all_lists[15] =  b.queens[BLACK_INDEX];*/