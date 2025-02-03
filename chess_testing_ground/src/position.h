#ifndef POSITION_H_INC
#define POSITION_H_INC

#include <cassert>
#include <deque>

#include "bitboard.h"
#include "types.h"

namespace engine {

	class transposition_table;

	struct state_info {
		//copied
		uint64_t material_key;
		uint64_t pawn_key;
		uint64_t major_piece_key;
		uint64_t minor_piece_key;
		uint64_t non_pawn_key[COLOR_NB];
		int non_pawn_material[COLOR_NB];
		int castling_rights;
		int move_rule_50; 
		int plys_from_null;
		square ep_s; //en passant square

		//recomputed 
		uint64_t key;
		bb checkers_bb;
		state_info* prev;
		state_info* next;
		bb blockers_for_king[COLOR_NB];
		bb pinners[COLOR_NB];
		bb check_squares[PIECE_TYPE_NB];
		piece captured_piece;
		int repititon; 
	};

	class position {
	public:
		static void init();

		//constructors
		position() = default;
		position(const position&) = delete;
		position& operator=(const position&) = delete; //stops positions from being able to be copied or assinged to another instance of a position, technically unecessary but saved me

		//fen stuff
		position& set(const std::string& fen_str, state_info* si);	//modify in place instead of creating a copy, that would be a waste of resources
		position& set(const std::string& code, color c, state_info* si);
		std::string fen() const; 

		//position stuff
		bb pieces(piece_type pt = ALL_PIECES) const; 
		template<typename... piece_types> //variadic template to take multiple pieces in
		bb pieces(piece_type pt, piece_types... pts) const;
		bb pieces(color c) const;
		template<typename... piece_types>
		bb pieces(color c, piece_types... pts) const;
		piece piece_on(square s) const;
		square ep_square() const;
		bool empty(square s) const;
		template<piece_type pt>
		int count(color c) const;
		template<piece_type pt>
		int count() const;
		template<piece_type pt>
		square _square(color c) const;
		
		//castling stuff
		castling_rights _castling_rights(color c) const;
		bool can_castle(castling_rights cr) const;
		bool castling_impeded(castling_rights cr) const;
		square castling_rook_square(castling_rights cr) const;

		//check
		bb checkers() const;
		bb blockers_for_king(color c) const;
		bb check_squares(piece_type pt) const;
		bb pinners(color c) const;

		//attacks to/from 
		bb attackers_to(square s) const;
		bb attackers_to(square s, bb occupied) const;
		bool attackers_to_exist(square s, bb occupied, color c) const;
		void update_slider_blockers(color c) const;
		template<piece_type pt>
		bb attacks_by(color c) const;

		//properties
		bool legal(move m) const;
		bool psuedo_legal(const move m) const; //can mark move as const because its not modified in the function
		bool capture(move m) const;
		bool capture_stage(move m) const;
		bool gives_check(move m) const;
		piece moved_piece(move m) const;
		piece captured_piece() const;

		//do and undo, necessary in search
		void do_move(move m, state_info& new_st, const transposition_table* tt);
		void do_move(move m, state_info& new_st, bool gives_check, const transposition_table* tt);
		void undo_move(move m);
		void do_null_move(state_info& new_st, const transposition_table& tt); //null meaning move where player does nothing, like passing a turn. Also pass tt in by reference because readonly
		void undo_null_move();

		//static exchange eval -> evaluate an material in an exchange without actually making the moves 
		bool see_ge(move m, int threshold = 0) const;

		//read hash keys
		uint64_t r_key() const;
		uint64_t r_material_key() const;
		uint64_t r_pawn_key() const;
		uint64_t r_major_piece_key() const;
		uint64_t r_minor_piece_key() const;
		uint64_t r_non_pawn_key(color c) const;

		//others
		color side_to_move() const;
		int game_ply() const;
		bool is_draw(int ply) const;
		bool is_repetition(int ply) const;
		bool upcoming_repition(int ply) const;
		bool has_repeated() const;
		int move_rule_50_count() const;
		int non_pawn_material(color c) const;
		int non_pawn_material() const;

		//debug
		bool pos_isnt_bad() const;
		void flip();

		//board stuff
		void put_piece(piece p, square s);
		void remove_piece(square s);

	private:
		//init helpers
		void set_castling_rights(color c, square rfrom);
		void set_state() const; 
		void set_check() const;

		//helpers
		void move_piece(square from, square to);
		template<bool _do>
		void do_castling(color c, square from, square& to, square& rfrom, square& rto); //pass in by refernce to move them
		template<bool post_move>
		uint64_t adjust_key50(uint64_t k) const;

		//data 
		piece board[SQUARE_NB];
		bb type_bb[PIECE_TYPE_NB];
		bb color_bb[COLOR_NB];
		int piece_count[PIECE_NB];
		int castling_rights_mask[SQUARE_NB];
		square _castling_rook_square[CASTLING_RIGHT_NB];
		bb castling_path[CASTLING_RIGHT_NB];
		state_info* st;
		int _game_ply;
		color _side_to_move;
	}; 

	std::ostream& operator<<(std::ostream& os, const position& pos);
	
	inline color position::side_to_move() const { return _side_to_move; }
	inline piece position::piece_on(square s) const {
		assert(is_in_bounds(s));
		return board[s];
	}
	inline bool position::empty(square s) const { return piece_on(s) == NO_PIECE; }
	inline piece position::moved_piece(move m) const { return piece_on(m.from_sq()); }
	inline bb position::pieces(piece_type pt) const { return type_bb[pt]; }
	template<typename... piece_types>
	inline bb position::pieces(piece_type pt, piece_types... pts) const {
		return pieces(pt) | pieces(pts...);
	}
	inline bb position::pieces(color c) const { return color_bb[c]; }
	template<typename... piece_types>
	inline bb position::pieces(color c, piece_types... pts) const {
		return pieces(c) & pieces(pts...);
	}
	template<piece_type pt>
	inline int position::count(color c) const {
		return piece_count[make_piece(c, pt)];
	}
	template<piece_type pt>
	inline int position::count() const {
		return count<pt>(WHITE) + count<pt>(BLACK);
	}
	template<piece_type pt>
	inline square position::_square(color c) const {
		assert(count<pt>(c) == 1); //if theres more than one piece theres ambiguity on which one to take the position of
		return lsb(pieces(c, pt));
	}
	inline square position::ep_square() const { return st->ep_s; }
	inline bool position::can_castle(castling_rights cr) const { return st->castling_rights & cr; }
	inline castling_rights position::_castling_rights(color c) const { return c & castling_rights(st->castling_rights); }
	inline bool position::castling_impeded(castling_rights cr) const {
		assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO); // throw error if not one of these
		return pieces() & castling_path[cr];
	}
	inline square position::castling_rook_square(castling_rights cr) const {
		assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);
		return _castling_rook_square[cr];
	}
	inline bb position::attackers_to(square s) const { return attackers_to(s, pieces()); }
	template<piece_type pt>
	inline bb position::attacks_by(color c) const {
		if constexpr (pt == PAWN)
			return c == WHITE ? pawn_attacks_bb<WHITE>(pieces(WHITE, PAWN)) : pawn_attacks_bb<BLACK>(pieces(BLACK, PAWN));
		else {
			bb threats = 0;
			bb attackers = pieces(c, pt);
			while (attackers)
				threats |= attacks_bb<pt>(pop_lsb(attackers), pieces());
			return threats;
		}
	}
	inline bb position::checkers() const { return st->checkers_bb; }
	inline bb position::blockers_for_king(color c) const { return st->blockers_for_king[c]; }
	inline bb position::pinners(color c) const { return st->pinners[c]; }
	inline bb position::check_squares(piece_type pt) const { return st->check_squares[pt]; }

	inline uint64_t position::r_key() const { return adjust_key50<false>(st->key); }

	template<bool post_move>
	inline uint64_t position::adjust_key50(uint64_t k) const {
		return st->move_rule_50 < 14 - post_move ? k : k ^ make_key((st->move_rule_50 - (14 - post_move)) / 8); //zobrist
	}

	inline uint64_t position::r_pawn_key() const { return st->pawn_key; }
	inline uint64_t position::r_material_key() const { return st->material_key; }
	inline uint64_t position::r_major_piece_key() const { return st->major_piece_key; }
	inline uint64_t position::r_minor_piece_key() const { return st->minor_piece_key; }
	inline uint64_t position::r_non_pawn_key(color c) const { return st->non_pawn_key[c]; }
	inline int position::non_pawn_material(color c) const { return st->non_pawn_material[c]; }
	inline int position::non_pawn_material() const {
		return non_pawn_material(WHITE) + non_pawn_material(BLACK);
	}
	inline int position::game_ply() const { return _game_ply; }
	inline int position::move_rule_50_count() const { return st->move_rule_50; }
	inline bool position::capture(move m) const {
		assert(m.is_ok()); //debug
		return (!empty(m.to_sq()) && m.type_of() != CASTLING) || m.type_of() == EN_PASSANT;
	}
	//returns true if move is generated from capture stage, including queen promos. need consistency to avoid duplicate move generation, queen promos and captures are usually high impact
	inline bool position::capture_stage(move m) const {
		assert(m.is_ok());
		return capture(m) || m.promotion_type() == QUEEN;
	}
	inline piece position::captured_piece() const { return st->captured_piece; }
	inline void position::put_piece(piece p, square s) {
		board[s] = p;
		type_bb[ALL_PIECES] |= type_bb[type_of(p)] |= s;
		color_bb[color_of(p)] |= s;
		piece_count[p]++;
		piece_count[make_piece(color_of(p), ALL_PIECES)]++;
	}
	inline void position::remove_piece(square s) {

		piece p = board[s];
		type_bb[ALL_PIECES] ^= s; //not exponentiation, but xor operator, i didn't know this for a while
		type_bb[type_of(p)] ^= s;
		color_bb[color_of(p)] ^= s;
		board[s] = NO_PIECE;
		piece_count[p]--;
		piece_count[make_piece(color_of(p), ALL_PIECES)]--;
	}
	inline void position::move_piece(square from, square to) {

		piece p = board[from];
		bb fromTo = from | to;
		type_bb[ALL_PIECES] ^= fromTo;
		type_bb[type_of(p)] ^= fromTo;
		color_bb[color_of(p)] ^= fromTo;
		board[from] = NO_PIECE;
		board[to] = p;
	}
	inline void position::do_move(move m, state_info& new_st, const transposition_table* tt = nullptr) {
		do_move(m, new_st, gives_check(m), tt);
	}
}
#endif