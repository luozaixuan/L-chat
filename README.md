# C++ 聊天应用

一个跨平台的 C++ 聊天应用，包含服务器、客户端守护进程和用户界面客户端三个组件。

## 功能特性

- **跨平台支持**：可在 Windows 和 Linux 上运行
- **多客户端支持**：服务器支持多个客户端连接
- **单聊天室**：每个服务器只有一个聊天室
- **聊天历史记录**：使用 XML 文件存储聊天记录
- **历史记录同步**：客户端可以从服务器同步全部聊天记录
- **配置文件**：使用 TOML 配置文件管理设置
- **信号处理**：服务器能够在客户端断开连接后继续运行
- **用户名冲突检测**：服务器会拒绝重复的用户名连接

## 组件说明

### 1. 服务器 (server)
- 监听指定端口
- 管理聊天室
- 处理客户端连接
- 广播消息
- 存储聊天历史
- 支持从服务器同步历史记录

### 2. 客户端守护进程 (client-deamon)
- 连接到服务器
- 本地启动一个服务器供客户端 UI 连接
- 处理服务器消息和 UI 消息
- 存储聊天历史
- 支持从服务器同步历史记录（使用 `-s` 参数）
- 同步历史记录前会自动清除本地历史记录

### 3. 客户端 UI (client-ui)
- 连接到客户端守护进程
- 提供命令行界面
- 显示聊天消息
- 发送消息

## 高解耦性设计

本项目采用了高解耦的设计架构，具体表现为：

1. **分层架构**：
   - 服务器层：负责处理网络连接、消息广播和聊天历史存储
   - 客户端守护进程层：负责连接服务器和本地 UI 客户端，处理消息转发
   - UI 层：负责用户交互和界面展示

2. **通信协议**：
   - 客户端守护进程与服务器之间使用简单的文本协议
   - 客户端 UI 与守护进程之间使用简单的文本协议

3. **独立部署**：
   - 三个组件可以独立部署和运行
   - 客户端 UI 可以单独开发和替换

4. **可扩展性**：
   - 支持开发第三方客户端 UI
   - 支持多种 UI 形式（命令行、GUI、Web 等）

## 开发第三方客户端 UI

如果您想开发自己的第三方客户端 UI，只需遵循以下步骤：

### 1. 连接到客户端守护进程

客户端守护进程默认在本地端口 8081 上运行（可在配置文件中修改）。您需要：

- 创建一个 TCP 套接字连接到 `localhost:8081`
- 实现与守护进程的通信协议

### 2. 通信协议

客户端 UI 与守护进程之间的通信协议非常简单：

#### 获取服务器信息
- 发送：`GET_SERVER_INFO`
- 接收：`server_addr:server_port:room_key`（例如：`127.0.0.1:8080:my_secure_key`）

#### 获取聊天历史
- 发送：`GET_CHAT_HISTORY`
- 接收：多条聊天记录，每条格式为 `timestamp user: message`，最后以 `HISTORY_END` 结束

#### 发送消息
- 发送：直接发送消息内容
- 接收：无（消息会被转发到服务器）

### 3. 示例代码

以下是一个简单的 Python 客户端 UI 示例：

```python
import socket

# 连接到客户端守护进程
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(("localhost", 8081))

# 获取服务器信息
sock.send(b"GET_SERVER_INFO")
server_info = sock.recv(1024).decode()
print(f"Server info: {server_info}")

# 获取聊天历史
sock.send(b"GET_CHAT_HISTORY")
history_data = ""
while True:
    data = sock.recv(1024).decode()
    history_data += data
    if "HISTORY_END" in history_data:
        break
history_data = history_data.replace("HISTORY_END", "")
print("Chat history:")
print(history_data)

# 发送消息
while True:
    message = input("Enter message: ")
    sock.send(message.encode())

# 关闭连接
sock.close()
```

### 4. 开发建议

- **界面设计**：根据您的需求设计合适的界面，如命令行、GUI、Web 等
- **错误处理**：添加适当的错误处理，确保客户端 UI 能够优雅处理连接失败等情况
- **消息格式**：遵循守护进程的消息格式，确保与守护进程的通信正常
- **用户体验**：添加消息提示、历史记录显示等功能，提升用户体验

通过这种高解耦的设计，您可以轻松开发自己的客户端 UI，而不需要修改服务器或客户端守护进程的代码。

## 安装步骤

### 1. 编译项目

#### Windows (MinGW)
```bash
# 使用 MinGW 编译
$env:PATH += ";D:\gitwork\L-chat\w64devkit\bin"
D:\gitwork\L-chat\w64devkit\bin\g++.exe -std=c++11 -Iinclude src/config_parser.cpp src/chat_storage.cpp src/server/server.cpp -o src\server\server.exe -lws2_32
D:\gitwork\L-chat\w64devkit\bin\g++.exe -std=c++11 -Iinclude src/config_parser.cpp src/chat_storage.cpp src/client-deamon/client-deamon.cpp -o src\client-deamon\client-deamon.exe -lws2_32
D:\gitwork\L-chat\w64devkit\bin\g++.exe -std=c++11 -Iinclude src/config_parser.cpp src/client-ui/client-ui.cpp -o src\client-ui\client-ui.exe -lws2_32
```

#### Windows (MSVC)
```bash
# 使用 Visual Studio Developer Command Prompt
cd /d D:\gitwork\L-chat
cl /EHsc /Iinclude src/config_parser.cpp src/chat_storage.cpp src/server/server.cpp /link ws2_32.lib /OUT:server.exe
cl /EHsc /Iinclude src/config_parser.cpp src/chat_storage.cpp src/client-deamon/client-deamon.cpp /link ws2_32.lib /OUT:client-deamon.exe
cl /EHsc /Iinclude src/config_parser.cpp src/client-ui/client-ui.cpp /link ws2_32.lib /OUT:client-ui.exe
```

#### Linux
```bash
# 使用 g++ 编译
g++ -std=c++11 -Iinclude src/config_parser.cpp src/chat_storage.cpp src/server/server.cpp -o src/server/server
g++ -std=c++11 -Iinclude src/config_parser.cpp src/chat_storage.cpp src/client-deamon/client-deamon.cpp -o src/client-deamon/client-deamon
g++ -std=c++11 -Iinclude src/config_parser.cpp src/client-ui/client-ui.cpp -o src/client-ui/client-ui
```

### 2. 配置文件

#### 服务器配置 (config/server.toml)
```toml
port = 8080
room_name = "Default Room"
room_key = "my_secure_key"
```

#### 客户端守护进程配置 (config/client-deamon.toml)
```toml
server_addr = "127.0.0.1"
server_port = 8080
local_port = 8081
username = "User1"
room_key = "my_secure_key"
```

#### 客户端 UI 配置 (config/client-ui.toml)
```toml
local_addr = "127.0.0.1"
local_port = 8081
```

## 使用方法

### 1. 启动服务器
```bash
cd src/server
./server.exe  # Windows
./server      # Linux
```

### 2. 启动客户端守护进程
```bash
cd src/client-deamon
# 普通启动
./client-deamon.exe  # Windows
./client-deamon      # Linux

# 启动并同步历史记录（会先清除本地历史记录）
./client-deamon.exe -s  # Windows
./client-deamon -s      # Linux

# 指定配置文件
./client-deamon.exe -c path/to/config.toml  # Windows
./client-deamon -c path/to/config.toml      # Linux
```

### 3. 启动客户端 UI
```bash
cd src/client-ui
./client-ui.exe  # Windows
./client-ui      # Linux
```

## 命令行参数

### 客户端守护进程参数
- `-s` 或 `--sync-history`：启动时从服务器同步全部聊天记录（会先清除本地历史记录）
- `-c` 或 `--config`：指定配置文件路径

## 目录结构

```
L-chat/
├── include/            # 头文件
│   ├── config_parser.h
│   └── chat_storage.h
├── src/                # 源代码
│   ├── config_parser.cpp
│   ├── chat_storage.cpp
│   ├── server/         # 服务器代码
│   │   └── server.cpp
│   ├── client-deamon/  # 客户端守护进程代码
│   │   └── client-deamon.cpp
│   └── client-ui/      # 客户端 UI 代码
│       └── client-ui.cpp
├── config/             # 配置文件
│   ├── server.toml
│   ├── client-deamon.toml
│   └── client-ui.toml
├── storage/            # 聊天记录存储
└── README.md           # 项目说明
```

## 注意事项

1. 确保配置文件中的 `room_key` 与服务器一致，否则客户端无法连接
2. 用户名不能重复，否则会被服务器拒绝连接
3. 聊天历史记录存储在 `storage/` 目录下的 XML 文件中
4. 客户端守护进程启动时使用 `-s` 参数可以从服务器同步所有历史记录，并会先清除本地历史记录
5. 服务器会在客户端断开连接后继续运行，等待新的客户端连接

## 故障排除

- **连接失败**：检查服务器是否启动，配置文件中的 IP 和端口是否正确
- **密钥错误**：确保客户端和服务器的 `room_key` 一致
- **用户名已存在**：更换一个不同的用户名
- **历史记录同步失败**：确保服务器正在运行，网络连接正常
- **服务器退出**：检查是否有其他进程或服务终止了服务器进程

## 许可证

本项目采用 MIT 许可证。