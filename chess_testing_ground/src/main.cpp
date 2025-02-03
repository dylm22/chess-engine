#include <iostream>

#include "bitboard.h"
#include "utils.h"
#include "position.h"
#include "types.h"
#include "uci.h"

using namespace engine;

int main(int argc, char* argv[]) {
	
	std::cout << "." << std::endl;

	bit_board::init();
	std::cout << "bb initialized" << std::endl;
	position::init();
	std::cout << "position initialized" << std::endl;

	uci_engine uci(argc, argv);
	std::cout << "uci initialized" << std::endl;
	uci.loop();

	return 0;
}