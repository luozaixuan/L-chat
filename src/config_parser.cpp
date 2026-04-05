#include "config_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

bool ConfigParser::load(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string current_section;

    while (std::getline(file, line)) {
        // 去除首尾空格
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // 跳过空行和注释
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 处理section
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            // 去除section中的空格
            current_section.erase(std::remove(current_section.begin(), current_section.end(), ' '), current_section.end());
            continue;
        }

        // 处理键值对
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);

            // 去除键和值中的空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // 去除值中的引号
            if ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\'')) {
                value = value.substr(1, value.size() - 2);
            }

            // 构建完整的键名
            std::string full_key = current_section.empty() ? key : current_section + "." + key;
            configs[full_key] = value;
        }
    }

    file.close();
    return true;
}

std::string ConfigParser::get(const std::string& key, const std::string& default_val) const {
    auto it = configs.find(key);
    return it != configs.end() ? it->second : default_val;
}

int ConfigParser::get_int(const std::string& key, int default_val) const {
    auto it = configs.find(key);
    if (it != configs.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return default_val;
        }
    }
    return default_val;
}

std::vector<std::string> ConfigParser::get_keys(const std::string& prefix) const {
    std::vector<std::string> keys;
    std::string full_prefix = prefix.empty() ? "" : prefix + ".";
    size_t prefix_len = full_prefix.size();

    for (const auto& pair : configs) {
        const std::string& key = pair.first;
        if (key.substr(0, prefix_len) == full_prefix) {
            // 提取不带前缀的键名
            std::string sub_key = key.substr(prefix_len);
            // 确保只提取直接子键（不包含更深层次的子键）
            if (sub_key.find('.') == std::string::npos) {
                keys.push_back(sub_key);
            }
        }
    }

    return keys;
}
