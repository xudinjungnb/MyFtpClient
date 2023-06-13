cmake_minimum_required(VERSION 3.0)
project(FTPClient)
set(CMAKE_C_FLAGS   "-g")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# CMAKE_CURRENT_SOURCE_DIR是CMakeList.txt所在路径
file(GLOB SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
# 指定源文件或者动态库对应的头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)
# 添加并生成一个可执行程序
add_executable(ftp ${SRC_LIST})
target_link_libraries(ftp m pthread)

