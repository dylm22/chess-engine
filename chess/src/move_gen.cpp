#include "move_gen.hpp"
#include "vector"
#include "pre_computed_data.hpp"
#include "utils.hpp"
#include "iostream"

std::vector<move> move_gen::generate_moves(board b) {
	this->b = b;
	init();

	calculate_attack_map();

	generate_sliding_moves();
	gen_knight_moves();

	return moves;
}

void move_gen::init() {
	moves = std::vector<move>();
	check = false;
	double_check = false;
	pins_exist = false;
	
	check_bitmask = 0;
	pin_bitmask = 0;

	is_black = b.white_turn ? false : true;
	mover_color = b.white_turn ? WHITE : BLACK;
	opponent_color = b.white_turn ? BLACK : WHITE;

	mover_index = b.white_turn ? 0 : 1;
	opponent_index = 1 - mover_index;
}
void move_gen::gen_sliding_piece_moves(int start_square, int first_dir_index, int last_dir_index) {
	bool pinned = is_pinned(start_square); 
	if (check && pinned) {
		return; //no legal moves
	}
	for (int dir_i = first_dir_index; dir_i < last_dir_index; dir_i++) {
		int cur_dir_offset = precomputed_data::dir_offsets[dir_i];

		if (pinned && !is_moving_along_ray(cur_dir_offset,b.king_squares[mover_index], start_square)) {
			continue;
		}
		
		for (int n = 0; n < precomputed_data::num_squares_to_edge[start_square][dir_i]; n++) {
			int target_square = start_square + cur_dir_offset * (n + 1);
			int target_square_piece = b.squares[target_square];

			bool is_capture = target_square_piece != NONE;
			bool move_prevents_check = square_is_in_check_ray(target_square);

			if (is_capture && (utils::is_black(target_square_piece) == is_black)) {
				break;
			}

			
			if (move_prevents_check || !check) {
				
				moves.push_back(move(start_square, target_square));
			}

			if (is_capture || move_prevents_check) {
				break;
			}
			
		}
	}
	
}

void move_gen::generate_sliding_moves() {
	piece_list rooks = b.rooks[mover_index];
	for (int i = 0; i < rooks.count(); i++) {
		gen_sliding_piece_moves(rooks[i], 0, 4);
	}	
	piece_list bishops = b.bishops[mover_index];
	for (int i = 0; i < bishops.count(); i++) {
		gen_sliding_piece_moves(bishops[i], 4, 8);
	}
	piece_list queens = b.queens[mover_index];
	for (int i = 0; i < queens.count(); i++) {
		gen_sliding_piece_moves(queens[i], 0, 8);
	}
}

void move_gen::gen_knight_moves() {
	piece_list knights = b.knights[mover_index];

	for (int i = 0; i < knights.count(); i++) {
		int square = knights[i];
		
		if (is_pinned(square)) {
			continue;
		}

		for (int kni = 0; kni < 8; kni++) {
			int target_square = precomputed_data::knight_moves[square][kni];
			if (target_square == 255) {
				break;
			}
			int target_square_piece = b.squares[target_square];

			bool is_capture = target_square_piece != NONE;

			if ((utils::is_black(target_square_piece) != is_black) || (check && !square_is_in_check_ray(target_square))) { 
				continue;
			}
			moves.push_back(move(square, target_square));
		}
	}
}

bool move_gen::is_pinned(int square) {
	return pins_exist && ((pin_bitmask >> square) & 1) != 0; //bitshift by square so that & 1 is comparing to the square we are on
}

bool move_gen::is_moving_along_ray(int ray_dir, int start_square, int target_square) {
	int move_dir = precomputed_data::dir_lookup[target_square - start_square + 63];
	return (ray_dir == move_dir || -ray_dir == move_dir);
}
void move_gen::update_sliding_attack_map(int start_square, int start_dir, int end_dir) {
	for (int dir_i = start_dir; dir_i < end_dir; dir_i++) {
		int cur_dir_offset = precomputed_data::dir_offsets[dir_i];

		for (int n = 0; n < precomputed_data::num_squares_to_edge[start_square][dir_i]; n++) {
			int target_square = start_square + cur_dir_offset * (n + 1);
			int target_square_piece = b.squares[target_square];

			opponent_sliding_attack_mask |= 1ul << target_square;
			if (target_square != b.king_squares[mover_index]) {
				if (target_square_piece != NONE) {
					break;
				}
			}
		}
	}
}
void move_gen::gen_sliding_attack_map() {
	opponent_sliding_attack_mask = 0;

	piece_list enemy_rooks = b.rooks[opponent_index];
	for (int i = 0; i < enemy_rooks.count(); i++) {
		update_sliding_attack_map(enemy_rooks[i], 0, 4);
	}
	piece_list enemy_queens = b.queens[opponent_index];
	for (int i = 0; i < enemy_queens.count(); i++) {
		update_sliding_attack_map(enemy_queens[i], 0, 8);
	}
	piece_list enemy_bishops = b.bishops[opponent_index];
	for (int i = 0; i < enemy_bishops.count(); i++) {
		update_sliding_attack_map(enemy_bishops[i], 4, 8);
	}
}

void move_gen::calculate_attack_map() {
	
	gen_sliding_attack_map();
	
	int start_dir_index = 0;
	int final_dir_index = 8;

	if (b.queens[opponent_index].count() == 0) {
		start_dir_index = (b.rooks[opponent_index].count() > 0) ? 0 : 4;
		final_dir_index = (b.bishops[opponent_index].count() > 0) ? 8 : 4;
	}

	for (int d = start_dir_index; d < final_dir_index; d++) {
		bool is_diagonal = d > 3;

		int n = precomputed_data::num_squares_to_edge[b.king_squares[mover_index]][d];
		int dir_offset = precomputed_data::dir_offsets[d];
		bool is_friendly_piece_along_path = false;
		unsigned long ray_mask = 0;
	
		for (int i = 0; i < n; i++) {
			int square_index = b.king_squares[mover_index] + dir_offset * (i + 1);
			ray_mask |= 1ul << square_index;
			int piece = b.squares[square_index];



			if (piece != NONE) { // encountered piece
				if (utils::is_black(piece) == is_black) {// is friendly
					if (!is_friendly_piece_along_path) {
						is_friendly_piece_along_path = true;
					}
					else {
						break;
					}
				} else { // is enemy
					int piece_type = utils::piece_type(piece);

					if (is_diagonal && utils::is_diagonal_sliding(piece_type) || !is_diagonal && utils::is_horizontal_sliding(piece_type)) { //is able to move in current dir
						if (is_friendly_piece_along_path) { // friendly blocks check so its a pin
							pins_exist = true;
							pin_bitmask |= ray_mask;
						}
						else {
							check_bitmask |= ray_mask; //no friendly, its a check
							double_check = check; //already in check, then its double
							check = true;
						}
						break;
					}
					else {
						break; //cannot move in this dir, is blocking any checks or pins
					}
				}
			}

		}

		if (double_check) {
			break; //don't look for pins in double check, king is the only legal move
		}
			
	}

	piece_list opponent_knights = b.knights[opponent_index];
	opponent_knight_attack_mask = 0;
	bool is_knight_check = false;

	for (int ki = 0; ki < opponent_knights.count(); ki++) {
		int start_square = opponent_knights[ki];
		opponent_knight_attack_mask |= precomputed_data::knight_attack_bitboards[start_square];

		if (!is_knight_check && utils::bitboard_contains_square(opponent_knight_attack_mask, b.king_squares[mover_index])) {
			is_knight_check = true;
			double_check = check; // same as above, if already in check, then its double
			check = true;
			check_bitmask |= 1ul << start_square;
		}
	}

	piece_list opponent_pawns = b.pawns[opponent_index];
	opponent_pawn_attack_mask = 0;
	bool is_pawn_check = false;

	for (int pi = 0; pi < opponent_pawns.count(); pi++) {
		int square = opponent_pawns[pi];
		unsigned long pawn_attacks = precomputed_data::pawn_attack_bitboards[square][opponent_index];
		opponent_pawn_attack_mask |= pawn_attacks;

		if (!is_pawn_check && utils::bitboard_contains_square(pawn_attacks, b.king_squares[mover_index])) {
			is_pawn_check = true;
			double_check = check;
			check = true;
			check_bitmask |= 1ul << square;
		}
	}
}


bool move_gen::square_is_in_check_ray(int square) {
	return check && ((check_bitmask >> square) & 1) != 0;
}