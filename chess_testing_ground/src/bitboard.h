#ifndef BB_H_INC 
#define BB_H_INC

#include "types.h"

#include <cassert>
#include <nmmintrin.h>
#include <algorithm>
#include <string>

namespace engine {
	namespace bit_board {
		void init();
		std::string display(bb b);
	}

	constexpr bb FILEABB = 0x0101010101010101ULL;
	constexpr bb FILEBBB = FILEABB << 1;
	constexpr bb FILECBB = FILEABB << 2;
	constexpr bb FILEDBB = FILEABB << 3;
	constexpr bb FILEEBB = FILEABB << 4;
	constexpr bb FILEFBB = FILEABB << 5;
	constexpr bb FILEGBB = FILEABB << 6;
	constexpr bb FILEHBB = FILEABB << 7;

	constexpr bb RANK1BB = 0xFF;
	constexpr bb RANK2BB = RANK1BB << (8 * 1);
	constexpr bb RANK3BB = RANK1BB << (8 * 2);
	constexpr bb RANK4BB = RANK1BB << (8 * 3);
	constexpr bb RANK5BB = RANK1BB << (8 * 4);
	constexpr bb RANK6BB = RANK1BB << (8 * 5);
	constexpr bb RANK7BB = RANK1BB << (8 * 6);
	constexpr bb RANK8BB = RANK1BB << (8 * 7);

	extern uint8_t pop_cnt16[1 << 16];
	extern uint8_t square_distance[SQUARE_NB][SQUARE_NB];

	extern bb between_BB[SQUARE_NB][SQUARE_NB];
	extern bb line_BB[SQUARE_NB][SQUARE_NB];
	extern bb pseudo_attacks[PIECE_TYPE_NB][SQUARE_NB];
	extern bb pawn_attacks[COLOR_NB][SQUARE_NB];

	//done at compile time not runtime
	//imma also keep it a stack i saw good programmers do this so fake it till you make it

	struct magic {
		bb mask;
		bb* attacks;

		bb magic;
		unsigned shift;

		unsigned index(bb occupied) const { //magic bitboards, gets the index from the magic and the blockers
			return unsigned(((occupied & mask) * magic) >> shift);
		}
		
		bb attacks_bb(bb occupied) const { return attacks[index(occupied)]; }
	};

	extern magic magics[SQUARE_NB][2];

	constexpr bb square_bb(square s) {
		assert(is_in_bounds(s)); //if square is out of bounds, breakpoint and error, something has gone very wrong
		return (1ULL << s);
	}

	inline bb  operator&(bb b, square s) { return b & square_bb(s); }
	inline bb  operator|(bb b, square s) { return b | square_bb(s); }
	inline bb  operator^(bb b, square s) { return b ^ square_bb(s); }
	inline bb& operator|=(bb& b, square s) { return b |= square_bb(s); }
	inline bb& operator^=(bb& b, square s) { return b ^= square_bb(s); }
		   
	inline bb operator&(square s, bb b) { return b & s; }
	inline bb operator|(square s, bb b) { return b | s; }
	inline bb operator^(square s, bb b) { return b ^ s; }
		   
	inline bb operator|(square s1, square s2) { return square_bb(s1) | s2; }

	constexpr bool more_than_one(bb b) { return b & (b - 1); }

	constexpr bb rank_bb(rank r) { return RANK1BB << (8 * r); }
	constexpr bb rank_bb(square s) { return rank_bb(rank_of(s)); }
	constexpr bb file_bb(file f) { return FILEABB << f; }
	constexpr bb file_bb(square s) { return file_bb(file_of(s)); }

	template<direction D> //i dont really use templates so this might be messed up
	constexpr bb shift(bb b) {
		return D == NORTH ? b << 8
			: D == SOUTH ? b >> 8
			: D == NORTH + NORTH ? b << 16
			: D == SOUTH + SOUTH ? b >> 16
			: D == EAST ? (b & ~FILEHBB) << 1
			: D == WEST ? (b & ~FILEABB) >> 1
			: D == NORTH_EAST ? (b & ~FILEHBB) << 9
			: D == NORTH_WEST ? (b & ~FILEABB) << 7
			: D == SOUTH_EAST ? (b & ~FILEHBB) >> 7
			: D == SOUTH_WEST ? (b & ~FILEABB) >> 9
			: 0;
	}


	//returns attacked squares of pawns of a color
	template<color C> //im doing this now
	constexpr bb pawn_attacks_bb(bb b) {
		return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
			: shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
	}

	
	inline bb pawn_attacks_bb(color c, square s) {
		assert(is_in_bounds(s));
		return pawn_attacks[c][s];
	}

	//returns a bitboard thats a line from board edge to board edge
	//returns 0 for diagonals
	inline bb line_bb(square s1, square s2) {
		assert(is_in_bounds(s1) && is_in_bounds(s2));
		return line_BB[s1][s2];
	}

	//returns bitboard with squres between (excluding) s1 and (including) s2
	inline bb between_bb(square s1, square s2) {

		assert(is_in_bounds(s1) && is_in_bounds(s2));
		return between_BB[s1][s2];
	}

	inline bool aligned(square s1, square s2, square s3) { return line_bb(s1, s2) & s3; }

	template<typename T1 = square>
	inline int distance(square x, square y);

	template<>
	inline int distance<file>(square x, square y) {
		return std::abs(file_of(x) - file_of(y));
	}

	template<>
	inline int distance<rank>(square x, square y) {
		return std::abs(rank_of(x) - rank_of(y));
	}

	template<>
	inline int distance<square>(square x, square y) { //yo specialized template functions are sick
		return square_distance[x][y];
	}

	inline int edge_distance(file f) { return std::min(f, file(FILE_H - f)); }

	//returns attacks assuming empty board for that piece
	template<piece_type pt>
	inline bb attacks_bb(square s) {
		assert((pt != PAWN) && (is_in_bounds(s)));
		return pseudo_attacks[pt][s];
	}
	
	//gen attakcs based on the board passed in, sliding pieces stop at occupied squares

	template<piece_type pt>
	inline bb attacks_bb(square s, bb occupied) {

		assert((pt != PAWN) && (is_in_bounds(s)));

		switch (pt)
		{
		case BISHOP: //say im clever
		case ROOK:
			return magics[s][pt - BISHOP].attacks_bb(occupied);
		case QUEEN:
			return attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied);
		default:
			return pseudo_attacks[pt][s];
		}
	}

	//return attacks given by the piece and bb passed in

	inline bb attacks_bb(piece_type pt, square s, bb occupied) {

		assert((pt != PAWN) && (is_in_bounds(s)));

		switch (pt)
		{
		case BISHOP:
			return attacks_bb<BISHOP>(s, occupied); //use the template
		case ROOK:
			return attacks_bb<ROOK>(s, occupied);
		case QUEEN:
			return attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied);
		default:
			return pseudo_attacks[pt][s];
		}
	}

	inline int pop_count(bb b) {
		return int(_mm_popcnt_u64(b)); //i saw this in stockfish, internal macro for the hardware accelerated pop_count function, very fast
	}

	//return least significant bit
	inline square lsb(bb b) {
		assert(b); //make sure its non-zero

		unsigned long idx;
		_BitScanForward64(&idx, b);
		return square(idx);
	}

	inline square msb(bb b) {
		assert(b);

		unsigned long idx;
		_BitScanReverse64(&idx, b);
		return square(idx);
	}

	inline bb least_signifcant_square_bb(bb b) {
		assert(b);
		return b & -b;
	}

	inline square pop_lsb(bb& b) {
		assert(b); 

		const square s = lsb(b);
		b &= b - 1;
		return s;
	}
};

//from the one at the top
#endif 

