/**
 * @file SudokuEncoder.cpp
 * @brief Implementation of SAT encoder for Sudoku puzzles
 */

#include "SudokuEncoder.h"
#include <chrono>
#include <algorithm>
#include <set>

namespace sudoku
{

    SudokuEncoder::SudokuEncoder()
        : solver(nullptr), numVariables(0), numClauses(0)
    {
    }

    SudokuEncoder::~SudokuEncoder()
    {
        if (solver)
        {
            delete solver;
        }
    }

    void SudokuEncoder::reset()
    {
        if (solver)
        {
            delete solver;
        }
        solver = new Minisat::Solver();
        numVariables = 0;
        numClauses = 0;

        // Create variables for all cells and values
        // Variable index: row * 81 + col * 9 + (value - 1)
        for (int i = 0; i < GRID_SIZE * GRID_SIZE * GRID_SIZE; i++)
        {
            solver->newVar();
            numVariables++;
        }
    }

    Minisat::Var SudokuEncoder::getVar(int row, int col, int value)
    {
        // value is 1-9, convert to 0-8 for indexing
        return row * GRID_SIZE * GRID_SIZE + col * GRID_SIZE + (value - 1);
    }

    Minisat::Lit SudokuEncoder::getLit(int row, int col, int value, bool positive)
    {
        return Minisat::mkLit(getVar(row, col, value), !positive);
    }

    void SudokuEncoder::addClause(const std::vector<Minisat::Lit> &lits)
    {
        Minisat::vec<Minisat::Lit> clause;
        for (const auto &lit : lits)
        {
            clause.push(lit);
        }
        solver->addClause(clause);
        numClauses++;
    }

    void SudokuEncoder::addClause(Minisat::Lit a)
    {
        solver->addClause(a);
        numClauses++;
    }

    void SudokuEncoder::addClause(Minisat::Lit a, Minisat::Lit b)
    {
        solver->addClause(a, b);
        numClauses++;
    }

    void SudokuEncoder::addClause(Minisat::Lit a, Minisat::Lit b, Minisat::Lit c)
    {
        solver->addClause(a, b, c);
        numClauses++;
    }

    void SudokuEncoder::addAtMostOne(const std::vector<Minisat::Lit> &lits)
    {
        // Pairwise encoding: for each pair (i, j), add clause (~li OR ~lj)
        for (size_t i = 0; i < lits.size(); i++)
        {
            for (size_t j = i + 1; j < lits.size(); j++)
            {
                addClause(~lits[i], ~lits[j]);
            }
        }
    }

    void SudokuEncoder::addExactlyOne(const std::vector<Minisat::Lit> &lits)
    {
        // At least one: OR of all literals
        addClause(lits);
        // At most one: pairwise
        addAtMostOne(lits);
    }

    void SudokuEncoder::encodeCellConstraints()
    {
        // Each cell must have exactly one value (1-9)
        for (int row = 0; row < GRID_SIZE; row++)
        {
            for (int col = 0; col < GRID_SIZE; col++)
            {
                std::vector<Minisat::Lit> lits;
                for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
                {
                    lits.push_back(getLit(row, col, val));
                }
                addExactlyOne(lits);
            }
        }
    }

    void SudokuEncoder::encodeRowConstraints()
    {
        // Each row must have each value exactly once
        for (int row = 0; row < GRID_SIZE; row++)
        {
            for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
            {
                std::vector<Minisat::Lit> lits;
                for (int col = 0; col < GRID_SIZE; col++)
                {
                    lits.push_back(getLit(row, col, val));
                }
                addExactlyOne(lits);
            }
        }
    }

    void SudokuEncoder::encodeColumnConstraints()
    {
        // Each column must have each value exactly once
        for (int col = 0; col < GRID_SIZE; col++)
        {
            for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
            {
                std::vector<Minisat::Lit> lits;
                for (int row = 0; row < GRID_SIZE; row++)
                {
                    lits.push_back(getLit(row, col, val));
                }
                addExactlyOne(lits);
            }
        }
    }

    void SudokuEncoder::encodeBoxConstraints()
    {
        // Each 3x3 box must have each value exactly once
        for (int boxRow = 0; boxRow < BOX_SIZE; boxRow++)
        {
            for (int boxCol = 0; boxCol < BOX_SIZE; boxCol++)
            {
                for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
                {
                    std::vector<Minisat::Lit> lits;
                    for (int r = 0; r < BOX_SIZE; r++)
                    {
                        for (int c = 0; c < BOX_SIZE; c++)
                        {
                            int row = boxRow * BOX_SIZE + r;
                            int col = boxCol * BOX_SIZE + c;
                            lits.push_back(getLit(row, col, val));
                        }
                    }
                    addExactlyOne(lits);
                }
            }
        }
    }

    void SudokuEncoder::encodeGivenValues(const SudokuPuzzle &puzzle)
    {
        // Add unit clauses for given values
        for (int row = 0; row < GRID_SIZE; row++)
        {
            for (int col = 0; col < GRID_SIZE; col++)
            {
                int val = puzzle.grid[row][col];
                if (val >= MIN_VALUE && val <= MAX_VALUE)
                {
                    addClause(getLit(row, col, val));
                }
            }
        }
    }

    void SudokuEncoder::encodeCageConstraints(const std::vector<Cage> &cages)
    {
        for (const auto &cage : cages)
        {
            if (cage.isValid())
            {
                encodeCageSum(cage);
                encodeCageUniqueness(cage);
            }
        }
    }

    void SudokuEncoder::generateSumCombinationsHelper(
        int numCells, int targetSum, int minVal,
        std::vector<int> &current,
        std::vector<std::vector<int>> &result)
    {

        if (numCells == 0)
        {
            if (targetSum == 0)
            {
                result.push_back(current);
            }
            return;
        }

        for (int v = minVal; v <= MAX_VALUE; v++)
        {
            if (v > targetSum)
                break;

            // Check if we can still reach the target with remaining cells
            int remainingCells = numCells - 1;
            int minRemainingSum = 0;
            for (int i = 0; i < remainingCells; i++)
            {
                minRemainingSum += (v + 1 + i);
            }
            if (minRemainingSum > targetSum - v)
                continue;

            int maxRemainingSum = 0;
            for (int i = 0; i < remainingCells; i++)
            {
                maxRemainingSum += (MAX_VALUE - i);
            }
            if (maxRemainingSum < targetSum - v)
                continue;

            current.push_back(v);
            generateSumCombinationsHelper(numCells - 1, targetSum - v, v + 1, current, result);
            current.pop_back();
        }
    }

    void SudokuEncoder::generateSumCombinations(
        int numCells, int targetSum,
        std::vector<std::vector<int>> &result)
    {

        std::vector<int> current;
        generateSumCombinationsHelper(numCells, targetSum, MIN_VALUE, current, result);
    }

    void SudokuEncoder::encodeCageSum(const Cage &cage)
    {
        // Generate all valid combinations of values that sum to target
        std::vector<std::vector<int>> combinations;
        generateSumCombinations(static_cast<int>(cage.cells.size()), cage.targetSum, combinations);

        if (combinations.empty())
        {
            // No valid combinations - add empty clause to make UNSAT
            Minisat::vec<Minisat::Lit> empty;
            solver->addClause(empty);
            return;
        }

        // For each combination, at least one assignment of values to cells must work
        // This is encoded by considering all permutations of each combination

        // For small cages, enumerate all valid assignments
        std::vector<std::vector<std::pair<Cell, int>>> validAssignments;

        for (const auto &combo : combinations)
        {
            // Generate all permutations of this combination
            std::vector<int> perm = combo;
            std::sort(perm.begin(), perm.end());

            do
            {
                std::vector<std::pair<Cell, int>> assignment;
                for (size_t i = 0; i < cage.cells.size(); i++)
                {
                    assignment.push_back({cage.cells[i], perm[i]});
                }
                validAssignments.push_back(assignment);
            } while (std::next_permutation(perm.begin(), perm.end()));
        }

        // Encode: at least one valid assignment must be chosen
        // For each valid assignment, create a fresh variable that represents it
        // Then add clause that at least one of these variables is true
        // And for each assignment variable, imply the cell values

        if (validAssignments.size() == 1)
        {
            // Only one valid assignment - just assert it directly
            for (const auto &[cell, val] : validAssignments[0])
            {
                addClause(getLit(cell.row, cell.col, val));
            }
        }
        else
        {
            // Create auxiliary variables for each valid assignment
            std::vector<Minisat::Var> assignVars;
            for (size_t i = 0; i < validAssignments.size(); i++)
            {
                Minisat::Var v = solver->newVar();
                assignVars.push_back(v);
                numVariables++;
            }

            // At least one assignment must be true
            std::vector<Minisat::Lit> atLeastOne;
            for (auto v : assignVars)
            {
                atLeastOne.push_back(Minisat::mkLit(v));
            }
            addClause(atLeastOne);

            // For each assignment variable, if it's true, the cells must have those values
            for (size_t i = 0; i < validAssignments.size(); i++)
            {
                Minisat::Lit assignLit = Minisat::mkLit(assignVars[i]);
                for (const auto &[cell, val] : validAssignments[i])
                {
                    // assignVar -> cell has value val
                    // ~assignVar OR cell[val]
                    addClause(~assignLit, getLit(cell.row, cell.col, val));
                }
            }

            // Also: if cell values match an assignment, that assignment var could be true
            // This is handled implicitly by the "at least one" clause

            // For correctness, we also need: if all cells in an assignment have their values,
            // then the assignment var must be true (for the sum constraint to be enforced)
            // Actually, this is already enforced because we said at least one assignment is true,
            // and the implications ensure consistency
        }
    }

    void SudokuEncoder::encodeCageUniqueness(const Cage &cage)
    {
        // All cells in a cage must have different values
        // For each value v, at most one cell in the cage can have v
        for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
        {
            std::vector<Minisat::Lit> lits;
            for (const auto &cell : cage.cells)
            {
                lits.push_back(getLit(cell.row, cell.col, val));
            }
            addAtMostOne(lits);
        }
    }

    void SudokuEncoder::encodeInequalityConstraints(
        const std::vector<InequalityConstraint> &inequalities)
    {
        for (const auto &ineq : inequalities)
        {
            if (ineq.isValid())
            {
                encodeInequality(ineq);
            }
        }
    }

    void SudokuEncoder::encodeInequality(const InequalityConstraint &ineq)
    {
        // cell1 > cell2 or cell1 < cell2
        // For cell1 > cell2: for all (v1, v2) where v1 <= v2,
        //   we cannot have both cell1=v1 and cell2=v2
        //   i.e., ~cell1[v1] OR ~cell2[v2]

        const Cell &c1 = ineq.cell1;
        const Cell &c2 = ineq.cell2;

        if (ineq.type == InequalityType::GREATER_THAN)
        {
            // cell1 > cell2
            for (int v1 = MIN_VALUE; v1 <= MAX_VALUE; v1++)
            {
                for (int v2 = v1; v2 <= MAX_VALUE; v2++)
                {
                    // Forbid: cell1=v1 AND cell2=v2 when v1 <= v2
                    addClause(~getLit(c1.row, c1.col, v1), ~getLit(c2.row, c2.col, v2));
                }
            }
        }
        else
        {
            // cell1 < cell2
            for (int v1 = MIN_VALUE; v1 <= MAX_VALUE; v1++)
            {
                for (int v2 = MIN_VALUE; v2 <= v1; v2++)
                {
                    // Forbid: cell1=v1 AND cell2=v2 when v1 >= v2
                    addClause(~getLit(c1.row, c1.col, v1), ~getLit(c2.row, c2.col, v2));
                }
            }
        }
    }

    SudokuSolution SudokuEncoder::solve(const SudokuPuzzle &puzzle)
    {
        SudokuSolution solution;

        auto startTime = std::chrono::high_resolution_clock::now();

        // Reset and initialize solver
        reset();

        // Encode basic Sudoku constraints
        encodeCellConstraints();
        encodeRowConstraints();
        encodeColumnConstraints();
        encodeBoxConstraints();
        encodeGivenValues(puzzle);

        // Encode Killer Sudoku constraints
        if (puzzle.hasKillerConstraints())
        {
            encodeCageConstraints(puzzle.cages);
        }

        // Encode inequality constraints
        if (puzzle.hasInequalityConstraints())
        {
            encodeInequalityConstraints(puzzle.inequalities);
        }

        // Solve
        bool sat = solver->solve();

        auto endTime = std::chrono::high_resolution_clock::now();
        solution.solveTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        if (sat)
        {
            solution.solved = true;
            // Extract solution from model
            for (int row = 0; row < GRID_SIZE; row++)
            {
                for (int col = 0; col < GRID_SIZE; col++)
                {
                    for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
                    {
                        if (solver->modelValue(getVar(row, col, val)) == Minisat::l_True)
                        {
                            solution.grid[row][col] = val;
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            solution.solved = false;
            solution.errorMessage = "No solution exists for the given puzzle.";
        }

        return solution;
    }

} // namespace sudoku
