#include "config_parser.h"
#include "chat_storage.h"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <string>
#include <sstream>
#include <chrono>
#include <map>

#define VERSION "1.0.0"

// 跨平台支持
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define CLOSE_SOCKET closesocket
    #define CLEANUP_NETWORK WSACleanup()
    #define INIT_NETWORK() \
        WSADATA wsaData; \
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { \
            std::cerr << "WSAStartup failed" << std::endl; \
            return; \
        }
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <string.h>
    #include <signal.h>
    typedef int socket_t;
    #define INVALID_SOCKET_VAL -1
    #define SOCKET_ERROR_VAL -1
    #define CLOSE_SOCKET close
    #define CLEANUP_NETWORK
    #define INIT_NETWORK()
#endif

// 信号处理

class Server {
private:
    int port;
    std::string room_name;
    std::string room_key;
    socket_t server_socket;
    std::vector<socket_t> clients;
    std::vector<std::string> usernames;
    std::map<std::string, std::string> user_passwords;
    std::mutex clients_mutex;
    ChatStorage storage;

public:
    Server(const std::string& config_file, const std::string& storage_file) : storage(storage_file) {
        ConfigParser config;
        if (!config.load(config_file)) {
            std::cerr << "Failed to load config file" << std::endl;
            exit(1);
        }

        port = config.get_int("port", 8080);
        room_name = config.get("room_name", "Default Room");
        room_key = config.get("room_key", "default_key");
        
        // 加载用户密码映射
        std::vector<std::string> user_keys = config.get_keys("users");
        for (const auto& key : user_keys) {
            std::string user = key;
            std::string password = config.get("users." + key, "");
            if (!user.empty() && !password.empty()) {
                user_passwords[user] = password;
            }
        }
    }

    void start() {
        INIT_NETWORK();

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET_VAL) {
            std::cerr << "Socket creation failed" << std::endl;
            CLEANUP_NETWORK;
            return;
        }

        int opt = 1;
#ifdef _WIN32
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(server_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VAL) {
            std::cerr << "Bind failed" << std::endl;
            CLOSE_SOCKET(server_socket);
            CLEANUP_NETWORK;
            return;
        }

        if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR_VAL) {
            std::cerr << "Listen failed" << std::endl;
            CLOSE_SOCKET(server_socket);
            CLEANUP_NETWORK;
            return;
        }

        std::cout << "Server started on port " << port << std::endl;
        std::cout << "Room: " << room_name << std::endl;

        while (true) {
            socket_t client_socket = accept(server_socket, nullptr, nullptr);
            if (client_socket == INVALID_SOCKET_VAL) {
                #ifdef _WIN32
                    int error = WSAGetLastError();
                    if (error == WSAEINTR) {
                        // 被信号中断，继续循环
                        continue;
                    } else {
                        std::cerr << "Accept failed: " << error << std::endl;
                        // 其他错误，继续循环
                        continue;
                    }
                #else
                    if (errno == EINTR) {
                        // 被信号中断，继续循环
                        continue;
                    } else {
                        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
                        // 其他错误，继续循环
                        continue;
                    }
                #endif
            }

            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.push_back(client_socket);
            }

            std::thread client_thread(&Server::handle_client, this, client_socket);
            client_thread.detach();
        }
    }

    void handle_client(socket_t client_socket) {
        char buffer[1024];
        std::string user;
        std::string password;

        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            std::string received_key(buffer, room_key.size());
            if (received_key != room_key) {
                send(client_socket, "ERROR: Invalid key", 18, 0);
                CLOSE_SOCKET(client_socket);
                return;
            }
            
            // 提取用户名和密码
            int offset = room_key.size();
            if (bytes_received > offset) {
                // 查找用户名和密码的分隔符
                std::string data(buffer + offset, bytes_received - offset);
                size_t sep_pos = data.find('\n');
                if (sep_pos != std::string::npos) {
                    user = data.substr(0, sep_pos);
                    password = data.substr(sep_pos + 1);
                } else {
                    // 可能需要继续接收数据
                    user = data;
                    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
                    if (bytes_received > 0) {
                        password = std::string(buffer, bytes_received);
                    }
                }
            } else {
                // 接收用户名
                bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
                if (bytes_received > 0) {
                    user = std::string(buffer, bytes_received);
                }
                // 接收密码
                bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
                if (bytes_received > 0) {
                    password = std::string(buffer, bytes_received);
                }
            }
        }

        // 验证用户名和密码
        if (user.empty() || password.empty()) {
            send(client_socket, "ERROR: Missing username or password", 33, 0);
            CLOSE_SOCKET(client_socket);
            return;
        }
        
        // 检查用户密码是否正确
        auto it = user_passwords.find(user);
        if (it == user_passwords.end() || it->second != password) {
            send(client_socket, "ERROR: Invalid username or password", 33, 0);
            CLOSE_SOCKET(client_socket);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            auto user_it = std::find(usernames.begin(), usernames.end(), user);
            if (user_it != usernames.end()) {
                send(client_socket, "ERROR: Username already exists", 27, 0);
                CLOSE_SOCKET(client_socket);
                return;
            }
            usernames.push_back(user);
        }

        std::cout << "Client connected: " << user << std::endl;

        std::string join_message = "SYSTEM: " + user + " joined the chat";
        broadcast_message(join_message);
        storage.save_message("SYSTEM", user + " joined the chat", room_name);

        while (true) {
            bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }

            buffer[bytes_received] = '\0';
            std::string message = buffer;

            if (message == "SYNC_HISTORY") {
                // 发送聊天历史记录
                std::vector<ChatMessage> messages = storage.load_messages();
                for (const auto& msg : messages) {
                    std::stringstream history_entry;
                    history_entry << msg.user << ": " << msg.message << "\n";
                    send(client_socket, history_entry.str().c_str(), history_entry.str().size(), 0);
                }
                send(client_socket, "HISTORY_END", 11, 0);
            } else {
                storage.save_message(user, message, room_name);

                std::string broadcast_msg = user + ": " + message;
                broadcast_message(broadcast_msg);
            }
        }

        std::cout << "Client disconnected: " << user << std::endl;

        std::string leave_message = "SYSTEM: " + user + " left the chat";
        broadcast_message(leave_message);
        storage.save_message("SYSTEM", user + " left the chat", room_name);

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            auto it = std::find(clients.begin(), clients.end(), client_socket);
            if (it != clients.end()) {
                clients.erase(it);
            }
            auto user_it = std::find(usernames.begin(), usernames.end(), user);
            if (user_it != usernames.end()) {
                usernames.erase(user_it);
            }
        }

        CLOSE_SOCKET(client_socket);
    }

    void broadcast_message(const std::string& message) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (socket_t client : clients) {
            send(client, message.c_str(), message.size(), 0);
        }
        std::cout << message << std::endl;
    }
};

// 信号处理函数
#ifdef _WIN32
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
    // 忽略控制台控制信号
    return TRUE;
}
#else
void signal_handler(int signum) {
    // 忽略信号
}
#endif

int main(int argc, char* argv[]) {
    // 输出版本信息
    std::cout << "L-chat version: " << VERSION << std::endl;
    
    // 设置信号处理
    #ifdef _WIN32
        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    #else
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGPIPE, signal_handler); // 忽略管道破裂信号
    #endif
    
    std::string config_file = "config/server.toml";
    
    ConfigParser config;
    if (!config.load(config_file)) {
        std::cerr << "Failed to load config file" << std::endl;
        exit(1);
    }
    std::string room_name = config.get("room_name", "Default Room");
    
    std::stringstream storage_file;
    storage_file << "storage/chat_log_" << room_name << ".xml";

    Server server(config_file, storage_file.str());
    server.start();

    return 0;
}
