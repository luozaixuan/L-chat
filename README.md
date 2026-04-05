# C++ 聊天应用

一个跨平台的 C++ 聊天应用，包含服务器、客户端守护进程和用户界面客户端三个组件。

## 功能特性

- **跨平台支持**：可在 Windows 和 Linux 上运行
- **多客户端支持**：服务器支持多个客户端连接
- **单聊天室**：每个服务器只有一个聊天室
- **聊天历史记录**：使用 XML 文件存储聊天记录
- **历史记录同步**：客户端可以从服务器同步全部聊天记录
- **配置文件**：使用 TOML 配置文件管理设置

## 组件说明

### 1. 服务器 (server)
- 监听指定端口
- 管理聊天室
- 处理客户端连接
- 广播消息
- 存储聊天历史

### 2. 客户端守护进程 (client-deamon)
- 连接到服务器
- 本地启动一个服务器供客户端 UI 连接
- 处理服务器消息和 UI 消息
- 存储聊天历史
- 支持从服务器同步历史记录

### 3. 客户端 UI (client-ui)
- 连接到客户端守护进程
- 提供命令行界面
- 显示聊天消息
- 发送消息

## 安装步骤

### 1. 编译项目

#### Windows
```bash
# 使用 MinGW 编译
$env:PATH += ";D:\gitwork\L-chat\w64devkit\bin"
D:\gitwork\L-chat\w64devkit\bin\g++.exe -std=c++11 -Iinclude src/config_parser.cpp src/chat_storage.cpp src/server/server.cpp -o src\server\server.exe -lws2_32
D:\gitwork\L-chat\w64devkit\bin\g++.exe -std=c++11 -Iinclude src/config_parser.cpp src/chat_storage.cpp src/client-deamon/client-deamon.cpp -o src\client-deamon\client-deamon.exe -lws2_32
D:\gitwork\L-chat\w64devkit\bin\g++.exe -std=c++11 -Iinclude src/config_parser.cpp src/client-ui/client-ui.cpp -o src\client-ui\client-ui.exe -lws2_32
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

# 启动并同步历史记录
./client-deamon.exe -s  # Windows
./client-deamon -s      # Linux
```

### 3. 启动客户端 UI
```bash
cd src/client-ui
./client-ui.exe  # Windows
./client-ui      # Linux
```

## 命令行参数

### 客户端守护进程参数
- `-s` 或 `--sync-history`：启动时从服务器同步全部聊天记录
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
4. 客户端守护进程启动时使用 `-s` 参数可以从服务器同步所有历史记录

## 故障排除

- **连接失败**：检查服务器是否启动，配置文件中的 IP 和端口是否正确
- **密钥错误**：确保客户端和服务器的 `room_key` 一致
- **用户名已存在**：更换一个不同的用户名
- **历史记录同步失败**：确保服务器正在运行，网络连接正常

## 许可证

本项目采用 MIT 许可证。