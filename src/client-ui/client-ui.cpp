#include "config_parser.h"
#include <iostream>
#include <thread>
#include <string>

#define VERSION "1.0.0"

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

class ClientUI {
private:
    std::string deamon_addr;
    int deamon_port;
    socket_t deamon_socket;
    bool running;

public:
    ClientUI(const std::string& config_file) : running(true) {
        ConfigParser config;
        if (!config.load(config_file)) {
            std::cerr << "Failed to load config file" << std::endl;
            exit(1);
        }

        deamon_addr = config.get("deamon_addr", "127.0.0.1");
        deamon_port = config.get_int("deamon_port", 8081);
    }

    void start() {
        try {
            if (!connect_to_deamon()) {
                std::cerr << "Failed to connect to client-deamon" << std::endl;
                return;
            }

            std::cout << "Connected to client-deamon" << std::endl;
            
            std::cout << "Getting server info..." << std::endl;
            send(deamon_socket, "GET_SERVER_INFO", 15, 0);
            char buffer[1024];
            int bytes_received = recv(deamon_socket, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                std::cout << "Server info: " << buffer << std::endl;
            }
            
            std::cout << "Loading chat history..." << std::endl;
            send(deamon_socket, "GET_CHAT_HISTORY", 16, 0);
            std::string history_buffer;
            while (true) {
                bytes_received = recv(deamon_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received <= 0) {
                    break;
                }
                buffer[bytes_received] = '\0';
                history_buffer += buffer;
                
                size_t pos;
                while ((pos = history_buffer.find('\n')) != std::string::npos) {
                    std::string line = history_buffer.substr(0, pos);
                    history_buffer.erase(0, pos + 1);
                    
                    if (line == "HISTORY_END") {
                        break;
                    }
                    std::cout << line << std::endl;
                }
                
                if (history_buffer.find("HISTORY_END") != std::string::npos) {
                    break;
                }
            }
            
            std::cout << "Type messages to send, or 'exit' to quit" << std::endl;

            std::thread receive_thread(&ClientUI::receive_messages, this);
            receive_thread.detach();

            std::string message;
            while (running) {
                std::cout << "> ";
                std::getline(std::cin, message);

                if (message == "exit") {
                    running = false;
                    break;
                }

                send(deamon_socket, message.c_str(), message.size(), 0);
            }

            CLOSE_SOCKET(deamon_socket);
            CLEANUP_NETWORK;
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            CLOSE_SOCKET(deamon_socket);
            CLEANUP_NETWORK;
        }
    }

    bool connect_to_deamon() {
        INIT_NETWORK();

        deamon_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (deamon_socket == INVALID_SOCKET_VAL) {
            std::cerr << "Socket creation failed" << std::endl;
            CLEANUP_NETWORK;
            return false;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(deamon_port);
        inet_pton(AF_INET, deamon_addr.c_str(), &addr.sin_addr);

        if (connect(deamon_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VAL) {
            std::cerr << "Connection failed" << std::endl;
            CLOSE_SOCKET(deamon_socket);
            CLEANUP_NETWORK;
            return false;
        }

        return true;
    }

    void receive_messages() {
        char buffer[1024];

        while (running) {
            int bytes_received = recv(deamon_socket, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }

            buffer[bytes_received] = '\0';
            std::string message = buffer;

            std::cout << "\r" << message << std::endl;
            std::cout << "> " << std::flush;
        }

        running = false;
        std::cout << "Disconnected from client-deamon" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    // 输出版本信息
    std::cout << "L-chat version: " << VERSION << std::endl;
    
    std::string config_file = "config/client-ui.toml";
    
    ClientUI ui(config_file);
    ui.start();

    return 0;
}
