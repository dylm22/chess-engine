#pragma once 

#include "def.hpp"
#include <string>
#include <vector>
#include "board.hpp"

#define NONE 46
#define KING 10
#define PAWN 15
#define KNIGHT 13
#define BISHOP 1
#define ROOK 17
#define QUEEN 16

#define WHITE 65
#define BLACK 97

#define STARTING_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

class game {
public: 
	std::vector<move> moves;
	static board b;

	//std::vector<move> gen_moves();
	int notation_to_number(std::string notation);
	void handle_input();
	void load_position_from_fen(std::string fen);
	void create_board();
	void draw_board();
};


class piece_list {
public: 
	int* locations;
	int* map;
	int num_pieces; 
	
	piece_list(int mpc = 16) {
		locations = new int[mpc];
		map = new int[64];
		num_pieces = 0;
	}
	~piece_list() {
		delete[] locations;
		delete[] map;
	}
	int count() {
		return num_pieces;
	}
	void add_piece_at_square(int square) {
		locations[num_pieces] = square;
		map[square] = num_pieces;
		num_pieces++;
	}
	void remove_piece_at_square(int square) {
		int pieceIndex = map[square];
		locations[pieceIndex] = locations[num_pieces - 1];
		map[locations[pieceIndex]] = pieceIndex; 
		num_pieces--;
	}
	void MovePiece(int startSquare, int targetSquare) {
		int pieceIndex = map[startSquare]; 
		locations[pieceIndex] = targetSquare;
		map[targetSquare] = pieceIndex;
	}
	int& operator[](int i) {
		return locations[i];
	}
};