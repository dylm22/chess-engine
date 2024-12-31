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
unsigned int cur_game_state;
uint64_t board::cur_game_state;


const uint64_t board::white_castle_kingside_mask = 0b1111111111111110;
const uint64_t board::white_castle_queenside_mask = 0b1111111111111101;
const uint64_t board::black_castle_kingside_mask = 0b1111111111111011;
const uint64_t board::black_castle_queenside_mask = 0b1111111111110111;
const uint64_t board::white_castle_mask = white_castle_kingside_mask & white_castle_queenside_mask;
const uint64_t board::black_castle_mask = black_castle_kingside_mask & black_castle_queenside_mask;

piece_list *board::get_piece_list(int color_index, int piece){
	return all_lists[color_index * 8 + (piece % 10)];
}
void board::make_move(move move, bool in_search) {
	uint64_t old_enpassant_file = (cur_game_state >> 4) & 15;
	uint64_t old_castle_state = cur_game_state & 15;
	uint64_t new_castle_state = old_castle_state;
	cur_game_state = 0;
	int move_from = move.get_start_square();
	int move_to = move.get_target_square();
	int move_piece = squares[move_from];
	
	int piece_type = utils::piece_type(move_piece);
	int color = white_turn ? 0 : 1;
	int captured_piece_type = utils::piece_type(squares[move_to]);

	int move_flag = move.get_flag(); //move 12 bits over to get to the flag part

	bool is_promotion = false; //FIX LATER

	//cur_game_state |= (unsigned short)((captured_piece_type%10) << 8);
	if (squares[move_to] != NONE) { //capturing
		get_piece_list(1 - color, captured_piece_type)->remove_piece_at_square(move_to);
	}

	if (piece_type == KING) {
		king_squares[color] = move_to;
		new_castle_state &= (white_turn) ? white_castle_mask : black_castle_mask;
	}
	else {
		get_piece_list(color, piece_type)->move_piece(move_from, move_to);
	}
	if (is_promotion) {

	}
	else {
		switch (move_flag)
		{
		case EN_PASSANT_FLAG:
			{
			int en_passant_square = move_to + ((white_turn) ? -8 : 8);
			squares[en_passant_square] = NONE;
			pawns[1 - color].remove_piece_at_square(en_passant_square);
			break;
			}
		case CASTLE_FLAG:
			{
			bool king_side = (move_to == 6 || move_to == 62); // g1 and g8 in index form
			int castling_rook_from = (king_side) ? move_to + 1 : move_to - 2;
			int castling_rook_to = (king_side) ? move_to - 1 : move_to + 1;

			squares[castling_rook_from] = NONE;
			squares[castling_rook_to] = ROOK + ((white_turn) ? WHITE : BLACK);

			rooks[color].move_piece(castling_rook_from, castling_rook_to);
			break;
			}
		}
	}
	squares[move_to] = move_piece;
	squares[move_from] = NONE;

	if (move_flag == TWO_PAWN_PUSH_FLAG) {
		int file = (move_from & 0b000111) + 1;
		cur_game_state |= (unsigned char)(file << 4);
	}

	if (old_castle_state != 0) {
		if (move_to == 7 || move_from == 7) { //h1
			new_castle_state &= white_castle_kingside_mask;
		} else if (move_to == 0 || move_from == 0) {//a1
			new_castle_state &= white_castle_queenside_mask;
		}
		if (move_to == 63 || move_from == 63){ //h8 
			new_castle_state &= black_castle_kingside_mask;
		}
		else if (move_to == 56 || move_from == 56) {
			new_castle_state &= black_castle_queenside_mask;
		}
	}

	cur_game_state |= new_castle_state;
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