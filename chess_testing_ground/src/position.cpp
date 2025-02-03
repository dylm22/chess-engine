#include "position.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <iostream>
#include <sstream>
#include <utility>

#include "uci.h"
#include "bitboard.h"
#include "utils.h"
#include "move_gen.h"
#include "trans_table.h"

using std::string;

namespace engine {
	namespace zobrist {
		uint64_t psq[PIECE_NB][SQUARE_NB];
		uint64_t en_passant[FILE_NB];
		uint64_t castling[CASTLING_RIGHT_NB];
		uint64_t side, no_pawns;
	}

	namespace {
		constexpr std::string_view piece_to_char(" PNBRQK  pnbrqk");
		constexpr piece _pieces[] = { W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
									 B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING };
	}

	std::ostream& operator<<(std::ostream& os, const position& pos) {
		for (rank r = RANK_8; r >= RANK_1; --r) {
			for (file f = FILE_A; f <= FILE_H; ++f)
				os << ' ' << piece_to_char[pos.piece_on(make_square(f, r))];
			os << ' ' << (1 + r) << std::endl;
		}

		return os;
	}

	//http://web.archive.org/web/20201107002606/https://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf
	//Cuckoo algorithm for repition of positions

	//two hash functions for indexing into the table
	inline int h1(uint64_t h) { return h & 0x1fff; }
	inline int h2(uint64_t h) { return (h >> 16) & 0x1fff; }

	//cuckoo tables with zobrist hashes of valid reversible moves and the actual moves
	std::array<uint64_t, 8192> cuckoo;
	std::array<move, 8192> cuckoo_move;

	void position::init() {
		PRNG rng(1070372); //it has to be this way

		for (piece p : _pieces) {
			for (square s = SQ_A1; s <= SQ_H8; ++s) {
				zobrist::psq[p][s] = rng.rand<uint64_t>();
			}
		}

		for (file f = FILE_A; f <= FILE_H; ++f) {
			zobrist::en_passant[f] = rng.rand<uint64_t>();
		}

		for (int cr = NO_CASTLING; cr <= ANY_CASTLING; ++cr) {
			zobrist::castling[cr] = rng.rand<uint64_t>();
		}

		zobrist::side = rng.rand<uint64_t>();
		zobrist::no_pawns = rng.rand<uint64_t>();

		cuckoo.fill(0);
		cuckoo_move.fill(move::none());
		[[maybe_unused]] int count = 0;

		for (piece p : _pieces) {
			for (square s1 = SQ_A1; s1 <= SQ_H8; ++s1) {
				for (square s2 = square(s1 + 1); s2 <= SQ_H8; ++s2) {
					if ((type_of(p) != PAWN) && (attacks_bb(type_of(p), s1, 0) & s2)) {
						move _move = move(s1, s2);
						uint64_t key = zobrist::psq[p][s1] ^ zobrist::psq[p][s2] ^ zobrist::side;
						int i = h1(key);
						while (true) {
							std::swap(cuckoo[i], key);
							std::swap(cuckoo_move[i], _move);
							if (_move == move::none()) //is empty slot
								break;
							i = (i == h1(key)) ? h2(key) : h1(key); // "push victim to alternative slot"
						}
						count++;
					}
				}
			}
		}
		assert(count == 3668);
	}

	//init pos with fen string
	position& position::set(const string& fen_str, state_info* si) {
		unsigned char col, row, token;
		size_t idx;
		square s = SQ_A8;
		std::istringstream ss(fen_str);

		std::memset(this, 0, sizeof(position));
		std::memset(si, 0, sizeof(state_info));
		st = si;

		ss >> std::noskipws; //dont skip whitespace, its important

		while ((ss >> token) && !isspace(token)) {
			if (isdigit(token))
				s += (token - '0') * EAST; //step a file
			else if (token == '/') // step a rank
				s += 2 * SOUTH;
			else if ((idx = piece_to_char.find(token)) != string::npos) { //if its a piece
				put_piece(piece(idx), s);
				++s;
			}
		}

		ss >> token;
		_side_to_move = (token == 'w' ? WHITE : BLACK);
		ss >> token;

		while ((ss >> token) && !isspace(token)) {
			square rsq;
			color c = islower(token) ? BLACK : WHITE;
			piece rook = make_piece(c, ROOK);

			token = char(toupper(token));

			if (token == 'K')
				for (rsq = relative_square(c, SQ_H1); piece_on(rsq) != rook; --rsq) {} //do not write code like this 
			else if (token == 'Q')
				for (rsq = relative_square(c, SQ_A1); piece_on(rsq) != rook; ++rsq) {}
			else if (token >= 'A' && token <= 'H')
				rsq = make_square(file(token - 'A'), relative_rank(c, RANK_1));
			else
				continue; 

			set_castling_rights(c, rsq);
		}

		bool en_passant = false;

		if (((ss >> col) && (col >= 'a' && col <= 'h')) && ((ss >> row) && (row == (_side_to_move == WHITE ? '6' : '3')))) {
			st->ep_s = make_square(file(col - 'a'), rank(row - '1'));

			en_passant = pawn_attacks_bb(~_side_to_move, st->ep_s) & pieces(_side_to_move, PAWN) && (pieces(~_side_to_move, PAWN) & (st->ep_s + pawn_push(~_side_to_move))) && !(pieces() & (st->ep_s | (st->ep_s + pawn_push(_side_to_move))));
		}

		if (!en_passant)
			st->ep_s = SQ_NONE;

		ss >> std::skipws >> st->move_rule_50 >> _game_ply;

		_game_ply = std::max(2 * (_game_ply - 1), 0) + (_side_to_move == BLACK);

		set_state();

		assert(pos_isnt_bad());

		return *this;
	}

	void position::set_castling_rights(color c, square rfrom) {
		square kfrom = _square<KING>(c);
		castling_rights cr = c & (kfrom < rfrom ? KING_SIDE : QUEEN_SIDE);

		st->castling_rights |= cr;
		castling_rights_mask[kfrom] |= cr;
		castling_rights_mask[rfrom] |= cr;
		_castling_rook_square[cr] = rfrom;

		square kto = relative_square(c, cr & KING_SIDE ? SQ_G1 : SQ_C1);
		square rto = relative_square(c, cr & KING_SIDE ? SQ_F1 : SQ_D1);

		castling_path[cr] = (between_bb(rfrom, rto) | between_bb(rfrom, kto)) & ~(kfrom | rfrom);
	}
	void position::set_check() const {
		update_slider_blockers(WHITE);
		update_slider_blockers(BLACK);

		square ks = _square<KING>(~_side_to_move);

		st->check_squares[PAWN] = pawn_attacks_bb(~_side_to_move, ks);
		st->check_squares[KNIGHT] = attacks_bb<KNIGHT>(ks);
		st->check_squares[BISHOP] = attacks_bb<BISHOP>(ks, pieces());
		st->check_squares[ROOK] = attacks_bb<ROOK>(ks, pieces());
		st->check_squares[QUEEN] = st->check_squares[BISHOP] | st->check_squares[ROOK];
		st->check_squares[KING] = 0;
	}
	//compute hash keys of pos and other data that is incremented as moves are made
	void position::set_state() const {
		st->key = st->material_key = 0;
		st->major_piece_key = st->minor_piece_key = 0;
		st->non_pawn_key[WHITE] = st->non_pawn_key[BLACK] = 0;
		st->pawn_key = zobrist::no_pawns;
		st->non_pawn_material[WHITE] = st->non_pawn_material[BLACK] = VALUE_ZERO;
		st->checkers_bb = attackers_to(_square<KING>(_side_to_move)) & pieces(~_side_to_move);

		set_check();

		for (bb b = pieces(); b;) { //don't need any increment statement because pop_lsb
			square s = pop_lsb(b);
			piece p = piece_on(s);
			st->key ^= zobrist::psq[p][s];

			if (type_of(p) == PAWN)
				st->pawn_key ^= zobrist::psq[p][s];
			else {
				st->non_pawn_key[color_of(p)] ^= zobrist::psq[p][s];

				if (type_of(p) != KING) {
					st->non_pawn_material[color_of(p)] += piece_value[p];

					if (type_of(p) >= ROOK)
						st->major_piece_key ^= zobrist::psq[p][s];
					else
						st->minor_piece_key ^= zobrist::psq[p][s];
				}

				else {
					st->major_piece_key ^= zobrist::psq[p][s];
					st->minor_piece_key ^= zobrist::psq[p][s];
				}
			}
		}

		if (st->ep_s != SQ_NONE)
			st->key ^= zobrist::en_passant[file_of(st->ep_s)];
		
		if (_side_to_move == BLACK)
			st->key ^= zobrist::side;

		st->key ^= zobrist::castling[st->castling_rights];

		for(piece p : _pieces)
			for (int cnt = 0; cnt < piece_count[p]; cnt++) {
				st->material_key ^= zobrist::psq[p][cnt];
			}
	}

	//overload for positions with endgame code strings
	//mainly used for material key
	position& position::set(const string& code, color c, state_info* si) {
		assert(code[0] == 'K');
		string sides[] = { code.substr(code.find('K', 1)), code.substr(0, std::min(code.find('v'), code.find('K', 1))) }; 

		assert(sides[0].length() > 0 && sides[0].length() < 8);
		assert(sides[1].length() > 0 && sides[1].length() < 8);

		std::transform(sides[c].begin(), sides[c].end(), sides[c].begin(), tolower);

		string fen_str = "8/" + sides[0] + char(8 - sides[0].length() + '0') + "/8/8/8/8/" + sides[1]+ char(8 - sides[1].length() + '0') + "/8 w - - 0 10";

		return set(fen_str, si);
	}

	string position::fen() const {

		int empty_cnt;
		std::ostringstream ss;

		for (rank r = RANK_8; r >= RANK_1; --r)
		{
			for (file f = FILE_A; f <= FILE_H; ++f)
			{
				for (empty_cnt = 0; f <= FILE_H && empty(make_square(f, r)); ++f)
					++empty_cnt;

				if (empty_cnt)
					ss << empty_cnt;

				if (f <= FILE_H)
					ss << piece_to_char[piece_on(make_square(f, r))];
			}

			if (r > RANK_1)
				ss << '/';
		}

		ss << (_side_to_move == WHITE ? " w " : " b ");

		if (can_castle(WHITE_OO))
			ss << 'K';

		if (can_castle(WHITE_OOO))
			ss << 'Q';

		if (can_castle(BLACK_OO))
			ss << 'k';

		if (can_castle(BLACK_OOO))
			ss << 'q';

		if (!can_castle(ANY_CASTLING))
			ss << '-';

		ss << (ep_square() == SQ_NONE ? " - " : " " + uci_engine::n_square(ep_square()) + " ") << st->move_rule_50 << " " << 1 + (_game_ply - (_side_to_move == BLACK)) / 2;

		return ss.str();
	}

	void position::update_slider_blockers(color c) const {
		square ks = _square<KING>(c);

		st->blockers_for_king[c] = 0;
		st->pinners[~c] = 0;

		// snipers are sliders that attack when a piece and other snipers are removed
		bb snipers = ((attacks_bb<ROOK>(ks) & pieces(QUEEN, ROOK)) | (attacks_bb<BISHOP>(ks) & pieces(QUEEN, BISHOP))) & pieces(~c);
		bb occupancy = pieces() ^ snipers;

		while (snipers)
		{
			square   sniper_s = pop_lsb(snipers);
			bb b = between_bb(ks, sniper_s) & occupancy;

			if (b && !more_than_one(b))
			{
				st->blockers_for_king[c] |= b;
				if (b & pieces(c))
					st->pinners[~c] |= sniper_s;
			}
		}
	}
	//compute bb of all pieces that attakc a square
	bb position::attackers_to(square s, bb occupied) const {
		return (attacks_bb<ROOK>(s, occupied) & pieces(ROOK, QUEEN))
			| (attacks_bb<BISHOP>(s, occupied) & pieces(BISHOP, QUEEN))
			| (pawn_attacks_bb(BLACK, s) & pieces(WHITE, PAWN))
			| (pawn_attacks_bb(WHITE, s) & pieces(BLACK, PAWN))
			| (attacks_bb<KNIGHT>(s) & pieces(KNIGHT)) | (attacks_bb<KING>(s) & pieces(KING));
	}

	bool position::attackers_to_exist(square s, bb occupied, color c) const {
		return ((attacks_bb<ROOK>(s) & pieces(c, ROOK, QUEEN))
			&& (attacks_bb<ROOK>(s, occupied) & pieces(c, ROOK, QUEEN)))
			|| ((attacks_bb<BISHOP>(s) & pieces(c, BISHOP, QUEEN))
				&& (attacks_bb<BISHOP>(s, occupied) & pieces(c, BISHOP, QUEEN)))
			|| (((pawn_attacks_bb(~c, s) & pieces(PAWN)) | (attacks_bb<KNIGHT>(s) & pieces(KNIGHT))
				| (attacks_bb<KING>(s) & pieces(KING)))
				& pieces(c)); //these just work and i dont want to bother with them
	}

	bool position::legal(move m) const {

		assert(m.is_ok());

		color us = _side_to_move;
		square from = m.from_sq();
		square to = m.to_sq();

		assert(color_of(moved_piece(m)) == us);
		assert(piece_on(_square<KING>(us)) == make_piece(us, KING));

		//this is cooked and i hate en passant
		if (m.type_of() == EN_PASSANT) {
			square ks = _square<KING>(us);
			square cap_s = to - pawn_push(us);
			bb occupied = (pieces() ^ from ^ cap_s) | to;

			assert(to == ep_square());
			assert(moved_piece(m) == make_piece(us, PAWN));
			assert(piece_on(cap_s) == make_piece(~us, PAWN));
			assert(piece_on(to) == NO_PIECE);

			return !(attacks_bb<ROOK>(ks, occupied) & pieces(~us, QUEEN, ROOK)) && !(attacks_bb<BISHOP>(ks, occupied) & pieces(~us, QUEEN, ROOK));
		}

		//only need to check if castling path is clear from enemy attacks 
		if (m.type_of() == CASTLING) {
			to = relative_square(us, to > from ? SQ_G1 : SQ_C1);
			direction step = to > from ? WEST : EAST;

			for (square s = to; s != from; s += step)
				if (attackers_to_exist(s, pieces(), ~us))
					return false;
			return true;
		}
		//if its a king move cannot move into an attack
		if (type_of(piece_on(from)) == KING)
			return !(attackers_to_exist(to, pieces() ^ from, ~us));

		//non king move is legal IFF not pinned or moving along ray to or from the king
		return !(blockers_for_king(us) & from) || aligned(from, to, _square<KING>(us));
	}
	//used to validate moves from TT that can get corrupted, especially if i decide to add multithreading
	bool position::psuedo_legal(const move m) const {
		color us = _side_to_move;
		square from = m.from_sq();
		square to = m.to_sq();
		piece p = moved_piece(m);

		//use the slower but simpler function for uncommon (promotion, castling, en passant) cases
		if (m.type_of() != NORMAL)
			return checkers() ? move_list<EVASIONS>(*this).contains(m) : move_list<NON_EVASIONS>(*this).contains(m);

		assert(m.promotion_type() - KNIGHT == NO_PIECE_TYPE);

		if (p == NO_PIECE || color_of(p) != us) //if there is no piece or the color isn't our color, move is obviously illegal
			return false;

		if (pieces(us) & to)
			return false;
		
		//handle special pawn moves
		if (type_of(p) == PAWN) {
			if ((RANK8BB | RANK1BB) & to) //cannot be last ranks because no promotion
				return false;

			if (!(pawn_attacks_bb(us, from) & pieces(~us) & to)  // not a capture
				&& !((from + pawn_push(us) == to) && empty(to))  // not a single push
				&& !((from + 2 * pawn_push(us) == to) && (relative_rank(us, from) == RANK_2) && empty(to) && empty(to - pawn_push(us)))) //not a double push
				return false;
		}
		else if (!(attacks_bb(type_of(p), from, pieces()) & to))
			return false;

		if (checkers()) { //this checking has to be more robust because legal() relies on evasion moves being good
			if (type_of(p) != KING) {
				if (more_than_one(checkers())) //no other moves in double check
					return false;

				if (!(between_bb(_square<KING>(us), lsb(checkers())) & to)) //if not interposing check its illegal
					return false;
			}
			else if (attackers_to_exist(to, pieces() ^ from, ~us)) //you have to remove the king to catch some invalid moves
				return false;
		}
		return true;
	}

	bool position::gives_check(move m) const {
		assert(m.is_ok());
		assert(color_of(moved_piece(m)) == _side_to_move);

		square from = m.from_sq();
		square to = m.to_sq();

		if (check_squares(type_of(piece_on(from))) & to)
			return true;

		if (blockers_for_king(~_side_to_move) & from) {
			return !aligned(from, to, _square<KING>(~_side_to_move)) || m.type_of() == CASTLING;
		}

		switch (m.type_of()) {
		case NORMAL: 
			return false;

		case PROMOTION: 
			return attacks_bb(m.promotion_type(), to, pieces() ^ from) & _square<KING>(~_side_to_move);

		case EN_PASSANT: {
			square cap_s = make_square(file_of(to), rank_of(from));
			bb b = (pieces() ^ from ^ cap_s) | to;

			return (attacks_bb<ROOK>(_square<KING>(~_side_to_move), b) & pieces(_side_to_move, QUEEN, ROOK))
				| (attacks_bb<BISHOP>(_square<KING>(~_side_to_move), b)
					& pieces(_side_to_move, QUEEN, BISHOP));
		}
		default: //castling
		{
			square r_to = relative_square(_side_to_move, to > from ? SQ_F1 : SQ_D1);
			return check_squares(ROOK) & r_to;
		}
		}
	}

	void position::do_move(move m, state_info& new_st, bool gives_check, const transposition_table* tt = nullptr) {

		assert(m.is_ok());
		assert(&new_st != st);

		uint64_t k = st->key ^ zobrist::side;

		//copy some of the state, not the recalculated stuff and change the ptr
		std::memcpy(&new_st, st, offsetof(state_info, key));
		new_st.prev = st;
		st->next = &new_st;
		st = &new_st;

		++_game_ply;
		++st->move_rule_50;
		++st->plys_from_null;

		color  us = _side_to_move;
		color  them = ~us;
		square from = m.from_sq();
		square to = m.to_sq();
		piece  pc = piece_on(from);
		piece  captured = m.type_of() == EN_PASSANT ? make_piece(them, PAWN) : piece_on(to);

		assert(color_of(pc) == us);
		assert(captured == NO_PIECE || color_of(captured) == (m.type_of() != CASTLING ? them : us));
		assert(type_of(captured) != KING);

		if (m.type_of() == CASTLING)
		{
			assert(pc == make_piece(us, KING));
			assert(captured == make_piece(us, ROOK));

			square rfrom, rto;
			do_castling<true>(us, from, to, rfrom, rto);

			k ^= zobrist::psq[captured][rfrom] ^ zobrist::psq[captured][rto];
			st->non_pawn_key[us] ^= zobrist::psq[captured][rfrom] ^ zobrist::psq[captured][rto];
			captured = NO_PIECE;
		}

		if (captured)
		{
			square capsq = to;

			// If the captured piece is a pawn, update pawn hash key, otherwise
			// update non-pawn material.
			if (type_of(captured) == PAWN)
			{
				if (m.type_of() == EN_PASSANT)
				{
					capsq -= pawn_push(us);

					assert(pc == make_piece(us, PAWN));
					assert(to == st->ep_s);
					assert(relative_rank(us, to) == RANK_6);
					assert(piece_on(to) == NO_PIECE);
					assert(piece_on(capsq) == make_piece(them, PAWN));
				}

				st->pawn_key ^= zobrist::psq[captured][capsq];
			}
			else
			{
				st->non_pawn_material[them] -= piece_value[captured];
				st->non_pawn_key[them] ^= zobrist::psq[captured][capsq];

				if (type_of(captured) <= BISHOP)
					st->minor_piece_key ^= zobrist::psq[captured][capsq];
			}

			remove_piece(capsq);

			k ^= zobrist::psq[captured][capsq];
			st->material_key ^= zobrist::psq[captured][piece_count[captured]];

			st->move_rule_50 = 0;
		}

		// Update hash key
		k ^= zobrist::psq[pc][from] ^ zobrist::psq[pc][to];

		if (st->ep_s != SQ_NONE)
		{
			k ^= zobrist::en_passant[file_of(st->ep_s)];
			st->ep_s = SQ_NONE;
		}

		if (st->castling_rights && (castling_rights_mask[from] | castling_rights_mask[to]))
		{
			k ^= zobrist::castling[st->castling_rights];
			st->castling_rights &= ~(castling_rights_mask[from] | castling_rights_mask[to]);
			k ^= zobrist::castling[st->castling_rights];
		}

		if (m.type_of() != CASTLING){
			move_piece(from, to);
		}

		// If the moving piece is a pawn do some special extra work
		if (type_of(pc) == PAWN)
		{
			// Set en passant square if the moved pawn can be captured
			if ((int(to) ^ int(from)) == 16
				&& (pawn_attacks_bb(us, to - pawn_push(us)) & pieces(them, PAWN)))
			{
				st->ep_s = to - pawn_push(us);
				k ^= zobrist::en_passant[file_of(st->ep_s)];
			}

			else if (m.type_of() == PROMOTION)
			{
				piece     promotion = make_piece(us, m.promotion_type());
				piece_type promotionType = type_of(promotion);

				assert(relative_rank(us, to) == RANK_8);
				assert(type_of(promotion) >= KNIGHT && type_of(promotion) <= QUEEN);

				remove_piece(to);
				put_piece(promotion, to);

				k ^= zobrist::psq[pc][to] ^ zobrist::psq[promotion][to];
				st->pawn_key ^= zobrist::psq[pc][to];
				st->material_key ^=
					zobrist::psq[promotion][piece_count[promotion] - 1] ^ zobrist::psq[pc][piece_count[pc]];

				if (promotionType <= BISHOP)
					st->minor_piece_key ^= zobrist::psq[promotion][to];

				st->non_pawn_material[us] += piece_value[promotion];
			}

			// Update pawn hash key
			st->pawn_key ^= zobrist::psq[pc][from] ^ zobrist::psq[pc][to];

			// Reset rule 50 draw counter
			st->move_rule_50 = 0;
		}
		else
		{
			st->non_pawn_key[us] ^= zobrist::psq[pc][from] ^ zobrist::psq[pc][to];

			if (type_of(pc) <= BISHOP)
				st->minor_piece_key ^= zobrist::psq[pc][from] ^ zobrist::psq[pc][to];
		}

		st->key = k;
		if (tt)
			prefetch(tt->first_entry(r_key()));

		st->captured_piece = captured;

		st->checkers_bb = gives_check ? attackers_to(_square<KING>(them)) & pieces(us) : 0;

		_side_to_move = ~_side_to_move;

		set_check();

		// calculate the repetition info. It is the ply distance from the previous
		// occurrence of the same position, negative in the 3-fold case, or zero
		// if the position was not repeated.
		st->repititon = 0;
		int end = std::min(st->move_rule_50, st->plys_from_null);
		if (end >= 4)
		{
			state_info* stp = st->prev->prev;
			for (int i = 4; i <= end; i += 2)
			{
				stp = stp->prev->prev;
				if (stp->key == st->key)
				{
					st->repititon = stp->repititon ? -i : i;
					break;
				}
			}
		}

		assert(pos_isnt_bad());
	}

	void position::undo_move(move m) {

		assert(m.is_ok());

		_side_to_move = ~_side_to_move;

		color  us = _side_to_move;
		square from = m.from_sq();
		square to = m.to_sq();
		piece  pc = piece_on(to);

		assert(empty(from) || m.type_of() == CASTLING);
		assert(type_of(st->captured_piece) != KING);

		if (m.type_of() == PROMOTION)
		{
			assert(relative_rank(us, to) == RANK_8);
			assert(type_of(pc) == m.promotion_type());
			assert(type_of(pc) >= KNIGHT && type_of(pc) <= QUEEN);

			remove_piece(to);
			pc = make_piece(us, PAWN);
			put_piece(pc, to);
		}

		if (m.type_of() == CASTLING)
		{
			square rfrom, rto;
			do_castling<false>(us, from, to, rfrom, rto);
		}
		else
		{
			move_piece(to, from);  

			if (st->captured_piece)
			{
				square capsq = to;

				if (m.type_of() == EN_PASSANT)
				{
					capsq -= pawn_push(us);

					assert(type_of(pc) == PAWN);
					assert(to == st->prev->ep_s);
					assert(relative_rank(us, to) == RANK_6);
					assert(piece_on(capsq) == NO_PIECE);
					assert(st->captured_piece == make_piece(~us, PAWN));
				}

				put_piece(st->captured_piece, capsq);  
			}
		}

		st = st->prev;
		--_game_ply;

		assert(pos_isnt_bad());
	}

	template<bool _do>
	void position::do_castling(color c, square from, square & to, square & rfrom, square & rto) {

		bool kingSide = to > from;
		rfrom = to; 
		rto = relative_square(c, kingSide ? SQ_F1 : SQ_D1);
		to = relative_square(c, kingSide ? SQ_G1 : SQ_C1);

		remove_piece(_do ? from : to);
		remove_piece(_do ? rfrom : rto);
		board[_do ? from : to] = board[_do ? rfrom : rto] = NO_PIECE; 
		put_piece(make_piece(c, KING), _do ? to : from);
		put_piece(make_piece(c, ROOK), _do ? rto : rfrom);
	}

	void position::do_null_move(state_info& new_st, const transposition_table& tt) {

		assert(!checkers());
		assert(&new_st != st);

		new_st.prev = st;
		st->next = &new_st;
		st = &new_st;

		if (st->ep_s != SQ_NONE)
		{
			st->key ^= zobrist::en_passant[file_of(st->ep_s)];
			st->ep_s = SQ_NONE;
		}

		st->key ^= zobrist::side;
		prefetch(tt.first_entry(r_key()));


		st->plys_from_null = 0;

		_side_to_move = ~_side_to_move;

		set_check();

		st->repititon = 0;

		assert(pos_isnt_bad());
	}

	void position::undo_null_move() {

		assert(!checkers());

		st = st->prev;
		_side_to_move = ~_side_to_move;
	}

	// test if the SEE (Static Exchange Evaluation) value of move is greater or equal to the given threshold
	bool position::see_ge(move m, int threshold) const {

		assert(m.is_ok());

		// only deal with normal moves, assume others pass a simple SEE
		if (m.type_of() != NORMAL)
			return VALUE_ZERO >= threshold;

		square from = m.from_sq(), to = m.to_sq();

		int swap = piece_value[piece_on(to)] - threshold;
		if (swap < 0)
			return false;

		swap = piece_value[piece_on(from)] - swap;
		if (swap <= 0)
			return true;

		assert(color_of(piece_on(from)) == _side_to_move);
		bb occupied = pieces() ^ from ^ to;  // xoring to is important for pinned piece logic
		color stm = _side_to_move;
		bb attackers = attackers_to(to, occupied);
		bb stmAttackers, bb;
		int res = 1;

		while (true)
		{
			stm = ~stm;
			attackers &= occupied;

			// if stm has no more attackers then give up: stm loses
			if (!(stmAttackers = attackers & pieces(stm)))
				break;

			// don't allow pinned pieces to attack as long as there are
			// pinners on their original square.
			if (pinners(~stm) & occupied){
				stmAttackers &= ~blockers_for_king(stm);

				if (!stmAttackers)
					break;
			}

			res ^= 1;

			// locate and remove the next least valuable attacker, and add to
			// the bitboard 'attackers' any X-ray attackers behind it.
			if ((bb = stmAttackers & pieces(PAWN)))
			{
				if ((swap = pawn_value - swap) < res)
					break;
				occupied ^= least_signifcant_square_bb(bb);

				attackers |= attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN);
			}
			else if ((bb = stmAttackers & pieces(KNIGHT))){
				if ((swap = knight_value - swap) < res)
					break;
				occupied ^= least_signifcant_square_bb(bb);
			}
			else if ((bb = stmAttackers & pieces(BISHOP))){
				if ((swap = bishop_value - swap) < res)
					break;
				occupied ^= least_signifcant_square_bb(bb);

				attackers |= attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN);
			}
			else if ((bb = stmAttackers & pieces(ROOK))){
				if ((swap = rook_value - swap) < res)
					break;
				occupied ^= least_signifcant_square_bb(bb);

				attackers |= attacks_bb<ROOK>(to, occupied) & pieces(ROOK, QUEEN);
			}
			else if ((bb = stmAttackers & pieces(QUEEN))){
				if ((swap = queen_value - swap) < res)
					break;
				occupied ^= least_signifcant_square_bb(bb);

				attackers |= (attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN))
					| (attacks_bb<ROOK>(to, occupied) & pieces(ROOK, QUEEN));
			}
			else  // KING  if we "capture" with the king but the opponent still has attackers, reverse the result.
				return (attackers & ~pieces(stm)) ? res ^ 1 : res;
		}

		return bool(res);
	}

	bool position::is_draw(int ply) const {//only checks for 50 move rule and repetition, not stalemate
		if (st->move_rule_50 > 99 && (!checkers() || move_list<LEGAL>(*this).size()))
			return true;

		return is_repetition(ply);
	}

	bool position::is_repetition(int ply) const { return st->repititon && st->repititon < ply; }

	bool position::has_repeated() const { //test whether theres been a repition since the last capture or pawn move
		state_info* stc = st;
		int end = std::min(st->move_rule_50, st->plys_from_null);
		while (end-- >= 4) {
			if (stc->repititon)
				return true;
			stc = stc->prev;
		}
		return false;
	}

	bool position::upcoming_repition(int ply) const {
		int k;
		int end = std::min(st->move_rule_50, st->plys_from_null);

		if (end < 3)
			return false;

		uint64_t original_key = st->key;
		state_info* stp = st->prev;
		uint64_t other = original_key ^ stp->key ^ zobrist::side;

		for (int i = 3; i <= end; i += 2) {
			stp = stp->prev;
			other ^= stp->key ^ stp->prev->key ^ zobrist::side;
			stp = stp->prev;

			if (other != 0)
				continue;

			uint64_t moveKey = original_key ^ stp->key;
			if ((k = h1(moveKey), cuckoo[k] == moveKey) || (k = h2(moveKey), cuckoo[k] == moveKey)) {
				move   move = cuckoo_move[k];
				square s1 = move.from_sq();
				square s2 = move.to_sq();

				if (!((between_bb(s1, s2) ^ s2) & pieces()))
				{
					if (ply > i)
						return true;

					// for nodes before or at the root, check that the move is a
					// repetition rather than a move to the current position.
					if (stp->repititon)
						return true;
				}
			}
		}
		return false;
	}

	void position::flip() { //flips white and black colors, thats it, only for debug
		string f, token;
		std::stringstream ss(fen());

		for (rank r = RANK_8; r >= RANK_1; --r)
		{
			std::getline(ss, token, r > RANK_1 ? '/' : ' ');
			f.insert(0, token + (f.empty() ? " " : "/"));
		}

		ss >> token;
		f += (token == "w" ? "B " : "W ");

		ss >> token;
		f += token + " ";

		std::transform(f.begin(), f.end(), f.begin(),
			[](char c) { return char(islower(c) ? toupper(c) : tolower(c)); });

		ss >> token;
		f += (token == "-" ? token : token.replace(1, 1, token[1] == '3' ? "6" : "3"));

		std::getline(ss, token);
		f += token;

		set(f, st);

		assert(pos_isnt_bad());
	}

	bool position::pos_isnt_bad() const { //debug, checks pos for consistency, could add a more thorough check but this is ok for now

		if ((_side_to_move != WHITE && _side_to_move != BLACK) || piece_on(_square<KING>(WHITE)) != W_KING || piece_on(_square<KING>(BLACK)) != B_KING || (ep_square() != SQ_NONE && relative_rank(_side_to_move, ep_square()) != RANK_6)){
			assert(0 && "pos_is_ok: default");
		}
		return true;
	}
}