#include "bitboard.h"
#include "utils.h"

#include <bitset>

namespace engine {
	uint8_t pop_cnt16[1 << 16];
	uint8_t square_distance[SQUARE_NB][SQUARE_NB];

	bb between_BB[SQUARE_NB][SQUARE_NB];
	bb line_BB[SQUARE_NB][SQUARE_NB];
	bb pseudo_attacks[PIECE_TYPE_NB][SQUARE_NB];
	bb pawn_attacks[COLOR_NB][SQUARE_NB];

	magic magics[SQUARE_NB][2];

	namespace { //anonymous namespace to make these only visible in this file, like static but fancy
		bb rook_table[0x19000]; //weird constants are max number of possible occupancy configurations
		bb bishop_table[0x1480];

		void init_magics(piece_type pt, bb table[], magic magics[][2]);

		bb is_safe_dest(square s, int step) { //really should be is_legal_dest but I used a diff name when i wrote it and its not changing
			square to = square(s + step);
			return is_in_bounds(to) && distance(s, to) <= 2 ? square_bb(to) : bb(0);
		}
	}

	std::string bit_board::display(bb b) { //stole this straight from stockfish

		std::string s = "+---+---+---+---+---+---+---+---+\n";

		for (rank r = RANK_8; r >= RANK_1; --r) //had to write a custom operator overload so it stays like this
		{
			for (file f = FILE_A; f <= FILE_H; ++f)
				s += b & make_square(f, r) ? "| X " : "|   ";

			s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
		}
		s += "  a   b   c   d   e   f   g   h\n";

		return s;
	}

	void bit_board::init() {
		for (unsigned i = 0; i < (1 << 16); i++) {
			pop_cnt16[i] = uint8_t(std::bitset<16>(i).count()); //honestly legacy, probably could delete
		}

		for (square s1 = SQ_A1; s1 <= SQ_H8; ++s1) {
			for (square s2 = SQ_A1; s2 <= SQ_H8; ++s2) {
				square_distance[s1][s2] = std::max(distance<file>(s1, s2), distance<rank>(s1, s2));
			}
		}

		init_magics(ROOK, rook_table, magics);
		init_magics(BISHOP, bishop_table, magics);

		for (square s1 = SQ_A1; s1 <= SQ_H8; ++s1) {
			pawn_attacks[WHITE][s1] = pawn_attacks_bb<WHITE>(square_bb(s1));
			pawn_attacks[BLACK][s1] = pawn_attacks_bb<BLACK>(square_bb(s1));

			for (int step : {-9, -8, -7, -1, 1, 7, 8, 9}) { //offsets for king moves
				pseudo_attacks[KING][s1] |= is_safe_dest(s1, step);
			}

			for (int step : {-17, -15, -10, -6, 6, 10, 15, 17}) { //offsets for knight moves
				pseudo_attacks[KNIGHT][s1] |= is_safe_dest(s1, step);
			}

			pseudo_attacks[QUEEN][s1] = pseudo_attacks[BISHOP][s1] = attacks_bb<BISHOP>(s1, 0);
			pseudo_attacks[QUEEN][s1] |= pseudo_attacks[ROOK][s1] = attacks_bb<ROOK>(s1, 0);

			for (piece_type pt : {BISHOP, ROOK}) {
				for (square s2 = SQ_A1; s2 <= SQ_H8; ++s2) {
					if (pseudo_attacks[pt][s1] & s2) {
						line_BB[s1][s2] = (attacks_bb(pt, s1, 0) & attacks_bb(pt, s2, 0)) | s1 | s2;
						between_BB[s1][s2] = (attacks_bb(pt, s1, square_bb(s2)) & attacks_bb(pt, s2, square_bb(s1)));
					}
					between_BB[s1][s2] |= s2; //populate the line and between bb's using the already calculated magics
				}
			}
		}
	}

	namespace { //another anonymous namespace, need it for the definition of init_magics
		bb sliding_attack(piece_type pt, square s, bb occupied) {
			bb attacks = 0;
			direction rook_dir[4] = { NORTH,SOUTH,EAST,WEST };
			direction bishop_dir[4] = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };

			for (direction d : (pt == ROOK ? rook_dir : bishop_dir)) {
				square _s = s; 
				while (is_safe_dest(_s, d)) {
					attacks |= (_s += d); //add every step to the attacks bb
					if (occupied & _s) {
						break; //blocker on the square, ignore it and dont go further
					}
				}
			}
			return attacks;
		}

		//https://www.chessprogramming.org/Magic_Bitboards fancy implementation 

		void init_magics(piece_type pt, bb table[], magic magics[][2]) {

			// optimal PRNG seeds to pick the correct magics in the shortest time
			// straight from stockfish
			int seeds[][RANK_NB] = { {8977, 44560, 54343, 38998, 5731, 95205, 104912, 17020},
									{728, 10316, 55013, 32803, 12281, 15100, 16645, 255} };

			bb occupancy[4096];
			int epoch[4096] = {}, cnt = 0;

			bb reference[4096];
			int size = 0;

			for (square s = SQ_A1; s <= SQ_H8; ++s)
			{
				// board edges are not considered in the relevant occupancies
				bb edges = ((RANK1BB | RANK8BB) & ~rank_bb(s)) | ((FILEABB | FILEHBB) & ~file_bb(s));

				//rook is 4 bishop is 3, in magics bishop is index 0, rook is 1
				magic& m = magics[s][pt - BISHOP]; 
				m.mask = sliding_attack(pt, s, 0) & ~edges;

				m.shift = 64 - pop_count(m.mask);

				//set offset of attacks table 
				m.attacks = s == SQ_A1 ? table : magics[s - 1][pt - BISHOP].attacks + size;
				size = 0;

				// carry-rippler trick to enumerate all subsets of masks[s] https://www.chessprogramming.org/Traversing_Subsets_of_a_Set
				// store the corresponding sliding attack bitboard in reference[].
				bb b = 0;
				do
				{
					occupancy[size] = b;

					reference[size] = sliding_attack(pt, s, b);

					size++;
					b = (b - m.mask) & m.mask;
				} while (b);

				PRNG rng(seeds[1][rank_of(s)]);

				// find magic for square s
				for (int i = 0; i < size;)
				{
					for (m.magic = 0; pop_count((m.magic * m.mask) >> 56) < 6;)
						m.magic = rng.sparse_rand<bb>();


					//verify magics and at the same time fill out a database for square 's'	
					for (++cnt, i = 0; i < size; ++i)
					{
						unsigned idx = m.index(occupancy[i]);

						if (epoch[idx] < cnt)
						{
							epoch[idx] = cnt;
							m.attacks[idx] = reference[i];
						}
						else if (m.attacks[idx] != reference[i])
							break;
					}
				}
			}
		}
	}
}
