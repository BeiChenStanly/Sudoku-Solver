// 数独类型定义

export interface Cell {
  row: number;
  col: number;
}

export interface Cage {
  sum: number;
  cells: Cell[];
  color?: string;
}

export interface Inequality {
  cell1: Cell;
  cell2: Cell;
  type: ">" | "<";
}

export interface SudokuPuzzle {
  grid: number[][];
  cages: Cage[];
  inequalities: Inequality[];
}

export interface SolveResult {
  solved: boolean;
  grid?: number[][];
  solveTimeMs?: number;
  variables?: number;
  clauses?: number;
  uniqueness?: "unique" | "not_unique" | "unknown";
  error?: string;
}

export type PuzzleType = "standard" | "killer" | "inequality" | "mixed";

// 笼子颜色方案
export const cageColors = [
  "rgba(255, 200, 200, 0.4)", // 浅红
  "rgba(200, 230, 255, 0.4)", // 浅蓝
  "rgba(200, 255, 200, 0.4)", // 浅绿
  "rgba(255, 240, 200, 0.4)", // 浅黄
  "rgba(230, 200, 255, 0.4)", // 浅紫
  "rgba(200, 255, 255, 0.4)", // 浅青
  "rgba(255, 220, 230, 0.4)", // 浅粉
  "rgba(230, 255, 220, 0.4)", // 浅绿黄
  "rgba(255, 230, 200, 0.4)", // 浅橙
  "rgba(220, 220, 255, 0.4)", // 浅蓝紫
];

// 解析谜题字符串
export function parsePuzzle(input: string): SudokuPuzzle {
  const lines = input.trim().split("\n");
  const grid: number[][] = [];
  const cages: Cage[] = [];
  const inequalities: Inequality[] = [];

  let section = "";

  for (const line of lines) {
    const trimmed = line.trim();

    if (trimmed === "GRID") {
      section = "GRID";
      continue;
    } else if (trimmed === "CAGES") {
      section = "CAGES";
      continue;
    } else if (trimmed === "INEQUALITIES") {
      section = "INEQUALITIES";
      continue;
    } else if (trimmed === "SOLUTION") {
      break; // 忽略解答部分
    }

    if (!trimmed) continue;

    if (section === "GRID") {
      const row = trimmed.split(/\s+/).map((s) => parseInt(s) || 0);
      if (row.length === 9) {
        grid.push(row);
      }
    } else if (section === "CAGES") {
      const parts = trimmed.split(/\s+/).map((s) => parseInt(s) || 0);
      if (parts.length >= 3) {
        const sum = parts[0]!;
        const cells: Cell[] = [];
        for (let i = 1; i < parts.length; i += 2) {
          cells.push({ row: parts[i]!, col: parts[i + 1]! });
        }
        cages.push({
          sum,
          cells,
          color: cageColors[cages.length % cageColors.length],
        });
      }
    } else if (section === "INEQUALITIES") {
      const match = trimmed.match(/(\d+)\s+(\d+)\s+([><])\s+(\d+)\s+(\d+)/);
      if (match) {
        inequalities.push({
          cell1: { row: parseInt(match[1]!), col: parseInt(match[2]!) },
          cell2: { row: parseInt(match[4]!), col: parseInt(match[5]!) },
          type: match[3]! as ">" | "<",
        });
      }
    }
  }

  // 确保 grid 是 9x9
  while (grid.length < 9) {
    grid.push(Array(9).fill(0));
  }

  return { grid, cages, inequalities };
}

// 将谜题转为自定义格式字符串
export function puzzleToString(puzzle: SudokuPuzzle): string {
  let result = "GRID\n";
  for (const row of puzzle.grid) {
    result += row.join(" ") + "\n";
  }

  if (puzzle.cages.length > 0) {
    result += "\nCAGES\n";
    for (const cage of puzzle.cages) {
      result += cage.sum;
      for (const cell of cage.cells) {
        result += ` ${cell.row} ${cell.col}`;
      }
      result += "\n";
    }
  }

  if (puzzle.inequalities.length > 0) {
    result += "\nINEQUALITIES\n";
    for (const ineq of puzzle.inequalities) {
      result += `${ineq.cell1.row} ${ineq.cell1.col} ${ineq.type} ${ineq.cell2.row} ${ineq.cell2.col}\n`;
    }
  }

  return result;
}

// 获取格子所属的笼子
export function getCageForCell(
  cages: Cage[],
  row: number,
  col: number
): Cage | undefined {
  return cages.find((cage) =>
    cage.cells.some((cell) => cell.row === row && cell.col === col)
  );
}

// 获取笼子左上角格子
export function getCageTopLeft(cage: Cage): Cell {
  const initial = cage.cells[0]!;
  return cage.cells.reduce((best, cell) => {
    if (cell.row < best.row || (cell.row === best.row && cell.col < best.col)) {
      return cell;
    }
    return best;
  }, initial);
}

// 检查两个格子是否相邻
export function areAdjacent(c1: Cell, c2: Cell): boolean {
  return Math.abs(c1.row - c2.row) + Math.abs(c1.col - c2.col) === 1;
}

// 获取两个格子之间的不等式
export function getInequalityBetween(
  inequalities: Inequality[],
  row1: number,
  col1: number,
  row2: number,
  col2: number
): Inequality | undefined {
  return inequalities.find(
    (ineq) =>
      (ineq.cell1.row === row1 &&
        ineq.cell1.col === col1 &&
        ineq.cell2.row === row2 &&
        ineq.cell2.col === col2) ||
      (ineq.cell1.row === row2 &&
        ineq.cell1.col === col2 &&
        ineq.cell2.row === row1 &&
        ineq.cell2.col === col1)
  );
}

// 检查格子边界是否与同笼相邻格子共享
export function isSameCageBorder(
  cages: Cage[],
  row: number,
  col: number,
  neighborRow: number,
  neighborCol: number
): boolean {
  const cage = getCageForCell(cages, row, col);
  if (!cage) return false;
  return cage.cells.some((c) => c.row === neighborRow && c.col === neighborCol);
}
