#ifndef  MOVE_GEN_H_INC
#define  MOVE_GEN_H_INC

#include <algorithm>
#include <cstddef>

#include "types.h"


namespace engine {
	class position;

	enum gen_type {
		CAPS,
		QUIETS,
		EVASIONS,
		NON_EVASIONS,
		LEGAL
	};

	struct ext_move : public move { //inherit from move
		int value;

		void operator=(move m) { data = m.raw(); }

		operator float() const = delete; //don't allow implicit conversions to move
	};

	inline bool operator<(const ext_move& f, const ext_move& s) { return f.value < s.value;  }

	template<gen_type>
	ext_move* generate(const position& pos, ext_move* move_list);


	//saw something like this in stockfish
	//wraps the generate() function and gives a list of moves back
	//faster and better in some cases to just call this instead of generate()
	template<gen_type t> 
	struct move_list {
		explicit move_list(const position& pos) : 
			last(generate<t>(pos, _move_list)){}
		const ext_move* begin() const { return _move_list; }
		const ext_move* end() const { return last; }
		size_t size() const { return last - _move_list; }
		bool contains(move _move) const { return std::find(begin(), end(), _move) != end(); }

	private: 
		ext_move _move_list[MAX_MOVES], *last;
	};
}

#endif
