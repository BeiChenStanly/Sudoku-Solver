/**
 * @file SudokuSolver.h
 * @brief High-level Sudoku solver interface
 */

#ifndef SUDOKU_SOLVER_H
#define SUDOKU_SOLVER_H

#include "SudokuTypes.h"
#include "SudokuEncoder.h"
#include "SudokuParser.h"

namespace sudoku
{

    /**
     * @brief High-level Sudoku solver class
     *
     * Provides a simple interface for solving all types of Sudoku puzzles.
     */
    class SudokuSolver
    {
    public:
        SudokuSolver();
        ~SudokuSolver();

        /**
         * @brief Solve a Sudoku puzzle
         * @param puzzle The puzzle to solve
         * @return The solution
         */
        SudokuSolution solve(const SudokuPuzzle &puzzle);

        /**
         * @brief Solve a Sudoku from a string
         * @param input String representation of the puzzle
         * @return The solution
         */
        SudokuSolution solveFromString(const std::string &input);

        /**
         * @brief Solve a Sudoku from a file
         * @param filename Path to the puzzle file
         * @return The solution
         */
        SudokuSolution solveFromFile(const std::string &filename);

        /**
         * @brief Verify that a solution is valid
         * @param puzzle The original puzzle
         * @param solution The solution to verify
         * @return true if the solution is valid
         */
        static bool verifySolution(const SudokuPuzzle &puzzle, const SudokuSolution &solution);

        /**
         * @brief Get statistics from the last solve
         */
        int getNumVariables() const { return encoder.getNumVariables(); }
        int getNumClauses() const { return encoder.getNumClauses(); }

    private:
        SudokuEncoder encoder;

        // Verification helpers
        static bool verifyBasicConstraints(const SudokuSolution &solution);
        static bool verifyGivenValues(const SudokuPuzzle &puzzle, const SudokuSolution &solution);
        static bool verifyCageConstraints(const SudokuPuzzle &puzzle, const SudokuSolution &solution);
        static bool verifyInequalityConstraints(const SudokuPuzzle &puzzle, const SudokuSolution &solution);
    };

} // namespace sudoku

#endif // SUDOKU_SOLVER_H
