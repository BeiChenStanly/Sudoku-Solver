/**
 * @file SudokuGenerator.cpp
 * @brief Implementation of Sudoku puzzle generator
 */

#include "SudokuGenerator.h"
#include "SudokuParser.h"
#include <algorithm>
#include <sstream>
#include <chrono>
#include <queue>

namespace sudoku
{

    SudokuGenerator::SudokuGenerator()
    {
        // Initialize RNG with random seed
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        rng.seed(static_cast<unsigned int>(seed));
    }

    SudokuGenerator::~SudokuGenerator()
    {
    }

    SudokuPuzzle SudokuGenerator::generate()
    {
        GeneratorConfig config;
        return generate(config);
    }

    SudokuPuzzle SudokuGenerator::generate(const GeneratorConfig &config)
    {
        SudokuSolution solution;
        return generateWithSolution(config, solution);
    }

    SudokuPuzzle SudokuGenerator::generateWithSolution(const GeneratorConfig &config,
                                                       SudokuSolution &solution)
    {
        // Set seed if specified
        if (config.seed != 0)
        {
            rng.seed(config.seed);
        }

        SudokuPuzzle puzzle;

        // Step 1: Generate a complete valid solution
        if (!generateCompleteSolution(solution))
        {
            // Fallback: use solver to generate any valid grid
            SudokuPuzzle empty;
            solution = solver.solve(empty);
        }

        // Step 2: Generate constraints based on puzzle type
        if (config.type == SudokuType::KILLER || config.type == SudokuType::KILLER_INEQUALITY)
        {
            if (config.fillAllCells)
            {
                // Generate cages that cover all cells
                generateCagesFillingAll(puzzle, solution, config.minCageSize, config.maxCageSize);
            }
            else
            {
                std::uniform_int_distribution<int> cageDist(config.minCages, config.maxCages);
                int numCages = cageDist(rng);
                generateCages(puzzle, solution, numCages, config.minCageSize, config.maxCageSize);
            }
        }

        if (config.type == SudokuType::INEQUALITY || config.type == SudokuType::KILLER_INEQUALITY)
        {
            std::uniform_int_distribution<int> ineqDist(config.minInequalities, config.maxInequalities);
            int numInequalities = ineqDist(rng);
            generateInequalities(puzzle, solution, numInequalities);
        }

        // Step 3: Add given values if configured
        if (config.maxGivens > 0)
        {
            std::uniform_int_distribution<int> givenDist(config.minGivens, config.maxGivens);
            int numGivens = givenDist(rng);
            addGivens(puzzle, solution, numGivens);
        }

        // Step 4: Verify unique solution if required
        if (config.ensureUniqueSolution)
        {
            // Check if the puzzle has a unique solution
            auto testSolution = solver.solve(puzzle, true); // Check uniqueness

            // Maximum attempts to achieve uniqueness through constraints
            const int kMaxConstraintAttempts = 10;
            // Maximum given values to add (failsafe to prevent infinite loop)
            const int kMaxGivensToAdd = 81; // Can't add more than 81 givens

            int attempts = 0;
            while (testSolution.solved && !testSolution.isUnique() && attempts < kMaxConstraintAttempts)
            {
                // Add more constraints to ensure uniqueness
                if (config.type == SudokuType::INEQUALITY || config.type == SudokuType::KILLER_INEQUALITY)
                {
                    // Try adding more inequalities first
                    generateInequalities(puzzle, solution, 5);
                }
                else
                {
                    // Add given values
                    addGivens(puzzle, solution, 3);
                }

                testSolution = solver.solve(puzzle, true);
                attempts++;
            }

            // If still not unique after max constraint attempts, add givens one at a time
            int givensAdded = 0;
            while (testSolution.solved && !testSolution.isUnique() && givensAdded < kMaxGivensToAdd)
            {
                addGivens(puzzle, solution, 1);
                testSolution = solver.solve(puzzle, true);
                givensAdded++;
            }

            // Step 5: Minimize constraints while maintaining uniqueness
            // This makes the puzzle harder for humans by removing redundant constraints
            minimizeConstraints(puzzle, solution);
        }

        return puzzle;
    }

    bool SudokuGenerator::generateCompleteSolution(SudokuSolution &solution)
    {
        // Use SAT solver to generate a random valid solution
        // We do this by solving an empty grid with random variable ordering
        SudokuPuzzle empty;

        // Add some random initial values to diversify solutions
        std::vector<std::pair<Cell, int>> candidates;
        for (int r = 0; r < GRID_SIZE; r++)
        {
            for (int c = 0; c < GRID_SIZE; c++)
            {
                for (int v = MIN_VALUE; v <= MAX_VALUE; v++)
                {
                    candidates.push_back({{r, c}, v});
                }
            }
        }

        std::shuffle(candidates.begin(), candidates.end(), rng);

        // Try to set a few random values
        int set = 0;
        for (const auto &[cell, val] : candidates)
        {
            if (set >= 11)
                break; // Set about 11 random values

            // Check if this cell is already set
            if (empty.grid[cell.row][cell.col] != EMPTY_CELL)
                continue;

            // Check if this value is valid (not already in row/col/box)
            bool valid = true;

            // Check row
            for (int c = 0; c < GRID_SIZE && valid; c++)
            {
                if (empty.grid[cell.row][c] == val)
                    valid = false;
            }

            // Check column
            for (int r = 0; r < GRID_SIZE && valid; r++)
            {
                if (empty.grid[r][cell.col] == val)
                    valid = false;
            }

            // Check box
            int boxR = (cell.row / BOX_SIZE) * BOX_SIZE;
            int boxC = (cell.col / BOX_SIZE) * BOX_SIZE;
            for (int r = boxR; r < boxR + BOX_SIZE && valid; r++)
            {
                for (int c = boxC; c < boxC + BOX_SIZE && valid; c++)
                {
                    if (empty.grid[r][c] == val)
                        valid = false;
                }
            }

            if (valid)
            {
                empty.grid[cell.row][cell.col] = val;
                set++;
            }
        }

        // Solve to get complete grid
        solution = solver.solve(empty);
        return solution.solved;
    }

    std::vector<Cell> SudokuGenerator::getAdjacentCells(const Cell &cell)
    {
        std::vector<Cell> adjacent;

        // 4-connected neighbors
        const int dr[] = {-1, 1, 0, 0};
        const int dc[] = {0, 0, -1, 1};

        for (int i = 0; i < 4; i++)
        {
            int nr = cell.row + dr[i];
            int nc = cell.col + dc[i];
            if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE)
            {
                adjacent.push_back({nr, nc});
            }
        }

        return adjacent;
    }

    std::vector<Cell> SudokuGenerator::generateConnectedCage(
        const SudokuSolution &solution,
        std::set<Cell> &usedCells,
        int targetSize)
    {

        std::vector<Cell> cage;

        // Find a starting cell that's not used
        std::vector<Cell> availableCells;
        for (int r = 0; r < GRID_SIZE; r++)
        {
            for (int c = 0; c < GRID_SIZE; c++)
            {
                Cell cell(r, c);
                if (usedCells.find(cell) == usedCells.end())
                {
                    availableCells.push_back(cell);
                }
            }
        }

        if (availableCells.empty())
            return cage;

        // Pick random starting cell
        std::uniform_int_distribution<size_t> dist(0, availableCells.size() - 1);
        Cell start = availableCells[dist(rng)];

        cage.push_back(start);
        usedCells.insert(start);

        // Grow cage using BFS-like expansion
        int maxAttempts = 100; // 防止无限循环
        int attempts = 0;
        while (static_cast<int>(cage.size()) < targetSize && attempts < maxAttempts)
        {
            attempts++;

            // Collect all neighbors of current cage cells
            std::vector<Cell> neighbors;
            for (const auto &cell : cage)
            {
                for (const auto &adj : getAdjacentCells(cell))
                {
                    if (usedCells.find(adj) == usedCells.end())
                    {
                        // Check if this neighbor is not already in neighbors list
                        bool found = false;
                        for (const auto &n : neighbors)
                        {
                            if (n == adj)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                            neighbors.push_back(adj);
                    }
                }
            }

            if (neighbors.empty())
                break; // Can't grow anymore

            // Pick random neighbor
            std::uniform_int_distribution<size_t> ndist(0, neighbors.size() - 1);
            Cell next = neighbors[ndist(rng)];

            // Check that adding this cell doesn't create duplicate values in cage
            int nextVal = solution.grid[next.row][next.col];
            bool duplicate = false;
            for (const auto &cell : cage)
            {
                if (solution.grid[cell.row][cell.col] == nextVal)
                {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate)
            {
                cage.push_back(next);
                usedCells.insert(next);
            }
            // 注意：不再将重复值的单元格标记为已使用
            // 这样它们可以被后续的笼子正确覆盖
        }

        return cage;
    }

    int SudokuGenerator::calculateCageSum(const std::vector<Cell> &cells,
                                          const SudokuSolution &solution)
    {
        int sum = 0;
        for (const auto &cell : cells)
        {
            sum += solution.grid[cell.row][cell.col];
        }
        return sum;
    }

    void SudokuGenerator::generateCages(SudokuPuzzle &puzzle,
                                        const SudokuSolution &solution,
                                        int numCages, int minSize, int maxSize)
    {
        std::set<Cell> usedCells;
        std::uniform_int_distribution<int> sizeDist(minSize, maxSize);

        for (int i = 0; i < numCages; i++)
        {
            int targetSize = sizeDist(rng);

            auto cells = generateConnectedCage(solution, usedCells, targetSize);

            if (cells.size() >= 2)
            { // Only add cages with at least 2 cells
                int sum = calculateCageSum(cells, solution);
                puzzle.addCage(Cage(cells, sum));
            }
        }
    }

    void SudokuGenerator::generateCagesFillingAll(SudokuPuzzle &puzzle,
                                                  const SudokuSolution &solution,
                                                  int minSize, int maxSize)
    {
        std::set<Cell> usedCells;
        std::uniform_int_distribution<int> sizeDist(minSize, maxSize);

        // Keep generating cages until all cells are covered
        while (usedCells.size() < GRID_SIZE * GRID_SIZE)
        {
            int targetSize = sizeDist(rng);

            // Adjust target size if not enough cells remaining
            int remainingCells = GRID_SIZE * GRID_SIZE - static_cast<int>(usedCells.size());
            if (targetSize > remainingCells)
            {
                targetSize = remainingCells;
            }
            if (targetSize < minSize && remainingCells >= minSize)
            {
                targetSize = minSize;
            }

            auto cells = generateConnectedCage(solution, usedCells, targetSize);

            if (cells.size() >= 2)
            { // Only add cages with at least 2 cells
                int sum = calculateCageSum(cells, solution);
                puzzle.addCage(Cage(cells, sum));
            }
            else if (cells.size() == 1)
            {
                // Single cell left - create a single-cell cage (sum = value)
                int sum = calculateCageSum(cells, solution);
                puzzle.addCage(Cage(cells, sum));
            }
            else
            {
                // No more cells available or couldn't create cage
                break;
            }
        }
    }

    void SudokuGenerator::generateInequalities(SudokuPuzzle &puzzle,
                                               const SudokuSolution &solution,
                                               int numInequalities)
    {
        // Collect all possible adjacent pairs
        std::vector<std::pair<Cell, Cell>> pairs;

        for (int r = 0; r < GRID_SIZE; r++)
        {
            for (int c = 0; c < GRID_SIZE; c++)
            {
                // Horizontal pair
                if (c + 1 < GRID_SIZE)
                {
                    pairs.push_back({{r, c}, {r, c + 1}});
                }
                // Vertical pair
                if (r + 1 < GRID_SIZE)
                {
                    pairs.push_back({{r, c}, {r + 1, c}});
                }
            }
        }

        // Shuffle and pick
        std::shuffle(pairs.begin(), pairs.end(), rng);

        int added = 0;
        for (const auto &[cell1, cell2] : pairs)
        {
            if (added >= numInequalities)
                break;

            int val1 = solution.grid[cell1.row][cell1.col];
            int val2 = solution.grid[cell2.row][cell2.col];

            if (val1 != val2)
            { // Only add if values are different
                InequalityType type = (val1 > val2) ? InequalityType::GREATER_THAN
                                                    : InequalityType::LESS_THAN;
                puzzle.addInequality(InequalityConstraint(cell1, cell2, type));
                added++;
            }
        }
    }

    void SudokuGenerator::addGivens(SudokuPuzzle &puzzle,
                                    const SudokuSolution &solution,
                                    int numGivens)
    {
        // Collect all cells
        std::vector<Cell> cells;
        for (int r = 0; r < GRID_SIZE; r++)
        {
            for (int c = 0; c < GRID_SIZE; c++)
            {
                if (puzzle.grid[r][c] == EMPTY_CELL)
                {
                    cells.push_back({r, c});
                }
            }
        }

        std::shuffle(cells.begin(), cells.end(), rng);

        int added = 0;
        for (const auto &cell : cells)
        {
            if (added >= numGivens)
                break;
            puzzle.grid[cell.row][cell.col] = solution.grid[cell.row][cell.col];
            added++;
        }
    }

    bool SudokuGenerator::hasUniqueSolution(const SudokuPuzzle &puzzle)
    {
        // For now, just check if it has at least one solution
        // A full uniqueness check would require enumeration
        auto solution = solver.solve(puzzle);
        return solution.solved;
    }

    void SudokuGenerator::minimizeConstraints(SudokuPuzzle &puzzle, const SudokuSolution &solution)
    {
        // Try to remove constraints one by one while maintaining uniqueness
        // This makes the puzzle harder by having fewer hints

        // First, try removing inequalities (they tend to be more redundant)
        if (!puzzle.inequalities.empty())
        {
            std::vector<InequalityConstraint> originalInequalities = puzzle.inequalities;
            std::vector<size_t> indices(originalInequalities.size());
            for (size_t i = 0; i < indices.size(); i++)
            {
                indices[i] = i;
            }
            std::shuffle(indices.begin(), indices.end(), rng);

            std::vector<bool> removed(originalInequalities.size(), false);

            for (size_t idx : indices)
            {
                // Try removing this inequality
                puzzle.inequalities.clear();
                for (size_t i = 0; i < originalInequalities.size(); i++)
                {
                    if (!removed[i] && i != idx)
                    {
                        puzzle.inequalities.push_back(originalInequalities[i]);
                    }
                }

                // Check if still unique
                auto testSolution = solver.solve(puzzle, true);
                if (testSolution.solved && testSolution.isUnique())
                {
                    // Can remove this inequality
                    removed[idx] = true;
                }
                else
                {
                    // Must keep this inequality - restore it
                    puzzle.inequalities.clear();
                    for (size_t i = 0; i < originalInequalities.size(); i++)
                    {
                        if (!removed[i])
                        {
                            puzzle.inequalities.push_back(originalInequalities[i]);
                        }
                    }
                }
            }
        }

        // Then, try removing cages (more important constraints, try fewer)
        if (!puzzle.cages.empty())
        {
            std::vector<Cage> originalCages = puzzle.cages;
            std::vector<size_t> indices(originalCages.size());
            for (size_t i = 0; i < indices.size(); i++)
            {
                indices[i] = i;
            }
            std::shuffle(indices.begin(), indices.end(), rng);

            std::vector<bool> removed(originalCages.size(), false);

            for (size_t idx : indices)
            {
                // Try removing this cage
                puzzle.cages.clear();
                for (size_t i = 0; i < originalCages.size(); i++)
                {
                    if (!removed[i] && i != idx)
                    {
                        puzzle.cages.push_back(originalCages[i]);
                    }
                }

                // Check if still unique
                auto testSolution = solver.solve(puzzle, true);
                if (testSolution.solved && testSolution.isUnique())
                {
                    // Can remove this cage
                    removed[idx] = true;
                }
                else
                {
                    // Must keep this cage - restore it
                    puzzle.cages.clear();
                    for (size_t i = 0; i < originalCages.size(); i++)
                    {
                        if (!removed[i])
                        {
                            puzzle.cages.push_back(originalCages[i]);
                        }
                    }
                }
            }
        }

        // Finally, try removing given values
        std::vector<Cell> givenCells;
        for (int r = 0; r < GRID_SIZE; r++)
        {
            for (int c = 0; c < GRID_SIZE; c++)
            {
                if (puzzle.grid[r][c] != EMPTY_CELL)
                {
                    givenCells.push_back({r, c});
                }
            }
        }

        if (!givenCells.empty())
        {
            std::shuffle(givenCells.begin(), givenCells.end(), rng);

            for (const auto &cell : givenCells)
            {
                int originalValue = puzzle.grid[cell.row][cell.col];

                // Try removing this given
                puzzle.grid[cell.row][cell.col] = EMPTY_CELL;

                // Check if still unique
                auto testSolution = solver.solve(puzzle, true);
                if (!testSolution.solved || !testSolution.isUnique())
                {
                    // Must keep this given - restore it
                    puzzle.grid[cell.row][cell.col] = originalValue;
                }
            }
        }
    }

    std::string SudokuGenerator::toCustomFormat(const SudokuPuzzle &puzzle)
    {
        std::ostringstream oss;

        // Grid section
        oss << "GRID\n";
        for (int r = 0; r < GRID_SIZE; r++)
        {
            for (int c = 0; c < GRID_SIZE; c++)
            {
                if (c > 0)
                    oss << " ";
                oss << puzzle.grid[r][c];
            }
            oss << "\n";
        }

        // Cages section
        if (!puzzle.cages.empty())
        {
            oss << "\nCAGES\n";
            for (const auto &cage : puzzle.cages)
            {
                oss << cage.targetSum;
                for (const auto &cell : cage.cells)
                {
                    oss << " " << cell.row << " " << cell.col;
                }
                oss << "\n";
            }
        }

        // Inequalities section
        if (!puzzle.inequalities.empty())
        {
            oss << "\nINEQUALITIES\n";
            for (const auto &ineq : puzzle.inequalities)
            {
                oss << ineq.cell1.row << " " << ineq.cell1.col << " ";
                oss << (ineq.type == InequalityType::GREATER_THAN ? ">" : "<");
                oss << " " << ineq.cell2.row << " " << ineq.cell2.col << "\n";
            }
        }

        return oss.str();
    }

    std::string SudokuGenerator::toCustomFormatWithSolution(const SudokuPuzzle &puzzle,
                                                            const SudokuSolution &solution)
    {
        std::ostringstream oss;

        // Puzzle
        oss << toCustomFormat(puzzle);

        // Solution
        oss << "\nSOLUTION\n";
        for (int r = 0; r < GRID_SIZE; r++)
        {
            for (int c = 0; c < GRID_SIZE; c++)
            {
                if (c > 0)
                    oss << " ";
                oss << solution.grid[r][c];
            }
            oss << "\n";
        }

        return oss.str();
    }

    template <typename T>
    std::vector<T> SudokuGenerator::shuffleAndPick(std::vector<T> items, int count)
    {
        std::shuffle(items.begin(), items.end(), rng);
        if (count >= static_cast<int>(items.size()))
        {
            return items;
        }
        return std::vector<T>(items.begin(), items.begin() + count);
    }

} // namespace sudoku
