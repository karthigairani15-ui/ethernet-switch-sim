#pragma once

#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include "Switch.hpp"

struct BucketStats {
    uint64_t bucket_index = 0;
    uint64_t bucket_start_time = 0;
    unsigned int frame_count = 0;
    unsigned int hits = 0;
    unsigned int floods_unknown = 0;
    unsigned int floods_broadcast = 0;
    unsigned int drops = 0;
    unsigned int table_size_at_end = 0;
    unsigned int evicted_count = 0;
};

class StatsCollector {
public:
    explicit StatsCollector(uint64_t bucket_size) : bucket_size_(bucket_size) {}

    void observe_frame(uint64_t timestamp, ForwardDecision decision) {
        uint64_t bucket_idx = timestamp / bucket_size_;
        ensure_current_bucket(bucket_idx, timestamp);

        current_.frame_count++;
        switch (decision) {
            case ForwardDecision::UNICAST_HIT:           current_.hits++; break;
            case ForwardDecision::FLOOD_UNKNOWN_UNICAST:  current_.floods_unknown++; break;
            case ForwardDecision::FLOOD_BROADCAST:        current_.floods_broadcast++; break;
            case ForwardDecision::DROP_SAME_PORT:         current_.drops++; break;
        }
    }

    // Call once per bucket boundary from the driver, after an aging sweep
    void record_table_state(unsigned int table_size, unsigned int evicted_count) {
        current_.table_size_at_end = table_size;
        current_.evicted_count += evicted_count;
    }

    void finalize_remaining() {
        if (has_open_bucket_) {
            history_.push_back(current_);
            has_open_bucket_ = false;
        }
    }

    void write_stats_csv(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) throw std::runtime_error("Could not open " + path + " for writing");

        out << "bucket_index,bucket_start_time,frame_count,hits,floods_unknown,"
               "floods_broadcast,drops,flood_ratio,hit_rate,table_size\n";

        for (const auto& b : history_) {
            double flood_ratio = b.frame_count > 0
                ? static_cast<double>(b.floods_unknown + b.floods_broadcast) / b.frame_count
                : 0.0;
            unsigned int hit_denom = b.hits + b.floods_unknown;
            double hit_rate = hit_denom > 0 ? static_cast<double>(b.hits) / hit_denom : 0.0;

            out << b.bucket_index << "," << b.bucket_start_time << "," << b.frame_count << ","
                << b.hits << "," << b.floods_unknown << "," << b.floods_broadcast << ","
                << b.drops << "," << flood_ratio << "," << hit_rate << "," << b.table_size_at_end << "\n";
        }
    }

    void write_table_evolution_csv(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) throw std::runtime_error("Could not open " + path + " for writing");

        out << "bucket_index,bucket_start_time,table_size,evicted_count\n";
        for (const auto& b : history_) {
            out << b.bucket_index << "," << b.bucket_start_time << ","
                << b.table_size_at_end << "," << b.evicted_count << "\n";
        }
    }

private:
    void ensure_current_bucket(uint64_t bucket_idx, uint64_t timestamp) {
        if (!has_open_bucket_) {
            current_ = BucketStats{};
            current_.bucket_index = bucket_idx;
            current_.bucket_start_time = (timestamp / bucket_size_) * bucket_size_;
            has_open_bucket_ = true;
            return;
        }
        if (bucket_idx != current_.bucket_index) {
            history_.push_back(current_);
            current_ = BucketStats{};
            current_.bucket_index = bucket_idx;
            current_.bucket_start_time = (timestamp / bucket_size_) * bucket_size_;
        }
    }

    uint64_t bucket_size_;
    BucketStats current_;
    bool has_open_bucket_ = false;
    std::vector<BucketStats> history_;
};