#include "move_gen.hpp"
#include "vector"
#include "pre_computed_data.hpp"
#include "utils.hpp"
#include "iostream"

std::vector<move> move_gen::generate_moves(board b) {
	this->b = b;
	init();

	calculate_attack_map();
	gen_king_moves();

	if (double_check) { //only legal moves would be king moves
		return moves;
	}

	generate_sliding_moves();
	gen_knight_moves();
	gen_pawn_moves();

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
	determine_castling_rights();
}

void move_gen::gen_king_moves() {
	for (int i = 0; i < 8; i++) {
		int target_square = precomputed_data::king_moves[b.king_squares[mover_index]][i];

		if (target_square == 255) { //target square was ineligble in the precomputation
			break;
		}

		int piece_on_target_square = b.squares[target_square];

		bool is_enemy_piece = (piece_on_target_square != NONE && utils::is_black(piece_on_target_square) != is_black);
		if (piece_on_target_square != NONE && !is_enemy_piece) {
			continue;
		}

		if (!is_enemy_piece) {
			if (square_is_in_check_ray(target_square)) {
				continue;
			}
		}

		if (!square_is_attacked(target_square)) {
			moves.push_back(move(b.king_squares[mover_index], target_square));

			if (!check && !is_enemy_piece) {
				if ((target_square == 5 || target_square == 61) && can_kingside_castle){
					int castle_king_square = target_square + 1;
					if (b.squares[castle_king_square] == NONE) {
						if (!square_is_attacked(castle_king_square)) {
							moves.push_back(move(b.king_squares[mover_index], castle_king_square, CASTLE_FLAG));
						}
					}
				}
				else if ((target_square == 3 || target_square == 59) && can_queenside_castle) {
					int castle_queen_square = target_square - 1;
					if (b.squares[castle_queen_square] == NONE && b.squares[castle_queen_square - 1] == NONE) {
						if (!square_is_attacked(castle_queen_square)) {
							moves.push_back(move(b.king_squares[mover_index], castle_queen_square, CASTLE_FLAG));
						}
					}
				}
			}
		}
	}
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

			if ((is_capture && utils::is_black(target_square_piece) == is_black) || (check && !square_is_in_check_ray(target_square))) { 
				continue;
			}
			moves.push_back(move(square, target_square));
		}
	}
}

void move_gen::gen_pawn_moves() {
	piece_list friendly_pawns = b.pawns[mover_index];
	int pawn_offset = (b.white_turn) ? 8 : -8;
	int start_rank = (b.white_turn) ? 1 : 6;
	int rank_before_promotion = (b.white_turn) ? 6 : 1;

	int en_passant_file = ((int)(b.cur_game_state >> 4) & 15) - 1;
	int en_passant_square = -1;

	if (en_passant_file != -1) {
		en_passant_square = 8 * ((b.white_turn) ? 5 : 2) + en_passant_file;
	}

	for (int i = 0; i < friendly_pawns.count(); i++) {
		int start_square = friendly_pawns[i];
		int rank = start_square >> 3; //crazy math amiright
		bool one_square_from_promotion = (rank == rank_before_promotion);

		int square_one_forward = start_square + pawn_offset;

		if (b.squares[square_one_forward] == NONE) {
			if (!is_pinned(start_square) || is_moving_along_ray(pawn_offset, start_square, b.king_squares[mover_index])) {
				if (!check || square_is_in_check_ray(square_one_forward)) {
					if (one_square_from_promotion) {
						make_promotion_moves(start_square, square_one_forward);
					}
					else {
						moves.push_back(move(start_square, square_one_forward));
					}
				}

				if (rank == start_rank) {
					int square_two_forward = square_one_forward + pawn_offset;
					if (b.squares[square_two_forward] == NONE) {
						if (!check || square_is_in_check_ray(square_two_forward)) {
							moves.push_back(move(start_square, square_two_forward, TWO_PAWN_PUSH_FLAG));
						}
					}
				}
			}
		}

		for (int k = 0; k < 2; k++) {
			if (precomputed_data::num_squares_to_edge[start_square][precomputed_data::pawn_attack_dirs[mover_index][k]] > 0) {
				int pawn_capture_dir = precomputed_data::dir_offsets[precomputed_data::pawn_attack_dirs[mover_index][k]];
				int target_square = start_square + pawn_capture_dir;
				int target_piece = b.squares[target_square];

				if (is_pinned(start_square) && !is_moving_along_ray(pawn_capture_dir, b.king_squares[mover_index], start_square)) {
					continue;
				}
				if (is_black != utils::is_black(target_piece) && target_piece != NONE) {
					if (check && !square_is_in_check_ray(target_square)) {
						continue;
					}
					if (one_square_from_promotion) {
						make_promotion_moves(start_square, target_square);
					}
					else {
						moves.push_back(move(start_square, target_square));
					}
				}
				if (target_square == en_passant_square) {
					int en_passant_captured_pawn_square = target_square + ((b.white_turn) ? -8 : 8);
					if (!check_after_en_passant(start_square, target_piece, en_passant_captured_pawn_square)) {
						moves.push_back(move(start_square,target_square,EN_PASSANT_FLAG));
					}
				}
			}
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

			opponent_sliding_attack_mask |= 1ull << target_square;
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
		uint64_t ray_mask = 0;
	
		for (int i = 0; i < n; i++) {
			int square_index = b.king_squares[mover_index] + dir_offset * (i + 1);
			ray_mask |= (1ull << square_index);
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
					} else {
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
			check_bitmask |= 1ull << start_square;
		}
	}

	piece_list opponent_pawns = b.pawns[opponent_index];
	opponent_pawn_attack_mask = 0;
	bool is_pawn_check = false;

	for (int pi = 0; pi < opponent_pawns.count(); pi++) {
		int square = opponent_pawns[pi];
		uint64_t pawn_attacks = precomputed_data::pawn_attack_bitboards[square][opponent_index];
		opponent_pawn_attack_mask |= pawn_attacks;

		if (!is_pawn_check && utils::bitboard_contains_square(pawn_attacks, b.king_squares[mover_index])) {
			is_pawn_check = true;
			double_check = check;
			check = true;
			check_bitmask |= 1ull << square;
		}
	}
	pawnless_opponent_attack_mask = opponent_sliding_attack_mask | opponent_knight_attack_mask | precomputed_data::king_attack_bitboards[b.king_squares[opponent_index]];
	opponent_attack_mask = pawnless_opponent_attack_mask | opponent_pawn_attack_mask;
}

void move_gen::make_promotion_moves(int start_square, int target_square) {
	moves.push_back(move(start_square, target_square, GENERAL_PROMOTION_FLAG));
}


bool move_gen::square_is_in_check_ray(int square) {
	return check && ((check_bitmask >> square) & 1) != 0;
}

bool move_gen::square_is_attacked(int square) {
	return utils::bitboard_contains_square(opponent_attack_mask, square);
}

void move_gen::determine_castling_rights() {
	int m1 = (b.white_turn) ? 1 : 4;
	can_kingside_castle = (b.cur_game_state & m1) != 0;

	int m2 = (b.white_turn) ? 2 : 8;
	can_queenside_castle = (b.cur_game_state & m2) != 0;
}

bool move_gen::check_after_en_passant(int start_square, int target_square, int en_passant_captured_square) {
	b.squares[target_square] = b.squares[start_square];
	b.squares[start_square] = NONE;
	b.squares[en_passant_captured_square] = NONE;

	bool in_check_after = false;
	if (square_attacked_after_en_passant(en_passant_captured_square, start_square)) {
		in_check_after = true;
	}

	b.squares[target_square] = NONE;
	b.squares[start_square] = PAWN + mover_color;
	b.squares[en_passant_captured_square] = PAWN + opponent_color;
	return in_check_after;
}

bool move_gen::square_attacked_after_en_passant(int en_passant_capture_square, int start_square) {
	if (utils::bitboard_contains_square(pawnless_opponent_attack_mask, b.king_squares[mover_index])) {
		return true;
	}

	int dir_index = (en_passant_capture_square < b.king_squares[mover_index]) ? 2 : 3;
	for (int i = 0; i < precomputed_data::num_squares_to_edge[b.king_squares[mover_index]][dir_index]; i++) {
		int square_i = b.king_squares[mover_index] + precomputed_data::dir_offsets[dir_index] * (i + 1);
		int piece = b.squares[square_i];

		if (piece != NONE) {
			if (is_black && utils::is_black(piece)) {
				break;
			}
			else {
				if (utils::is_horizontal_sliding(piece)) {
					return true;
				}
				else {
					break;
				}
			}
		}
	}

	for (int i = 0; i < 2; i++) {
		if (precomputed_data::num_squares_to_edge[b.king_squares[mover_index]][precomputed_data::pawn_attack_dirs[mover_index][i]] > 0) {
			int piece = b.squares[b.king_squares[mover_index] + precomputed_data::dir_offsets[precomputed_data::pawn_attack_dirs[mover_index][i]]];
			if (piece == (PAWN + opponent_color)) {
				return true;
			}
		}
	}

	return false;
}

bool move_gen::in_check() {
	return check;
}