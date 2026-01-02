/**
 * @file main.cpp
 * @brief Command-line interface for Sudoku solver and generator
 *
 * Usage:
 *   sudoku_solve <puzzle_file>
 *   sudoku_solve --string "<81-char grid>"
 *   sudoku_solve --generate [options]
 *   sudoku_solve --help
 */

#include "SudokuSolver.h"
#include "SudokuParser.h"
#include "SudokuGenerator.h"
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>

void printUsage(const char *progName)
{
    std::cout << "Sudoku Solver & Generator - Using MiniSat SAT Solver\n\n";
    std::cout << "Supports: Standard Sudoku, Killer Sudoku, Inequality Sudoku, and Mixed\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << progName << " <puzzle_file>        Solve puzzle from file\n";
    std::cout << "  " << progName << " --string \"<grid>\"    Solve from 81-char string\n";
    std::cout << "  " << progName << " --generate [options] Generate a new puzzle\n";
    std::cout << "  " << progName << " --help               Show this help\n\n";
    std::cout << "Solve Options:\n";
    std::cout << "  --unique, -u         Check if solution is unique\n\n";
    std::cout << "Generate Options:\n";
    std::cout << "  --type <TYPE>        Puzzle type: standard, killer, inequality, mixed (default: mixed)\n";
    std::cout << "  --cages <MIN> <MAX>  Number of cages (default: 10 20)\n";
    std::cout << "  --ineq <MIN> <MAX>   Number of inequalities (default: 10 20)\n";
    std::cout << "  --givens <MIN> <MAX> Number of given values (default: 0 10)\n";
    std::cout << "  --seed <N>           Random seed for reproducibility\n";
    std::cout << "  --output <file>      Output file (default: stdout)\n";
    std::cout << "  --with-solution      Include solution in output\n\n";
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

sudoku::SudokuType parseType(const std::string &typeStr)
{
    if (typeStr == "standard")
        return sudoku::SudokuType::STANDARD;
    if (typeStr == "killer")
        return sudoku::SudokuType::KILLER;
    if (typeStr == "inequality")
        return sudoku::SudokuType::INEQUALITY;
    if (typeStr == "mixed")
        return sudoku::SudokuType::KILLER_INEQUALITY;
    throw std::runtime_error("Unknown puzzle type: " + typeStr);
}

int runGenerate(int argc, char *argv[])
{
    sudoku::GeneratorConfig config;
    config.type = sudoku::SudokuType::KILLER_INEQUALITY; // Default: mixed

    std::string outputFile;
    bool withSolution = false;

    // Parse generate options
    for (int i = 2; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--type" && i + 1 < argc)
        {
            config.type = parseType(argv[++i]);
        }
        else if (arg == "--cages" && i + 2 < argc)
        {
            config.minCages = std::stoi(argv[++i]);
            config.maxCages = std::stoi(argv[++i]);
        }
        else if (arg == "--ineq" && i + 2 < argc)
        {
            config.minInequalities = std::stoi(argv[++i]);
            config.maxInequalities = std::stoi(argv[++i]);
        }
        else if (arg == "--givens" && i + 2 < argc)
        {
            config.minGivens = std::stoi(argv[++i]);
            config.maxGivens = std::stoi(argv[++i]);
        }
        else if (arg == "--seed" && i + 1 < argc)
        {
            config.seed = static_cast<unsigned int>(std::stoul(argv[++i]));
        }
        else if (arg == "--output" && i + 1 < argc)
        {
            outputFile = argv[++i];
        }
        else if (arg == "--with-solution")
        {
            withSolution = true;
        }
    }

    // Adjust config based on type
    if (config.type == sudoku::SudokuType::STANDARD)
    {
        config.minCages = config.maxCages = 0;
        config.minInequalities = config.maxInequalities = 0;
        config.minGivens = 17; // Minimum for unique standard sudoku
        config.maxGivens = 30;
    }
    else if (config.type == sudoku::SudokuType::KILLER)
    {
        config.minInequalities = config.maxInequalities = 0;
    }
    else if (config.type == sudoku::SudokuType::INEQUALITY)
    {
        config.minCages = config.maxCages = 0;
    }

    std::string typeName;
    switch (config.type)
    {
    case sudoku::SudokuType::STANDARD:
        typeName = "Standard Sudoku";
        break;
    case sudoku::SudokuType::KILLER:
        typeName = "Killer Sudoku";
        break;
    case sudoku::SudokuType::INEQUALITY:
        typeName = "Inequality Sudoku";
        break;
    case sudoku::SudokuType::KILLER_INEQUALITY:
        typeName = "Mixed (Killer + Inequality) Sudoku";
        break;
    }
    std::cerr << "Generating " << typeName << " puzzle...\n";

    sudoku::SudokuGenerator generator;
    sudoku::SudokuSolution solution;
    auto puzzle = generator.generateWithSolution(config, solution);

    std::string output;
    if (withSolution)
    {
        output = generator.toCustomFormatWithSolution(puzzle, solution);
    }
    else
    {
        output = generator.toCustomFormat(puzzle);
    }

    if (outputFile.empty())
    {
        std::cout << output;
    }
    else
    {
        std::ofstream file(outputFile);
        if (!file)
        {
            std::cerr << "Error: Cannot write to file " << outputFile << "\n";
            return 1;
        }
        file << output;
        std::cerr << "Puzzle saved to " << outputFile << "\n";
    }

    // Print info to stderr
    std::cerr << "\nGenerated puzzle info:\n";
    std::cerr << "  Cages: " << puzzle.cages.size() << "\n";
    std::cerr << "  Inequalities: " << puzzle.inequalities.size() << "\n";
    int givens = 0;
    for (int r = 0; r < 9; r++)
    {
        for (int c = 0; c < 9; c++)
        {
            if (puzzle.grid[r][c] != 0)
                givens++;
        }
    }
    std::cerr << "  Given values: " << givens << "\n";

    return 0;
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

    // Handle generate command
    if (arg1 == "--generate" || arg1 == "-g")
    {
        return runGenerate(argc, argv);
    }

    try
    {
        sudoku::SudokuSolver solver;
        sudoku::SudokuPuzzle puzzle;
        bool checkUniqueness = false;

        // Parse arguments
        int fileArgIndex = 1;
        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            if (arg == "--unique" || arg == "-u")
            {
                checkUniqueness = true;
            }
            else if (arg == "--string" || arg == "-s")
            {
                fileArgIndex = -1; // Will handle string input
                if (i + 1 < argc)
                {
                    puzzle = sudoku::SudokuParser::parseFromString(argv[++i]);
                }
                else
                {
                    std::cerr << "Error: --string requires a puzzle string\n";
                    return 1;
                }
            }
            else if (arg[0] != '-')
            {
                fileArgIndex = i;
            }
        }

        if (fileArgIndex > 0)
        {
            puzzle = sudoku::SudokuParser::parseFromFile(argv[fileArgIndex]);
        }

        printPuzzleInfo(puzzle);
        std::cout << "\nSolving" << (checkUniqueness ? " (with uniqueness check)" : "") << "...\n\n";

        auto solution = solver.solve(puzzle, checkUniqueness);

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

            if (checkUniqueness)
            {
                if (solution.isUnique)
                {
                    std::cout << "✓ Solution is unique!\n";
                }
                else
                {
                    std::cout << "✗ Multiple solutions exist.\n";
                }
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
