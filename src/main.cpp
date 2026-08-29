#include <iostream>
#include "Switch.hpp"
#include "TrafficGenerator.hpp"

int main() {
    Switch sw(/*num_ports=*/8, /*aging_ttl=*/100000);
    TrafficGenerator gen(/*num_hosts=*/20, /*num_ports=*/8,
                          /*locality=*/0.8, /*recent_window=*/5, /*seed=*/42);

    unsigned int hits = 0, floods = 0, drops = 0, broadcasts = 0;

    for (uint64_t t = 0; t < 1000; ++t) {
        Frame f = gen.next_frame(t);
        auto result = sw.process_frame(f);
        switch (result.decision) {
            case ForwardDecision::UNICAST_HIT:           ++hits; break;
            case ForwardDecision::FLOOD_UNKNOWN_UNICAST:  ++floods; break;
            case ForwardDecision::FLOOD_BROADCAST:        ++broadcasts; break;
            case ForwardDecision::DROP_SAME_PORT:         ++drops; break;
        }
    }

    std::cout << "hits=" << hits << " floods=" << floods
              << " broadcasts=" << broadcasts << " drops=" << drops << std::endl;
    std::cout << "final table size=" << sw.table_size() << std::endl;

    return 0;
}