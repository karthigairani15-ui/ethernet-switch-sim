#include <iostream>
#include "MacAddress.hpp"
#include "Frame.hpp"
#include "MacTableEntry.hpp"

int main() {
    MacAddress mac1{{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
    MacAddress mac2{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    std::cout << "mac1 = " << mac1.to_string() << std::endl;
    std::cout << "mac1 is broadcast? " << mac1.is_broadcast() << std::endl;
    std::cout << "mac2 = " << mac2.to_string() << std::endl;
    std::cout << "mac2 is broadcast? " << mac2.is_broadcast() << std::endl;

    Frame f{mac1, mac2, /*ingress_port=*/3, /*timestamp=*/1000, /*size_bytes=*/64};
    std::cout << "Frame src=" << f.src_mac.to_string()
              << " dst=" << f.dst_mac.to_string()
              << " port=" << f.ingress_port << std::endl;

    MacTableEntry entry{5, 2000};
    std::cout << "Entry port=" << entry.port << " last_seen=" << entry.last_seen << std::endl;

    return 0;
}