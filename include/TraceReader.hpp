#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include "Frame.hpp"

inline MacAddress parse_mac(const std::string& s) {
    MacAddress mac{};
    unsigned int vals[6];
    if (sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x",
               &vals[0], &vals[1], &vals[2], &vals[3], &vals[4], &vals[5]) != 6) {
        throw std::runtime_error("Malformed MAC address: " + s);
    }
    for (int i = 0; i < 6; ++i) {
        mac.bytes[i] = static_cast<uint8_t>(vals[i]);
    }
    return mac;
}

inline unsigned int infer_port(const MacAddress& mac, unsigned int num_ports) {
    // Stable hash of the MAC -> port, so the same host always lands on the same port
    size_t h = std::hash<MacAddress>{}(mac);
    return static_cast<unsigned int>(h % num_ports);
}

inline std::vector<Frame> load_trace(const std::string& tsv_path, unsigned int num_ports) {
    std::vector<Frame> frames;
    std::ifstream file(tsv_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open trace file: " + tsv_path);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string ts_str, src_str, dst_str, len_str, num_str;
        if (!std::getline(iss, ts_str, '\t')) continue;
        if (!std::getline(iss, src_str, '\t')) continue;
        if (!std::getline(iss, dst_str, '\t')) continue;
        if (!std::getline(iss, len_str, '\t')) continue;
        std::getline(iss, num_str, '\t'); // frame number, unused for now

        if (src_str.empty() || dst_str.empty()) continue; // skip malformed rows

        double ts_seconds = std::stod(ts_str);
        uint64_t ts_micros = static_cast<uint64_t>(ts_seconds * 1'000'000.0);

        MacAddress src = parse_mac(src_str);
        MacAddress dst = parse_mac(dst_str);
        unsigned int len = static_cast<unsigned int>(std::stoul(len_str));
        unsigned int ingress_port = infer_port(src, num_ports);

        frames.push_back(Frame{src, dst, ingress_port, ts_micros, len});
    }

    return frames;
}