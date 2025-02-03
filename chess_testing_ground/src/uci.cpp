#include "uci.h"

#include <sstream>
#include <string>

#include "types.h"
#include "position.h"
#include "move_gen.h"

namespace engine {
    constexpr auto start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    uci_engine::uci_engine(int argc, char** argv) : e(argv[0]), cli(argc, argv) {
        
    }
    std::string uci_engine::n_square(square s) {
        return std::string{ char('a' + file_of(s)), char('1' + rank_of(s)) };
    }

    void uci_engine::loop() {
        std::string token, cmd;

        for (int i = 1; i < cli.argc; i++) {
            cmd += std::string(cli.argv[i]) + " ";
        }
        
        do {
            if (cli.argc == 1 && !getline(std::cin, cmd)) 
                cmd = "quit";

            std::istringstream is(cmd);
            
            token.clear();
            is >> std::skipws >> token;

            if (token == "position") {
                pos(is);
                std::cout << e.visualize();
            }

            
        } while (token != "quit" && cli.argc == 1);
    }

    void uci_engine::pos(std::istringstream& is) {
        std::string token, fen;

        is >> token;

        if (token == "startpos")
        {
            fen = start_fen;
            is >> token;  
        }
        else if (token == "fen")
            while (is >> token && token != "moves")
                fen += token + " ";
        else
            return;

        std::vector<std::string> moves;

        while (is >> token){
            moves.push_back(token);
        }

        e.set_position(fen, moves);
    }

    move uci_engine::to_move(const position& _pos, std::string str) {
        str = to_lower(str);

        for (const auto& m : move_list<LEGAL>(_pos))
            if (str == n_move(m))
                return m;

        return move::none();
    }

    std::string uci_engine::to_lower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](auto c) { return std::tolower(c); });

        return str;
    }

    std::string uci_engine::n_move(move m) {
        if (m == move::none())
            return "(none)";

        if (m == move::null())
            return "0000";

        square from = m.from_sq();
        square to = m.to_sq();

        if (m.type_of() == CASTLING)
            to = make_square(to > from ? FILE_G : FILE_C, rank_of(from));

        std::string move = n_square(from) + n_square(to);

        if (m.type_of() == PROMOTION)
            move += " pnbrqk"[m.promotion_type()];

        return move;
    }
}

