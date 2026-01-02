/**
 * @file test_inequality_sudoku.cpp
 * @brief Tests for Inequality Sudoku (Greater-Than Sudoku) solving
 */

#include <gtest/gtest.h>
#include "SudokuSolver.h"
#include "SudokuParser.h"

using namespace sudoku;

class InequalitySudokuTest : public ::testing::Test
{
protected:
    SudokuSolver solver;
};

// Test: Simple greater-than constraint
TEST_F(InequalitySudokuTest, SimpleGreaterThan)
{
    SudokuPuzzle puzzle;

    // Set cell (0,0) > cell (0,1)
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_GT(solution.grid[0][0], solution.grid[0][1]);
}

// Test: Simple less-than constraint
TEST_F(InequalitySudokuTest, SimpleLessThan)
{
    SudokuPuzzle puzzle;

    // Set cell (0,0) < cell (0,1)
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::LESS_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_LT(solution.grid[0][0], solution.grid[0][1]);
}

// Test: Chain of inequalities
TEST_F(InequalitySudokuTest, InequalityChain)
{
    SudokuPuzzle puzzle;

    // Create chain: (0,0) < (0,1) < (0,2)
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::LESS_THAN));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 1), Cell(0, 2), InequalityType::LESS_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_LT(solution.grid[0][0], solution.grid[0][1]);
    EXPECT_LT(solution.grid[0][1], solution.grid[0][2]);
}

// Test: Vertical inequality
TEST_F(InequalitySudokuTest, VerticalInequality)
{
    SudokuPuzzle puzzle;

    // (0,0) > (1,0) > (2,0)
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(1, 0), InequalityType::GREATER_THAN));
    puzzle.addInequality(InequalityConstraint(
        Cell(1, 0), Cell(2, 0), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_GT(solution.grid[0][0], solution.grid[1][0]);
    EXPECT_GT(solution.grid[1][0], solution.grid[2][0]);
}

// Test: Multiple inequalities in first row
TEST_F(InequalitySudokuTest, MultipleInequalitiesRow)
{
    SudokuPuzzle puzzle;

    // Create pattern: 1 < 2 < 3 < 4 < 5 < 6 < 7 < 8 < 9
    for (int c = 0; c < 8; c++)
    {
        puzzle.addInequality(InequalityConstraint(
            Cell(0, c), Cell(0, c + 1), InequalityType::LESS_THAN));
    }

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // First row should be 1,2,3,4,5,6,7,8,9
    for (int c = 0; c < 9; c++)
    {
        EXPECT_EQ(solution.grid[0][c], c + 1);
    }
}

// Test: Contradictory inequalities (should fail)
TEST_F(InequalitySudokuTest, ContradictoryInequalities)
{
    SudokuPuzzle puzzle;

    // Set (0,0) as 5
    puzzle.setCell(0, 0, 5);
    puzzle.setCell(0, 1, 5); // Same value

    // Try to add inequality between cells with same value
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    EXPECT_FALSE(solution.solved);
}

// Test: Inequality with given values
TEST_F(InequalitySudokuTest, InequalityWithGivens)
{
    SudokuPuzzle puzzle;

    // Set (0,0) = 5, require (0,1) < (0,0), so (0,1) must be 1,2,3, or 4
    puzzle.setCell(0, 0, 5);
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_EQ(solution.grid[0][0], 5);
    EXPECT_LT(solution.grid[0][1], 5);
}

// Test: Custom format parsing for Inequality Sudoku
TEST_F(InequalitySudokuTest, CustomFormatParsing)
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
        INEQUALITIES
        0 0 > 0 1
        0 1 < 0 2
        1 0 > 2 0
    )";

    auto puzzle = SudokuParser::parseFromString(input);

    EXPECT_EQ(puzzle.type, SudokuType::INEQUALITY);
    EXPECT_EQ(puzzle.inequalities.size(), 3);
    EXPECT_EQ(puzzle.inequalities[0].type, InequalityType::GREATER_THAN);
    EXPECT_EQ(puzzle.inequalities[1].type, InequalityType::LESS_THAN);
}

// Test: Box-local inequalities
TEST_F(InequalitySudokuTest, BoxLocalInequalities)
{
    SudokuPuzzle puzzle;

    // Create descending pattern in first box
    // (0,0) > (0,1) > (0,2)
    // (1,0) > (1,1) > (1,2)
    // (2,0) > (2,1) > (2,2)
    for (int r = 0; r < 3; r++)
    {
        for (int c = 0; c < 2; c++)
        {
            puzzle.addInequality(InequalityConstraint(
                Cell(r, c), Cell(r, c + 1), InequalityType::GREATER_THAN));
        }
    }

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Verify inequalities
    for (int r = 0; r < 3; r++)
    {
        for (int c = 0; c < 2; c++)
        {
            EXPECT_GT(solution.grid[r][c], solution.grid[r][c + 1]);
        }
    }
}

// Test: Complex inequality pattern
TEST_F(InequalitySudokuTest, ComplexInequalityPattern)
{
    SudokuPuzzle puzzle;

    // Create a zigzag pattern
    // (0,0) > (0,1) < (0,2) > (0,3) < (0,4)
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 1), Cell(0, 2), InequalityType::LESS_THAN));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 2), Cell(0, 3), InequalityType::GREATER_THAN));
    puzzle.addInequality(InequalityConstraint(
        Cell(0, 3), Cell(0, 4), InequalityType::LESS_THAN));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Verify pattern
    EXPECT_GT(solution.grid[0][0], solution.grid[0][1]);
    EXPECT_LT(solution.grid[0][1], solution.grid[0][2]);
    EXPECT_GT(solution.grid[0][2], solution.grid[0][3]);
    EXPECT_LT(solution.grid[0][3], solution.grid[0][4]);
}

// Test: Extreme constraint (force value to be 9)
TEST_F(InequalitySudokuTest, ForceMaxValue)
{
    SudokuPuzzle puzzle;

    // Make (0,0) greater than 8 other cells in the row
    // This forces (0,0) to be 9
    for (int c = 1; c < 9; c++)
    {
        puzzle.addInequality(InequalityConstraint(
            Cell(0, 0), Cell(0, c), InequalityType::GREATER_THAN));
    }

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_EQ(solution.grid[0][0], 9);
}

// Test: Force minimum value (1)
TEST_F(InequalitySudokuTest, ForceMinValue)
{
    SudokuPuzzle puzzle;

    // Make (0,0) less than 8 other cells in the row
    // This forces (0,0) to be 1
    for (int c = 1; c < 9; c++)
    {
        puzzle.addInequality(InequalityConstraint(
            Cell(0, 0), Cell(0, c), InequalityType::LESS_THAN));
    }

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_EQ(solution.grid[0][0], 1);
}
