#pragma once

#define NONE 46
#define KING 10
#define PAWN 15
#define KNIGHT 13
#define BISHOP 1
#define ROOK 17
#define QUEEN 16

#define WHITE 65
#define BLACK 97

#define STARTING_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

namespace utils {
	inline bool is_black(int piece) {
		return (((piece >> 5) & 1) != 0);
	}

	inline int piece_type(int piece) {
		return ((piece & 0b0011111) - 1);
	}

	inline bool is_diagonal_sliding(int piece) {
		return (piece == BISHOP || piece == QUEEN);
	}

	inline bool is_horizontal_sliding(int piece) {
		return (piece == ROOK || piece == QUEEN);
	}

	inline bool bitboard_contains_square(unsigned long bitboard, int square) {
		return ((bitboard >> square) & 1) != 0;
	}
	inline int sign(int val) {
		return (0 < val) - (val < 0);
	}
}