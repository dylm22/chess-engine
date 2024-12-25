#pragma once

namespace utils {
	inline bool is_black(int piece) {
		return (((piece >> 5) & 1) != 0);
	}
}