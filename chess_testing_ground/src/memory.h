#ifndef MEMORY_H_INCLUDED
#define MEMORY_H_INCLUDED

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "types.h"

namespace engine {
	void* aligned_large_pages_alloc(size_t size);
	void  aligned_large_pages_free(void* mem);
}
#endif