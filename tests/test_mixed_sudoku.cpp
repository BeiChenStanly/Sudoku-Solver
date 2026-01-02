/**
 * @file test_mixed_sudoku.cpp
 * @brief Tests for Mixed Sudoku (Killer + Inequality combined)
 */

#include <gtest/gtest.h>
#include "SudokuSolver.h"
#include "SudokuParser.h"

using namespace sudoku;

class MixedSudokuTest : public ::testing::Test
{
protected:
    SudokuSolver solver;
};

// Test: Simple combination of cage and inequality
TEST_F(MixedSudokuTest, SimpleCageAndInequality)
{
    SudokuPuzzle puzzle;

    // Cage: (0,0) + (0,1) = 5
    // Inequality: (0,0) > (0,1)
    // This means: (0,0) = 3, (0,1) = 2 OR (0,0) = 4, (0,1) = 1
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 5));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Verify cage sum
    EXPECT_EQ(solution.grid[0][0] + solution.grid[0][1], 5);
    // Verify inequality
    EXPECT_GT(solution.grid[0][0], solution.grid[0][1]);
}

// Test: Cage with inequality narrowing possibilities
TEST_F(MixedSudokuTest, InequalityNarrowsCage)
{
    SudokuPuzzle puzzle;

    // Cage: (0,0) + (0,1) = 3 (must be 1+2)
    // Inequality: (0,0) < (0,1)
    // Therefore: (0,0) = 1, (0,1) = 2
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 3));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::LESS_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Exact values should be determined
    EXPECT_EQ(solution.grid[0][0], 1);
    EXPECT_EQ(solution.grid[0][1], 2);
}

// Test: Multiple cages with inequalities
TEST_F(MixedSudokuTest, MultipleCagesWithInequalities)
{
    SudokuPuzzle puzzle;

    // Cage 1: (0,0) + (0,1) = 9
    // Cage 2: (0,2) + (0,3) = 11
    // Inequality: (0,1) > (0,2)
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 9));
    puzzle.addCage(Cage({{0, 2}, {0, 3}}, 11));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 1), Cell(0, 2), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Verify constraints
    EXPECT_EQ(solution.grid[0][0] + solution.grid[0][1], 9);
    EXPECT_EQ(solution.grid[0][2] + solution.grid[0][3], 11);
    EXPECT_GT(solution.grid[0][1], solution.grid[0][2]);
}

// Test: Conflicting cage and inequality (no solution)
TEST_F(MixedSudokuTest, ConflictingConstraints)
{
    SudokuPuzzle puzzle;

    // Cage: (0,0) + (0,1) = 3 (must be 1+2)
    // Inequality: (0,0) > (0,1) means (0,0)=2, (0,1)=1
    // But also set (0,0) = 1 which conflicts
    puzzle.setCell(0, 0, 1);
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 3));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    EXPECT_FALSE(solution.solved);
}

// Test: Complex mixed puzzle
TEST_F(MixedSudokuTest, ComplexMixedPuzzle)
{
    SudokuPuzzle puzzle;

    // Simple cage with inequality - cage sum 5 means 1+4 or 2+3
    // With > inequality, only 4+1 or 3+2 (first > second) are valid
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 5));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));

    // Another simple cage with inequality
    puzzle.addCage(Cage({{0, 2}, {0, 3}}, 11)); // 2+9, 3+8, 4+7, 5+6
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 2), Cell(0, 3), InequalityType::LESS_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
}

// Test: Custom format parsing for mixed Sudoku
TEST_F(MixedSudokuTest, CustomFormatParsing)
{
    std::string input = R"(
        GRID
        0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0
        CAGES
        3 0 0 0 1
        15 0 2 0 3 0 4
        INEQUALITIES
        0 0 > 0 1
        0 2 < 0 3
    )";

    auto puzzle = SudokuParser::parseFromString(input);

    EXPECT_EQ(puzzle.type, SudokuType::KILLER_INEQUALITY);
    EXPECT_EQ(puzzle.cages.size(), 2);
    EXPECT_EQ(puzzle.inequalities.size(), 2);
}

// Test: Cross-box cage with inequality
TEST_F(MixedSudokuTest, CrossBoxCageWithInequality)
{
    SudokuPuzzle puzzle;

    // Cage spanning two boxes: (0,2) + (0,3) = 8
    puzzle.addCage(Cage({{0, 2}, {0, 3}}, 8));

    // Inequality across box boundary
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 2), Cell(0, 3), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    EXPECT_EQ(solution.grid[0][2] + solution.grid[0][3], 8);
    EXPECT_GT(solution.grid[0][2], solution.grid[0][3]);
}

// Test: Full mixed puzzle with given values
TEST_F(MixedSudokuTest, FullMixedWithGivens)
{
    SudokuPuzzle puzzle;

    // Some given values
    puzzle.setCell(4, 4, 5); // Center
    puzzle.setCell(0, 8, 9);
    puzzle.setCell(8, 0, 1);

    // Cages
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 7));
    puzzle.addCage(Cage({{0, 2}, {1, 2}}, 12));
    puzzle.addCage(Cage({{8, 7}, {8, 8}}, 11));

    // Inequalities
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(1, 0), InequalityType::GREATER_THAN));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 1), Cell(0, 2), InequalityType::LESS_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Verify givens preserved
    EXPECT_EQ(solution.grid[4][4], 5);
    EXPECT_EQ(solution.grid[0][8], 9);
    EXPECT_EQ(solution.grid[8][0], 1);
}

// Test: Chain of cage-inequality dependencies
TEST_F(MixedSudokuTest, ChainedDependencies)
{
    SudokuPuzzle puzzle;

    // Create a chain: cage1 -> inequality -> cage2 -> inequality -> cage3
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 5)); // Must be 1+4, 2+3
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 1), Cell(0, 2), InequalityType::LESS_THAN));
    puzzle.addCage(Cage({{0, 2}, {0, 3}}, 13)); // Must be 4+9, 5+8, 6+7
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 3), Cell(0, 4), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
}

// Test: Performance with many mixed constraints
TEST_F(MixedSudokuTest, PerformanceManyConstraints)
{
    SudokuPuzzle puzzle;

    // Add some simple cages with inequalities
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 5)); // 1+4 or 2+3
    puzzle.addCage(Cage({{0, 2}, {0, 3}}, 9)); // various combinations

    // Add inequalities
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 2), Cell(0, 3), InequalityType::LESS_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_LT(solution.solveTimeMs, 5000.0) << "Solve time exceeds 5 seconds";
}

// Test: Verify mixed type detection
TEST_F(MixedSudokuTest, TypeDetection)
{
    SudokuPuzzle puzzle;

    EXPECT_EQ(puzzle.type, SudokuType::STANDARD);

    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 3));
    EXPECT_EQ(puzzle.type, SudokuType::KILLER);

    SudokuPuzzle puzzle2;
    puzzle2.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));
    EXPECT_EQ(puzzle2.type, SudokuType::INEQUALITY);

    puzzle2.addCage(Cage({{1, 0}, {1, 1}}, 5));
    EXPECT_EQ(puzzle2.type, SudokuType::KILLER_INEQUALITY);
}
