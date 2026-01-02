/**
 * @file SudokuParser.h
 * @brief Parser for reading Sudoku puzzles from various formats
 */

#ifndef SUDOKU_PARSER_H
#define SUDOKU_PARSER_H

#include "SudokuTypes.h"
#include <string>
#include <istream>
#include <fstream>

namespace sudoku
{

    /**
     * @brief Parser for Sudoku puzzles
     *
     * Supports multiple input formats:
     *
     * 1. Simple grid format (for standard Sudoku):
     *    - 9 lines of 9 characters
     *    - Digits 1-9 for given values
     *    - 0 or . for empty cells
     *
     * 2. JSON-like format (for all variants):
     *    {
     *      "type": "killer" | "inequality" | "mixed" | "standard",
     *      "grid": [
     *        [0,0,0,0,0,0,0,0,0],
     *        ...
     *      ],
     *      "cages": [
     *        {"cells": [[0,0], [0,1]], "sum": 10},
     *        ...
     *      ],
     *      "inequalities": [
     *        {"cell1": [0,0], "cell2": [0,1], "type": ">"},
     *        ...
     *      ]
     *    }
     *
     * 3. Custom text format:
     *    GRID
     *    0 0 0 0 0 0 0 0 0
     *    ... (9 lines)
     *    CAGES
     *    sum r1 c1 r2 c2 ...
     *    ...
     *    INEQUALITIES
     *    r1 c1 > r2 c2
     *    ...
     */
    class SudokuParser
    {
    public:
        /**
         * @brief Parse a puzzle from a string
         * @param input The input string
         * @return The parsed puzzle
         */
        static SudokuPuzzle parseFromString(const std::string &input);

        /**
         * @brief Parse a puzzle from a file
         * @param filename Path to the file
         * @return The parsed puzzle
         */
        static SudokuPuzzle parseFromFile(const std::string &filename);

        /**
         * @brief Parse a simple grid string (81 characters)
         * @param grid The grid string
         * @return The parsed puzzle
         */
        static SudokuPuzzle parseSimpleGrid(const std::string &grid);

        /**
         * @brief Parse custom text format
         * @param input The input string
         * @return The parsed puzzle
         */
        static SudokuPuzzle parseCustomFormat(const std::string &input);

        /**
         * @brief Convert a puzzle to a printable string
         * @param puzzle The puzzle to convert
         * @return String representation
         */
        static std::string toString(const SudokuPuzzle &puzzle);

        /**
         * @brief Convert a solution to a printable string
         * @param solution The solution to convert
         * @return String representation
         */
        static std::string toString(const SudokuSolution &solution);

        /**
         * @brief Convert to a pretty-printed grid
         * @param grid 9x9 array of values
         * @return Pretty string with box separators
         */
        static std::string toPrettyGrid(const int grid[GRID_SIZE][GRID_SIZE]);

    private:
        static void parseLine(const std::string &line, int row, SudokuPuzzle &puzzle);
        static std::vector<std::string> splitLines(const std::string &input);
        static std::string trim(const std::string &str);
        static std::vector<std::string> split(const std::string &str, char delimiter);
    };

} // namespace sudoku

#endif // SUDOKU_PARSER_H
