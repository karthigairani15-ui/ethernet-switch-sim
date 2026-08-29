#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>

class CliArgs {
public:
    CliArgs(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            auto eq_pos = arg.find('=');
            if (arg.substr(0, 2) != "--" || eq_pos == std::string::npos) {
                throw std::runtime_error("Malformed argument: " + arg + " (expected --key=value)");
            }
            std::string key = arg.substr(2, eq_pos - 2);
            std::string value = arg.substr(eq_pos + 1);
            values_[key] = value;
        }
    }

    std::string get_string(const std::string& key, const std::string& default_val) const {
        auto it = values_.find(key);
        return it != values_.end() ? it->second : default_val;
    }

    double get_double(const std::string& key, double default_val) const {
        auto it = values_.find(key);
        return it != values_.end() ? std::stod(it->second) : default_val;
    }

    unsigned int get_uint(const std::string& key, unsigned int default_val) const {
        auto it = values_.find(key);
        return it != values_.end() ? static_cast<unsigned int>(std::stoul(it->second)) : default_val;
    }

    uint64_t get_uint64(const std::string& key, uint64_t default_val) const {
        auto it = values_.find(key);
        return it != values_.end() ? std::stoull(it->second) : default_val;
    }

    bool has(const std::string& key) const {
        return values_.find(key) != values_.end();
    }

private:
    std::unordered_map<std::string, std::string> values_;
};