#pragma once 

#include "def.hpp"
#include "board.hpp"

#include <vector>

class move_gen {
public: 
	unsigned long opponent_attack_mask;
	unsigned long opponent_pawn_attack_mask;

	std::vector<move> generate_moves(board b);
	void init();
	void generate_sliding_moves();
	void gen_sliding_piece_moves(int start_square, int first_dir_index, int last_dir_index);
	bool is_pinned(int square);
private:
	std::vector<move> moves;
	board b;
	int mover_color;
	int opponent_color;
	int mover_index;
	int opponent_index;

	bool is_black;
	bool check;
	bool double_check;
	bool pins_exist;

	unsigned long check_bitmask;
	unsigned long pin_bitmask;

	unsigned long opponent_knight_attack_mask;
	unsigned long pawnless_opponent_attack_mask;
	unsigned long opponent_sliding_attack_mask;

};