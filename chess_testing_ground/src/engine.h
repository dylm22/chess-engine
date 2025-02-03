#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "position.h"
#include "trans_table.h"

namespace engine {

    class _engine {
    public:
        _engine(std::optional<std::string> path = std::nullopt);
        
        void set_tt_size(size_t mb);
        void trace_eval() const;
        void stop();
        void set_position(const std::string& fen, const std::vector<std::string>& moves);

        int get_hashfull(int maxAge = 0) const;

        std::string fen() const;
        void flip();
        std::string visualize() const;

    private:
        const std::string binary_directory;

        position     pos;
        std::unique_ptr<std::deque<state_info>> states;

        transposition_table tt;

    };

}  


#endif  // #ifndef ENGINE_H_INCLUDED