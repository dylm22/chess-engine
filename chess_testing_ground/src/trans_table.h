#ifndef TT_H_INC
#define TT_H_INC

#include <cstddef>
#include <cstdint>
#include <tuple>

#include "types.h"
#include "memory.h"

namespace engine {
	struct tt_entry;
	struct cluster;

	//there is one global hash table for the engine 
	//collisions are possible and could cause crazy mistakes, however they are too costly to fix, however risk also decreases with a large table size
	//this also definitley has race moments between threads but syncing it takes too long

	struct tt_data {
		move _move;
		int value, eval;
		int depth;
		bound _bound;
		bool is_pv;

		tt_data() = delete;
		tt_data(move m, int v, int ev, int d, bound b, bool pv) :
			_move(m), value(v), eval(ev), depth(d), _bound(b), is_pv(pv) {};
	};

	struct tt_writer {
	public:
		void write(uint64_t k, int v, bool pv, bound b, int d, move m, int ev, uint8_t gen_8);

	private:
		friend class transposition_table; //friend gives transposition_table access to private member variables
		tt_entry* entry;
		tt_writer(tt_entry* tte);
	};

	class transposition_table {
	public:
		~transposition_table() { aligned_large_pages_free(table); }

		void resize(size_t mb_size); //set size
		void clear(); //clear table 
		//void clear(thread_pool& threads); //re-init memory
		int hash_full(int max_age = 0) const; //approximate what fraction of entries have been writtent o

		void new_search(); // must be called at the begining of each root search to track age
		uint8_t generation() const; // cur age
		std::tuple<bool, tt_data, tt_writer>probe(const uint64_t key) const; //return tuple saying whether the entry already has a position, a copy of prior data, and a writer object to the entry
		tt_entry* first_entry(const uint64_t key) const;

	private:
		friend struct tt_entry;
		size_t cluster_count;
		cluster* table = nullptr;

		uint8_t gen_8 = 0;
	};
}

#endif