#ifndef CHAT_STORAGE_H
#define CHAT_STORAGE_H

#include <string>
#include <vector>

struct ChatMessage {
    int id;
    std::string user;
    std::string message;
    std::string timestamp;
    std::string room_name;
};

class ChatStorage {
private:
    std::string file_path;
    int next_id;

public:
    ChatStorage(const std::string& path);
    bool save_message(const std::string& user, const std::string& message, const std::string& room_name);
    std::vector<ChatMessage> load_messages();
    int get_next_id();
    void set_next_id(int id);
    void clear_messages();
};

#endif // CHAT_STORAGE_H
