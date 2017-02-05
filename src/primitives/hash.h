#pragma once
#include <stdint.h>

// Simple, non-crypto string hashing algorithm djb2 by Dan Bernstein: compile-time and runtime versions, 
// plus user-literal version is provided to allow "abc"_hash to return hash of "abc".

constexpr uint32_t static_strlen(const char *str) {
    return (*str == 0) ? 0 : (1 + static_strlen(str+1));
}

constexpr uint32_t static_hash_prefix(const char *str, uint32_t prefix_len) {
    return (prefix_len == 0) ? 5381 : ((static_hash_prefix(str, prefix_len-1) * 33) ^ str[prefix_len-1]);
}

constexpr uint32_t static_hash(const char *str) {
    return static_hash_prefix(str, static_strlen(str));
}

constexpr uint32_t operator "" _hash(const char* str, unsigned int len) {  // User literal.
    return static_hash_prefix(str, len);
}

// Runtime version of our hash. 'hash' argument can be used for 'streaming mode'.
inline uint32_t runtime_hash(const char *str, uint32_t hash = 5381) {  
    while (*str)
        hash = (hash * 33) ^ *str;
    return hash;
}


// Simple, append-only hash table with constant capacity.
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
        if (full()) {            
            return;  // TODO: Assert.
        }
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


