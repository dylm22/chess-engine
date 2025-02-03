#include "engine.h"

#include <cassert>
#include <sstream>

#include "utils.h"
#include "position.h"
#include "trans_table.h"
#include "uci.h"


constexpr auto start_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int  MaxHashMB = 33554432;

namespace engine{

    _engine::_engine(std::optional<std::string> path) :
        binary_directory(path ? command_line::get_binary_directory(*path) : ""),
        states(new std::deque<state_info>(1)) {
        pos.set(start_FEN, &states->back());

        set_tt_size(16);
    }

    void _engine::set_tt_size(size_t mb) {
        //wait_for_search_finished();
        tt.resize(mb);
    }

    std::string _engine::visualize() const {
        std::stringstream ss;
        ss << pos;
        return ss.str();
    }

    void _engine::set_position(const std::string& fen, const std::vector<std::string>& moves) {
        // Drop the old state and create a new one
        states = std::unique_ptr<std::deque<state_info>>(new std::deque<state_info>(1));
        pos.set(fen, &states->back());

        for (const auto& move : moves)
        {
            auto m = uci_engine::to_move(pos, move);

            if (m == move::none())
                break;

            states->emplace_back();
            pos.do_move(m, states->back());
        }
    }
 }