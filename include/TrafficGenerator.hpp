#pragma once

#include <vector>
#include <random>
#include <deque>
#include "Frame.hpp"

class TrafficGenerator {
public:
    // num_hosts: how many distinct MACs exist in the simulated network
    // num_ports: hosts are assigned round-robin to ports 0..num_ports-1
    // locality: probability (0.0-1.0) the destination is drawn from recently-seen hosts
    // recent_window: how many recent sources count as "local"
    // seed: for reproducible runs
    TrafficGenerator(unsigned int num_hosts, unsigned int num_ports,
                      double locality, size_t recent_window, unsigned int seed)
        : num_ports_(num_ports), locality_(locality), recent_window_(recent_window),
          rng_(seed), uniform01_(0.0, 1.0)
    {
        for (unsigned int i = 0; i < num_hosts; ++i) {
            hosts_.push_back(make_mac(i));
            host_ports_.push_back(i % num_ports); // round-robin port assignment
        }
        host_index_dist_ = std::uniform_int_distribution<unsigned int>(0, num_hosts - 1);
    }

    // Generate the next frame at the given timestamp
    Frame next_frame(uint64_t timestamp, unsigned int size_bytes = 64) {
        unsigned int src_idx = host_index_dist_(rng_);
        unsigned int dst_idx = choose_destination(src_idx);

        Frame f{hosts_[src_idx], hosts_[dst_idx], host_ports_[src_idx], timestamp, size_bytes};

        // Track this source as "recently active" for future locality-based picks
        recent_hosts_.push_back(src_idx);
        if (recent_hosts_.size() > recent_window_) {
            recent_hosts_.pop_front();
        }

        return f;
    }

private:
    static MacAddress make_mac(unsigned int host_id) {
        MacAddress mac{};
        mac.bytes[0] = 0x02; // locally administered, unicast (I/G bit clear)
        mac.bytes[5] = static_cast<uint8_t>(host_id & 0xFF);
        mac.bytes[4] = static_cast<uint8_t>((host_id >> 8) & 0xFF);
        return mac;
    }

    unsigned int choose_destination(unsigned int src_idx) {
        if (!recent_hosts_.empty() && uniform01_(rng_) < locality_) {
            // pick from recently active hosts (excluding src itself if possible)
            std::uniform_int_distribution<size_t> pick(0, recent_hosts_.size() - 1);
            unsigned int candidate = recent_hosts_[pick(rng_)];
            if (candidate != src_idx || recent_hosts_.size() == 1) {
                return candidate;
            }
        }
        // fallback: uniform random across all hosts (excluding src)
        unsigned int dst_idx;
        do {
            dst_idx = host_index_dist_(rng_);
        } while (dst_idx == src_idx && hosts_.size() > 1);
        return dst_idx;
    }

    std::vector<MacAddress> hosts_;
    std::vector<unsigned int> host_ports_;
    unsigned int num_ports_;
    double locality_;
    size_t recent_window_;
    std::deque<unsigned int> recent_hosts_;

    std::mt19937 rng_;
    std::uniform_real_distribution<double> uniform01_;
    std::uniform_int_distribution<unsigned int> host_index_dist_;
};
