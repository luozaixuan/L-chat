#include "config_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>

bool ConfigParser::load(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 去除空白字符
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // 解析键值对
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = line.substr(0, equals_pos);
            std::string value = line.substr(equals_pos + 1);
            
            // 去除引号
            if (!value.empty() && (value[0] == '"' || value[0] == '\'')) {
                value = value.substr(1, value.size() - 2);
            }
            
            configs[key] = value;
        }
    }

    file.close();
    return true;
}

std::string ConfigParser::get(const std::string& key, const std::string& default_val) const {
    auto it = configs.find(key);
    if (it != configs.end()) {
        return it->second;
    }
    return default_val;
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
