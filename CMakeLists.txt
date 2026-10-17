cmake_minimum_required(VERSION 3.16)
project(reading_goal CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)                 # httplib 需要线程库
find_package(nlohmann_json REQUIRED)

add_library(core src/streak.cpp)                 # 纯逻辑（日期、连续天数），不依赖网络
target_include_directories(core PUBLIC src)
target_compile_options(core PRIVATE -Wall -Wextra)

add_library(kavita src/kavita_client.cpp)        # 和 Kavita API 通信
target_include_directories(kavita PUBLIC src third_party)
target_link_libraries(kavita PUBLIC Threads::Threads nlohmann_json::nlohmann_json)
target_compile_options(kavita PRIVATE -Wall -Wextra)

add_executable(reading_goal src/main.cpp)
target_link_libraries(reading_goal PRIVATE core kavita)
target_compile_options(reading_goal PRIVATE -Wall -Wextra)

include(CTest)
if(BUILD_TESTING)
    add_executable(streak_tests tests/streak_tests.cpp)
    target_include_directories(streak_tests PRIVATE src)
    target_link_libraries(streak_tests PRIVATE core)
    add_test(NAME streak_tests COMMAND streak_tests)
endif()
