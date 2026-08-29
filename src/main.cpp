#include <iostream>
#include "Switch.hpp"

void print_result(const std::string& label, const ForwardResult& r) {
    std::cout << label << " -> decision=";
    switch (r.decision) {
        case ForwardDecision::UNICAST_HIT:           std::cout << "UNICAST_HIT"; break;
        case ForwardDecision::FLOOD_UNKNOWN_UNICAST:  std::cout << "FLOOD_UNKNOWN_UNICAST"; break;
        case ForwardDecision::FLOOD_BROADCAST:        std::cout << "FLOOD_BROADCAST"; break;
        case ForwardDecision::DROP_SAME_PORT:         std::cout << "DROP_SAME_PORT"; break;
    }
    std::cout << ", out_ports=[";
    for (size_t i = 0; i < r.out_ports.size(); ++i) {
        if (i != 0) std::cout << ",";
        std::cout << r.out_ports[i];
    }
    std::cout << "]" << std::endl;
}

int main() {
    MacAddress macA{{0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
    MacAddress macB{{0x02, 0x00, 0x00, 0x00, 0x00, 0x02}};
    MacAddress broadcast{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    Switch sw(/*num_ports=*/4, /*aging_ttl=*/1000);

    // Frame 1: A (port 0) -> B, B unknown -> should flood unknown unicast
    Frame f1{macA, macB, /*ingress_port=*/0, /*timestamp=*/0, /*size_bytes=*/64};
    print_result("f1 (A->B, B unknown)", sw.process_frame(f1));
    std::cout << "table size: " << sw.table_size() << std::endl;

    // Frame 2: B (port 1) -> A, A is now known (learned from f1) on port 0 -> unicast hit
    Frame f2{macB, macA, /*ingress_port=*/1, /*timestamp=*/1, /*size_bytes=*/64};
    print_result("f2 (B->A, A known on port 0)", sw.process_frame(f2));

    // Frame 3: A (port 0) -> B, B now known on port 1 -> unicast hit
    Frame f3{macA, macB, /*ingress_port=*/0, /*timestamp=*/2, /*size_bytes=*/64};
    print_result("f3 (A->B, B known on port 1)", sw.process_frame(f3));

    // Frame 4: someone on port 0 sends to B, but B is on port 0 (same as ingress) -> drop
    Frame f4{macA, macB, /*ingress_port=*/1, /*timestamp=*/3, /*size_bytes=*/64};
    // Note: B is currently known on port 1 (learned in f2's reverse... let's construct a clean same-port case:
    Frame f4b{macB, macB, /*ingress_port=*/1, /*timestamp=*/4, /*size_bytes=*/64};
    // src==dst is unusual for a real network, so instead test properly: send FROM port1 TO a mac known on port1
    // B is known on port 1 (from f2). Send a frame arriving on port 1 destined for B itself:
    Frame f4c{macA, macB, /*ingress_port=*/1, /*timestamp=*/5, /*size_bytes=*/64};
    print_result("f4c (dst B known on port1, arrives on port1 -> drop)", sw.process_frame(f4c));

    // Frame 5: broadcast
    Frame f5{macA, broadcast, /*ingress_port=*/0, /*timestamp=*/6, /*size_bytes=*/64};
    print_result("f5 (broadcast)", sw.process_frame(f5));

    return 0;
}