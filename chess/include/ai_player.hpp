#pragma once

#include "def.hpp"


class ai_player {
public: 
	move make_move();
	void init(board b, bool color);
	void handle_turn(move* move_ptr);
private:
	static board b;
	bool white;
};