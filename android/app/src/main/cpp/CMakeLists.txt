cmake_minimum_required(VERSION 3.22.1)
project(PocketEngineerAndroid LANGUAGES CXX)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../../.. ${CMAKE_BINARY_DIR}/pocket-engineer-core)

add_library(pocketengineer_jni SHARED jni_bridge.cpp)
target_link_libraries(pocketengineer_jni PRIVATE pocket_engineer_core log)
