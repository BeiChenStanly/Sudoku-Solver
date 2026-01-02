/**
 * @file main.cpp
 * @brief Command-line interface for Sudoku solver
 *
 * Usage:
 *   sudoku_solve <puzzle_file>
 *   sudoku_solve --string "<81-char grid>"
 *   sudoku_solve --help
 */

#include "SudokuSolver.h"
#include "SudokuParser.h"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char *progName)
{
    std::cout << "Sudoku Solver - Using MiniSat SAT Solver\n\n";
    std::cout << "Supports: Standard Sudoku, Killer Sudoku, Inequality Sudoku, and Mixed\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " <puzzle_file>        Solve puzzle from file\n";
    std::cout << "  " << progName << " --string \"<grid>\"    Solve from 81-char string\n";
    std::cout << "  " << progName << " --help               Show this help\n\n";
    std::cout << "Input Formats:\n";
    std::cout << "  1. Simple grid (81 characters, use . or 0 for empty cells):\n";
    std::cout << "     530070000600195000098000060800060003400803001700020006060000280000419005000080079\n\n";
    std::cout << "  2. Custom text format:\n";
    std::cout << "     GRID\n";
    std::cout << "     5 3 0 0 7 0 0 0 0\n";
    std::cout << "     6 0 0 1 9 5 0 0 0\n";
    std::cout << "     ... (9 lines)\n";
    std::cout << "     CAGES\n";
    std::cout << "     10 0 0 0 1       (sum r1 c1 r2 c2 ...)\n";
    std::cout << "     INEQUALITIES\n";
    std::cout << "     0 0 > 0 1        (r1 c1 > r2 c2)\n";
}

void printPuzzleInfo(const sudoku::SudokuPuzzle &puzzle)
{
    std::cout << "Puzzle Type: " << puzzle.getTypeString() << "\n";
    if (puzzle.hasKillerConstraints())
    {
        std::cout << "Cages: " << puzzle.cages.size() << "\n";
    }
    if (puzzle.hasInequalityConstraints())
    {
        std::cout << "Inequalities: " << puzzle.inequalities.size() << "\n";
    }
    std::cout << "\nInput Grid:\n";
    std::cout << sudoku::SudokuParser::toPrettyGrid(puzzle.grid);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string arg1 = argv[1];

    if (arg1 == "--help" || arg1 == "-h")
    {
        printUsage(argv[0]);
        return 0;
    }

    try
    {
        sudoku::SudokuSolver solver;
        sudoku::SudokuPuzzle puzzle;

        if (arg1 == "--string" || arg1 == "-s")
        {
            if (argc < 3)
            {
                std::cerr << "Error: --string requires a puzzle string\n";
                return 1;
            }
            puzzle = sudoku::SudokuParser::parseFromString(argv[2]);
        }
        else
        {
            puzzle = sudoku::SudokuParser::parseFromFile(arg1);
        }

        printPuzzleInfo(puzzle);
        std::cout << "\nSolving...\n\n";

        auto solution = solver.solve(puzzle);

        std::cout << sudoku::SudokuParser::toString(solution);

        // Verify solution
        if (solution.solved)
        {
            if (sudoku::SudokuSolver::verifySolution(puzzle, solution))
            {
                std::cout << "\n✓ Solution verified correct!\n";
            }
            else
            {
                std::cout << "\n✗ Solution verification failed!\n";
                return 1;
            }

            std::cout << "\nStatistics:\n";
            std::cout << "  Variables: " << solver.getNumVariables() << "\n";
            std::cout << "  Clauses: " << solver.getNumClauses() << "\n";
            std::cout << "  Solve time: " << solution.solveTimeMs << " ms\n";
        }

        return solution.solved ? 0 : 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
