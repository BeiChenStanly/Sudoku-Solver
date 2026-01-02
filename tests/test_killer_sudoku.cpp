/**
 * @file test_killer_sudoku.cpp
 * @brief Tests for Killer Sudoku solving
 */

#include <gtest/gtest.h>
#include "SudokuSolver.h"
#include "SudokuParser.h"

using namespace sudoku;

class KillerSudokuTest : public ::testing::Test
{
protected:
    SudokuSolver solver;
};

// Test: Simple 2-cell cage
TEST_F(KillerSudokuTest, SimpleTwoCellCage)
{
    SudokuPuzzle puzzle;

    // Add a cage: cells (0,0) and (0,1) sum to 3
    // Only valid values: 1+2=3
    Cage cage;
    cage.cells = {{0, 0}, {0, 1}};
    cage.targetSum = 3;
    puzzle.addCage(cage);

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Check cage sum
    int sum = solution.grid[0][0] + solution.grid[0][1];
    EXPECT_EQ(sum, 3);

    // Check cage uniqueness
    EXPECT_NE(solution.grid[0][0], solution.grid[0][1]);
}

// Test: 3-cell cage
TEST_F(KillerSudokuTest, ThreeCellCage)
{
    SudokuPuzzle puzzle;

    // Add a cage: cells (0,0), (0,1), (0,2) sum to 6
    // Only valid: 1+2+3=6
    Cage cage;
    cage.cells = {{0, 0}, {0, 1}, {0, 2}};
    cage.targetSum = 6;
    puzzle.addCage(cage);

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Check cage sum
    int sum = solution.grid[0][0] + solution.grid[0][1] + solution.grid[0][2];
    EXPECT_EQ(sum, 6);
}

// Test: Multiple cages
TEST_F(KillerSudokuTest, MultipleCages)
{
    SudokuPuzzle puzzle;

    // First row cages
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 3));  // 1+2
    puzzle.addCage(Cage({{0, 2}, {0, 3}}, 7));  // 3+4
    puzzle.addCage(Cage({{0, 4}, {0, 5}}, 11)); // 5+6
    puzzle.addCage(Cage({{0, 6}, {0, 7}}, 15)); // 7+8
    puzzle.addCage(Cage({{0, 8}}, 9));          // 9

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
}

// Test: Killer Sudoku with given values
TEST_F(KillerSudokuTest, WithGivenValues)
{
    SudokuPuzzle puzzle;

    // Set some given values
    puzzle.setCell(0, 0, 5);
    puzzle.setCell(4, 4, 5);
    puzzle.setCell(8, 8, 5);

    // Add cage that must include the given value
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 8)); // 5+3=8

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
    EXPECT_EQ(solution.grid[0][0], 5);
    EXPECT_EQ(solution.grid[0][1], 3);
}

// Test: Impossible cage constraints - conflicting cages
TEST_F(KillerSudokuTest, ImpossibleConflictingCages)
{
    SudokuPuzzle puzzle;

    // Two overlapping cages with impossible constraints
    // Cell (0,0) must be part of cage summing to 3 (so (0,0) + (0,1) = 3)
    // But also (0,0) must be 9 from another constraint
    puzzle.setCell(0, 0, 9);
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 3)); // 9 + x = 3 is impossible

    auto solution = solver.solve(puzzle);

    EXPECT_FALSE(solution.solved);
}

// Test: Impossible cage - cells in same row sum to invalid value
TEST_F(KillerSudokuTest, ImpossibleCageInRow)
{
    SudokuPuzzle puzzle;

    // Force all cells except (0,8) to be 1-8
    for (int c = 0; c < 8; c++)
    {
        puzzle.setCell(0, c, c + 1);
    }
    // Cell (0,8) must be 9 due to row constraint
    // But add a cage that requires (0,8) to be less than 9
    puzzle.addCage(Cage({{0, 8}}, 1)); // This cage says (0,8)=1, but row says it must be 9

    auto solution = solver.solve(puzzle);

    EXPECT_FALSE(solution.solved);
}

// Test: Full Killer Sudoku puzzle (simplified)
TEST_F(KillerSudokuTest, FullKillerPuzzle)
{
    SudokuPuzzle puzzle;

    // Define cages that constrain the first row completely
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 3));  // 1+2=3
    puzzle.addCage(Cage({{0, 2}, {0, 3}}, 7));  // 3+4=7
    puzzle.addCage(Cage({{0, 4}, {0, 5}}, 11)); // 5+6=11
    puzzle.addCage(Cage({{0, 6}, {0, 7}}, 15)); // 7+8=15
    puzzle.addCage(Cage({{0, 8}}, 9));          // 9

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
}

// Test: Custom format parsing for Killer Sudoku
TEST_F(KillerSudokuTest, CustomFormatParsing)
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
        7 0 2 0 3
        17 0 4 0 5 0 6 0 7 0 8
    )";

    auto puzzle = SudokuParser::parseFromString(input);

    EXPECT_EQ(puzzle.type, SudokuType::KILLER);
    EXPECT_EQ(puzzle.cages.size(), 3);
    EXPECT_EQ(puzzle.cages[0].targetSum, 3);
    EXPECT_EQ(puzzle.cages[0].cells.size(), 2);
}

// Test: Large cage (9 cells)
TEST_F(KillerSudokuTest, LargeCage)
{
    SudokuPuzzle puzzle;

    // A cage with all 9 cells of first row, sum = 45 (1+2+...+9)
    std::vector<Cell> cells;
    for (int c = 0; c < 9; c++)
    {
        cells.push_back({0, c});
    }
    puzzle.addCage(Cage(cells, 45));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Verify sum
    int sum = 0;
    for (int c = 0; c < 9; c++)
    {
        sum += solution.grid[0][c];
    }
    EXPECT_EQ(sum, 45);
}

// Test: Cage uniqueness constraint
TEST_F(KillerSudokuTest, CageUniqueness)
{
    SudokuPuzzle puzzle;

    // 2 cells summing to 4: could be 1+3 or 2+2, but 2+2 violates uniqueness
    puzzle.addCage(Cage({{0, 0}, {0, 1}}, 4));

    auto solution = solver.solve(puzzle);

    ASSERT_TRUE(solution.solved);

    // Both cells should have different values
    EXPECT_NE(solution.grid[0][0], solution.grid[0][1]);

    // And sum should be 4
    EXPECT_EQ(solution.grid[0][0] + solution.grid[0][1], 4);
}
