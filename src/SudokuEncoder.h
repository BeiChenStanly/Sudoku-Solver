/**
 * @file SudokuEncoder.h
 * @brief SAT encoder for Sudoku puzzles
 *
 * Encodes Sudoku puzzles as SAT problems using MiniSat.
 * Supports standard, killer, inequality, and mixed Sudoku variants.
 */

#ifndef SUDOKU_ENCODER_H
#define SUDOKU_ENCODER_H

#include "SudokuTypes.h"
#include "minisat/core/Solver.h"
#include <vector>
#include <map>

namespace sudoku
{

    /**
     * @brief Encodes Sudoku puzzles as SAT formulas
     *
     * Variable encoding: x(r, c, v) means cell (r, c) has value v
     * Variable index: r * 81 + c * 9 + (v - 1)
     */
    class SudokuEncoder
    {
    public:
        SudokuEncoder();
        ~SudokuEncoder();

        /**
         * @brief Encode a Sudoku puzzle as SAT and solve it
         * @param puzzle The puzzle to solve
         * @param checkUniqueness If true, verify that the solution is unique
         * @return The solution (check solved field for success)
         */
        SudokuSolution solve(const SudokuPuzzle &puzzle, bool checkUniqueness = false);

        /**
         * @brief Get statistics about the encoding
         */
        int getNumVariables() const { return numVariables; }
        int getNumClauses() const { return numClauses; }

    private:
        Minisat::Solver *solver;
        int numVariables;
        int numClauses;

        // Variable mapping: (row, col, value) -> SAT variable
        Minisat::Var getVar(int row, int col, int value);
        Minisat::Lit getLit(int row, int col, int value, bool positive = true);

        // Reset solver for new puzzle
        void reset();

        // Basic Sudoku constraints
        void encodeCellConstraints();                       // Each cell has exactly one value
        void encodeRowConstraints();                        // Each row has each value exactly once
        void encodeColumnConstraints();                     // Each column has each value exactly once
        void encodeBoxConstraints();                        // Each 3x3 box has each value exactly once
        void encodeGivenValues(const SudokuPuzzle &puzzle); // Given clues

        // Killer Sudoku constraints
        void encodeCageConstraints(const std::vector<Cage> &cages);
        void encodeCageSum(const Cage &cage);
        void encodeCageUniqueness(const Cage &cage);

        // Inequality constraints
        void encodeInequalityConstraints(const std::vector<InequalityConstraint> &inequalities);
        void encodeInequality(const InequalityConstraint &ineq);

        // Helper: At-most-one encoding
        void addAtMostOne(const std::vector<Minisat::Lit> &lits);

        // Helper: Exactly-one encoding
        void addExactlyOne(const std::vector<Minisat::Lit> &lits);

        // Helper: Add clause to solver
        void addClause(const std::vector<Minisat::Lit> &lits);
        void addClause(Minisat::Lit a);
        void addClause(Minisat::Lit a, Minisat::Lit b);
        void addClause(Minisat::Lit a, Minisat::Lit b, Minisat::Lit c);

        // Helper: Generate all subsets summing to target
        void generateSumCombinations(int numCells, int targetSum,
                                     std::vector<std::vector<int>> &result);
        void generateSumCombinationsHelper(int numCells, int targetSum, int minVal,
                                           std::vector<int> &current,
                                           std::vector<std::vector<int>> &result);
    };

} // namespace sudoku

#endif // SUDOKU_ENCODER_H
