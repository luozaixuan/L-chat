#include "chat_storage.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>

ChatStorage::ChatStorage(const std::string& path) : file_path(path), next_id(1) {
    // 加载现有消息以确定下一个ID
    std::vector<ChatMessage> messages = load_messages();
    if (!messages.empty()) {
        next_id = messages.back().id + 1;
    }
}

bool ChatStorage::save_message(const std::string& user, const std::string& message, const std::string& room_name) {
    // 获取当前时间
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 创建消息
    ChatMessage msg;
    msg.id = next_id++;
    msg.user = user;
    msg.message = message;
    msg.timestamp = timestamp;
    msg.room_name = room_name;

    // 加载现有消息
    std::vector<ChatMessage> messages = load_messages();
    messages.push_back(msg);

    // 写入xml文件
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return false;
    }

    file << "<chat_log>\n";
    for (const auto& m : messages) {
        file << "  <message>\n";
        file << "    <id>" << m.id << "</id>\n";
        file << "    <user>" << m.user << "</user>\n";
        file << "    <message>" << m.message << "</message>\n";
        file << "    <timestamp>" << m.timestamp << "</timestamp>\n";
        file << "    <room_name>" << m.room_name << "</room_name>\n";
        file << "  </message>\n";
    }
    file << "</chat_log>\n";

    file.close();
    return true;
}

std::vector<ChatMessage> ChatStorage::load_messages() {
    std::vector<ChatMessage> messages;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return messages;
    }

    std::string line;
    ChatMessage msg;
    bool in_message = false;
    std::string current_tag;

    while (std::getline(file, line)) {
        // 只去除行首和行尾的空白字符，保留内容中的空格
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);

        if (line == "<message>") {
            in_message = true;
            msg = ChatMessage();
        } else if (line == "</message>") {
            in_message = false;
            messages.push_back(msg);
        } else if (in_message) {
            size_t start = line.find('<') + 1;
            size_t end = line.find('>');
            if (start < end) {
                current_tag = line.substr(start, end - start);
                size_t value_start = end + 1;
                size_t value_end = line.find("</" + current_tag + ">");
                if (value_start < value_end) {
                    std::string value = line.substr(value_start, value_end - value_start);
                    if (current_tag == "id") {
                        msg.id = std::stoi(value);
                    } else if (current_tag == "user") {
                        msg.user = value;
                    } else if (current_tag == "message") {
                        msg.message = value;
                    } else if (current_tag == "timestamp") {
                        msg.timestamp = value;
                    } else if (current_tag == "room_name") {
                        msg.room_name = value;
                    }
                }
            }
        }
    }

    file.close();
    return messages;
}

int ChatStorage::get_next_id() {
    return next_id;
}

void ChatStorage::set_next_id(int id) {
    next_id = id;
}

void ChatStorage::clear_messages() {
    // 清空 XML 文件
    std::ofstream file(file_path);
    if (file.is_open()) {
        file << "<chat_log>\n";
        file << "</chat_log>\n";
        file.close();
    }
    // 重置 next_id 为 1
    next_id = 1;
}
