/**
 * @file test_uniqueness.cpp
 * @brief Tests for solution uniqueness checking
 */

#include <gtest/gtest.h>
#include "SudokuSolver.h"
#include "SudokuParser.h"

using namespace sudoku;

// Test configuration constants
constexpr double kMaxSolveTimeMs = 5000.0;  // Maximum acceptable solve time in milliseconds

class UniquenessTest : public ::testing::Test
{
protected:
    SudokuSolver solver;
};

// Test: Puzzle with unique solution reports isUnique=true
TEST_F(UniquenessTest, UniqueSolutionReportsTrue)
{
    // A well-constrained puzzle that should have a unique solution
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
    auto solution = solver.solve(parsed, true); // Check uniqueness

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(solution.uniquenessChecked());
    EXPECT_TRUE(solution.isUnique());
    EXPECT_EQ(solution.uniqueness, UniquenessStatus::UNIQUE);
}

// Test: Puzzle with multiple solutions reports isUnique=false
TEST_F(UniquenessTest, MultipleSolutionsReportsFalse)
{
    // An empty grid has many solutions
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
    auto solution = solver.solve(parsed, true); // Check uniqueness

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(solution.uniquenessChecked());
    EXPECT_FALSE(solution.isUnique());
    EXPECT_EQ(solution.uniqueness, UniquenessStatus::NOT_UNIQUE);
}

// Test: Without uniqueness check, status is NOT_CHECKED
TEST_F(UniquenessTest, NoCheckReportsNotChecked)
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
    auto solution = solver.solve(parsed, false); // Don't check uniqueness

    ASSERT_TRUE(solution.solved);
    EXPECT_FALSE(solution.uniquenessChecked());
    EXPECT_EQ(solution.uniqueness, UniquenessStatus::NOT_CHECKED);
}

// Test: Partially filled puzzle with unique solution
TEST_F(UniquenessTest, PartiallyFilledUnique)
{
    // World's hardest Sudoku - known to have unique solution
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
    auto solution = solver.solve(parsed, true);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(solution.uniquenessChecked());
    EXPECT_TRUE(solution.isUnique());
}

// Test: Killer Sudoku with unique solution
TEST_F(UniquenessTest, KillerSudokuUnique)
{
    SudokuPuzzle puzzle;

    // Create a constrained Killer Sudoku with specific cages
    // First row cages that force specific values
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 3));  // 1+2=3
    puzzle.addCage(Cage({{0, 2}, {0, 3}}, 7));  // 3+4=7
    puzzle.addCage(Cage({{0, 4}, {0, 5}}, 11)); // 5+6=11
    puzzle.addCage(Cage({{0, 6}, {0, 7}}, 15)); // 7+8=15
    puzzle.addCage(Cage({{0, 8}}, 9));          // 9

    auto solution = solver.solve(puzzle, true);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(solution.uniquenessChecked());
    // Note: This might or might not be unique depending on other cells
}

// Test: Inequality Sudoku uniqueness check
TEST_F(UniquenessTest, InequalitySudokuUniqueness)
{
    SudokuPuzzle puzzle;

    // Add some inequality constraints
    puzzle.addInequality(InequalityConstraint(Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));
    puzzle.addInequality(InequalityConstraint(Cell(0, 1), Cell(0, 2), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle, true);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(solution.uniquenessChecked());
    // Solution exists, uniqueness depends on constraints
}

// Test: Blocking clause correctly prevents same solution
TEST_F(UniquenessTest, BlockingClauseWorks)
{
    // A puzzle with exactly 2 solutions
    // Minimal puzzle that has two solutions
    std::string puzzle =
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "000000000"
        "123456780"; // Fixed last row except last cell

    auto parsed = SudokuParser::parseSimpleGrid(puzzle);
    
    // First solve without uniqueness check
    auto solution1 = solver.solve(parsed, false);
    ASSERT_TRUE(solution1.solved);

    // Second solve with uniqueness check - should find it's not unique
    auto solution2 = solver.solve(parsed, true);
    ASSERT_TRUE(solution2.solved);
    EXPECT_FALSE(solution2.isUnique());
}

// Test: Invalid puzzle returns no solution (uniqueness check doesn't crash)
TEST_F(UniquenessTest, InvalidPuzzleNoSolution)
{
    // Invalid puzzle - two 5s in first row
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
    auto solution = solver.solve(parsed, true);

    EXPECT_FALSE(solution.solved);
    // Uniqueness status doesn't matter for unsolved puzzles
}

// Test: Performance - uniqueness check doesn't take too long
TEST_F(UniquenessTest, PerformanceUniquenessCheck)
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
    auto solution = solver.solve(parsed, true);

    ASSERT_TRUE(solution.solved);
    // Solve time (including uniqueness check) should be reasonable
    EXPECT_LT(solution.solveTimeMs, kMaxSolveTimeMs) << "Uniqueness check took too long";
}
