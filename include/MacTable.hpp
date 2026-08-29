#pragma once

#include <unordered_map>
#include <optional>
#include <cstdint>
#include "MacAddress.hpp"
#include "MacTableEntry.hpp"

class MacTable {
public:
    explicit MacTable(uint64_t aging_ttl) : aging_ttl_(aging_ttl) {}

    void learn(const MacAddress& mac, unsigned int port, uint64_t timestamp) {
        table_[mac] = MacTableEntry{port, timestamp};
    }

    std::optional<unsigned int> lookup(const MacAddress& mac, uint64_t current_time) {
        auto it = table_.find(mac);
        if (it == table_.end()) {
            return std::nullopt;
        }
        if (current_time - it->second.last_seen > aging_ttl_) {
            table_.erase(it);
            return std::nullopt;
        }
        return it->second.port;
    }

    unsigned int age_out(uint64_t current_time) {
        unsigned int evicted = 0;
        for (auto it = table_.begin(); it != table_.end(); ) {
            if (current_time - it->second.last_seen > aging_ttl_) {
                it = table_.erase(it);
                ++evicted;
            } else {
                ++it;
            }
        }
        return evicted;
    }

    size_t size() const {
        return table_.size();
    }

private:
    std::unordered_map<MacAddress, MacTableEntry> table_;
    uint64_t aging_ttl_;
};