#include <iostream>
#include "Switch.hpp"
#include "TrafficGenerator.hpp"
#include "TraceReader.hpp"
#include "CliArgs.hpp"
#include "StatsCollector.hpp"

int main(int argc, char** argv) {
    try {
        CliArgs args(argc, argv);

        std::string input_mode = args.get_string("input", "synthetic");
        unsigned int num_ports = args.get_uint("num-ports", 8);
        uint64_t aging_ttl = args.get_uint64("aging-ttl", 1000000);
        uint64_t bucket_size = args.get_uint64("bucket-size", 100000); // 100ms default
        std::string stats_out = args.get_string("stats-out", "data/stats.csv");
        std::string table_evo_out = args.get_string("table-evolution-out", "data/table_evolution.csv");

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
            if (trace_file.empty()) throw std::runtime_error("--trace-file is required when --input=trace");
            frames = load_trace(trace_file, num_ports);
        } else {
            throw std::runtime_error("Unknown --input mode: " + input_mode);
        }

        std::cout << "Loaded " << frames.size() << " frames (mode=" << input_mode << ")" << std::endl;

        StatsCollector stats(bucket_size);
        uint64_t last_bucket_idx = frames.empty() ? 0 : frames.front().timestamp / bucket_size;

        unsigned int hits = 0, floods = 0, drops = 0, broadcasts = 0;
        for (const auto& f : frames) {
            uint64_t bucket_idx = f.timestamp / bucket_size;
            if (bucket_idx != last_bucket_idx) {
                // crossing a bucket boundary: sweep aging and snapshot table state
                unsigned int evicted = sw.run_aging_sweep(f.timestamp);
                stats.record_table_state(static_cast<unsigned int>(sw.table_size()), evicted);
                last_bucket_idx = bucket_idx;
            }

            auto result = sw.process_frame(f);
            stats.observe_frame(f.timestamp, result.decision);

            switch (result.decision) {
                case ForwardDecision::UNICAST_HIT:           ++hits; break;
                case ForwardDecision::FLOOD_UNKNOWN_UNICAST:  ++floods; break;
                case ForwardDecision::FLOOD_BROADCAST:        ++broadcasts; break;
                case ForwardDecision::DROP_SAME_PORT:         ++drops; break;
            }
        }
        // final snapshot + flush
        if (!frames.empty()) {
            unsigned int evicted = sw.run_aging_sweep(frames.back().timestamp);
            stats.record_table_state(static_cast<unsigned int>(sw.table_size()), evicted);
        }
        stats.finalize_remaining();

        stats.write_stats_csv(stats_out);
        stats.write_table_evolution_csv(table_evo_out);

        std::cout << "hits=" << hits << " floods=" << floods
                  << " broadcasts=" << broadcasts << " drops=" << drops << std::endl;
        std::cout << "final table size=" << sw.table_size() << std::endl;
        std::cout << "wrote " << stats_out << " and " << table_evo_out << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}