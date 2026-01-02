/**
 * @file SudokuParser.cpp
 * @brief Implementation of Sudoku puzzle parser
 */

#include "SudokuParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace sudoku
{

    std::string SudokuParser::trim(const std::string &str)
    {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    std::vector<std::string> SudokuParser::splitLines(const std::string &input)
    {
        std::vector<std::string> lines;
        std::istringstream stream(input);
        std::string line;
        while (std::getline(stream, line))
        {
            line = trim(line);
            if (!line.empty())
            {
                lines.push_back(line);
            }
        }
        return lines;
    }

    std::vector<std::string> SudokuParser::split(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::istringstream stream(str);
        std::string token;
        while (std::getline(stream, token, delimiter))
        {
            token = trim(token);
            if (!token.empty())
            {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    void SudokuParser::parseLine(const std::string &line, int row, SudokuPuzzle &puzzle)
    {
        int col = 0;
        for (char c : line)
        {
            if (col >= GRID_SIZE)
                break;
            if (c == '.' || c == '0' || c == '_' || c == '*')
            {
                puzzle.grid[row][col] = EMPTY_CELL;
                col++;
            }
            else if (c >= '1' && c <= '9')
            {
                puzzle.grid[row][col] = c - '0';
                col++;
            }
            // Skip spaces and other characters
        }
    }

    SudokuPuzzle SudokuParser::parseSimpleGrid(const std::string &grid)
    {
        SudokuPuzzle puzzle;
        std::string cleaned;

        // Extract only valid characters
        for (char c : grid)
        {
            if (c == '.' || c == '0' || c == '_' || c == '*' ||
                (c >= '1' && c <= '9'))
            {
                cleaned += c;
            }
        }

        // Should have exactly 81 characters
        if (cleaned.length() < 81)
        {
            throw std::runtime_error("Grid must have at least 81 cells");
        }

        for (int i = 0; i < 81; i++)
        {
            int row = i / GRID_SIZE;
            int col = i % GRID_SIZE;
            char c = cleaned[i];

            if (c >= '1' && c <= '9')
            {
                puzzle.grid[row][col] = c - '0';
            }
            else
            {
                puzzle.grid[row][col] = EMPTY_CELL;
            }
        }

        return puzzle;
    }

    SudokuPuzzle SudokuParser::parseCustomFormat(const std::string &input)
    {
        SudokuPuzzle puzzle;
        auto lines = splitLines(input);

        enum Section
        {
            NONE,
            GRID,
            CAGES,
            INEQUALITIES
        };
        Section currentSection = NONE;
        int gridRow = 0;

        for (const auto &line : lines)
        {
            std::string upper = line;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

            if (upper == "GRID")
            {
                currentSection = GRID;
                gridRow = 0;
                continue;
            }
            else if (upper == "CAGES")
            {
                currentSection = CAGES;
                continue;
            }
            else if (upper == "INEQUALITIES")
            {
                currentSection = INEQUALITIES;
                continue;
            }

            switch (currentSection)
            {
            case GRID:
            {
                if (gridRow < GRID_SIZE)
                {
                    parseLine(line, gridRow, puzzle);
                    gridRow++;
                }
                break;
            }
            case CAGES:
            {
                // Format: sum r1 c1 r2 c2 ...
                auto tokens = split(line, ' ');
                if (tokens.size() >= 3 && (tokens.size() - 1) % 2 == 0)
                {
                    int sum = std::stoi(tokens[0]);
                    std::vector<Cell> cells;
                    for (size_t i = 1; i < tokens.size(); i += 2)
                    {
                        int r = std::stoi(tokens[i]);
                        int c = std::stoi(tokens[i + 1]);
                        cells.push_back(Cell(r, c));
                    }
                    puzzle.addCage(Cage(cells, sum));
                }
                break;
            }
            case INEQUALITIES:
            {
                // Format: r1 c1 > r2 c2 or r1 c1 < r2 c2
                auto tokens = split(line, ' ');
                if (tokens.size() == 5)
                {
                    int r1 = std::stoi(tokens[0]);
                    int c1 = std::stoi(tokens[1]);
                    std::string op = tokens[2];
                    int r2 = std::stoi(tokens[3]);
                    int c2 = std::stoi(tokens[4]);

                    InequalityType type;
                    if (op == ">" || op == "gt")
                    {
                        type = InequalityType::GREATER_THAN;
                    }
                    else if (op == "<" || op == "lt")
                    {
                        type = InequalityType::LESS_THAN;
                    }
                    else
                    {
                        continue;
                    }

                    puzzle.addInequality(InequalityConstraint(
                        Cell(r1, c1), Cell(r2, c2), type));
                }
                break;
            }
            default:
                // Try to parse as simple grid line
                if (line.length() >= GRID_SIZE)
                {
                    bool isGridLine = true;
                    int validChars = 0;
                    for (char c : line)
                    {
                        if (c == '.' || c == '0' || c == '_' || c == '*' ||
                            (c >= '1' && c <= '9') || c == ' ')
                        {
                            if (c != ' ')
                                validChars++;
                        }
                    }
                    if (validChars >= GRID_SIZE && gridRow < GRID_SIZE)
                    {
                        currentSection = GRID;
                        parseLine(line, gridRow, puzzle);
                        gridRow++;
                    }
                }
                break;
            }
        }

        return puzzle;
    }

    SudokuPuzzle SudokuParser::parseFromString(const std::string &input)
    {
        std::string trimmed = trim(input);

        // Try to detect format
        if (trimmed.find("GRID") != std::string::npos ||
            trimmed.find("CAGES") != std::string::npos ||
            trimmed.find("INEQUALITIES") != std::string::npos)
        {
            return parseCustomFormat(input);
        }

        // Count valid grid characters
        int validChars = 0;
        for (char c : trimmed)
        {
            if (c == '.' || c == '0' || c == '_' || c == '*' ||
                (c >= '1' && c <= '9'))
            {
                validChars++;
            }
        }

        if (validChars >= 81)
        {
            return parseSimpleGrid(trimmed);
        }

        // Try custom format
        return parseCustomFormat(input);
    }

    SudokuPuzzle SudokuParser::parseFromFile(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return parseFromString(buffer.str());
    }

    std::string SudokuParser::toPrettyGrid(const int grid[GRID_SIZE][GRID_SIZE])
    {
        std::ostringstream oss;

        oss << "+-------+-------+-------+\n";
        for (int row = 0; row < GRID_SIZE; row++)
        {
            if (row > 0 && row % BOX_SIZE == 0)
            {
                oss << "+-------+-------+-------+\n";
            }
            oss << "|";
            for (int col = 0; col < GRID_SIZE; col++)
            {
                if (col > 0 && col % BOX_SIZE == 0)
                {
                    oss << " |";
                }
                int val = grid[row][col];
                if (val >= MIN_VALUE && val <= MAX_VALUE)
                {
                    oss << " " << val;
                }
                else
                {
                    oss << " .";
                }
            }
            oss << " |\n";
        }
        oss << "+-------+-------+-------+\n";

        return oss.str();
    }

    std::string SudokuParser::toString(const SudokuPuzzle &puzzle)
    {
        std::ostringstream oss;

        oss << "Type: " << puzzle.getTypeString() << "\n\n";
        oss << "Grid:\n";
        oss << toPrettyGrid(puzzle.grid);

        if (puzzle.hasKillerConstraints())
        {
            oss << "\nCages (" << puzzle.cages.size() << "):\n";
            for (size_t i = 0; i < puzzle.cages.size(); i++)
            {
                const auto &cage = puzzle.cages[i];
                oss << "  Cage " << (i + 1) << ": sum=" << cage.targetSum << ", cells=[";
                for (size_t j = 0; j < cage.cells.size(); j++)
                {
                    if (j > 0)
                        oss << ", ";
                    oss << "(" << cage.cells[j].row << "," << cage.cells[j].col << ")";
                }
                oss << "]\n";
            }
        }

        if (puzzle.hasInequalityConstraints())
        {
            oss << "\nInequalities (" << puzzle.inequalities.size() << "):\n";
            for (const auto &ineq : puzzle.inequalities)
            {
                oss << "  (" << ineq.cell1.row << "," << ineq.cell1.col << ") ";
                oss << (ineq.type == InequalityType::GREATER_THAN ? ">" : "<");
                oss << " (" << ineq.cell2.row << "," << ineq.cell2.col << ")\n";
            }
        }

        return oss.str();
    }

    std::string SudokuParser::toString(const SudokuSolution &solution)
    {
        std::ostringstream oss;

        if (solution.solved)
        {
            oss << "Solution found in " << solution.solveTimeMs << " ms:\n\n";
            oss << toPrettyGrid(solution.grid);
        }
        else
        {
            oss << "No solution found.\n";
            if (!solution.errorMessage.empty())
            {
                oss << "Error: " << solution.errorMessage << "\n";
            }
        }

        return oss.str();
    }

} // namespace sudoku
