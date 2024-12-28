
#include "pre_computed_data.hpp"
#include "utils.hpp"

#include <algorithm>
#include <vector>

namespace precomputed_data {
	int num_squares_to_edge[64][8];
	int dir_offsets[8] = { 8,-8,-1,1,7,-7,9,-9 };
	int all_knight_jumps[] = { 15, 17, -17, -15, 10, -6, 6, -10 };
	unsigned char knight_moves[64][8];
	unsigned long knight_attack_bitboards[64];
	unsigned long pawn_attack_bitboards[64][2];
	int dir_lookup[127];

	void precompute_dir_lookup() {
		for (int i = 0; i < 127; i++) {
			int offset = i - 63;
			int absoffset = std::abs(offset);
			int abs_dir = 1;

			if (absoffset % 9 == 0) {
				abs_dir = 9;
			}
			else if (absoffset % 8 == 0) {
				abs_dir = 8;
			}
			else if (absoffset % 7 == 0) {
				abs_dir = 7;
			}

			dir_lookup[i] = abs_dir * utils::sign(offset);
		}
	}

	void precompute_edge_dist(int i) {
			int y = i / 8;
			int x = i - y * 8;
			
			int N = 7 - y;
			int S = y;
			int W = x;
			int E = 7 - x;

			num_squares_to_edge[i][0] = N;
			num_squares_to_edge[i][1] = S;
			num_squares_to_edge[i][2] = W;
			num_squares_to_edge[i][3] = E;
			num_squares_to_edge[i][4] = std::min(N, W);
			num_squares_to_edge[i][5] = std::min(S, E);
			num_squares_to_edge[i][6] = std::min(N, E);
			num_squares_to_edge[i][7] = std::min(S, W);
	}

	void precompute_knight_moves(int i) {
		std::vector<unsigned char> legal_knight_moves;
		unsigned long knight_bitboard = 0;

		int y = i / 8;
		int x = i - y * 8;

		for (int k = 0; k < 8; k++) {
			int knight_target_square = i + all_knight_jumps[k];

			if (knight_target_square >= 0 && knight_target_square < 64) {
				int knight_y = knight_target_square / 8;
				int knight_x = knight_target_square - (knight_y * 8);

				int max_coord_dst = std::max(std::abs(x - knight_x), std::abs(y - knight_y));
				if (max_coord_dst == 2) {
					legal_knight_moves.push_back((unsigned char)knight_target_square);
					knight_bitboard |= 1ul << knight_target_square;
				}
			}
		}
		for (int k = 0; k < 8; k++) {
			if (k < legal_knight_moves.size()) {
				knight_moves[i][k] = legal_knight_moves[k];
			}
			else {
				knight_moves[i][k] = 255;
			}
		}
		knight_attack_bitboards[i] = knight_bitboard;
	}
	void precompute_pawn_moves(int i) {
		int y = i / 8;
		int x = i - y * 8;
		
		std::vector<int> pawn_w;
		std::vector<int> pawn_b;

		if (x > 0) {
			if (y < 7) {
				pawn_w.push_back(i + 7);
				pawn_attack_bitboards[i][0] |= 1ul << (i + 7); //7 is diagonally up to the left for white, 0 is the index of the white color
			}
			if (y > 0) {
				pawn_b.push_back(i - 9);
				pawn_attack_bitboards[i][1] |= 1ul << (i - 9); //-9 is diagonally down to the left for black, 1 is black index
			}
		}
		if (x < 7) {
			if (y < 7) {
				pawn_w.push_back(i + 9);
				pawn_attack_bitboards[i][0] |= 1ul << (i + 9); //same thing other direction, up right this time
			}
			if (y > 0) {
				pawn_b.push_back(i - 7);
				pawn_attack_bitboards[i][1] |= 1ul << (i - 7); 
			}
		}
	}

	void precompute_data() {
		for (int i = 0; i < 64; i++) {
			precompute_edge_dist(i);
			precompute_knight_moves(i);
			precompute_pawn_moves(i);
			precompute_dir_lookup();
		}
	}
}
