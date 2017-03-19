#pragma once
#include <stdint.h>
#include <cstring>

// MurmurHash3 32 bit, derived from https://github.com/aappleby/smhasher
// Both compile-time and runtime versions provided.
// User literal version is also defined to allow "abc"_hash to be compiled to a hash of "abc".
// One useful property of this hash is that hash("") == 0.

constexpr uint32_t rotl32(uint32_t x, int8_t r) {
    // GCC is smart enough to convert this to a single 'ROR' instruction on ARM.
    return (x << r) | (x >> (32 - r));
}

template<bool static_version>
constexpr uint32_t MurmurHash3_32(const char *data, uint32_t len, uint32_t seed = 0) {
    uint32_t h1 = seed, k1 = 0, i = 0;
    for(; i+3 < len; i += 4) {
        if (static_version) {
            k1 = ((uint8_t)data[i+0] << 0) | 
                 ((uint8_t)data[i+1] << 8) | 
                 ((uint8_t)data[i+2] << 16) | 
                 ((uint8_t)data[i+3] << 24); 
        } else {
            k1 = *(uint32_t *)(data + i);
        }
        k1 *= 0xcc9e2d51; k1 = rotl32(k1, 15); k1 *= 0x1b873593;
        h1 ^= k1; h1 = rotl32(h1, 13); h1 = h1 * 5 + 0xe6546b64;
    }
    k1 = 0;
    switch(len & 3) {
    case 3: k1 ^= (uint8_t)data[i+2] << 16;
    case 2: k1 ^= (uint8_t)data[i+1] << 8;
    case 1: k1 ^= (uint8_t)data[i];
            k1 *= 0xcc9e2d51; k1 = rotl32(k1, 15); k1 *= 0x1b873593; 
            h1 ^= k1;
    };
    h1 ^= len;
    h1 ^= h1 >> 16; h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13; h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    return h1;
} 

constexpr uint32_t static_strlen(const char *str) {
    uint32_t len = 0;
    while (str[len]) len++;
    return len;
}

constexpr uint32_t static_hash(const char *str) {
    return MurmurHash3_32<true>(str, static_strlen(str));
}

// String user literal.
constexpr uint32_t operator ""_hash(const char* str, size_t len) {  
    return MurmurHash3_32<true>(str, len);
}

// Runtime version of our hash - to be used on dynamic data.
inline uint32_t runtime_hash(const char *str, size_t len) {
    return MurmurHash3_32<false>(str, len);
}


// Tests that the algorithm implemented correctly
static_assert("abc123123"_hash == 2841232309, "MurmurHash3 validity test");
static_assert(""_hash == 0, "MurmurHash3 validity test");
static_assert("a"_hash == 1009084850, "MurmurHash3 validity test");
static_assert("a§"_hash == 221796761, "MurmurHash3 validity test");
static_assert("ab§da"_hash == 3670539345, "MurmurHash3 validity test");


// Simple, append-only hash table with constant capacity.
/*
// NOTE: This is not tested.
template<typename T, uint32_t C>
class HashTable {
    static_assert(C > 0, "Capacity should be positive.");
    constexpr static uint32_t kModulo = C * 2 + 1;  // We allocate 2x of Capacity to keep searches fast.
public:
    HashTable() : size_{0}, elems_{0}, hashes_{0} {}
    inline uint32_t size() const { return size_; }
    constexpr uint32_t max_size() { return C; }
    inline bool empty() const { return size_ == 0; }
    inline bool full() const { return size_ == max_size(); }

    // Returns pointer to element or nullptr if no such element.
    T *get(uint32_t hash) {
        uint32_t pos = search(hash);
        return hashes_[pos] ? elems_[pos] : nullptr;
    }

    // Set value for the given hash.
    void set(uint32_t hash, T* val) {
        assert(!full());            
        uint32_t pos = search(hash);
        if (!hashes_[pos]) size_++;
        hashes_[pos] = hash;
        elems_[pos] = val;
    }

    void clear() {
        size_ = 0;
        // Don't want to include stdlib for memset. We promised simple, not performant, right?
        for (uint32_t i = 0; i < kModulo; i++) {
            elems_[i] = nullptr;
            hashes_[i] = 0;
        }
    }

private:
    // Search for given hash in hash table. Returns position.
    uint32_t search(uint32_t hash) {
        if (hash == 0) hash = 1; // We use hash = 0 to mark empty place.
        uint32_t pos = hash % kModulo;
        while (hashes_[pos] && hashes_[pos] != hash)
            pos = (pos + 1) % kModulo;
        return pos;
    }

    uint32_t size_;
    T *elems_[kModulo];
    uint32_t hashes_[kModulo];
};
*/

