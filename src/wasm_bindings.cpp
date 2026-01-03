/**
 * @file wasm_bindings.cpp
 * @brief WebAssembly bindings for Sudoku Solver
 *
 * Provides JavaScript-callable functions for solving and generating Sudoku puzzles.
 */

#include "SudokuSolver.h"
#include "SudokuGenerator.h"
#include "SudokuParser.h"
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <string>
#include <sstream>

using namespace sudoku;
using namespace emscripten;

// Global instances for reuse
static SudokuSolver g_solver;
static SudokuGenerator g_generator;

/**
 * @brief Solve a Sudoku puzzle from custom format string
 * @param input Puzzle in custom format (GRID/CAGES/INEQUALITIES)
 * @param checkUniqueness Whether to check for unique solution
 * @return JSON string with solution and metadata
 */
std::string solvePuzzle(const std::string &input, bool checkUniqueness)
{
    std::ostringstream result;

    try
    {
        SudokuPuzzle puzzle = SudokuParser::parseFromString(input);
        SudokuSolution solution = g_solver.solve(puzzle, checkUniqueness);

        result << "{";
        result << "\"solved\":" << (solution.solved ? "true" : "false") << ",";
        result << "\"solveTimeMs\":" << solution.solveTimeMs << ",";
        result << "\"variables\":" << g_solver.getNumVariables() << ",";
        result << "\"clauses\":" << g_solver.getNumClauses() << ",";

        if (checkUniqueness)
        {
            std::string uniqueStatus = "unknown";
            if (solution.uniqueness == UniquenessStatus::UNIQUE)
            {
                uniqueStatus = "unique";
            }
            else if (solution.uniqueness == UniquenessStatus::NOT_UNIQUE)
            {
                uniqueStatus = "not_unique";
            }
            result << "\"uniqueness\":\"" << uniqueStatus << "\",";
        }

        if (solution.solved)
        {
            result << "\"grid\":[";
            for (int r = 0; r < GRID_SIZE; r++)
            {
                if (r > 0)
                    result << ",";
                result << "[";
                for (int c = 0; c < GRID_SIZE; c++)
                {
                    if (c > 0)
                        result << ",";
                    result << solution.grid[r][c];
                }
                result << "]";
            }
            result << "]";
        }
        else
        {
            result << "\"error\":\"" << solution.errorMessage << "\"";
        }

        result << "}";
    }
    catch (const std::exception &e)
    {
        result.str("");
        result << "{\"solved\":false,\"error\":\"" << e.what() << "\"}";
    }

    return result.str();
}

/**
 * @brief Generate a new Sudoku puzzle
 * @param typeStr Puzzle type: "standard", "killer", "inequality", "mixed"
 * @param minCages Minimum number of cages (for killer/mixed)
 * @param maxCages Maximum number of cages
 * @param minInequalities Minimum number of inequalities (for inequality/mixed)
 * @param maxInequalities Maximum number of inequalities
 * @param seed Random seed (0 for random)
 * @param includeSolution Whether to include solution in output
 * @param fillAllCells Whether cages should fill all cells
 * @param ensureUniqueSolution Whether to ensure unique solution
 * @param difficulty Difficulty percentage (0-100): controls constraint removal ratio
 *                   0 = easiest (keep most constraints), 100 = hardest (remove most constraints)
 * @return Puzzle in custom format string
 */
std::string generatePuzzle(
    const std::string &typeStr,
    int minCages,
    int maxCages,
    int minInequalities,
    int maxInequalities,
    unsigned int seed,
    bool includeSolution,
    bool fillAllCells,
    bool ensureUniqueSolution,
    int difficulty)
{
    GeneratorConfig config;

    // Parse type
    if (typeStr == "standard")
    {
        config.type = SudokuType::STANDARD;
    }
    else if (typeStr == "killer")
    {
        config.type = SudokuType::KILLER;
    }
    else if (typeStr == "inequality")
    {
        config.type = SudokuType::INEQUALITY;
    }
    else
    {
        config.type = SudokuType::KILLER_INEQUALITY;
    }

    // Use the provided constraint counts directly (not scaled by difficulty)
    config.minCages = minCages;
    config.maxCages = maxCages;
    config.minInequalities = minInequalities;
    config.maxInequalities = maxInequalities;
    config.seed = seed;
    config.ensureUniqueSolution = ensureUniqueSolution;
    config.fillAllCells = fillAllCells;

    // Set difficulty for constraint removal ratio
    config.difficulty = std::max(0, std::min(100, difficulty));

    // Adjust for type
    if (config.type == SudokuType::STANDARD)
    {
        config.minCages = config.maxCages = 0;
        config.minInequalities = config.maxInequalities = 0;
        // Difficulty affects givens for standard sudoku
        // Easy: 35-40 givens, Hard: 17-25 givens
        int baseGivens = 40 - static_cast<int>(20 * (static_cast<float>(difficulty) / 100.0f));
        config.minGivens = std::max(17, baseGivens - 5); // 17 is minimum for unique solution
        config.maxGivens = baseGivens;
    }
    else if (config.type == SudokuType::KILLER)
    {
        config.minInequalities = config.maxInequalities = 0;
    }
    else if (config.type == SudokuType::INEQUALITY)
    {
        config.minCages = config.maxCages = 0;
    }

    SudokuSolution solution;
    SudokuPuzzle puzzle = g_generator.generateWithSolution(config, solution);

    if (includeSolution)
    {
        return SudokuGenerator::toCustomFormatWithSolution(puzzle, solution);
    }
    else
    {
        return SudokuGenerator::toCustomFormat(puzzle);
    }
}

/**
 * @brief Verify if a solution is valid for a puzzle
 * @param puzzleStr Puzzle in custom format
 * @param solutionStr Solution grid as JSON array string
 * @return JSON string with verification result
 */
std::string verifySolution(const std::string &puzzleStr, const std::string &solutionStr)
{
    std::ostringstream result;

    try
    {
        SudokuPuzzle puzzle = SudokuParser::parseFromString(puzzleStr);

        // Parse solution from JSON array format [[1,2,3,...], ...]
        SudokuSolution solution;
        solution.solved = true;

        // Simple JSON array parser
        std::string s = solutionStr;
        int idx = 0;
        int row = 0, col = 0;

        for (size_t i = 0; i < s.length(); i++)
        {
            char c = s[i];
            if (c >= '1' && c <= '9')
            {
                if (row < GRID_SIZE && col < GRID_SIZE)
                {
                    solution.grid[row][col] = c - '0';
                    col++;
                    if (col >= GRID_SIZE)
                    {
                        col = 0;
                        row++;
                    }
                }
            }
        }

        bool valid = SudokuSolver::verifySolution(puzzle, solution);

        result << "{\"valid\":" << (valid ? "true" : "false") << "}";
    }
    catch (const std::exception &e)
    {
        result << "{\"valid\":false,\"error\":\"" << e.what() << "\"}";
    }

    return result.str();
}

/**
 * @brief Get version info
 */
std::string getVersion()
{
    return "{\"version\":\"1.0.0\",\"solver\":\"MiniSat\"}";
}

// Emscripten bindings
EMSCRIPTEN_BINDINGS(sudoku_solver)
{
    function("solvePuzzle", &solvePuzzle);
    function("generatePuzzle", &generatePuzzle);
    function("verifySolution", &verifySolution);
    function("getVersion", &getVersion);
}
