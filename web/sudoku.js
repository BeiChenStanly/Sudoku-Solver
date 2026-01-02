/**
 * Sudoku Solver Web Frontend
 * Canvas-based interactive Sudoku grid
 */

// Constants
const GRID_SIZE = 9;
const BOX_SIZE = 3;
const MIN_CELLS_TO_REMOVE = 45;
const MAX_CELLS_TO_REMOVE = 65;

class SudokuGrid {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.cellSize = 50;
        this.grid = Array(GRID_SIZE).fill(null).map(() => Array(GRID_SIZE).fill(0));
        this.solution = null;
        this.selectedCell = null;
        this.givenCells = new Set(); // Cells with initial values
        this.cages = []; // For killer sudoku
        this.inequalities = []; // For inequality sudoku
        this.puzzleType = 'standard';
        
        this.setupEventListeners();
        this.draw();
    }
    
    setupEventListeners() {
        // Canvas click for cell selection
        this.canvas.addEventListener('click', (e) => {
            const rect = this.canvas.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            const col = Math.floor(x / this.cellSize);
            const row = Math.floor(y / this.cellSize);
            
            if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE) {
                this.selectedCell = { row, col };
                this.draw();
            }
        });
        
        // Keyboard input for numbers
        document.addEventListener('keydown', (e) => {
            if (!this.selectedCell) return;
            
            const { row, col } = this.selectedCell;
            const key = e.key;
            
            if (key >= '1' && key <= '9') {
                this.grid[row][col] = parseInt(key);
                this.givenCells.add(`${row},${col}`);
                this.draw();
            } else if (key === '0' || key === 'Backspace' || key === 'Delete') {
                this.grid[row][col] = 0;
                this.givenCells.delete(`${row},${col}`);
                this.draw();
            } else if (key === 'ArrowUp' && row > 0) {
                this.selectedCell.row--;
                this.draw();
            } else if (key === 'ArrowDown' && row < 8) {
                this.selectedCell.row++;
                this.draw();
            } else if (key === 'ArrowLeft' && col > 0) {
                this.selectedCell.col--;
                this.draw();
            } else if (key === 'ArrowRight' && col < 8) {
                this.selectedCell.col++;
                this.draw();
            }
        });
    }
    
    draw() {
        const ctx = this.ctx;
        const size = this.cellSize;
        
        // Clear canvas
        ctx.fillStyle = 'white';
        ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        
        // Draw cage backgrounds for killer sudoku
        if (this.puzzleType === 'killer' || this.puzzleType === 'mixed') {
            this.drawCages();
        }
        
        // Draw selected cell highlight
        if (this.selectedCell) {
            ctx.fillStyle = '#e3f2fd';
            ctx.fillRect(
                this.selectedCell.col * size,
                this.selectedCell.row * size,
                size,
                size
            );
        }
        
        // Draw grid lines
        ctx.strokeStyle = '#ccc';
        ctx.lineWidth = 1;
        
        for (let i = 0; i <= GRID_SIZE; i++) {
            // Thin lines
            ctx.beginPath();
            ctx.moveTo(i * size, 0);
            ctx.lineTo(i * size, 9 * size);
            ctx.stroke();
            
            ctx.beginPath();
            ctx.moveTo(0, i * size);
            ctx.lineTo(9 * size, i * size);
            ctx.stroke();
        }
        
        // Draw thick box lines
        ctx.strokeStyle = '#333';
        ctx.lineWidth = 3;
        
        for (let i = 0; i <= 3; i++) {
            ctx.beginPath();
            ctx.moveTo(i * BOX_SIZE * size, 0);
            ctx.lineTo(i * BOX_SIZE * size, 9 * size);
            ctx.stroke();
            
            ctx.beginPath();
            ctx.moveTo(0, i * BOX_SIZE * size);
            ctx.lineTo(9 * size, i * BOX_SIZE * size);
            ctx.stroke();
        }
        
        // Draw inequalities
        if (this.puzzleType === 'inequality' || this.puzzleType === 'mixed') {
            this.drawInequalities();
        }
        
        // Draw numbers
        ctx.font = 'bold 24px Arial';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        
        for (let row = 0; row < GRID_SIZE; row++) {
            for (let col = 0; col < GRID_SIZE; col++) {
                const value = this.grid[row][col];
                if (value !== 0) {
                    const isGiven = this.givenCells.has(`${row},${col}`);
                    ctx.fillStyle = isGiven ? '#333' : '#4a90d9';
                    ctx.fillText(
                        value.toString(),
                        col * size + size / 2,
                        row * size + size / 2
                    );
                }
            }
        }
        
        // Draw cage sums
        if (this.puzzleType === 'killer' || this.puzzleType === 'mixed') {
            this.drawCageSums();
        }
    }
    
    drawCages() {
        const ctx = this.ctx;
        const size = this.cellSize;
        const colors = [
            'rgba(255, 200, 200, 0.4)',
            'rgba(200, 255, 200, 0.4)',
            'rgba(200, 200, 255, 0.4)',
            'rgba(255, 255, 200, 0.4)',
            'rgba(255, 200, 255, 0.4)',
            'rgba(200, 255, 255, 0.4)',
            'rgba(255, 230, 200, 0.4)',
            'rgba(230, 200, 255, 0.4)',
        ];
        
        this.cages.forEach((cage, index) => {
            ctx.fillStyle = colors[index % colors.length];
            cage.cells.forEach(cell => {
                ctx.fillRect(cell.col * size + 2, cell.row * size + 2, size - 4, size - 4);
            });
            
            // Draw dashed border around cage
            ctx.strokeStyle = '#666';
            ctx.lineWidth = 1;
            ctx.setLineDash([3, 3]);
            cage.cells.forEach(cell => {
                ctx.strokeRect(cell.col * size + 2, cell.row * size + 2, size - 4, size - 4);
            });
            ctx.setLineDash([]);
        });
    }
    
    drawCageSums() {
        const ctx = this.ctx;
        const size = this.cellSize;
        
        ctx.font = '10px Arial';
        ctx.fillStyle = '#666';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'top';
        
        this.cages.forEach(cage => {
            if (cage.cells.length > 0) {
                const firstCell = cage.cells[0];
                ctx.fillText(
                    cage.sum.toString(),
                    firstCell.col * size + 3,
                    firstCell.row * size + 2
                );
            }
        });
    }
    
    drawInequalities() {
        const ctx = this.ctx;
        const size = this.cellSize;
        
        ctx.font = 'bold 16px Arial';
        ctx.fillStyle = '#d32f2f';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        
        this.inequalities.forEach(ineq => {
            const { cell1, cell2, type } = ineq;
            const x1 = cell1.col * size + size / 2;
            const y1 = cell1.row * size + size / 2;
            const x2 = cell2.col * size + size / 2;
            const y2 = cell2.row * size + size / 2;
            
            const midX = (x1 + x2) / 2;
            const midY = (y1 + y2) / 2;
            
            const symbol = type === 'greater' ? '>' : '<';
            
            // Rotate symbol for vertical inequalities
            if (cell1.col === cell2.col) {
                ctx.save();
                ctx.translate(midX, midY);
                ctx.rotate(Math.PI / 2);
                ctx.fillText(symbol, 0, 0);
                ctx.restore();
            } else {
                ctx.fillText(symbol, midX, midY);
            }
        });
    }
    
    clear() {
        this.grid = Array(GRID_SIZE).fill(null).map(() => Array(GRID_SIZE).fill(0));
        this.solution = null;
        this.givenCells.clear();
        this.cages = [];
        this.inequalities = [];
        this.selectedCell = null;
        this.draw();
    }
    
    setSolution(solutionGrid) {
        this.solution = solutionGrid;
        for (let row = 0; row < GRID_SIZE; row++) {
            for (let col = 0; col < GRID_SIZE; col++) {
                this.grid[row][col] = solutionGrid[row][col];
            }
        }
        this.draw();
    }
    
    getGridString() {
        let str = '';
        for (let row = 0; row < GRID_SIZE; row++) {
            for (let col = 0; col < GRID_SIZE; col++) {
                str += this.grid[row][col];
            }
        }
        return str;
    }
    
    setGridFromString(str) {
        this.givenCells.clear();
        for (let i = 0; i < 81 && i < str.length; i++) {
            const row = Math.floor(i / 9);
            const col = i % 9;
            const val = str[i] === '.' ? 0 : parseInt(str[i]) || 0;
            this.grid[row][col] = val;
            if (val !== 0) {
                this.givenCells.add(`${row},${col}`);
            }
        }
        this.draw();
    }
    
    addCage(cells, sum) {
        this.cages.push({ cells, sum });
        this.draw();
    }
    
    addInequality(cell1, cell2, type) {
        this.inequalities.push({ cell1, cell2, type });
        this.draw();
    }
    
    setPuzzleType(type) {
        this.puzzleType = type;
        this.draw();
    }
    
    exportPuzzle() {
        let output = 'GRID\n';
        for (let row = 0; row < GRID_SIZE; row++) {
            output += this.grid[row].join(' ') + '\n';
        }
        
        if (this.cages.length > 0) {
            output += '\nCAGES\n';
            this.cages.forEach(cage => {
                let line = cage.sum.toString();
                cage.cells.forEach(cell => {
                    line += ` ${cell.row} ${cell.col}`;
                });
                output += line + '\n';
            });
        }
        
        if (this.inequalities.length > 0) {
            output += '\nINEQUALITIES\n';
            this.inequalities.forEach(ineq => {
                const symbol = ineq.type === 'greater' ? '>' : '<';
                output += `${ineq.cell1.row} ${ineq.cell1.col} ${symbol} ${ineq.cell2.row} ${ineq.cell2.col}\n`;
            });
        }
        
        return output;
    }
}

// Sudoku Solver API (simulated for frontend demo)
class SudokuSolverAPI {
    // Simple backtracking solver for frontend demo
    static solve(grid, checkUnique = false) {
        const solution = JSON.parse(JSON.stringify(grid));
        const startTime = performance.now();
        
        const isValid = (grid, row, col, num) => {
            // Check row
            for (let c = 0; c < GRID_SIZE; c++) {
                if (grid[row][c] === num) return false;
            }
            
            // Check column
            for (let r = 0; r < GRID_SIZE; r++) {
                if (grid[r][col] === num) return false;
            }
            
            // Check 3x3 box
            const boxRow = Math.floor(row / 3) * 3;
            const boxCol = Math.floor(col / 3) * 3;
            for (let r = boxRow; r < boxRow + 3; r++) {
                for (let c = boxCol; c < boxCol + 3; c++) {
                    if (grid[r][c] === num) return false;
                }
            }
            
            return true;
        };
        
        const solveBacktrack = (grid) => {
            for (let row = 0; row < GRID_SIZE; row++) {
                for (let col = 0; col < GRID_SIZE; col++) {
                    if (grid[row][col] === 0) {
                        for (let num = 1; num <= GRID_SIZE; num++) {
                            if (isValid(grid, row, col, num)) {
                                grid[row][col] = num;
                                if (solveBacktrack(grid)) {
                                    return true;
                                }
                                grid[row][col] = 0;
                            }
                        }
                        return false;
                    }
                }
            }
            return true;
        };
        
        const solved = solveBacktrack(solution);
        const endTime = performance.now();
        
        let isUnique = null;
        if (solved && checkUnique) {
            // Check for uniqueness by trying to find another solution
            const solution2 = JSON.parse(JSON.stringify(grid));
            let solutionCount = 0;
            
            const countSolutions = (grid, limit = 2) => {
                for (let row = 0; row < GRID_SIZE; row++) {
                    for (let col = 0; col < GRID_SIZE; col++) {
                        if (grid[row][col] === 0) {
                            for (let num = 1; num <= GRID_SIZE; num++) {
                                if (isValid(grid, row, col, num)) {
                                    grid[row][col] = num;
                                    countSolutions(grid, limit);
                                    if (solutionCount >= limit) return;
                                    grid[row][col] = 0;
                                }
                            }
                            return;
                        }
                    }
                }
                solutionCount++;
            };
            
            countSolutions(solution2, 2);
            isUnique = solutionCount === 1;
        }
        
        return {
            solved,
            solution: solved ? solution : null,
            solveTimeMs: endTime - startTime,
            isUnique
        };
    }
    
    static generate(type = 'standard') {
        // Generate a solved grid first
        const grid = Array(GRID_SIZE).fill(null).map(() => Array(GRID_SIZE).fill(0));
        
        // Fill diagonal boxes first (they don't affect each other)
        for (let box = 0; box < BOX_SIZE; box++) {
            const nums = [1, 2, 3, 4, 5, 6, 7, 8, 9];
            this.shuffleArray(nums);
            let idx = 0;
            for (let r = box * 3; r < box * 3 + 3; r++) {
                for (let c = box * 3; c < box * 3 + 3; c++) {
                    grid[r][c] = nums[idx++];
                }
            }
        }
        
        // Solve the rest
        const result = this.solve(grid);
        if (!result.solved) return null;
        
        const solution = result.solution;
        const puzzle = JSON.parse(JSON.stringify(solution));
        
        // Remove some cells based on difficulty
        const cellsToRemove = MIN_CELLS_TO_REMOVE + Math.floor(Math.random() * (MAX_CELLS_TO_REMOVE - MIN_CELLS_TO_REMOVE));
        const cells = [];
        for (let r = 0; r < GRID_SIZE; r++) {
            for (let c = 0; c < GRID_SIZE; c++) {
                cells.push({ row: r, col: c });
            }
        }
        this.shuffleArray(cells);
        
        let removed = 0;
        for (const cell of cells) {
            if (removed >= cellsToRemove) break;
            puzzle[cell.row][cell.col] = 0;
            removed++;
        }
        
        return { puzzle, solution };
    }
    
    static shuffleArray(array) {
        for (let i = array.length - 1; i > 0; i--) {
            const j = Math.floor(Math.random() * (i + 1));
            [array[i], array[j]] = [array[j], array[i]];
        }
    }
}

// Main Application
document.addEventListener('DOMContentLoaded', () => {
    const canvas = document.getElementById('sudoku-canvas');
    const sudokuGrid = new SudokuGrid(canvas);
    
    const solveBtn = document.getElementById('solve-btn');
    const clearBtn = document.getElementById('clear-btn');
    const generateBtn = document.getElementById('generate-btn');
    const exampleBtn = document.getElementById('example-btn');
    const puzzleTypeSelect = document.getElementById('puzzle-type');
    const checkUniqueCheckbox = document.getElementById('check-unique');
    const statusDiv = document.getElementById('status');
    const statsDiv = document.getElementById('stats');
    const solveTimeSpan = document.getElementById('solve-time');
    const uniquenessSpan = document.getElementById('uniqueness');
    const cageControls = document.getElementById('cage-controls');
    const inequalityControls = document.getElementById('inequality-controls');
    
    // Example puzzles
    const examples = {
        standard: '530070000600195000098000060800060003400803001700020006060000280000419005000080079',
        hard: '800000000003600000070090200050007000000045700000100030001000068008500010090000400'
    };
    
    // Update UI based on puzzle type
    puzzleTypeSelect.addEventListener('change', () => {
        const type = puzzleTypeSelect.value;
        sudokuGrid.setPuzzleType(type);
        
        cageControls.classList.toggle('hidden', type !== 'killer' && type !== 'mixed');
        inequalityControls.classList.toggle('hidden', type !== 'inequality' && type !== 'mixed');
    });
    
    // Solve button
    solveBtn.addEventListener('click', () => {
        statusDiv.textContent = '正在求解...';
        statusDiv.className = 'status solving';
        solveBtn.classList.add('solving-animation');
        
        setTimeout(() => {
            const checkUnique = checkUniqueCheckbox.checked;
            const result = SudokuSolverAPI.solve(sudokuGrid.grid, checkUnique);
            
            solveBtn.classList.remove('solving-animation');
            
            if (result.solved) {
                sudokuGrid.setSolution(result.solution);
                statusDiv.textContent = '✓ 求解成功！';
                statusDiv.className = 'status success';
                
                statsDiv.classList.remove('hidden');
                solveTimeSpan.textContent = result.solveTimeMs.toFixed(2) + ' ms';
                
                if (checkUnique) {
                    uniquenessSpan.textContent = result.isUnique ? '✓ 唯一解' : '✗ 多解';
                    uniquenessSpan.style.color = result.isUnique ? '#28a745' : '#dc3545';
                } else {
                    uniquenessSpan.textContent = '未检查';
                    uniquenessSpan.style.color = '#666';
                }
            } else {
                statusDiv.textContent = '✗ 无解 - 请检查输入';
                statusDiv.className = 'status error';
                statsDiv.classList.add('hidden');
            }
        }, 100);
    });
    
    // Clear button
    clearBtn.addEventListener('click', () => {
        sudokuGrid.clear();
        statusDiv.textContent = '请输入数独谜题，然后点击"求解"';
        statusDiv.className = 'status';
        statsDiv.classList.add('hidden');
    });
    
    // Generate button
    generateBtn.addEventListener('click', () => {
        statusDiv.textContent = '正在生成谜题...';
        statusDiv.className = 'status solving';
        
        setTimeout(() => {
            const result = SudokuSolverAPI.generate(puzzleTypeSelect.value);
            if (result) {
                sudokuGrid.clear();
                for (let row = 0; row < GRID_SIZE; row++) {
                    for (let col = 0; col < GRID_SIZE; col++) {
                        sudokuGrid.grid[row][col] = result.puzzle[row][col];
                        if (result.puzzle[row][col] !== 0) {
                            sudokuGrid.givenCells.add(`${row},${col}`);
                        }
                    }
                }
                sudokuGrid.draw();
                statusDiv.textContent = '✓ 谜题已生成！';
                statusDiv.className = 'status success';
            } else {
                statusDiv.textContent = '生成失败，请重试';
                statusDiv.className = 'status error';
            }
            statsDiv.classList.add('hidden');
        }, 100);
    });
    
    // Example button
    exampleBtn.addEventListener('click', () => {
        const puzzleStr = Math.random() > 0.5 ? examples.standard : examples.hard;
        sudokuGrid.clear();
        sudokuGrid.setGridFromString(puzzleStr);
        statusDiv.textContent = '已加载示例谜题';
        statusDiv.className = 'status';
        statsDiv.classList.add('hidden');
    });
});
