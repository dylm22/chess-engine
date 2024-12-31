#include "def.hpp"
#include "game.hpp"
#include <iostream>
#include "utils.hpp"
#include "move_gen.hpp"

board game::b;
ai_player game::bot;

void game::load_position_from_fen(std::string fen) {
	int file = 0; int rank = 7;

	int n = fen.length();
	int i = 0;

	for (; i < n && (fen[i] != ' '); i++) {
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

	b.white_turn = (fen[i + 1] == 'w') ? true : false;

	w_castle_king = false;
	w_castle_queen = false;
	b_castle_king = false;
	b_castle_queen = false;
	for (i += 3; i < n && (fen[i] != ' '); i++) {
		if (fen[i] == 'k') {
			b_castle_king = true;
		}
		else if (fen[i] == 'K') {
			w_castle_king = true;
		}
		else if (fen[i] == 'q') {
			b_castle_queen = true;
		}
		else if (fen[i] == 'Q') {
			w_castle_queen = true;
		}
	}
}

void game::create_board() {
	std::cout << "which side should the player control (type only w or b)" << std::endl;
	std::string color;
	std::getline(std::cin, color);

	player_is_white = (color == "w") ? true : false;

	bot.init(b, !player_is_white);

	load_position_from_fen(STARTING_POSITION);

	//b.squares[utils::notation_to_number("h8")] = WHITE + ROOK;

	//b.squares[utils::notation_to_number("f8")] = WHITE + KNIGHT;

	//b.squares[utils::notation_to_number("b1")] = WHITE + KING;

	//b.squares[utils::notation_to_number("d8")] = BLACK + KING;

	//b.squares[utils::notation_to_number("d7")] = BLACK + KNIGHT;

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

	int white_castle = ((w_castle_king) ? 1 << 0 : 0) | ((w_castle_queen) ? 1 << 1 : 0);
	int black_castle = ((b_castle_king) ? 1 << 2 : 0) | ((b_castle_queen) ? 1 << 3 : 0);
	//int en_passant_state = loadedPosition.epFile << 4;
	unsigned short initialGameState = (unsigned short)(white_castle | black_castle);
	b.cur_game_state = initialGameState;
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
void game::handle_play() {
	move move_to_play;
	
	if (b.white_turn) {
		if (player_is_white) {
			handle_input(&move_to_play);
		}
		else {
			bot.handle_turn(&move_to_play);
		}
	}
	else {
		if (!player_is_white) {
			handle_input(&move_to_play);
		}
		else {
			bot.handle_turn(&move_to_play);
		}
	}
	b.make_move(move_to_play, false);
}

void game::handle_input(move* move_ptr) {
	move_gen n;
	std::vector<move> legal_moves = n.generate_moves(b);
	
	reset:
	std::cout << std::endl << "enter move in the format [ e2e4 ] : ";
	std::string _move;
	std::getline(std::cin, _move);
	
	int start_index = utils::notation_to_number(_move.substr(0, 2));
	int target_index = utils::notation_to_number(_move.substr(2, 4));
	int flag = 0;

	bool is_legal = false;

	for (int i = 0; i < legal_moves.size(); i++) {
		move legal_move = legal_moves[i];
		if (b.squares[legal_moves[i].get_target_square()] == NONE) {
			b.squares[legal_moves[i].get_target_square()] = 120;
		}
		
		if (legal_move.get_start_square() == start_index && legal_move.get_target_square() == target_index) {
			is_legal = true;
			flag = legal_move.get_flag();
			break;
		}
	}
	draw_board();
	for (int i = 0; i < legal_moves.size(); i++) {
		if (b.squares[legal_moves[i].get_target_square()] == 120) {
			b.squares[legal_moves[i].get_target_square()] = NONE;
		}
		
	}

	if (is_legal) {
		move_ptr->move_value = move(start_index, target_index, flag).move_value;
	}
	else {
		std::cout << "move was illegal";
		goto reset;
	}	
}


bool game::game_ended() {
	move_gen n;
	std::vector<move> legal_moves = n.generate_moves(b);

	if (legal_moves.size() == 0) {
		if (n.in_check()) {
			std::cout << "game over, checkmate" << std::endl;
			return false;
		}
		std::cout << "game over, stalemate" << std::endl;
		return false;
	}
	//Im ignoring threefold, 50 move rule, and insufficient material right now 

	return true;
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