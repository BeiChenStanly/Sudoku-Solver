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

        // OPTIMIZED ENCODING:
        // Since cage uniqueness is encoded separately (each value appears at most once),
        // we only need to ensure the SET of values equals one of the valid combinations.
        // 
        // Key insight: No need to enumerate permutations! We work with combinations only.
        // For a cage with n cells, if we pick combination {v1, v2, ..., vn}:
        // - Each value in the combination must appear in SOME cell (at-least-one)
        // - Values outside the combination must NOT appear in any cell (forbidden)
        //
        // Combined with cage uniqueness (at-most-one per value), this ensures
        // exactly the right set of values.

        if (combinations.size() == 1)
        {
            // Only one valid combination - directly constrain cells
            const auto &combo = combinations[0];
            std::set<int> validValues(combo.begin(), combo.end());
            
            // For each value in the combination: at least one cell must have it
            // (Combined with uniqueness, exactly one cell has it)
            for (int val : combo)
            {
                std::vector<Minisat::Lit> atLeastOne;
                for (const auto &cell : cage.cells)
                {
                    atLeastOne.push_back(getLit(cell.row, cell.col, val));
                }
                addClause(atLeastOne);
            }
            
            // For each value NOT in the combination: no cell can have it
            for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
            {
                if (validValues.find(val) == validValues.end())
                {
                    for (const auto &cell : cage.cells)
                    {
                        addClause(~getLit(cell.row, cell.col, val));
                    }
                }
            }
        }
        else
        {
            // Multiple valid combinations - use auxiliary variables for combinations
            std::vector<Minisat::Var> comboVars;
            for (size_t i = 0; i < combinations.size(); i++)
            {
                Minisat::Var v = solver->newVar();
                comboVars.push_back(v);
                numVariables++;
            }

            // Exactly one combination must be chosen
            std::vector<Minisat::Lit> comboLits;
            for (auto v : comboVars)
            {
                comboLits.push_back(Minisat::mkLit(v));
            }
            addExactlyOne(comboLits);

            // For each combination:
            // If combo_i is true:
            //   - Each value in combo_i must appear in some cell
            //   - Each value NOT in combo_i must not appear in any cell
            for (size_t i = 0; i < combinations.size(); i++)
            {
                Minisat::Lit comboLit = Minisat::mkLit(comboVars[i]);
                const auto &combo = combinations[i];
                std::set<int> comboSet(combo.begin(), combo.end());

                // If combo is chosen, each value in it must appear in some cell
                for (int val : combo)
                {
                    // combo_i -> (cell_0[val] OR cell_1[val] OR ... OR cell_n[val])
                    // Equivalent to: ~combo_i OR cell_0[val] OR ... OR cell_n[val]
                    std::vector<Minisat::Lit> clause;
                    clause.push_back(~comboLit);
                    for (const auto &cell : cage.cells)
                    {
                        clause.push_back(getLit(cell.row, cell.col, val));
                    }
                    addClause(clause);
                }

                // If combo is chosen, values outside it must not appear
                for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
                {
                    if (comboSet.find(val) == comboSet.end())
                    {
                        // combo_i -> (~cell_0[val] AND ~cell_1[val] AND ... AND ~cell_n[val])
                        // Equivalent to: for each cell, ~combo_i OR ~cell[val]
                        for (const auto &cell : cage.cells)
                        {
                            addClause(~comboLit, ~getLit(cell.row, cell.col, val));
                        }
                    }
                }
            }

            // Channeling: if all cells have values from a combination, that combo must be true
            // For efficiency, encode the reverse implication for each cell-value pair
            for (const auto &cell : cage.cells)
            {
                for (int val = MIN_VALUE; val <= MAX_VALUE; val++)
                {
                    // cell[val] -> (combo_i1 OR combo_i2 OR ...) where val is in combo_i
                    // This ensures if a cell has value v, some combo containing v is chosen
                    std::vector<Minisat::Lit> supportingCombos;
                    for (size_t i = 0; i < combinations.size(); i++)
                    {
                        const auto &combo = combinations[i];
                        if (std::find(combo.begin(), combo.end(), val) != combo.end())
                        {
                            supportingCombos.push_back(Minisat::mkLit(comboVars[i]));
                        }
                    }
                    
                    if (supportingCombos.empty())
                    {
                        // No combination supports this value - forbid it
                        addClause(~getLit(cell.row, cell.col, val));
                    }
                    else if (supportingCombos.size() < combinations.size())
                    {
                        // cell[val] -> OR(supporting combos)
                        // ~cell[val] OR supporting_combo_1 OR supporting_combo_2 OR ...
                        std::vector<Minisat::Lit> clause;
                        clause.push_back(~getLit(cell.row, cell.col, val));
                        for (const auto &lit : supportingCombos)
                        {
                            clause.push_back(lit);
                        }
                        addClause(clause);
                    }
                    // If all combinations support this value, no constraint needed
                }
            }
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

    SudokuSolution SudokuEncoder::solve(const SudokuPuzzle &puzzle, bool checkUniqueness)
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

            // Check uniqueness if requested
            if (checkUniqueness)
            {
                // Block the current solution by adding a clause that says
                // at least one cell must have a different value
                Minisat::vec<Minisat::Lit> blockingClause;
                for (int row = 0; row < GRID_SIZE; row++)
                {
                    for (int col = 0; col < GRID_SIZE; col++)
                    {
                        int val = solution.grid[row][col];
                        // Add the negation of this assignment
                        blockingClause.push(~getLit(row, col, val));
                    }
                }
                solver->addClause(blockingClause);
                numClauses++;

                // Try to find another solution
                auto uniqueStartTime = std::chrono::high_resolution_clock::now();
                bool hasSecondSolution = solver->solve();
                auto uniqueEndTime = std::chrono::high_resolution_clock::now();
                double uniqueTimeMs = std::chrono::duration<double, std::milli>(uniqueEndTime - uniqueStartTime).count();
                solution.solveTimeMs += uniqueTimeMs;

                solution.isUnique = !hasSecondSolution;
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
