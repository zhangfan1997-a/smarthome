# 设置最低的 CMake 版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(smartHomeApp)

# 设置 C++ 编译标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# 手动查找 SQLite3 库和头文件
set(SQLITE3_INCLUDE_DIR /usr/include)
set(SQLITE3_LIBRARIES /usr/lib/x86_64-linux-gnu/libsqlite3.so)

# 包含 SQLite3
include_directories(${SQLITE3_INCLUDE_DIR})

# 包含其他头文件路径
include_directories(${CMAKE_SOURCE_DIR}/src/LogManager)
include_directories(${CMAKE_SOURCE_DIR}/src/Utils)
include_directories(${CMAKE_SOURCE_DIR}/src/ConfigManager)
include_directories(${CMAKE_SOURCE_DIR}/src/ConfigManager/libs)
include_directories(${CMAKE_SOURCE_DIR}/src/DatabaseManager)
include_directories(${CMAKE_SOURCE_DIR}/src/DeviceManager)
include_directories(${CMAKE_SOURCE_DIR}/src/ExpectionManager)
include_directories(${CMAKE_SOURCE_DIR}/src/MultiThreadManager)
include_directories(${CMAKE_SOURCE_DIR}/src/UserManager)
include_directories(${CMAKE_SOURCE_DIR}/src/MenuManager)

# 收集源文件
file(GLOB_RECURSE SRCS
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/ConfigManager/*.cpp
    ${CMAKE_SOURCE_DIR}/src/DatabaseManager/*.cpp
    ${CMAKE_SOURCE_DIR}/src/DeviceManager/*.cpp
    ${CMAKE_SOURCE_DIR}/src/ExpectionManager/*.cpp
    ${CMAKE_SOURCE_DIR}/src/LogManager/*.cpp
    ${CMAKE_SOURCE_DIR}/src/MultiThreadManager/*.cpp
    ${CMAKE_SOURCE_DIR}/src/UserManager/*.cpp
    ${CMAKE_SOURCE_DIR}/src/MenuManager/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/*.cpp
)

# 生成目标可执行文件
add_executable(${PROJECT_NAME} ${SRCS})

# 链接 SQLite3 库
target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})

# 设置 CMake 构建输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 为了兼容 Makefile 中的 'clean' 命令，CMake 提供 'clean' 目标
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_clean.cmake
)

# 清理 CMake 构建目录
include_directories(${CMAKE_SOURCE_DIR})
