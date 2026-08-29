#include <iostream>
#include "MacAddress.hpp"
#include "MacTable.hpp"

int main() {
    MacAddress mac1{{0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};
    MacAddress mac2{{0x11, 0x22, 0x33, 0x44, 0x55, 0x66}};

    MacTable table(/*aging_ttl=*/100);

    // Learn mac1 on port 3 at time 0
    table.learn(mac1, 3, 0);
    std::cout << "size after learning mac1: " << table.size() << std::endl;

    // Lookup mac1 at time 50 — should hit, still within ttl
    auto r1 = table.lookup(mac1, 50);
    std::cout << "lookup mac1 @50: " << (r1.has_value() ? std::to_string(*r1) : "MISS") << std::endl;

    // Lookup mac1 at time 150 — should miss, ttl expired (100), and evict it
    auto r2 = table.lookup(mac1, 150);
    std::cout << "lookup mac1 @150: " << (r2.has_value() ? std::to_string(*r2) : "MISS") << std::endl;
    std::cout << "size after expiry: " << table.size() << std::endl;

    // Lookup mac2 — never learned, should miss
    auto r3 = table.lookup(mac2, 10);
    std::cout << "lookup mac2 @10: " << (r3.has_value() ? std::to_string(*r3) : "MISS") << std::endl;

    // Learn mac1 again, then mac2, test age_out sweep
    table.learn(mac1, 3, 200);
    table.learn(mac2, 7, 205);
    std::cout << "size before sweep: " << table.size() << std::endl;
    unsigned int evicted = table.age_out(400);   // both older than ttl=100 relative to 400
    std::cout << "evicted by sweep: " << evicted << ", size after sweep: " << table.size() << std::endl;

    return 0;
}