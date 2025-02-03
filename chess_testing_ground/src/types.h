#ifndef TYPES_H_INC
#define TYPES_H_INC

#include <cstdint>
#include <cassert>

#pragma warning(disable: 4146) //unary minus sign applied to unsigned types, non-issue here but compiler complains

using bb = uint64_t;

constexpr int MAX_MOVES = 256;
constexpr int MAX_PLY = 246;

constexpr int VALUE_ZERO = 0;
constexpr int VALUE_DRAW = 0;
constexpr int VALUE_NONE = 32002;
constexpr int VALUE_INFINITE = 32001;
          
constexpr int VALUE_MATE = 32000;
constexpr int VALUE_MATE_IN_MAX_PLY = VALUE_MATE - MAX_PLY;
constexpr int VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY;
          
constexpr int VALUE_TB = VALUE_MATE_IN_MAX_PLY - 1;
constexpr int VALUE_TB_WIN_IN_MAX_PLY = VALUE_TB - MAX_PLY;
constexpr int VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY;

constexpr int pawn_value = 208;
constexpr int knight_value = 781;
constexpr int bishop_value = 825;
constexpr int rook_value = 1276;
constexpr int queen_value = 2538;

enum piece_type {
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    ALL_PIECES = 0,
    PIECE_TYPE_NB = 8
};

enum piece {
    NO_PIECE,
    W_PAWN = PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

constexpr int piece_value[PIECE_NB] = {
  VALUE_ZERO, pawn_value, knight_value, bishop_value, rook_value, queen_value, VALUE_ZERO, VALUE_ZERO,
  VALUE_ZERO, pawn_value, knight_value, bishop_value, rook_value, queen_value, VALUE_ZERO, VALUE_ZERO };

enum : int {
    DEPTH_QS = 0, DEPTH_UNSEARCHED = -2, DEPTH_ENTRY_OFFSET = -3
};

enum color {
	WHITE, BLACK, COLOR_NB = 2
};

enum square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE,

    SQUARE_ZERO = 0,
    SQUARE_NB = 64
}; //shoutout to chatgpt for this one

enum file : int {
    FILE_A,
    FILE_B,
    FILE_C,
    FILE_D,
    FILE_E,
    FILE_F,
    FILE_G,
    FILE_H,
    FILE_NB
};

enum rank : int {
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8,
    RANK_NB
};

enum direction : int {
    NORTH = 8,
    EAST = 1,
    SOUTH = -NORTH,
    WEST = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

enum castling_rights {
    NO_CASTLING,
    WHITE_OO,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,

    KING_SIDE = WHITE_OO | BLACK_OO,
    QUEEN_SIDE = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING,

    CASTLING_RIGHT_NB = 16
};

enum bound {
    BOUND_NONE,
    BOUND_UPPER,
    BOUND_LOWER,
    BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};
enum move_type {
    NORMAL,
    PROMOTION = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING = 3 << 14
};
// saw sebastian lague do this
// bit  0 - 5: destination square(from 0 to 63)
// bit  6-11: origin square (from 0 to 63)
// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)

//from MMIX by Donald Knuth psuedo-random number generator
constexpr uint64_t make_key(uint64_t seed) {
    return seed * 6364136223846793005ULL + 1442695040888963407ULL;
}

class move {
public:
    move() = default;
    constexpr explicit move(std::uint16_t d) : //dont allow implicit conversions cause things could get bad
        data(d) {}

    constexpr move(square from, square to) :
        data((from << 6) + to) {}

    template<move_type T>
    static constexpr move make(square from, square to, piece_type pt = KNIGHT) {
        return move(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
    }

    constexpr square from_sq() const {
        assert(is_ok());
        return square((data >> 6) & 0x3F);
    }

    constexpr square to_sq() const {
        assert(is_ok());
        return square(data & 0x3F);
    }

    constexpr int from_to() const { return data & 0xFFF; }

    constexpr piece make_piece(color c, piece_type pt) { return piece((c << 3) + pt); }
    constexpr move_type type_of() const { return move_type(data & (3 << 14)); }

    constexpr piece_type promotion_type() const { return piece_type(((data >> 12) & 3) + KNIGHT); }

    constexpr bool is_ok() const { return none().data != data && null().data != data; }

    static constexpr move null() { return move(65); }
    static constexpr move none() { return move(0); }

    constexpr bool operator==(const move& m) const { return data == m.data; }
    constexpr bool operator!=(const move& m) const { return data != m.data; }

    constexpr explicit operator bool() const { return data != 0; }

    constexpr std::uint16_t raw() const { return data; }

    struct move_hash {
        size_t operator()(const move& m) const { return make_key(m.data); }
    };

protected:
    std::uint16_t data;
};

constexpr castling_rights operator&(color c, castling_rights cr) {
    return castling_rights((c == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr);
}

inline color color_of(piece p) {
    assert(p != NO_PIECE);
    return color(p >> 3);
}

constexpr color operator~(color c) { return color(c ^ BLACK); }

inline piece_type& operator++(piece_type& d) { return d = piece_type(int(d) + 1); }
inline piece_type& operator--(piece_type& d) { return d = piece_type(int(d) - 1); }

constexpr direction operator+(direction d1, direction d2) { return direction(int(d1) + int(d2)); }
constexpr direction operator*(int i, direction d) { return direction(i * int(d)); }

inline square& operator++(square& d) { return d = square(int(d) + 1); }
inline square& operator--(square& d) { return d = square(int(d) - 1); }

constexpr square operator+(square s, direction d) { return square(int(s) + int(d)); }
constexpr square operator-(square s, direction d) { return square(int(s) - int(d)); }
inline square& operator+=(square& s, direction d) { return s = s + d; }
inline square& operator-=(square& s, direction d) { return s = s + d; }

inline file& operator++(file& d) { return d = file(int(d) + 1); } 
inline file& operator--(file& d) { return d = file(int(d) - 1); }

inline rank& operator++(rank& d) { return d = rank(int(d) + 1); }
inline rank& operator--(rank& d) { return d = rank(int(d) - 1); }

constexpr file file_of(square s) { return file(static_cast<int>(s) & 7); } //might be a problem
constexpr rank rank_of(square s) { return rank(s >> 3); }
constexpr piece_type type_of(piece pc) { return piece_type(pc & 7); }
constexpr piece make_piece(color c, piece_type pt) { return piece((c << 3) + pt); }
constexpr bool is_in_bounds(square s) { return s >= SQ_A1 && s <= SQ_H8; }
constexpr square make_square(file f, rank r) { return square((r << 3) + f); }
constexpr direction pawn_push(color c) { return c == WHITE ? NORTH : SOUTH; }
constexpr rank relative_rank(color c, rank r) { return rank(r ^ (c * 7)); }
constexpr rank relative_rank(color c, square s) { return relative_rank(c, rank_of(s)); }
constexpr square relative_square(color c, square s) { return square(static_cast<int>(s) ^ (c * 56)); }

#endif