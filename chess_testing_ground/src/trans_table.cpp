#include "trans_table.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "memory.h"
#include "utils.h"

namespace engine {

// tt_entry struct is the 10 bytes transposition table entry, defined as below:
//
// key        16 bit
// depth       8 bit
// generation  5 bit
// pv node     1 bit
// bound type  2 bit
// move       16 bit
// value      16 bit
// evaluation 16 bit

	struct tt_entry {
		tt_data read() const {
			return tt_data{ move(move16),int(value16),int(eval16), int(depth8 + DEPTH_ENTRY_OFFSET),bound(gen_bound8 & 0x3), bool(gen_bound8 & 0x4) };
		}
		bool is_occupied() const;
		void save(uint64_t k, int v, bool pv, bound b, int d, move m, int ev, uint8_t gen_8);
		uint8_t relative_age(const uint8_t gen_8) const;

	private:
		friend class transposition_table;

		uint16_t key16;
		uint8_t  depth8;
		uint8_t  gen_bound8;
		move     move16;
		int16_t  value16;
		int16_t  eval16;
	};

	static constexpr unsigned GENERATION_BITS = 3; //reserved bits
	static constexpr int GENERATION_DELTA = (1 << GENERATION_BITS); //increment for generation field
	static constexpr int GENERATION_CYCLE = 255 + GENERATION_DELTA; //cycle length
	static constexpr int GENERATION_MASK = (0xFF << GENERATION_BITS) & 0xFF; //mask to get gen number

	bool tt_entry::is_occupied() const { return bool(depth8); }

	void tt_entry::save(uint64_t k, int v, bool pv, bound b, int d, move m, int ev, uint8_t gen_8) {
		
		if (m || uint16_t(k) != key16) //preserve old move if theres no new one
			move16 = m;

		//overwrite less valuable entires, starting with cheapest checks first
		if (b == BOUND_EXACT || uint16_t(k) != key16 || d - DEPTH_ENTRY_OFFSET + 2 * pv > depth8 - 4 || relative_age(gen_8)) {
			assert(d > DEPTH_ENTRY_OFFSET);
			assert(d < 256 + DEPTH_ENTRY_OFFSET);

			key16 = uint16_t(k);
			depth8 = uint8_t(d - DEPTH_ENTRY_OFFSET);
			gen_bound8 = uint8_t(gen_8 | uint8_t(pv) << 2 | b);
			value16 = int16_t(v);
			eval16 = int16_t(ev);
		}
	}

	uint8_t tt_entry::relative_age(const uint8_t gen_8) const {
		//saw this in stockfish, idea is, as they put it,

		// Due to our packed storage format for generation and its cyclic
		// nature we add GENERATION_CYCLE (256 is the modulus, plus what
		// is needed to keep the unrelated lowest n bits from affecting
		// the result) to calculate the entry age correctly even after
		// generation8 overflows into the next cycle.

		return (GENERATION_CYCLE + gen_8 - gen_bound8) & GENERATION_MASK; 
	}

	tt_writer::tt_writer(tt_entry* tte) : entry(tte){}

	void tt_writer::write(uint64_t k, int v, bool pv, bound b, int d, move m, int ev, uint8_t gen_8) {
		entry->save(k, v, pv, b, d, m, ev, gen_8);
	}

	//again saw this on stockfish, as they put it, 

	// A TranspositionTable is an array of Cluster, of size clusterCount. Each cluster consists of ClusterSize number
	// of TTEntry. Each non-empty TTEntry contains information on exactly one position. The size of a Cluster should
	// divide the size of a cache line for best performance, as the cacheline is prefetched when possible.

	static constexpr int cluster_size = 3;

	struct cluster {
		tt_entry entry[cluster_size];
		char padding[2]; //pad to 32 bytes
	};

	static_assert(sizeof(cluster) == 32, "suboptimal cluster size");

	void transposition_table::resize(size_t mb_size) {
		aligned_large_pages_free(table);

		cluster_count = mb_size * 1024 * 1024 / sizeof(cluster);

		table = static_cast<cluster*>(aligned_large_pages_alloc(cluster_count * sizeof(cluster)));

		if (!table) {
			std::cerr << "failed to alloc " << mb_size << "MB for tt" << std::endl;
			exit(EXIT_FAILURE);
		}
	}	
	void transposition_table::clear() {
		gen_8 = 0;
		std::memset(&table[0], 0, cluster_count * sizeof(cluster)); //this might segfault
	}

	int transposition_table::hash_full(int max_age) const {
		int max_age_internal = max_age << GENERATION_BITS;
		int cnt = 0;
		for (int i = 0; i < 1000; i++) {
			for (int k = 0; k < cluster_size; k++) {
				cnt += table[i].entry[k].is_occupied() && table[i].entry[k].relative_age(gen_8) <= max_age_internal;
			}
		}
		return cnt / cluster_size;
	}

	void transposition_table::new_search() {
		gen_8 += GENERATION_DELTA;
	}

	uint8_t transposition_table::generation() const { return gen_8; }


	//looks up current position in the table, if its there it returns true
	//otherwise, returns false and gives a pointer to an empty or least valuable entry
	//value is calculated as depth - 8*relative age. higher replace value is more value.
	std::tuple<bool, tt_data, tt_writer> transposition_table::probe(const uint64_t key) const {
		tt_entry* const tte = first_entry(key);
		const uint16_t key16 = uint16_t(key); //only take the lowest 16 bits for use inside cluster

		for (int i = 0; i < cluster_size; i++) {
			if (tte[i].key16 == key16)
				return { tte[i].is_occupied(),tte[i].read(),tt_writer(&tte[i]) };
		}

		tt_entry* replace = tte;
		for (int i = 1; i < cluster_size; i++) {
			if (replace->depth8 - replace->relative_age(gen_8) * 2 > tte[i].depth8 - tte[i].relative_age(gen_8) * 2)
				replace = &tte[i];
		}

		return { false,tt_data{move::none(), VALUE_NONE,VALUE_NONE,DEPTH_ENTRY_OFFSET,BOUND_NONE,false}, tt_writer(replace) };
	}

	tt_entry* transposition_table::first_entry(const uint64_t key) const {
		return &table[mul_hi64(key, cluster_count)].entry[0];
	}
}