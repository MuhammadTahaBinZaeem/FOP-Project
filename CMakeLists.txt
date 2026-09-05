cmake_minimum_required(VERSION 3.21)
project(PocketEngineer VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(PE_ENABLE_IPO "Enable link-time optimization for Release builds when supported" ON)

# MSVC otherwise reads UTF-8 source literals using the active Windows code page.
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_library(pocket_engineer_core src/engine.cpp src/catalog.cpp src/request.cpp src/polynomial.cpp)
target_include_directories(pocket_engineer_core PUBLIC include)
if(MSVC)
    target_compile_options(pocket_engineer_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(pocket_engineer_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()
set_target_properties(pocket_engineer_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

function(pe_configure_target target)
    if(MSVC)
        target_compile_options(${target} PRIVATE "$<$<CONFIG:Release>:/O2>")
    else()
        target_compile_options(${target} PRIVATE
            "$<$<CONFIG:Release>:-O3>"
            "$<$<CONFIG:Release>:-ffunction-sections>"
            "$<$<CONFIG:Release>:-fdata-sections>")
        if(UNIX AND NOT APPLE)
            target_link_options(${target} PRIVATE "$<$<CONFIG:Release>:-Wl,--gc-sections>")
        endif()
    endif()
endfunction()

pe_configure_target(pocket_engineer_core)

if(PE_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT pe_ipo_supported OUTPUT pe_ipo_error LANGUAGES CXX)
    if(pe_ipo_supported)
        set_property(TARGET pocket_engineer_core PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    else()
        message(STATUS "Release IPO disabled: ${pe_ipo_error}")
    endif()
endif()

if(EMSCRIPTEN)
    add_executable(pocket-engineer-web apps/wasm.cpp)
    target_link_libraries(pocket-engineer-web PRIVATE pocket_engineer_core)
    target_compile_options(pocket_engineer_core PRIVATE -fexceptions)
    target_link_options(pocket-engineer-web PRIVATE
        --no-entry -fexceptions -sDISABLE_EXCEPTION_CATCHING=0 -sMODULARIZE=1
        -sEXPORT_NAME=createPocketEngineer -sENVIRONMENT=web,worker,node
        -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=134217728 -sSTACK_SIZE=1048576
        "-sEXPORTED_FUNCTIONS=['_pe_solve_json','_pe_identify_json','_pe_catalog_json','_pe_free_string']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','UTF8ToString']")
    set_target_properties(pocket-engineer-web PROPERTIES OUTPUT_NAME engine SUFFIX .js
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web")
endif()

if(NOT ANDROID AND NOT EMSCRIPTEN)
    add_executable(pocket-engineer apps/cli.cpp)
    target_link_libraries(pocket-engineer PRIVATE pocket_engineer_core)
    pe_configure_target(pocket-engineer)

    add_executable(pocket-engineer-server apps/server.cpp)
    target_link_libraries(pocket-engineer-server PRIVATE pocket_engineer_core)
    if(WIN32)
        target_link_libraries(pocket-engineer-server PRIVATE ws2_32)
    endif()
    pe_configure_target(pocket-engineer-server)

    add_executable(pe-generate-tests tools/generate_tests.cpp)
    target_link_libraries(pe-generate-tests PRIVATE pocket_engineer_core)
    pe_configure_target(pe-generate-tests)

    add_executable(pe-verify-corpus tools/verify_corpus.cpp)
    target_link_libraries(pe-verify-corpus PRIVATE pocket_engineer_core)
    pe_configure_target(pe-verify-corpus)

    add_executable(pe-generate-explanations tools/generate_explanation_corpus.cpp)
    target_link_libraries(pe-generate-explanations PRIVATE pocket_engineer_core)
    pe_configure_target(pe-generate-explanations)

    enable_testing()
    add_executable(pe-tests tests/engine_tests.cpp)
    target_link_libraries(pe-tests PRIVATE pocket_engineer_core)
    pe_configure_target(pe-tests)
    add_test(NAME engine COMMAND pe-tests)
    add_executable(pe-independent-tests tests/independent_tests.cpp)
    target_link_libraries(pe-independent-tests PRIVATE pocket_engineer_core)
    pe_configure_target(pe-independent-tests)
    add_test(NAME independent COMMAND pe-independent-tests)
    set_tests_properties(independent PROPERTIES TIMEOUT 180)

    include(GNUInstallDirs)
    install(TARGETS pocket-engineer pocket-engineer-server pe-generate-tests pe-verify-corpus pe-generate-explanations
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY www/ DESTINATION ${CMAKE_INSTALL_DATADIR}/pocket-engineer/www)
    install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
    install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs)

    set(CPACK_PACKAGE_NAME "PocketEngineer")
    set(CPACK_PACKAGE_VENDOR "Pocket Engineer")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Offline verified engineering tutor")
    set(CPACK_GENERATOR "ZIP;TGZ")
    include(CPack)
endif()
