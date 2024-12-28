#pragma once 

namespace precomputed_data {

	extern int num_squares_to_edge[64][8];
	extern int dir_offsets[8];
	extern unsigned char knight_moves[64][8];
	extern int all_knight_jumps[];
	extern unsigned long knight_attack_bitboards[64];
	extern unsigned long pawn_attack_bitboards[64][2];
	extern int dir_lookup[127];
	
	void precompute_data();
	void precompute_edge_dist(int i);
	void precompute_knight_moves(int i);

	void precompute_dir_lookup();
	void precompute_pawn_moves(int i);
};