#pragma once

#include <cstdint>

struct MacTableEntry {
    unsigned int port;
    uint64_t last_seen;    // same timestamp unit as Frame::timestamp
};