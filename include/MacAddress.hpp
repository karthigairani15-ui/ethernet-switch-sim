#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <functional>

struct MacAddress {
    std::array<uint8_t, 6> bytes{};

    bool operator==(const MacAddress& other) const {
        return bytes == other.bytes;
    }

    bool operator!=(const MacAddress& other) const {
        return !(*this == other);
    }

    std::string to_string() const {
        std::ostringstream oss;
        for (size_t i = 0; i < bytes.size(); ++i) {
            if (i != 0) oss << ':';
            oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(bytes[i]);
        }
        return oss.str();
    }

    bool is_broadcast() const {
        for (uint8_t b : bytes) {
            if (b != 0xFF) return false;
        }
        return true;
    }

    bool is_multicast() const {
        return (bytes[0] & 0x01) == 1;
    }
};

// Specialize std::hash so MacAddress can be used as an unordered_map key
namespace std {
    template <>
    struct hash<MacAddress> {
        size_t operator()(const MacAddress& mac) const {
            // Combine all 6 bytes into one hash using FNV-1a style mixing
            size_t h = 14695981039346656037ULL;
            for (uint8_t b : mac.bytes) {
                h ^= b;
                h *= 1099511628211ULL;
            }
            return h;
        }
    };
}