/**
 * @file test_generator.cpp
 * @brief Tests for Sudoku puzzle generator
 */

#include <gtest/gtest.h>
#include "SudokuGenerator.h"
#include "SudokuSolver.h"
#include "SudokuParser.h"
#include <queue>

using namespace sudoku;

class GeneratorTest : public ::testing::Test
{
protected:
    SudokuGenerator generator;
    SudokuSolver solver;
};

// Test generating a standard Sudoku (empty with solution)
TEST_F(GeneratorTest, GenerateStandardSudoku)
{
    GeneratorConfig config;
    config.type = SudokuType::STANDARD;
    config.minGivens = 20;
    config.maxGivens = 30;
    config.seed = 12345;

    SudokuSolution solution;
    auto puzzle = generator.generateWithSolution(config, solution);

    // Verify solution is complete
    ASSERT_TRUE(solution.solved);
    for (int r = 0; r < GRID_SIZE; r++)
    {
        for (int c = 0; c < GRID_SIZE; c++)
        {
            EXPECT_GE(solution.grid[r][c], 1);
            EXPECT_LE(solution.grid[r][c], 9);
        }
    }

    // Verify solution is valid
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
}

// Test generating a Killer Sudoku
TEST_F(GeneratorTest, GenerateKillerSudoku)
{
    GeneratorConfig config;
    config.type = SudokuType::KILLER;
    config.minCages = 10;
    config.maxCages = 15;
    config.seed = 42;

    SudokuSolution solution;
    auto puzzle = generator.generateWithSolution(config, solution);

    // Should have cages
    EXPECT_GT(puzzle.cages.size(), 0);
    EXPECT_TRUE(puzzle.hasKillerConstraints());
    EXPECT_FALSE(puzzle.hasInequalityConstraints());

    // Verify solution
    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Verify cage sums
    for (const auto &cage : puzzle.cages)
    {
        int sum = 0;
        for (const auto &cell : cage.cells)
        {
            sum += solution.grid[cell.row][cell.col];
        }
        EXPECT_EQ(sum, cage.targetSum);
    }
}

// Test generating an Inequality Sudoku
TEST_F(GeneratorTest, GenerateInequalitySudoku)
{
    GeneratorConfig config;
    config.type = SudokuType::INEQUALITY;
    config.minInequalities = 15;
    config.maxInequalities = 25;
    config.seed = 123;

    SudokuSolution solution;
    auto puzzle = generator.generateWithSolution(config, solution);

    // Should have inequalities
    EXPECT_GT(puzzle.inequalities.size(), 0);
    EXPECT_FALSE(puzzle.hasKillerConstraints());
    EXPECT_TRUE(puzzle.hasInequalityConstraints());

    // Verify solution
    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));

    // Verify inequalities
    for (const auto &ineq : puzzle.inequalities)
    {
        int val1 = solution.grid[ineq.cell1.row][ineq.cell1.col];
        int val2 = solution.grid[ineq.cell2.row][ineq.cell2.col];
        if (ineq.type == InequalityType::GREATER_THAN)
        {
            EXPECT_GT(val1, val2);
        }
        else
        {
            EXPECT_LT(val1, val2);
        }
    }
}

// Test generating a Mixed (Killer + Inequality) Sudoku
TEST_F(GeneratorTest, GenerateMixedSudoku)
{
    GeneratorConfig config;
    config.type = SudokuType::KILLER_INEQUALITY;
    config.minCages = 8;
    config.maxCages = 12;
    config.minInequalities = 10;
    config.maxInequalities = 15;
    config.seed = 999;

    SudokuSolution solution;
    auto puzzle = generator.generateWithSolution(config, solution);

    // Should have both cages and inequalities
    EXPECT_GT(puzzle.cages.size(), 0);
    EXPECT_GT(puzzle.inequalities.size(), 0);
    EXPECT_TRUE(puzzle.hasKillerConstraints());
    EXPECT_TRUE(puzzle.hasInequalityConstraints());

    // Verify solution
    ASSERT_TRUE(solution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, solution));
}

// Test that generated puzzles can be solved
TEST_F(GeneratorTest, GeneratedPuzzleIsSolvable)
{
    GeneratorConfig config;
    config.type = SudokuType::KILLER_INEQUALITY;
    config.minCages = 15;
    config.maxCages = 20;
    config.minInequalities = 15;
    config.maxInequalities = 20;
    config.seed = 777;

    SudokuSolution expectedSolution;
    auto puzzle = generator.generateWithSolution(config, expectedSolution);

    // Solve the puzzle
    auto foundSolution = solver.solve(puzzle);

    ASSERT_TRUE(foundSolution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(puzzle, foundSolution));
}

// Test custom format output
TEST_F(GeneratorTest, CustomFormatOutput)
{
    GeneratorConfig config;
    config.type = SudokuType::KILLER_INEQUALITY;
    config.minCages = 5;
    config.maxCages = 8;
    config.minInequalities = 5;
    config.maxInequalities = 8;
    config.seed = 555;

    SudokuSolution solution;
    auto puzzle = generator.generateWithSolution(config, solution);

    std::string output = generator.toCustomFormat(puzzle);

    // Verify format contains required sections
    EXPECT_NE(output.find("GRID"), std::string::npos);

    if (puzzle.hasKillerConstraints())
    {
        EXPECT_NE(output.find("CAGES"), std::string::npos);
    }

    if (puzzle.hasInequalityConstraints())
    {
        EXPECT_NE(output.find("INEQUALITIES"), std::string::npos);
    }
}

// Test custom format with solution
TEST_F(GeneratorTest, CustomFormatWithSolution)
{
    GeneratorConfig config;
    config.type = SudokuType::KILLER;
    config.minCages = 10;
    config.maxCages = 15;
    config.seed = 333;

    SudokuSolution solution;
    auto puzzle = generator.generateWithSolution(config, solution);

    std::string output = generator.toCustomFormatWithSolution(puzzle, solution);

    // Verify format contains SOLUTION section
    EXPECT_NE(output.find("SOLUTION"), std::string::npos);
}

// Test reproducibility with seed
TEST_F(GeneratorTest, ReproducibilityWithSeed)
{
    GeneratorConfig config1, config2;
    config1.type = config2.type = SudokuType::KILLER_INEQUALITY;
    config1.minCages = config2.minCages = 10;
    config1.maxCages = config2.maxCages = 15;
    config1.minInequalities = config2.minInequalities = 10;
    config1.maxInequalities = config2.maxInequalities = 15;
    config1.seed = config2.seed = 12345;

    SudokuGenerator gen1, gen2;
    SudokuSolution sol1, sol2;

    auto puzzle1 = gen1.generateWithSolution(config1, sol1);
    auto puzzle2 = gen2.generateWithSolution(config2, sol2);

    // With same seed, should produce same solution
    for (int r = 0; r < GRID_SIZE; r++)
    {
        for (int c = 0; c < GRID_SIZE; c++)
        {
            EXPECT_EQ(sol1.grid[r][c], sol2.grid[r][c]);
        }
    }

    // Same number of cages and inequalities
    EXPECT_EQ(puzzle1.cages.size(), puzzle2.cages.size());
    EXPECT_EQ(puzzle1.inequalities.size(), puzzle2.inequalities.size());
}

// Test that cages are connected (adjacent cells)
TEST_F(GeneratorTest, CagesAreConnected)
{
    GeneratorConfig config;
    config.type = SudokuType::KILLER;
    config.minCages = 15;
    config.maxCages = 20;
    config.minCageSize = 2;
    config.maxCageSize = 5;
    config.seed = 888;

    SudokuSolution solution;
    auto puzzle = generator.generateWithSolution(config, solution);

    // Check each cage is connected
    for (const auto &cage : puzzle.cages)
    {
        if (cage.cells.size() <= 1)
            continue;

        // BFS to check connectivity
        std::set<Cell> visited;
        std::queue<Cell> queue;
        queue.push(cage.cells[0]);
        visited.insert(cage.cells[0]);

        while (!queue.empty())
        {
            Cell current = queue.front();
            queue.pop();

            // Check neighbors
            const int dr[] = {-1, 1, 0, 0};
            const int dc[] = {0, 0, -1, 1};
            for (int i = 0; i < 4; i++)
            {
                Cell neighbor(current.row + dr[i], current.col + dc[i]);

                // Check if neighbor is in cage and not visited
                bool inCage = false;
                for (const auto &c : cage.cells)
                {
                    if (c == neighbor)
                    {
                        inCage = true;
                        break;
                    }
                }

                if (inCage && visited.find(neighbor) == visited.end())
                {
                    visited.insert(neighbor);
                    queue.push(neighbor);
                }
            }
        }

        // All cells should be reachable
        EXPECT_EQ(visited.size(), cage.cells.size());
    }
}

// Test round-trip: generate, serialize, parse, solve
TEST_F(GeneratorTest, RoundTripGenerateParseSolve)
{
    GeneratorConfig config;
    config.type = SudokuType::KILLER_INEQUALITY;
    config.minCages = 12;
    config.maxCages = 18;
    config.minInequalities = 12;
    config.maxInequalities = 18;
    config.seed = 111;

    SudokuSolution originalSolution;
    auto originalPuzzle = generator.generateWithSolution(config, originalSolution);

    // Serialize to custom format
    std::string serialized = generator.toCustomFormat(originalPuzzle);

    // Parse back
    auto parsedPuzzle = SudokuParser::parseFromString(serialized);

    // Solve parsed puzzle
    auto solvedSolution = solver.solve(parsedPuzzle);

    ASSERT_TRUE(solvedSolution.solved);
    EXPECT_TRUE(SudokuSolver::verifySolution(parsedPuzzle, solvedSolution));
}
