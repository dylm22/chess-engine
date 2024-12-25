
#include "pre_computed_data.hpp"
#include <algorithm>


namespace precomputed_data {
	int num_squares_to_edge[64][8];
	int dir_offsets[8] = { 8,-8,-1,1,7,-7,9,-9 };

	void precompute_edge_dist() {
		for (int i = 0; i < 64; i++) {
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
	}
}
