#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "Switch.hpp"

MacAddress make_mac(uint8_t last_byte) {
    return MacAddress{{0x02, 0x00, 0x00, 0x00, 0x00, last_byte}};
}

TEST_CASE("Fresh table floods unknown unicast") {
    Switch sw(4, 1000);
    MacAddress a = make_mac(0x01);
    MacAddress b = make_mac(0x02);

    Frame f{a, b, /*ingress_port=*/0, /*timestamp=*/0, 64};
    auto result = sw.process_frame(f);

    REQUIRE(result.decision == ForwardDecision::FLOOD_UNKNOWN_UNICAST);
    REQUIRE(result.out_ports == std::vector<unsigned int>{1, 2, 3});
}

TEST_CASE("Learned MAC produces unicast hit") {
    Switch sw(4, 1000);
    MacAddress a = make_mac(0x01);
    MacAddress b = make_mac(0x02);

    sw.process_frame(Frame{a, b, 0, 0, 64});          // learns A on port 0
    auto result = sw.process_frame(Frame{b, a, 1, 1, 64}); // B->A, A known on port 0

    REQUIRE(result.decision == ForwardDecision::UNICAST_HIT);
    REQUIRE(result.out_ports == std::vector<unsigned int>{0});
}

TEST_CASE("Same-port destination is dropped") {
    Switch sw(4, 1000);
    MacAddress a = make_mac(0x01);
    MacAddress b = make_mac(0x02);

    sw.process_frame(Frame{a, b, 0, 0, 64});          // learns A on port 0
    sw.process_frame(Frame{b, a, 1, 1, 64});          // learns B on port 1
    auto result = sw.process_frame(Frame{a, b, 1, 2, 64}); // dst B known on port1, arrives on port1

    REQUIRE(result.decision == ForwardDecision::DROP_SAME_PORT);
    REQUIRE(result.out_ports.empty());
}

TEST_CASE("Broadcast destination always floods") {
    Switch sw(4, 1000);
    MacAddress a = make_mac(0x01);
    MacAddress broadcast{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    auto result = sw.process_frame(Frame{a, broadcast, 0, 0, 64});

    REQUIRE(result.decision == ForwardDecision::FLOOD_BROADCAST);
    REQUIRE(result.out_ports == std::vector<unsigned int>{1, 2, 3});
}

TEST_CASE("Entry expires after aging TTL") {
    Switch sw(4, /*aging_ttl=*/100);
    MacAddress a = make_mac(0x01);
    MacAddress b = make_mac(0x02);

    sw.process_frame(Frame{a, b, 0, 0, 64});           // learns A on port 0 at t=0
    // B->A at t=200, past ttl=100 -> A's entry should have expired -> flood, not hit
    auto result = sw.process_frame(Frame{b, a, 1, 200, 64});

    REQUIRE(result.decision == ForwardDecision::FLOOD_UNKNOWN_UNICAST);
}

TEST_CASE("MAC learned on a new port overwrites the old entry") {
    Switch sw(4, 1000);
    MacAddress a = make_mac(0x01);
    MacAddress b = make_mac(0x02);

    sw.process_frame(Frame{a, b, /*ingress_port=*/0, 0, 64}); // A learned on port 0
    sw.process_frame(Frame{a, b, /*ingress_port=*/2, 1, 64}); // A now arrives on port 2 (moved)

    // B->A should now hit on port 2, not port 0
    auto result = sw.process_frame(Frame{b, a, 1, 2, 64});
    REQUIRE(result.decision == ForwardDecision::UNICAST_HIT);
    REQUIRE(result.out_ports == std::vector<unsigned int>{2});
}