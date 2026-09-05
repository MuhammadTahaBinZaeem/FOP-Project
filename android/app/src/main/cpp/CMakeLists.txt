cmake_minimum_required(VERSION 3.22.1)
project(PocketEngineerAndroid LANGUAGES CXX)

# The NDK's cross-linker is optimized independently; avoid host IPO probes.
set(PE_ENABLE_IPO OFF CACHE BOOL "Disable IPO for Android NDK builds" FORCE)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../../.. ${CMAKE_BINARY_DIR}/pocket-engineer-core)

add_library(pocketengineer_jni SHARED jni_bridge.cpp)
target_compile_features(pocketengineer_jni PRIVATE cxx_std_20)
target_link_libraries(pocketengineer_jni PRIVATE pocket_engineer_core log)
