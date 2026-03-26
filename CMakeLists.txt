cmake_minimum_required(VERSION 3.16)
project(ResourceMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# 源文件
set(SOURCES
    src/monitor.cpp
    src/console_ui.cpp
    src/history.cpp
    src/config.cpp
    src/logger.cpp
    src/main.cpp
)

# Windows特定配置
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    list(APPEND SOURCES)
    # Windows需要的库
    set(EXTRA_LIBS ws2_32 iphlpapi pdh psapi)
else()
    # Linux需要的库
    set(EXTRA_LIBS pthread)
endif()

# 可执行文件
add_executable(resource_monitor ${SOURCES})

# 链接库
target_link_libraries(resource_monitor PRIVATE ${EXTRA_LIBS})

# 安装
install(TARGETS resource_monitor
    RUNTIME DESTINATION bin
)

# 示例程序
add_executable(history_demo examples/history_demo.cpp)
target_link_libraries(history_demo PRIVATE resource_monitor)

add_executable(config_demo examples/config_demo.cpp)
target_link_libraries(config_demo PRIVATE resource_monitor)

# 打印信息
message(STATUS "Building Resource Monitor for: ${CMAKE_SYSTEM_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
