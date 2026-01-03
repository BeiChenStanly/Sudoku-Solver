cmake_minimum_required(VERSION 3.14)
project(SudokuSolver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compiler optimizations in Release mode
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Options
option(BUILD_TESTS "Build test suite" ON)
option(STATIC_BINARIES "Link binaries statically" ON)

# Compile definitions for MiniSat
add_definitions(-D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/minisat)
include_directories(${CMAKE_SOURCE_DIR}/src)

message(STATUS "System:${CMAKE_SYSTEM_NAME}")
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_ZLIB=1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_ZLIB=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_ZLIB=1")
endif()

# Find ZLIB (optional for MiniSat)
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    message(STATUS "ZLIB found: ${ZLIB_LIBRARIES}")
    include_directories(${ZLIB_INCLUDE_DIRS})
else()
    message(STATUS "ZLIB not found - MiniSat will be built without gzip support")
endif()

# MiniSat library sources
set(MINISAT_SOURCES
    minisat/minisat/utils/Options.cc
    minisat/minisat/utils/System.cc
    minisat/minisat/core/Solver.cc
    minisat/minisat/simp/SimpSolver.cc
)

# Create MiniSat static library
add_library(minisat STATIC ${MINISAT_SOURCES})
target_include_directories(minisat PUBLIC ${CMAKE_SOURCE_DIR}/minisat)
target_compile_options(minisat PRIVATE -Wno-reserved-user-defined-literal)
if(ZLIB_FOUND)
    target_link_libraries(minisat ${ZLIB_LIBRARY})
endif()

# Sudoku Solver library sources
set(SUDOKU_SOLVER_SOURCES
    src/SudokuTypes.h
    src/SudokuSolver.h
    src/SudokuSolver.cpp
    src/SudokuEncoder.h
    src/SudokuEncoder.cpp
    src/SudokuParser.h
    src/SudokuParser.cpp
    src/SudokuGenerator.h
    src/SudokuGenerator.cpp
)

# Create Sudoku Solver static library
add_library(sudoku_solver STATIC ${SUDOKU_SOLVER_SOURCES})
target_link_libraries(sudoku_solver minisat)
target_include_directories(sudoku_solver PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Main executable
add_executable(sudoku_solve src/main.cpp)
target_link_libraries(sudoku_solve sudoku_solver minisat)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    # Download and include Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # Test executable
    add_executable(sudoku_tests
        tests/test_main.cpp
        tests/test_standard_sudoku.cpp
        tests/test_killer_sudoku.cpp
        tests/test_inequality_sudoku.cpp
        tests/test_mixed_sudoku.cpp
        tests/test_generator.cpp
        tests/test_uniqueness.cpp
    )
    target_link_libraries(sudoku_tests 
        sudoku_solver 
        minisat 
        GTest::gtest 
        GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(sudoku_tests)
endif()

# Installation
install(TARGETS sudoku_solve DESTINATION bin)
install(TARGETS sudoku_solver DESTINATION lib)
install(FILES 
    src/SudokuTypes.h 
    src/SudokuSolver.h 
    src/SudokuEncoder.h 
    src/SudokuParser.h 
    src/SudokuGenerator.h
    DESTINATION include/sudoku
)

# WebAssembly build (only when using Emscripten)
if(EMSCRIPTEN)
    message(STATUS "Building for WebAssembly with Emscripten")
    
    # Disable tests for WASM build
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    
    # WASM module
    add_executable(sudoku_wasm src/wasm_bindings.cpp)
    target_link_libraries(sudoku_wasm sudoku_solver minisat)
    
    # Emscripten-specific flags
    set_target_properties(sudoku_wasm PROPERTIES
        SUFFIX ".js"
    )
    target_link_options(sudoku_wasm PRIVATE
        -lembind
        -sALLOW_MEMORY_GROWTH=1
        -sMODULARIZE=1
        -sEXPORT_NAME=createSudokuModule
        -sENVIRONMENT=web,worker
        -sSINGLE_FILE=0
        -sWASM=1
        -sNO_EXIT_RUNTIME=1
        -O3
    )
    
    # Install WASM files
    install(FILES 
        ${CMAKE_BINARY_DIR}/sudoku_wasm.js
        ${CMAKE_BINARY_DIR}/sudoku_wasm.wasm
        DESTINATION wasm
    )
endif()
