cmake_minimum_required(VERSION 3.16)
project(reading_goal CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)                 # httplib 需要线程库
find_package(nlohmann_json REQUIRED)

add_library(core src/streak.cpp)                 # 公共逻辑编译成一个库
target_compile_options(core PRIVATE -Wall -Wextra)

add_executable(reading_goal src/main.cpp)
target_include_directories(reading_goal PRIVATE third_party)
target_link_libraries(reading_goal PRIVATE core Threads::Threads nlohmann_json::nlohmann_json)

include(CTest)
if(BUILD_TESTING)
    add_executable(streak_tests tests/streak_tests.cpp)
    target_include_directories(streak_tests PRIVATE src)
    target_link_libraries(streak_tests PRIVATE core)
    add_test(NAME streak_tests COMMAND streak_tests)
endif()
