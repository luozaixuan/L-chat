#include "config_parser.h"
#include "chat_storage.h"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <sstream>
#include <chrono>

// 跨平台支持
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
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
            return false; \
        }
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    typedef int socket_t;
    #define INVALID_SOCKET_VAL -1
    #define SOCKET_ERROR_VAL -1
    #define CLOSE_SOCKET close
    #define CLEANUP_NETWORK
    #define INIT_NETWORK()
#endif

class ClientDeamon {
private:
    std::string server_addr;
    int server_port;
    int local_port;
    std::string username;
    std::string room_key;
    socket_t server_socket;
    socket_t local_server_socket;
    std::vector<socket_t> ui_clients;
    std::mutex ui_clients_mutex;
    ChatStorage storage;
    bool running;
    bool sync_history;

public:
    ClientDeamon(const std::string& config_file, const std::string& storage_file, bool sync) : storage(storage_file), running(true), sync_history(sync) {
        ConfigParser config;
        if (!config.load(config_file)) {
            std::cerr << "Failed to load config file" << std::endl;
            exit(1);
        }

        server_addr = config.get("server_addr", "127.0.0.1");
        server_port = config.get_int("server_port", 8080);
        local_port = config.get_int("local_port", 8081);
        username = config.get("username", "Anonymous");
        room_key = config.get("room_key", "default_key");
    }

    void start() {
        if (!connect_to_server()) {
            std::cerr << "Failed to connect to server" << std::endl;
            return;
        }

        start_local_server();

        std::thread server_thread(&ClientDeamon::receive_server_messages, this);
        server_thread.detach();

        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    bool connect_to_server() {
        INIT_NETWORK();

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET_VAL) {
            std::cerr << "Socket creation failed" << std::endl;
            CLEANUP_NETWORK;
            return false;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_addr.c_str(), &addr.sin_addr);

        if (connect(server_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VAL) {
            std::cerr << "Connection failed" << std::endl;
            CLOSE_SOCKET(server_socket);
            CLEANUP_NETWORK;
            return false;
        }

        send(server_socket, room_key.c_str(), room_key.size(), 0);
        send(server_socket, username.c_str(), username.size(), 0);

        // 检查密钥是否有效
        char buffer[1024];
        int bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string response = buffer;
            if (response == "ERROR: Invalid key") {
                std::cerr << "ERROR: Invalid key" << std::endl;
                CLOSE_SOCKET(server_socket);
                CLEANUP_NETWORK;
                return false;
            }
            if (response == "ERROR: Username already exists") {
                std::cerr << "ERROR: Username already exists" << std::endl;
                CLOSE_SOCKET(server_socket);
                CLEANUP_NETWORK;
                return false;
            }
        }

        std::cout << "Connected to server" << std::endl;

        // 如果需要同步历史记录
        if (sync_history) {
            std::cout << "Clearing previous chat history..." << std::endl;
            storage.clear_messages();
            std::cout << "Syncing chat history from server..." << std::endl;
            send(server_socket, "SYNC_HISTORY", 12, 0);

            // 接收历史记录
            char history_buffer[1024];
            std::string history_data;
            while (true) {
                int history_bytes = recv(server_socket, history_buffer, sizeof(history_buffer), 0);
                if (history_bytes <= 0) {
                    break;
                }
                history_buffer[history_bytes] = '\0';
                history_data += history_buffer;

                if (history_data.find("HISTORY_END") != std::string::npos) {
                    break;
                }
            }

            // 处理历史记录
            size_t end_pos = history_data.find("HISTORY_END");
            if (end_pos != std::string::npos) {
                history_data = history_data.substr(0, end_pos);
                std::istringstream history_stream(history_data);
                std::string line;
                while (std::getline(history_stream, line)) {
                    if (!line.empty()) {
                        size_t colon_pos = line.find(": ");
                        if (colon_pos != std::string::npos) {
                            std::string user = line.substr(0, colon_pos);
                            std::string msg = line.substr(colon_pos + 2);
                            storage.save_message(user, msg, "Default Room");
                            std::cout << line << std::endl;
                        }
                    }
                }
                std::cout << "Chat history synced successfully" << std::endl;
            }
        }

        return true;
    }

    void start_local_server() {
        local_server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (local_server_socket == INVALID_SOCKET_VAL) {
            std::cerr << "Local socket creation failed" << std::endl;
            return;
        }

        int opt = 1;
#ifdef _WIN32
        setsockopt(local_server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(local_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(local_port);

        if (bind(local_server_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VAL) {
            std::cerr << "Local bind failed" << std::endl;
            CLOSE_SOCKET(local_server_socket);
            return;
        }

        if (listen(local_server_socket, SOMAXCONN) == SOCKET_ERROR_VAL) {
            std::cerr << "Local listen failed" << std::endl;
            CLOSE_SOCKET(local_server_socket);
            return;
        }

        std::cout << "Local server started on port " << local_port << std::endl;

        std::thread accept_thread(&ClientDeamon::accept_ui_connections, this);
        accept_thread.detach();
    }

    void accept_ui_connections() {
        while (running) {
            socket_t ui_socket = accept(local_server_socket, nullptr, nullptr);
            if (ui_socket == INVALID_SOCKET_VAL) {
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(ui_clients_mutex);
                ui_clients.push_back(ui_socket);
            }

            std::thread ui_thread(&ClientDeamon::handle_ui_client, this, ui_socket);
            ui_thread.detach();
        }
    }

    void handle_ui_client(socket_t ui_socket) {
        char buffer[1024];

        while (running) {
            int bytes_received = recv(ui_socket, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }

            buffer[bytes_received] = '\0';
            std::string message = buffer;

            if (message == "GET_SERVER_INFO") {
                std::stringstream response;
                response << server_addr << ":" << server_port << ":" << room_key;
                send(ui_socket, response.str().c_str(), response.str().size(), 0);
            } else if (message == "GET_CHAT_HISTORY") {
                std::vector<ChatMessage> messages = storage.load_messages();
                for (const auto& msg : messages) {
                    std::stringstream history_entry;
                    history_entry << msg.timestamp << " " << msg.user << ": " << msg.message << "\n";
                    send(ui_socket, history_entry.str().c_str(), history_entry.str().size(), 0);
                }
                send(ui_socket, "HISTORY_END", 11, 0);
            } else {
                send(server_socket, message.c_str(), message.size(), 0);
            }
        }

        {
            std::lock_guard<std::mutex> lock(ui_clients_mutex);
            auto it = std::find(ui_clients.begin(), ui_clients.end(), ui_socket);
            if (it != ui_clients.end()) {
                ui_clients.erase(it);
            }
        }

        CLOSE_SOCKET(ui_socket);
    }

    void receive_server_messages() {
        char buffer[1024];

        while (running) {
            int bytes_received = recv(server_socket, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }

            buffer[bytes_received] = '\0';
            std::string message = buffer;

            size_t colon_pos = message.find(':');
            if (colon_pos != std::string::npos) {
                std::string user = message.substr(0, colon_pos);
                std::string msg = message.substr(colon_pos + 2);
                storage.save_message(user, msg, "Default Room");
            }

            broadcast_to_ui(message);

            std::cout << message << std::endl;
        }

        std::cout << "Disconnected from server" << std::endl;
        running = false;
    }

    void broadcast_to_ui(const std::string& message) {
        std::lock_guard<std::mutex> lock(ui_clients_mutex);
        for (socket_t ui_client : ui_clients) {
            send(ui_client, message.c_str(), message.size(), 0);
        }
    }
};

int main(int argc, char* argv[]) {
    std::string config_file = "config/client-deamon.toml";
    bool sync_history = false;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--sync-history" || std::string(argv[i]) == "-s") {
            sync_history = true;
        } else if (std::string(argv[i]) == "--config" || std::string(argv[i]) == "-c") {
            if (i + 1 < argc) {
                config_file = argv[i + 1];
                i++;
            }
        }
    }
    
    ConfigParser config;
    if (!config.load(config_file)) {
        std::cerr << "Failed to load config file" << std::endl;
        exit(1);
    }
    std::string server_addr = config.get("server_addr", "127.0.0.1");
    int server_port = config.get_int("server_port", 8080);
    std::string room_key = config.get("room_key", "default_key");
    
    std::stringstream storage_file;
    storage_file << "storage/chat_log_" << server_addr << "_" << server_port << "_" << room_key << ".xml";

    ClientDeamon deamon(config_file, storage_file.str(), sync_history);
    deamon.start();

    return 0;
}
