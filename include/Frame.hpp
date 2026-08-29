#pragma once

#include <cstdint>
#include "MacAddress.hpp"

struct Frame {
    MacAddress src_mac;
    MacAddress dst_mac;
    unsigned int ingress_port;
    uint64_t timestamp;     // microseconds since simulation/capture start — stay consistent everywhere
    unsigned int size_bytes;
};