#ifndef UTILS_H_INC
#define UTILS_H_INC

#include <cstdint>
#include <assert.h>
#include <string>
#include <xmmintrin.h>

class PRNG {

    uint64_t s;

    uint64_t rand64() {

        s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
        return s * 2685821657736338717LL;
    }

public:
    PRNG(uint64_t seed) :
        s(seed) {
        assert(seed);
    }

    template<typename T>
    T rand() {
        return T(rand64());
    }

    //gen fast rands and & them because on average only like 8 bits are set
    //saw this online
    template<typename T> 
    T sparse_rand() {
        return T(rand64() & rand64() & rand64());
    }
}; 
//multiply two 64 bit numbers and keep the high 64 bits from it
inline uint64_t mul_hi64(uint64_t a, uint64_t b) {
    uint64_t aL = uint32_t(a), aH = a >> 32;
    uint64_t bL = uint32_t(b), bH = b >> 32;
    uint64_t c1 = (aL * bL) >> 32;
    uint64_t c2 = aH * bL + c1;
    uint64_t c3 = aL * bH + uint32_t(c2);
    return aH * bH + (c2 >> 32) + (c3 >> 32);
}

struct command_line {
public:
    command_line(int _argc, char** _argv) :
        argc(_argc),
        argv(_argv) {}

    static std::string get_binary_directory(std::string argv0);
    static std::string get_working_directory();

    int    argc;
    char** argv;
};
void prefetch(const void* addr);
#endif 