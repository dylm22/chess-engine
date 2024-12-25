#pragma once 

namespace precomputed_data {

	static int* num_squares_to_edge[64];
	const int dir_offsets[8] = { 8,-8,1,-1,7,-7,9,-9 };

	inline void precompute_edge_dist() {
		for (int x = 0; x < 8; x++) {
			for (int y = 0; y < 8; y++) {
				int N = 7 - y;
				int S = y;
				int W = x;
				int E = 7 - x;

				int i = y * 8 + x;

				num_squares_to_edge[i] = new int[8] { N, S, W, E, std::min(N, W), std::min(S, E), std::min(N, E), std::min(S, W) };
			}
		}
	}
};