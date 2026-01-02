cmake_minimum_required(VERSION 3.14)
project(SudokuSolver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build test suite" ON)
option(STATIC_BINARIES "Link binaries statically" ON)

# Compile definitions for MiniSat
add_definitions(-D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/minisat)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Find ZLIB (optional for MiniSat)
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIR})
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
    DESTINATION include/sudoku
)
