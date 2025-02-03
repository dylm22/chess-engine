#ifndef UCI_H_INC
#define UCI_H_INC

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include "types.h"
#include "utils.h"
#include "engine.h"

//uci or universal chess interface, this is basically a bunch of helper functions to comply with these standards 


namespace engine {
	class uci_engine {
	public:
		uci_engine(int argc, char** argv);
		static std::string n_square(square s);
		static std::string to_lower(std::string str);
		static std::string n_move(move m);
		static move to_move(const position& _pos, std::string str);

		void pos(std::istringstream& is);
		void loop();
	private:
		command_line cli;
		_engine e;
	};
}

#endif 
