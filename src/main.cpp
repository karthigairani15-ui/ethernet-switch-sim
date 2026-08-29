#include <iostream>
#include "Switch.hpp"
#include "TrafficGenerator.hpp"
#include "TraceReader.hpp"
#include "CliArgs.hpp"

int main(int argc, char** argv) {
    try {
        CliArgs args(argc, argv);

        std::string input_mode = args.get_string("input", "synthetic");
        unsigned int num_ports = args.get_uint("num-ports", 8);
        uint64_t aging_ttl = args.get_uint64("aging-ttl", 1000000); // 1 second default

        Switch sw(num_ports, aging_ttl);

        std::vector<Frame> frames;

        if (input_mode == "synthetic") {
            unsigned int num_hosts = args.get_uint("num-hosts", 20);
            double locality = args.get_double("locality", 0.8);
            unsigned int recent_window = args.get_uint("recent-window", 5);
            unsigned int num_frames = args.get_uint("num-frames", 1000);
            unsigned int seed = args.get_uint("seed", 42);

            TrafficGenerator gen(num_hosts, num_ports, locality, recent_window, seed);
            for (uint64_t t = 0; t < num_frames; ++t) {
                frames.push_back(gen.next_frame(t));
            }
        } else if (input_mode == "trace") {
            std::string trace_file = args.get_string("trace-file", "");
            if (trace_file.empty()) {
                throw std::runtime_error("--trace-file is required when --input=trace");
            }
            frames = load_trace(trace_file, num_ports);
        } else {
            throw std::runtime_error("Unknown --input mode: " + input_mode + " (expected synthetic|trace)");
        }

        std::cout << "Loaded " << frames.size() << " frames (mode=" << input_mode << ")" << std::endl;

        unsigned int hits = 0, floods = 0, drops = 0, broadcasts = 0;
        for (const auto& f : frames) {
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
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}