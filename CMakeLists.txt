cmake_minimum_required(VERSION 3.21)
project(PocketEngineer VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(PE_ENABLE_IPO "Enable link-time optimization for Release builds when supported" ON)
option(PE_STATIC_LINUX "Link Linux desktop executables without a host ELF loader or shared C/C++ runtime" OFF)
if(PE_STATIC_LINUX AND (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID OR EMSCRIPTEN))
    message(FATAL_ERROR "PE_STATIC_LINUX is only for native Linux desktop builds")
endif()

# MSVC otherwise reads UTF-8 source literals using the active Windows code page.
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_library(pocket_engineer_core src/engine.cpp src/catalog.cpp src/request.cpp src/polynomial.cpp src/input.cpp
    src/workbench_json.cpp src/workbench.cpp src/circuit_network.cpp src/circuit_editor.cpp
    src/kmap_workbench.cpp src/state_machine.cpp src/signals.cpp src/digital_signals.cpp
    src/engineering_catalog.cpp)
target_include_directories(pocket_engineer_core PUBLIC include)
if(MSVC)
    target_compile_options(pocket_engineer_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(pocket_engineer_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()
set_target_properties(pocket_engineer_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

function(pe_configure_target target)
    get_target_property(pe_target_type ${target} TYPE)
    if(PE_STATIC_LINUX AND pe_target_type STREQUAL "EXECUTABLE")
        target_link_options(${target} PRIVATE -static)
    endif()
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
    # Native/JNI builds retain -O3. For the downloaded WASM engine, optimize
    # instruction size and cold-start cost; bounded algorithms keep UI work off
    # the main thread. Browser stress tests record throughput separately.
    target_compile_options(pocket_engineer_core PRIVATE -Oz)
    target_link_options(pocket-engineer-web PRIVATE
        --no-entry -Oz -fexceptions -sDISABLE_EXCEPTION_CATCHING=0 -sMODULARIZE=1
        -sEXPORT_NAME=createPocketEngineer -sENVIRONMENT=web,worker,node
        -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=134217728 -sSTACK_SIZE=1048576
        "-sEXPORTED_FUNCTIONS=['_pe_solve_json','_pe_identify_json','_pe_catalog_json','_pe_workbench_json','_pe_free_string']"
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
        target_link_libraries(pocket-engineer-server PRIVATE ws2_32 shell32)
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
    add_executable(pe-input-tests tests/input_tests.cpp)
    target_link_libraries(pe-input-tests PRIVATE pocket_engineer_core)
    pe_configure_target(pe-input-tests)
    add_test(NAME natural-input COMMAND pe-input-tests)
    set_tests_properties(natural-input PROPERTIES TIMEOUT 180)
    add_executable(pe-workbench-tests tests/workbench_tests.cpp)
    target_link_libraries(pe-workbench-tests PRIVATE pocket_engineer_core)
    pe_configure_target(pe-workbench-tests)
    add_test(NAME engineering-workbench COMMAND pe-workbench-tests)
    set_tests_properties(engineering-workbench PROPERTIES TIMEOUT 180)
    add_executable(pe-engineering-studies-tests tests/engineering_studies_tests.cpp)
    target_link_libraries(pe-engineering-studies-tests PRIVATE pocket_engineer_core)
    pe_configure_target(pe-engineering-studies-tests)
    add_test(NAME engineering-studies COMMAND pe-engineering-studies-tests)
    set_tests_properties(engineering-studies PROPERTIES TIMEOUT 180)

    include(GNUInstallDirs)
    install(TARGETS pocket-engineer pocket-engineer-server pe-generate-tests pe-verify-corpus pe-generate-explanations
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY www/ DESTINATION ${CMAKE_INSTALL_DATADIR}/pocket-engineer/www PATTERN "downloads" EXCLUDE)
    install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
    install(FILES packaging/START_HERE.txt DESTINATION .)
    if(WIN32)
        install(PROGRAMS packaging/Start-Pocket-Engineer.cmd DESTINATION .)
    elseif(APPLE)
        install(PROGRAMS packaging/Start-Pocket-Engineer.command DESTINATION .)
    else()
        install(PROGRAMS packaging/Start-Pocket-Engineer.sh DESTINATION .)
    endif()
    install(DIRECTORY docs/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/docs)

    set(CPACK_PACKAGE_NAME "PocketEngineer")
    set(CPACK_PACKAGE_VENDOR "Pocket Engineer")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Offline verified engineering tutor")
    set(CPACK_GENERATOR "ZIP;TGZ")
    include(CPack)
endif()
