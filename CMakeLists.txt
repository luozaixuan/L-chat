cmake_minimum_required(VERSION 3.10)
project(L-chat VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 公共源文件
set(COMMON_SOURCES
    src/config_parser.cpp
    src/chat_storage.cpp
)

# 检测操作系统
if(WIN32)
    # Windows 配置
    set(PLATFORM_LIBS ws2_32)
    set(EXE_EXT .exe)
else()
    # Linux 配置
    set(PLATFORM_LIBS pthread)
    set(EXE_EXT "")
endif()

# 创建存储和配置目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/storage)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/config)

# 编译服务器
add_executable(server${EXE_EXT}
    ${COMMON_SOURCES}
    src/server/server.cpp
)
target_link_libraries(server${EXE_EXT} ${PLATFORM_LIBS})

# 编译客户端守护进程
add_executable(client-deamon${EXE_EXT}
    ${COMMON_SOURCES}
    src/client-deamon/client-deamon.cpp
)
target_link_libraries(client-deamon${EXE_EXT} ${PLATFORM_LIBS})

# 编译客户端界面
add_executable(client-ui${EXE_EXT}
    src/config_parser.cpp
    src/client-ui/client-ui.cpp
)
target_link_libraries(client-ui${EXE_EXT} ${PLATFORM_LIBS})

# 设置输出目录
set_target_properties(server${EXE_EXT} client-deamon${EXE_EXT} client-ui${EXE_EXT}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装目标
install(TARGETS server${EXE_EXT} client-deamon${EXE_EXT} client-ui${EXE_EXT}
    RUNTIME DESTINATION bin
)

# 复制配置文件
install(DIRECTORY config/ DESTINATION config)

# 打印编译信息
message(STATUS "Operating System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform Libraries: ${PLATFORM_LIBS}")
