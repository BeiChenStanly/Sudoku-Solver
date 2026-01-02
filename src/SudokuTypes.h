/**
 * @file SudokuTypes.h
 * @brief Data structures for representing various Sudoku types
 *
 * Supports:
 * - Standard Sudoku
 * - Killer Sudoku (cage constraints with sums)
 * - Inequality Sudoku (greater-than constraints between cells)
 * - Mixed variants (combinations of the above)
 */

#ifndef SUDOKU_TYPES_H
#define SUDOKU_TYPES_H

#include <vector>
#include <string>
#include <utility>
#include <set>
#include <optional>

namespace sudoku
{

    // Constants
    constexpr int GRID_SIZE = 9;
    constexpr int BOX_SIZE = 3;
    constexpr int MIN_VALUE = 1;
    constexpr int MAX_VALUE = 9;
    constexpr int EMPTY_CELL = 0;

    /**
     * @brief Represents a cell position in the grid
     */
    struct Cell
    {
        int row;
        int col;

        Cell() : row(0), col(0) {}
        Cell(int r, int c) : row(r), col(c) {}

        bool operator==(const Cell &other) const
        {
            return row == other.row && col == other.col;
        }

        bool operator<(const Cell &other) const
        {
            if (row != other.row)
                return row < other.row;
            return col < other.col;
        }

        bool isValid() const
        {
            return row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE;
        }
    };

    /**
     * @brief Represents a cage in Killer Sudoku
     * A cage is a group of cells that must sum to a target value
     * and contain unique digits
     */
    struct Cage
    {
        std::vector<Cell> cells;
        int targetSum;

        Cage() : targetSum(0) {}
        Cage(const std::vector<Cell> &c, int sum) : cells(c), targetSum(sum) {}

        bool isValid() const
        {
            if (cells.empty() || targetSum < 1)
                return false;
            // Minimum possible sum for n cells: 1+2+...+n = n*(n+1)/2
            int n = static_cast<int>(cells.size());
            int minSum = n * (n + 1) / 2;
            // Maximum possible sum for n cells: (10-n)+(10-n+1)+...+9 = n*(19-n)/2
            int maxSum = n * (19 - n) / 2;
            return targetSum >= minSum && targetSum <= maxSum;
        }
    };

    /**
     * @brief Represents an inequality constraint between two adjacent cells
     */
    enum class InequalityType
    {
        GREATER_THAN, // cell1 > cell2
        LESS_THAN     // cell1 < cell2
    };

    struct InequalityConstraint
    {
        Cell cell1;
        Cell cell2;
        InequalityType type;

        InequalityConstraint() : type(InequalityType::GREATER_THAN) {}
        InequalityConstraint(const Cell &c1, const Cell &c2, InequalityType t)
            : cell1(c1), cell2(c2), type(t) {}

        bool isValid() const
        {
            return cell1.isValid() && cell2.isValid() && !(cell1 == cell2);
        }
    };

    /**
     * @brief Types of Sudoku puzzles supported
     */
    enum class SudokuType
    {
        STANDARD,         // Classic 9x9 Sudoku
        KILLER,           // Killer Sudoku with cage constraints
        INEQUALITY,       // Greater-than/Less-than Sudoku
        KILLER_INEQUALITY // Mixed: both cages and inequalities
    };

    /**
     * @brief Represents a complete Sudoku puzzle with all constraints
     */
    struct SudokuPuzzle
    {
        // Basic grid (0 = empty, 1-9 = given values)
        int grid[GRID_SIZE][GRID_SIZE];

        // Puzzle type
        SudokuType type;

        // Killer Sudoku constraints
        std::vector<Cage> cages;

        // Inequality constraints
        std::vector<InequalityConstraint> inequalities;

        SudokuPuzzle() : type(SudokuType::STANDARD)
        {
            for (int i = 0; i < GRID_SIZE; i++)
            {
                for (int j = 0; j < GRID_SIZE; j++)
                {
                    grid[i][j] = EMPTY_CELL;
                }
            }
        }

        // Set a cell value
        void setCell(int row, int col, int value)
        {
            if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
            {
                grid[row][col] = value;
            }
        }

        // Get a cell value
        int getCell(int row, int col) const
        {
            if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
            {
                return grid[row][col];
            }
            return EMPTY_CELL;
        }

        // Add a cage constraint
        void addCage(const Cage &cage)
        {
            cages.push_back(cage);
            if (type == SudokuType::STANDARD)
            {
                type = SudokuType::KILLER;
            }
            else if (type == SudokuType::INEQUALITY)
            {
                type = SudokuType::KILLER_INEQUALITY;
            }
        }

        // Add an inequality constraint
        void addInequality(const InequalityConstraint &ineq)
        {
            inequalities.push_back(ineq);
            if (type == SudokuType::STANDARD)
            {
                type = SudokuType::INEQUALITY;
            }
            else if (type == SudokuType::KILLER)
            {
                type = SudokuType::KILLER_INEQUALITY;
            }
        }

        // Check if the puzzle has killer constraints
        bool hasKillerConstraints() const
        {
            return !cages.empty();
        }

        // Check if the puzzle has inequality constraints
        bool hasInequalityConstraints() const
        {
            return !inequalities.empty();
        }

        // Get type as string
        std::string getTypeString() const
        {
            switch (type)
            {
            case SudokuType::STANDARD:
                return "Standard Sudoku";
            case SudokuType::KILLER:
                return "Killer Sudoku";
            case SudokuType::INEQUALITY:
                return "Inequality Sudoku";
            case SudokuType::KILLER_INEQUALITY:
                return "Killer + Inequality Sudoku";
            default:
                return "Unknown";
            }
        }
    };

    /**
     * @brief Represents the solution to a Sudoku puzzle
     */
    struct SudokuSolution
    {
        int grid[GRID_SIZE][GRID_SIZE];
        bool solved;
        std::string errorMessage;
        double solveTimeMs;

        SudokuSolution() : solved(false), solveTimeMs(0.0)
        {
            for (int i = 0; i < GRID_SIZE; i++)
            {
                for (int j = 0; j < GRID_SIZE; j++)
                {
                    grid[i][j] = EMPTY_CELL;
                }
            }
        }

        int getCell(int row, int col) const
        {
            if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
            {
                return grid[row][col];
            }
            return EMPTY_CELL;
        }

        void setCell(int row, int col, int value)
        {
            if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
            {
                grid[row][col] = value;
            }
        }
    };

} // namespace sudoku

#endif // SUDOKU_TYPES_H
