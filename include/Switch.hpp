#pragma once

#include <vector>
#include <cstdint>
#include "Frame.hpp"
#include "MacTable.hpp"

enum class ForwardDecision {
    UNICAST_HIT,
    FLOOD_UNKNOWN_UNICAST,
    FLOOD_BROADCAST,
    DROP_SAME_PORT
};

struct ForwardResult {
    ForwardDecision decision;
    std::vector<unsigned int> out_ports;
};

class Switch {
public:
    Switch(unsigned int num_ports, uint64_t aging_ttl)
        : num_ports_(num_ports), table_(aging_ttl) {}

    ForwardResult process_frame(const Frame& frame) {
        // Always learn the source first
        table_.learn(frame.src_mac, frame.ingress_port, frame.timestamp);

        // Broadcast/multicast always floods
        if (frame.dst_mac.is_broadcast() || frame.dst_mac.is_multicast()) {
            return ForwardResult{ForwardDecision::FLOOD_BROADCAST, all_ports_except(frame.ingress_port)};
        }

        auto maybe_port = table_.lookup(frame.dst_mac, frame.timestamp);

        if (!maybe_port.has_value()) {
            return ForwardResult{ForwardDecision::FLOOD_UNKNOWN_UNICAST, all_ports_except(frame.ingress_port)};
        }

        if (*maybe_port == frame.ingress_port) {
            return ForwardResult{ForwardDecision::DROP_SAME_PORT, {}};
        }

        return ForwardResult{ForwardDecision::UNICAST_HIT, {*maybe_port}};
    }

    size_t table_size() const {
        return table_.size();
    }
        unsigned int run_aging_sweep(uint64_t current_time) {
        return table_.age_out(current_time);
    }
private:
    std::vector<unsigned int> all_ports_except(unsigned int excluded) const {
        std::vector<unsigned int> ports;
        for (unsigned int p = 0; p < num_ports_; ++p) {
            if (p != excluded) {
                ports.push_back(p);
            }
        }
        return ports;
    }

    unsigned int num_ports_;
    MacTable table_;
};