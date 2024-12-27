#include "move_gen.hpp"
#include "vector"
#include "pre_computed_data.hpp"
#include "utils.hpp"
#include "iostream"

std::vector<move> move_gen::generate_moves(board b) {
	this->b = b;
	init();

	generate_sliding_moves();

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
	bool is_pinned = false; //FOR NOW CHANGE LATER
	if (check && is_pinned) {
		return; //no legal moves
	}
	for (int dir_i = first_dir_index; dir_i < last_dir_index; dir_i++) {
		int cur_dir_offset = precomputed_data::dir_offsets[dir_i];

		//add pin ray shit
		
		for (int n = 0; n < precomputed_data::num_squares_to_edge[start_square][dir_i]; n++) {
			int target_square = start_square + cur_dir_offset * (n + 1);
			int target_square_piece = b.squares[target_square];

			bool is_capture = target_square_piece != NONE;

			if (is_capture && (utils::is_black(target_square_piece) == is_black)) {
				break;
			}

			
			if (!check) {
				
				moves.push_back(move(start_square, target_square));
			}

			if (is_capture) {
				break;
			}
			
		}
	}
	
}

void move_gen::generate_sliding_moves() {
	piece_list rooks = b.rooks[mover_index];
	std::cout << rooks.count();
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

bool move_gen::is_pinned(int square) {
	return pins_exist && ((pin_bitmask >> square) & 1) != 0; //bitshift by square so that & 1 is comparing to the square we are on
}