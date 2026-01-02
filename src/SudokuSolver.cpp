/**
 * @file SudokuSolver.cpp
 * @brief Implementation of high-level Sudoku solver
 */

#include "SudokuSolver.h"
#include <set>

namespace sudoku
{

    SudokuSolver::SudokuSolver()
    {
    }

    SudokuSolver::~SudokuSolver()
    {
    }

    SudokuSolution SudokuSolver::solve(const SudokuPuzzle &puzzle)
    {
        return encoder.solve(puzzle);
    }

    SudokuSolution SudokuSolver::solveFromString(const std::string &input)
    {
        SudokuPuzzle puzzle = SudokuParser::parseFromString(input);
        return solve(puzzle);
    }

    SudokuSolution SudokuSolver::solveFromFile(const std::string &filename)
    {
        SudokuPuzzle puzzle = SudokuParser::parseFromFile(filename);
        return solve(puzzle);
    }

    bool SudokuSolver::verifyBasicConstraints(const SudokuSolution &solution)
    {
        // Check that all cells have valid values
        for (int row = 0; row < GRID_SIZE; row++)
        {
            for (int col = 0; col < GRID_SIZE; col++)
            {
                int val = solution.grid[row][col];
                if (val < MIN_VALUE || val > MAX_VALUE)
                {
                    return false;
                }
            }
        }

        // Check row constraints
        for (int row = 0; row < GRID_SIZE; row++)
        {
            std::set<int> seen;
            for (int col = 0; col < GRID_SIZE; col++)
            {
                int val = solution.grid[row][col];
                if (seen.count(val))
                    return false;
                seen.insert(val);
            }
        }

        // Check column constraints
        for (int col = 0; col < GRID_SIZE; col++)
        {
            std::set<int> seen;
            for (int row = 0; row < GRID_SIZE; row++)
            {
                int val = solution.grid[row][col];
                if (seen.count(val))
                    return false;
                seen.insert(val);
            }
        }

        // Check box constraints
        for (int boxRow = 0; boxRow < BOX_SIZE; boxRow++)
        {
            for (int boxCol = 0; boxCol < BOX_SIZE; boxCol++)
            {
                std::set<int> seen;
                for (int r = 0; r < BOX_SIZE; r++)
                {
                    for (int c = 0; c < BOX_SIZE; c++)
                    {
                        int row = boxRow * BOX_SIZE + r;
                        int col = boxCol * BOX_SIZE + c;
                        int val = solution.grid[row][col];
                        if (seen.count(val))
                            return false;
                        seen.insert(val);
                    }
                }
            }
        }

        return true;
    }

    bool SudokuSolver::verifyGivenValues(const SudokuPuzzle &puzzle, const SudokuSolution &solution)
    {
        for (int row = 0; row < GRID_SIZE; row++)
        {
            for (int col = 0; col < GRID_SIZE; col++)
            {
                int given = puzzle.grid[row][col];
                if (given >= MIN_VALUE && given <= MAX_VALUE)
                {
                    if (solution.grid[row][col] != given)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool SudokuSolver::verifyCageConstraints(const SudokuPuzzle &puzzle, const SudokuSolution &solution)
    {
        for (const auto &cage : puzzle.cages)
        {
            // Check sum
            int sum = 0;
            std::set<int> values;
            for (const auto &cell : cage.cells)
            {
                int val = solution.grid[cell.row][cell.col];
                sum += val;
                if (values.count(val))
                {
                    // Duplicate value in cage
                    return false;
                }
                values.insert(val);
            }
            if (sum != cage.targetSum)
            {
                return false;
            }
        }
        return true;
    }

    bool SudokuSolver::verifyInequalityConstraints(const SudokuPuzzle &puzzle, const SudokuSolution &solution)
    {
        for (const auto &ineq : puzzle.inequalities)
        {
            int val1 = solution.grid[ineq.cell1.row][ineq.cell1.col];
            int val2 = solution.grid[ineq.cell2.row][ineq.cell2.col];

            if (ineq.type == InequalityType::GREATER_THAN)
            {
                if (!(val1 > val2))
                {
                    return false;
                }
            }
            else
            {
                if (!(val1 < val2))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool SudokuSolver::verifySolution(const SudokuPuzzle &puzzle, const SudokuSolution &solution)
    {
        if (!solution.solved)
        {
            return false;
        }

        if (!verifyBasicConstraints(solution))
        {
            return false;
        }

        if (!verifyGivenValues(puzzle, solution))
        {
            return false;
        }

        if (puzzle.hasKillerConstraints())
        {
            if (!verifyCageConstraints(puzzle, solution))
            {
                return false;
            }
        }

        if (puzzle.hasInequalityConstraints())
        {
            if (!verifyInequalityConstraints(puzzle, solution))
            {
                return false;
            }
        }

        return true;
    }

} // namespace sudoku
