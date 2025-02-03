#include "move_gen.h"

#include <cassert>

#include "bitboard.h"
#include "position.h"

namespace engine {
	namespace {
		template<gen_type t, direction d, bool enemy>
		ext_move* make_promotions(ext_move* move_list, [[maybe_unused]] square to) { //saw this in stockfish, maybe_unused supressed compiler warnings saying that a variable isn't used in function body
			constexpr bool all = t == EVASIONS || t == NON_EVASIONS;

			if constexpr (t ==CAPS || all)
				*move_list++ = move::make<PROMOTION>(to - d, to, QUEEN); //gen queen promos in captures because they are big moves

			if constexpr ((t == CAPS && enemy) || (t == QUIETS && !enemy) || all) {
				*move_list++ = move::make<PROMOTION>(to - d, to, ROOK);
				*move_list++ = move::make<PROMOTION>(to - d, to, BISHOP);
				*move_list++ = move::make<PROMOTION>(to - d, to, KNIGHT);
			}
			return move_list;
		}
		
		template<color us, gen_type t>
		ext_move* generate_pawn_moves(const position& pos, ext_move* move_list, bb target) {
			constexpr color them = ~us;
			constexpr bb rank_7_bb = (us == WHITE ? RANK7BB : RANK2BB);
			constexpr bb rank_3_bb = (us == WHITE ? RANK3BB : RANK6BB);
			constexpr direction up = pawn_push(us);
			constexpr direction up_r = (us == WHITE ? NORTH_EAST : SOUTH_WEST);
			constexpr direction up_l = (us == WHITE ? NORTH_WEST : SOUTH_EAST);

			const bb empty_squares = ~pos.pieces();
			const bb enemies = t == EVASIONS ? pos.checkers() : pos.pieces(them); //if its an evading move you only need to look at checkers

			bb pawns_on_7 = pos.pieces(us, PAWN) & rank_7_bb;
			bb pawns_not_on_7 = pos.pieces(us, PAWN) & ~rank_7_bb;

			if constexpr (t != CAPS) { //handle single and double pushes
				bb b1 = shift<up>(pawns_not_on_7) & empty_squares; //& with empty squares so that pawns shifting into pieces will become 0s
				bb b2 = shift<up>(b1 & rank_3_bb) & empty_squares; //pawns who havent moved on the first bb move one space, and now they move again because they can

				if constexpr (t == EVASIONS) { //consider only blocking moves
					b1 &= target;
					b2 &= target;
				}

				while (b1) {
					square to = pop_lsb(b1);
					*move_list++ = move(to - up, to); //step back one square to find original
				}
				while (b2) {
					square to = pop_lsb(b2);
					*move_list++ = move(to - up - up, to);
				}
			}

			if (pawns_on_7) { //promotions
				bb b1 = shift<up_r>(pawns_on_7) & enemies;
				bb b2 = shift<up_l>(pawns_on_7) & enemies;
				bb b3 = shift<up>(pawns_on_7) & empty_squares;

				if constexpr (t == EVASIONS) //you never know
					b3 &= target;

				while (b1)
					move_list = make_promotions<t, up_r, true>(move_list, pop_lsb(b1));

				while (b2)
					move_list = make_promotions<t, up_l, true>(move_list, pop_lsb(b2));

				while (b3)
					move_list = make_promotions<t, up, false>(move_list, pop_lsb(b3));
			}

			if constexpr (t == CAPS || t == EVASIONS || t == NON_EVASIONS) { //normal captures and en passant
				bb b1 = shift<up_r>(pawns_not_on_7) & enemies;
				bb b2 = shift<up_l>(pawns_not_on_7) & enemies;

				while (b1) {
					square to = pop_lsb(b1);
					*move_list++ = move(to - up_r, to);
				}
				while (b2) {
					square to = pop_lsb(b2);
					*move_list++ = move(to - up_l, to);
				}

				if (pos.ep_square() != SQ_NONE) {
					assert(rank_of(pos.ep_square()) == relative_rank(us, RANK_6)); //make sure en_pasasnt is in a place it can actually happen, otherwise things are bad

					if (t == EVASIONS && (target * (pos.ep_square() + up))) //capture cannot resolve discoverd check
						return move_list;

					b1 = pawns_not_on_7 & pawn_attacks_bb(them, pos.ep_square());

					assert(b1);

					while (b1)
						*move_list++ = move::make<EN_PASSANT>(pop_lsb(b1), pos.ep_square());
				}
			}

			return move_list;
		}

		template<color us, piece_type p>
		ext_move* generate_moves(const position& pos, ext_move* move_list, bb target) {
			static_assert(p != KING && p != PAWN, "UNSUPPORTED PIECE TYPE IN GENERATE_MOVES()"); //dont use this for kings or pawns 

			bb b = pos.pieces(us, p);

			while (b) {
				square from = pop_lsb(b);
				bb _b = attacks_bb<p>(from, pos.pieces()) & target;

				while (_b)
					*move_list++ = move(from, pop_lsb(_b));
			}

			return move_list;
		}

		template<color us, gen_type t>
		ext_move* generate_all(const position& pos, ext_move* move_list) {
			static_assert(t != LEGAL, "UNSUPPORTED TYPE IN GENERATE_ALL()");

			const square ks = pos._square<KING>(us);
			bb target;

			//dont gen non king moves in double check
			if (t != EVASIONS || !more_than_one(pos.checkers())) {
				//in order, set target bb to either the line between the king and the checker, all the squares friendly pieces are NOT on, all the squares enemy piece are on, or squares without any pieces (quiet moves)
				target = t == EVASIONS ? between_bb(ks, lsb(pos.checkers())) : t == NON_EVASIONS ? ~pos.pieces(us) : t == CAPS ? pos.pieces(~us) : ~pos.pieces();
				
				move_list = generate_pawn_moves<us, t>(pos, move_list, target);
				move_list = generate_moves<us, KNIGHT>(pos, move_list, target);
				move_list = generate_moves<us, BISHOP>(pos, move_list, target);
				move_list = generate_moves<us, ROOK>(pos, move_list, target);
				move_list = generate_moves<us, QUEEN>(pos, move_list, target);
			}

			bb b = attacks_bb<KING>(ks) & (t == EVASIONS ? ~pos.pieces(us) : target);

			while (b)
				*move_list++ = move(ks, pop_lsb(b));

			if ((t == QUIETS || t == NON_EVASIONS) && pos.can_castle(us & ANY_CASTLING)) {
				for (castling_rights cr : {us& KING_SIDE, us& QUEEN_SIDE}) {
					if (!pos.castling_impeded(cr) && pos.can_castle(cr))
						*move_list++ = move::make<CASTLING>(ks, pos.castling_rook_square(cr));
				}
			}

			return move_list;
		}
	} //end of anonymous namespace

	//pointer to end of move_list
	template<gen_type t>
	ext_move* generate(const position& pos, ext_move* move_list) {
		static_assert(t != LEGAL, "UNSUPPORTED TYPE IN GENERATE()");
		assert((t == EVASIONS) == bool(pos.checkers())); //if its an evading move there needs to be a checker, otherwise things are bad

		color us = pos.side_to_move();

		return us == WHITE ? generate_all<WHITE, t>(pos, move_list) : generate_all<BLACK, t>(pos, move_list);
	}

	//explicitly declare template instantiations 
	template ext_move* generate<CAPS>(const position&, ext_move*);
	template ext_move* generate<QUIETS>(const position&, ext_move*);
	template ext_move* generate<EVASIONS>(const position&, ext_move*);
	template ext_move* generate<NON_EVASIONS>(const position&, ext_move*);

	
	template<>
	ext_move* generate<LEGAL>(const position& pos, ext_move* move_list) {
		color us = pos.side_to_move();
		bb pinned = pos.blockers_for_king(us) & pos.pieces(us);
		square ks = pos._square<KING>(us);
		ext_move* cur = move_list;
		//gen pseudo legals
		move_list = pos.checkers() ? generate<EVASIONS>(pos, move_list) : generate<NON_EVASIONS>(pos, move_list);

		//step cur through move_list
		while (cur != move_list) {
			if (((pinned & cur->from_sq()) || cur->from_sq() == ks || cur->type_of() == EN_PASSANT) && !pos.legal(*cur)) { 
				*cur = *(--move_list); //if illegal overwrite the move with the last move
			}
			else {
				++cur; // if legal then step forward in the list
			}
		}
		return move_list;
	}
}