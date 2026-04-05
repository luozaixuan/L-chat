#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>

class ConfigParser {
private:
    std::unordered_map<std::string, std::string> configs;

public:
    bool load(const std::string& file_path);
    std::string get(const std::string& key, const std::string& default_val = "") const;
    int get_int(const std::string& key, int default_val = 0) const;
    std::vector<std::string> get_keys(const std::string& prefix) const;
};

#endif // CONFIG_PARSER_H
