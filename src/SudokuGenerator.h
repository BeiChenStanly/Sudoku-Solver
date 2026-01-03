/**
 * @file SudokuGenerator.h
 * @brief Generator for creating Sudoku puzzles
 *
 * Supports generating:
 * - Standard Sudoku
 * - Killer Sudoku (with cage constraints)
 * - Inequality Sudoku (with greater-than/less-than constraints)
 * - Mixed Sudoku (combination of killer and inequality)
 */

#ifndef SUDOKU_GENERATOR_H
#define SUDOKU_GENERATOR_H

#include "SudokuTypes.h"
#include "SudokuSolver.h"
#include <random>
#include <string>

namespace sudoku
{

    /**
     * @brief Configuration for puzzle generation
     */
    struct GeneratorConfig
    {
        // Puzzle type to generate
        SudokuType type = SudokuType::KILLER_INEQUALITY;

        // Number of cages to generate (for killer/mixed)
        int minCages = 15;
        int maxCages = 25;

        // Cage size constraints
        int minCageSize = 2;
        int maxCageSize = 5;

        // Number of inequalities to generate (for inequality/mixed)
        int minInequalities = 20;
        int maxInequalities = 40;

        // Number of given cells (hints) in the grid
        int minGivens = 0;
        int maxGivens = 0; // 0 means no given values (pure constraint-based)

        // Random seed (0 = use random seed)
        unsigned int seed = 0;

        // Ensure unique solution
        bool ensureUniqueSolution = true;

        // Whether cages should cover all cells (for killer/mixed)
        bool fillAllCells = false;

        // Difficulty level (0-100): controls constraint removal ratio
        // 0 = easiest (keep most constraints), 100 = hardest (remove most constraints)
        int difficulty = 50;
    };

    /**
     * @brief Generates Sudoku puzzles using SAT solver
     */
    class SudokuGenerator
    {
    public:
        SudokuGenerator();
        ~SudokuGenerator();

        /**
         * @brief Generate a puzzle with default configuration
         * @return Generated puzzle
         */
        SudokuPuzzle generate();

        /**
         * @brief Generate a puzzle with custom configuration
         * @param config Generation configuration
         * @return Generated puzzle
         */
        SudokuPuzzle generate(const GeneratorConfig &config);

        /**
         * @brief Generate and return the puzzle along with its solution
         * @param config Generation configuration
         * @param solution Output parameter for the solution
         * @return Generated puzzle
         */
        SudokuPuzzle generateWithSolution(const GeneratorConfig &config, SudokuSolution &solution);

        /**
         * @brief Convert puzzle to custom text format
         * @param puzzle The puzzle to convert
         * @return String in custom format
         */
        static std::string toCustomFormat(const SudokuPuzzle &puzzle);

        /**
         * @brief Convert puzzle and solution to custom format
         * @param puzzle The puzzle
         * @param solution The solution
         * @return String with puzzle and solution
         */
        static std::string toCustomFormatWithSolution(const SudokuPuzzle &puzzle,
                                                      const SudokuSolution &solution);

    private:
        SudokuSolver solver;
        std::mt19937 rng;

        // Generate a complete valid Sudoku grid
        bool generateCompleteSolution(SudokuSolution &solution);

        // Generate cage constraints based on solution
        void generateCages(SudokuPuzzle &puzzle, const SudokuSolution &solution,
                           int numCages, int minSize, int maxSize);

        // Generate cages that cover all cells
        void generateCagesFillingAll(SudokuPuzzle &puzzle, const SudokuSolution &solution,
                                     int minSize, int maxSize);

        // Generate inequality constraints based on solution
        void generateInequalities(SudokuPuzzle &puzzle, const SudokuSolution &solution,
                                  int numInequalities);

        // Add given cells to puzzle
        void addGivens(SudokuPuzzle &puzzle, const SudokuSolution &solution, int numGivens);

        // Check if puzzle has unique solution
        bool hasUniqueSolution(const SudokuPuzzle &puzzle);

        // Minimize constraints while maintaining uniqueness, controlled by difficulty
        void minimizeConstraints(SudokuPuzzle &puzzle, const SudokuSolution &solution, int difficulty);

        // Helper: Get adjacent cells
        std::vector<Cell> getAdjacentCells(const Cell &cell);

        // Helper: Generate a connected cage starting from a cell
        std::vector<Cell> generateConnectedCage(const SudokuSolution &solution,
                                                std::set<Cell> &usedCells,
                                                int targetSize);

        // Helper: Calculate cage sum
        int calculateCageSum(const std::vector<Cell> &cells, const SudokuSolution &solution);

        // Helper: Shuffle and pick random elements
        template <typename T>
        std::vector<T> shuffleAndPick(std::vector<T> items, int count);
    };

} // namespace sudoku

#endif // SUDOKU_GENERATOR_H
