# L-chat 跨平台 Makefile
# 支持 Windows (MinGW) 和 Linux

CXX = g++
CXXFLAGS = -std=c++11 -Iinclude -pthread
LDFLAGS = -pthread

# 检测操作系统
ifeq ($(OS),Windows_NT)
    # Windows 配置
    LDFLAGS += -lws2_32
    EXE_EXT = .exe
else
    # Linux 配置
    EXE_EXT =
endif

SRC_DIR = src
INCLUDE_DIR = include

# 源文件
COMMON_SRC = $(SRC_DIR)/config_parser.cpp $(SRC_DIR)/chat_storage.cpp
SERVER_SRC = $(SRC_DIR)/server/server.cpp
DEAMON_SRC = $(SRC_DIR)/client-deamon/client-deamon.cpp
UI_SRC = $(SRC_DIR)/client-ui/client-ui.cpp

# 目标文件
TARGETS = $(SRC_DIR)/server/server$(EXE_EXT) \
          $(SRC_DIR)/client-deamon/client-deamon$(EXE_EXT) \
          $(SRC_DIR)/client-ui/client-ui$(EXE_EXT)

.PHONY: all clean

all: $(TARGETS)

# 编译服务器
$(SRC_DIR)/server/server$(EXE_EXT): $(COMMON_SRC) $(SERVER_SRC)
	@mkdir -p $(SRC_DIR)/server
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "✓ Server compiled successfully"

# 编译客户端守护进程
$(SRC_DIR)/client-deamon/client-deamon$(EXE_EXT): $(COMMON_SRC) $(DEAMON_SRC)
	@mkdir -p $(SRC_DIR)/client-deamon
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "✓ Client-deamon compiled successfully"

# 编译客户端界面
$(SRC_DIR)/client-ui/client-ui$(EXE_EXT): $(SRC_DIR)/config_parser.cpp $(UI_SRC)
	@mkdir -p $(SRC_DIR)/client-ui
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "✓ Client-ui compiled successfully"

clean:
	rm -f $(TARGETS)
	@echo "✓ Cleaned up build files"

# 创建必要的目录
init:
	@mkdir -p storage config
	@echo "✓ Created necessary directories"
