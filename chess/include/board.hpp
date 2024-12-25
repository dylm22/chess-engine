#pragma once

#include "def.hpp"
#include "game.hpp"

#define WHITE_INDEX 0
#define BLACK_INDEX 1

class board {
public:
	static bool white_turn;
	static int squares[64];

	static int king_squares[2];
	static piece_list *knights;
	static piece_list *bishops;
	static piece_list *pawns;
	static piece_list *queens;
	static piece_list *rooks;
	static std::vector<piece_list> all_lists;

	void make_move(move move, bool in_search);
	piece_list get_piece_list(int color_index, int piece);
	void init();
};

struct move {
	unsigned short move_value;

	const unsigned short start_square_mask =  0b0000000000111111;
	const unsigned short target_square_mask = 0b0000111111000000;
	const unsigned short flag_mask = 0b1111000000000000;
	
	move() {
		move_value = 0;
	}

	move(unsigned short _move_value) {
		this->move_value = _move_value;
	}
	move(int _start_square, int _target_square) {
		move_value = (unsigned short)(_start_square | _target_square << 6);
	}
	
	int get_start_square() {
		return move_value & start_square_mask;
	}
	int get_target_square() {
		return (move_value & target_square_mask) >> 6;
	}
	
};