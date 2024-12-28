#pragma once 

#include "def.hpp"
#include <string>
#include <vector>
#include "board.hpp"
#include <iostream>



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
	int map[64];
	int num_pieces; 
	
	piece_list(int mpc = 16) {
		locations = new int[mpc];
		std::fill(map, map + 64, 0);
		num_pieces = 0;
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
		locations[pieceIndex] = locations[num_pieces-1];
		map[locations[pieceIndex]] = pieceIndex; 
		num_pieces--;
	}
	void move_piece(int startSquare, int targetSquare) {
		int pieceIndex = map[startSquare]; 
		locations[pieceIndex] = targetSquare;
		map[targetSquare] = pieceIndex;
	}
	int& operator[](int i) {
		return locations[i];
	}
};