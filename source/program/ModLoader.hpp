#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Allocator.hpp"
/**
 * Re-implementation of libc++ std::string for memory-safe interaction
 * with the game's internal data structures.
 */
struct libcxx_string {
    union {
        struct { uint64_t cap; uint64_t size; char* data; } l;
        struct { unsigned char size_flag; char data[23]; } s;
    } u;

    bool is_long() const { return u.s.size_flag & 1; }
    const char* c_str() const { return is_long() ? u.l.data : u.s.data; }

    void assign(const char* str, size_t len) {
        if (is_long()) GameOperatorDelete(u.l.data);

        if (len < 23) {
            u.s.size_flag = static_cast<unsigned char>(len << 1);
            if (len > 0) std::memcpy(u.s.data, str, len);
            u.s.data[len] = 0;
        } else {
            size_t capacity = (len + 16) & ~15;
            u.l.data = (char*)GameOperatorNew(capacity);
            u.l.size = len;
            u.l.cap = capacity | 1;
            std::memcpy(u.l.data, str, len);
            u.l.data[len] = 0;
        }
    }
};

class ModLoader {
public:
    static std::vector<std::string> modDirectoryPaths;

    static void initMod(const std::string& path);
    static void init();
};
