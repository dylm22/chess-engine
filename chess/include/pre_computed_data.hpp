#pragma once 

namespace precomputed_data {

	extern int num_squares_to_edge[64][8];
	extern int dir_offsets[8];

	void precompute_edge_dist();
};