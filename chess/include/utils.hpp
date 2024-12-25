#pragma once

namespace utils {
	inline bool is_black(int piece) {
		return (((piece >> 5) & 1) != 0);
	}

	inline int piece_type(int piece) {
		return ((piece & 0b0011111) - 1);
	}
}