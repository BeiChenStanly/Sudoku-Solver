/**
 * @file test_standard_sudoku.cpp
 * @brief Tests for standard Sudoku solving
 */

#include <gtest/gtest.h>
#include "SudokuSolver.h"
#include "SudokuParser.h"

using namespace sudoku;

class StandardSudokuTest : public ::testing::Test
{
protected:
    SudokuSolver solver;
};

// Test: Simple puzzle with many givens
TEST_F(StandardSudokuTest, SimplePuzzle)
{
    // A simple Sudoku puzzle
    std::string puzzle =
        "530070000"
        "600195000"
        "098000060"
        "800060003"
        "400803001"
        "700020006"
        "060000280"
        "000419005"
        "000080079";

    auto parsed = SudokuParser::parseSimpleGrid(puzzle);
    auto solution = solver.solve(parsed);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(parsed, solution));
}

// Test: Empty grid (should find a solution)
TEST_F(StandardSudokuTest, EmptyGrid)
{
    std::string puzzle =
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000";

    auto parsed = SudokuParser::parseSimpleGrid(puzzle);
    auto solution = solver.solve(parsed);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(parsed, solution));
}

// Test: World's hardest Sudoku (by Arto Inkala)
TEST_F(StandardSudokuTest, HardPuzzle)
{
    std::string puzzle =
        "800000000"
        "003600000"
        "070090200"
        "050007000"
        "000045700"
        "000100030"
        "001000068"
        "008500010"
        "090000400";

    auto parsed = SudokuParser::parseSimpleGrid(puzzle);
    auto solution = solver.solve(parsed);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(parsed, solution));
}

// Test: Another difficult puzzle
TEST_F(StandardSudokuTest, DifficultPuzzle)
{
    std::string puzzle =
        "100007090"
        "030020008"
        "009600500"
        "005300900"
        "010080002"
        "600004000"
        "300000010"
        "040000007"
        "007000300";

    auto parsed = SudokuParser::parseSimpleGrid(puzzle);
    auto solution = solver.solve(parsed);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(parsed, solution));
}

// Test: Invalid puzzle (no solution)
TEST_F(StandardSudokuTest, InvalidPuzzle)
{
    // Two 5s in the first row - no solution possible
    std::string puzzle =
        "550000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000";

    auto parsed = SudokuParser::parseSimpleGrid(puzzle);
    auto solution = solver.solve(parsed);

    EXPECT_FALSE(solution.solved);
}

// Test: Parse with dots
TEST_F(StandardSudokuTest, ParseWithDots)
{
    std::string puzzle =
        "53..7...."
        "6..195..."
        ".98....6."
        "8...6...3"
        "4..8.3..1"
        "7...2...6"
        ".6....28."
        "...419..5"
        "....8..79";

    auto parsed = SudokuParser::parseSimpleGrid(puzzle);
    auto solution = solver.solve(parsed);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(parsed, solution));
}

// Test: Verify solution respects given values
TEST_F(StandardSudokuTest, RespectGivenValues)
{
    std::string puzzle =
        "530070000"
        "600195000"
        "098000060"
        "800060003"
        "400803001"
        "700020006"
        "060000280"
        "000419005"
        "000080079";

    auto parsed = SudokuParser::parseSimpleGrid(puzzle);
    auto solution = solver.solve(parsed);

    ASSERT_TRUE(solution.solved);

    // Check that all given values are preserved
    for (int row = 0; row < GRID_SIZE; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {
            int given = parsed.grid[row][col];
            if (given != EMPTY_CELL)
            {
                EXPECT_EQ(solution.grid[row][col], given)
                    << "Given value at (" << row << "," << col << ") not preserved";
            }
        }
    }
}

// Test: Multi-line format parsing
TEST_F(StandardSudokuTest, MultiLineFormat)
{
    std::string puzzle = R"(
        5 3 . . 7 . . . .
        6 . . 1 9 5 . . .
        . 9 8 . . . . 6 .
        8 . . . 6 . . . 3
        4 . . 8 . 3 . . 1
        7 . . . 2 . . . 6
        . 6 . . . . 2 8 .
        . . . 4 1 9 . . 5
        . . . . 8 . . 7 9
    )";

    auto parsed = SudokuParser::parseFromString(puzzle);
    auto solution = solver.solve(parsed);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(parsed, solution));
}

// Test: Performance - solve multiple puzzles
TEST_F(StandardSudokuTest, PerformanceMultiplePuzzles)
{
    std::vector<std::string> puzzles = {
        "530070000600195000098000060800060003400803001700020006060000280000419005000080079",
        "800000000003600000070090200050007000000045700000100030001000068008500010090000400",
        "100007090030020008009600500005300900010080002600004000300000010040000007007000300"};

    for (const auto &puzzle : puzzles)
    {
        auto parsed = SudokuParser::parseSimpleGrid(puzzle);
        auto solution = solver.solve(parsed);

        ASSERT_TRUE(solution.solved);
        EXPECT_TRUE(SudokuSolver::verifySolution(parsed, solution));
        EXPECT_LT(solution.solveTimeMs, 1000.0) << "Solve time exceeds 1 second";
    }
}
