<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount, watch } from "vue";
import type { SudokuPuzzle, Cell, PuzzleType } from "../types/sudoku";
import {
  parsePuzzle,
  puzzleToString,
  getCageForCell,
  getCageTopLeft,
  isSameCageBorder,
} from "../types/sudoku";
import { solvePuzzle, generatePuzzle, initWasm } from "../wasm/sudokuWasm";

const GRID_SIZE = 9;
const BOX_SIZE = 3;

function createEmptyPuzzle(): SudokuPuzzle {
  return {
    grid: Array.from({ length: GRID_SIZE }, () => Array(GRID_SIZE).fill(0)),
    cages: [],
    inequalities: [],
  };
}

// 状态
const puzzle = ref<SudokuPuzzle>(createEmptyPuzzle());
const grid = ref<number[][]>(puzzle.value.grid.map((row) => [...row]));
const selectedCell = ref<Cell | null>(null);
const conflictCells = ref<Set<string>>(new Set());
const givenCells = ref<Set<string>>(new Set());
const canvasRef = ref<HTMLCanvasElement | null>(null);
const boardSize = ref(540);
const wasmReady = ref(false);
const isLoading = ref(false);
const errorMsg = ref("");

// 生成参数
const showGeneratePanel = ref(false);
const puzzleType = ref<PuzzleType>("mixed");
const minCages = ref(12);
const maxCages = ref(18);
const minInequalities = ref(15);
const maxInequalities = ref(25);
const fillComplete = ref(false); // 是否填满（无空格）
const ensureUnique = ref(true); // 是否保证唯一解

// 选中的数字（用于高亮）
const selectedNumber = ref<number | null>(null);

// 波纹动画状态
interface Ripple {
  row: number;
  col: number;
  startTime: number;
  duration: number;
  cells: Cell[];
}
const ripples = ref<Ripple[]>([]);
let animationFrameId: number | null = null;

// 计时器状态
const timerSeconds = ref(0);
const timerRunning = ref(false);
const timerInterval = ref<number | null>(null);

// 胜利状态
const isVictory = ref(false);
const showVictoryModal = ref(false);

// 格式化时间显示
const formattedTime = computed(() => {
  const mins = Math.floor(timerSeconds.value / 60);
  const secs = timerSeconds.value % 60;
  return `${mins.toString().padStart(2, "0")}:${secs
    .toString()
    .padStart(2, "0")}`;
});

// 检查是否胜利
const checkVictory = computed(() => {
  // 检查所有格子是否填满
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      if ((grid.value[r] as number[])[c] === 0) {
        return false;
      }
    }
  }
  // 检查是否有冲突
  return conflictCells.value.size === 0;
});

// 监听胜利状态
watch(checkVictory, (won) => {
  if (won && !isVictory.value && timerRunning.value) {
    isVictory.value = true;
    stopTimer();
    showVictoryModal.value = true;
  }
});

// 计时器控制
function startTimer() {
  if (timerInterval.value) return;
  timerRunning.value = true;
  timerInterval.value = window.setInterval(() => {
    timerSeconds.value++;
  }, 1000);
}

function stopTimer() {
  timerRunning.value = false;
  if (timerInterval.value) {
    clearInterval(timerInterval.value);
    timerInterval.value = null;
  }
}

function resetTimer() {
  stopTimer();
  timerSeconds.value = 0;
  isVictory.value = false;
}

function toggleTimer() {
  if (timerRunning.value) {
    stopTimer();
  } else {
    startTimer();
  }
  renderBoard();
}

// 初始化
onMounted(async () => {
  try {
    await initWasm();
    wasmReady.value = true;
    await generate();
  } catch (err) {
    errorMsg.value = `WASM 加载失败: ${err}`;
  }
  resizeCanvas();
  window.addEventListener("resize", resizeCanvas);
  window.addEventListener("keydown", handleKeydown);
  renderBoard();
});

onBeforeUnmount(() => {
  window.removeEventListener("resize", resizeCanvas);
  window.removeEventListener("keydown", handleKeydown);
  stopTimer();
});

// 标记初始给定的数字
function markGivenCells() {
  givenCells.value.clear();
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      if ((puzzle.value.grid[r] as number[])?.[c] !== 0) {
        givenCells.value.add(`${r}-${c}`);
      }
    }
  }
}

// 检查冲突
function recomputeConflicts() {
  const conflicts = new Set<string>();

  // 检查重复数字
  const markDuplicates = (
    cells: Array<{ r: number; c: number; v: number }>
  ) => {
    const byVal = new Map<number, Array<{ r: number; c: number }>>();
    cells.forEach(({ r, c, v }) => {
      if (v === 0) return;
      if (!byVal.has(v)) byVal.set(v, []);
      byVal.get(v)!.push({ r, c });
    });
    byVal.forEach((list) => {
      if (list.length > 1) {
        list.forEach(({ r, c }) => conflicts.add(`${r}-${c}`));
      }
    });
  };

  // 行
  for (let r = 0; r < GRID_SIZE; r++) {
    const row = grid.value[r] as number[];
    markDuplicates(row.map((v, c) => ({ r, c, v })));
  }

  // 列
  for (let c = 0; c < GRID_SIZE; c++) {
    const col = grid.value.map((row, r) => ({
      r,
      c,
      v: (row as number[])[c] as number,
    }));
    markDuplicates(col);
  }

  // 宫
  for (let br = 0; br < BOX_SIZE; br++) {
    for (let bc = 0; bc < BOX_SIZE; bc++) {
      const box: Array<{ r: number; c: number; v: number }> = [];
      for (let r = 0; r < BOX_SIZE; r++) {
        for (let c = 0; c < BOX_SIZE; c++) {
          const row = br * BOX_SIZE + r;
          const col = bc * BOX_SIZE + c;
          box.push({
            r: row,
            c: col,
            v: (grid.value[row] as number[])[col] as number,
          });
        }
      }
      markDuplicates(box);
    }
  }

  // 不等式
  puzzle.value.inequalities.forEach(({ cell1, cell2, type }) => {
    const v1 = (grid.value[cell1.row] as number[])[cell1.col] as number;
    const v2 = (grid.value[cell2.row] as number[])[cell2.col] as number;
    if (v1 === 0 || v2 === 0) return;
    const valid = type === ">" ? v1 > v2 : v1 < v2;
    if (!valid) {
      conflicts.add(`${cell1.row}-${cell1.col}`);
      conflicts.add(`${cell2.row}-${cell2.col}`);
    }
  });

  // 笼子和
  puzzle.value.cages.forEach((cage) => {
    let sum = 0;
    let filled = true;
    cage.cells.forEach(({ row, col }) => {
      const v = (grid.value[row] as number[])[col] as number;
      if (v === 0) filled = false;
      sum += v;
    });
    if (sum > cage.sum || (filled && sum !== cage.sum)) {
      cage.cells.forEach(({ row, col }) => {
        if ((grid.value[row] as number[])[col] !== 0) {
          conflicts.add(`${row}-${col}`);
        }
      });
    }
  });

  conflictCells.value = conflicts;
}

// UI 交互
function selectCell(row: number, col: number) {
  if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) return;
  selectedCell.value = { row, col };
  renderBoard();
}

// 检查完成的行/列/宫/笼（返回完成区域的所有格子）
function checkCompletions(row: number, col: number): Cell[] {
  const completed: Cell[] = [];

  // 行
  const rowCells = grid.value[row] as number[];
  if (rowCells.every((v) => v !== 0) && new Set(rowCells).size === 9) {
    for (let c = 0; c < GRID_SIZE; c++) completed.push({ row, col: c });
  }

  // 列
  const colVals = grid.value.map((r) => (r as number[])[col]);
  if (colVals.every((v) => v !== 0) && new Set(colVals).size === 9) {
    for (let r = 0; r < GRID_SIZE; r++) completed.push({ row: r, col });
  }

  // 宫
  const boxR = Math.floor(row / BOX_SIZE) * BOX_SIZE;
  const boxC = Math.floor(col / BOX_SIZE) * BOX_SIZE;
  const boxVals: number[] = [];
  for (let r = boxR; r < boxR + BOX_SIZE; r++) {
    for (let c = boxC; c < boxC + BOX_SIZE; c++) {
      const val = (grid.value[r] as number[])[c];
      if (val !== undefined) boxVals.push(val);
    }
  }
  if (boxVals.every((v) => v !== 0) && new Set(boxVals).size === 9) {
    for (let r = boxR; r < boxR + BOX_SIZE; r++) {
      for (let c = boxC; c < boxC + BOX_SIZE; c++) {
        completed.push({ row: r, col: c });
      }
    }
  }

  // 笼
  const cage = getCageForCell(puzzle.value.cages, row, col);
  if (cage) {
    const cageVals = cage.cells.map(
      (c) => (grid.value[c.row] as number[])[c.col] ?? 0
    );
    if (cageVals.every((v) => v !== 0)) {
      const sum = cageVals.reduce((a, b) => (a ?? 0) + (b ?? 0), 0);
      if (sum === cage.sum) {
        cage.cells.forEach((c) => completed.push(c));
      }
    }
  }

  return completed;
}

// 波纹动画（只在完成行/列/宫/笼时，限定在对应区域扩散）
function startRipple(row: number, col: number, cells: Cell[]) {
  const now = performance.now();
  ripples.value.push({ row, col, startTime: now, duration: 900, cells });

  // 限制保留的波纹数量，避免累积
  if (ripples.value.length > 4) {
    ripples.value.shift();
  }

  if (!animationFrameId) {
    animateRipples();
  }
}

function animateRipples() {
  const now = performance.now();
  ripples.value = ripples.value.filter((r) => now - r.startTime < r.duration);

  if (ripples.value.length > 0) {
    renderBoard();
    animationFrameId = requestAnimationFrame(animateRipples);
  } else {
    animationFrameId = null;
  }
}

function setCellValue(value: number | null) {
  if (!selectedCell.value) return;
  const { row, col } = selectedCell.value;
  if (givenCells.value.has(`${row}-${col}`)) return;

  const oldValue = (grid.value[row] as number[])[col];
  const gridRow = grid.value[row] as number[];
  gridRow[col] = value ?? 0;

  recomputeConflicts();

  // 只有在填满行/列/宫/笼时触发波纹，并仅在对应区域扩散
  const inserted = value !== null && value !== 0;
  if (inserted && value !== oldValue) {
    const completedCells = checkCompletions(row, col);
    if (completedCells.length > 0) {
      startRipple(row, col, completedCells);
    }
  }

  renderBoard();
}

// 数字按钮点击处理 - 只选中数字，不填入
function handleNumberClick(n: number) {
  // 如果已选中此数字，取消选中
  if (selectedNumber.value === n) {
    selectedNumber.value = null;
  } else {
    selectedNumber.value = n;
    clearSelected.value = false; // 取消清除选择
  }
  renderBoard();
}

// 清除键选中状态
const clearSelected = ref(false);

// 清除键点击处理
function handleClearClick() {
  if (clearSelected.value) {
    clearSelected.value = false;
    selectedNumber.value = null;
  } else {
    clearSelected.value = true;
    selectedNumber.value = null; // 取消数字选择
  }
  renderBoard();
}

function handleKeydown(event: KeyboardEvent) {
  if (!selectedCell.value) return;
  if (event.key >= "1" && event.key <= "9") {
    setCellValue(parseInt(event.key));
  } else if (
    event.key === "0" ||
    event.key === "Backspace" ||
    event.key === "Delete"
  ) {
    setCellValue(null);
  } else if (event.key === "ArrowUp") {
    selectCell(selectedCell.value.row - 1, selectedCell.value.col);
  } else if (event.key === "ArrowDown") {
    selectCell(selectedCell.value.row + 1, selectedCell.value.col);
  } else if (event.key === "ArrowLeft") {
    selectCell(selectedCell.value.row, selectedCell.value.col - 1);
  } else if (event.key === "ArrowRight") {
    selectCell(selectedCell.value.row, selectedCell.value.col + 1);
  }
}

function handleCanvasClick(event: MouseEvent) {
  const canvas = canvasRef.value;
  if (!canvas) return;
  const rect = canvas.getBoundingClientRect();
  const totalWidth = boardSize.value + numPadWidth;

  // 将点击位置映射到画布的逻辑坐标（缩放后也正确）
  const x = ((event.clientX - rect.left) / rect.width) * totalWidth;
  const y = ((event.clientY - rect.top) / rect.height) * boardSize.value;

  if (y < 0 || y > boardSize.value) return;

  // 检查是否点击了右侧数字按钮区域
  const numPadStartX = boardSize.value + numPadOffset;
  const numPadEndX = numPadStartX + numPadBtnSize;
  if (x >= numPadStartX && x <= numPadEndX) {
    const startY = getNumPadStartY(boardSize.value);
    for (let i = 0; i < numPadCount; i++) {
      const btnY = startY + i * (numPadBtnSize + numPadBtnGap);
      if (y >= btnY && y <= btnY + numPadBtnSize) {
        if (i < 9) {
          handleNumberClick(i + 1);
        } else {
          // 清除键被点击
          handleClearClick();
        }
        return;
      }
    }
    return;
  }

  // 检查是否点击了棋盘区域（排除右侧间隙）
  if (x < 0 || x > boardSize.value) return;

  const cellSize = boardSize.value / GRID_SIZE;
  const col = Math.floor(x / cellSize);
  const row = Math.floor(y / cellSize);

  if (col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE) {
    selectCell(row, col);
    // 如果有选中的数字，填入该数字
    if (selectedNumber.value !== null) {
      setCellValue(selectedNumber.value);
    } else if (clearSelected.value) {
      // 如果清除键被选中，清除该格子
      setCellValue(null);
    }
  }
}

// 求解
async function solve() {
  if (!wasmReady.value) {
    errorMsg.value = "WASM 未就绪";
    return;
  }

  isLoading.value = true;
  errorMsg.value = "";

  try {
    const puzzleStr = puzzleToString(puzzle.value);
    const result = await solvePuzzle(puzzleStr, false);

    if (result.solved && result.grid) {
      grid.value = result.grid;
      selectedCell.value = null;
      recomputeConflicts();
      renderBoard();
    } else {
      errorMsg.value = result.error || "无法求解此谜题";
    }
  } catch (err) {
    errorMsg.value = `求解失败: ${err}`;
  } finally {
    isLoading.value = false;
  }
}

// 生成
async function generate() {
  if (!wasmReady.value) {
    errorMsg.value = "WASM 未就绪";
    return;
  }

  isLoading.value = true;
  errorMsg.value = "";
  showGeneratePanel.value = false;

  try {
    const puzzleStr = await generatePuzzle(puzzleType.value, {
      minCages: minCages.value,
      maxCages: maxCages.value,
      minInequalities: minInequalities.value,
      maxInequalities: maxInequalities.value,
      seed: 0,
      includeSolution: false,
      fillAllCells: fillComplete.value,
      ensureUniqueSolution: ensureUnique.value,
    });

    puzzle.value = parsePuzzle(puzzleStr);
    grid.value = puzzle.value.grid.map((row) => [...row]);
    selectedCell.value = null;
    markGivenCells();
    recomputeConflicts();
    renderBoard();

    // 重置并启动计时器
    resetTimer();
    startTimer();
    isVictory.value = false;
    showVictoryModal.value = false;
  } catch (err) {
    errorMsg.value = `生成失败: ${err}`;
  } finally {
    isLoading.value = false;
  }
}

function reset() {
  grid.value = puzzle.value.grid.map((row) => [...row]);
  selectedCell.value = null;
  recomputeConflicts();
  renderBoard();
  resetTimer();
  isVictory.value = false;
  showVictoryModal.value = false;
}

// 绘图
const numPadWidth = 60; // 数字按钮区域宽度
const numPadOffset = 10;
const numPadBtnSize = 40;
const numPadBtnGap = 4;
const numPadCount = 10; // 1-9 + 清除键

function getNumPadStartY(size: number) {
  return (
    (size - (numPadBtnSize * numPadCount + numPadBtnGap * (numPadCount - 1))) /
    2
  );
}

function resizeCanvas() {
  const canvas = canvasRef.value;
  if (!canvas) return;
  const size = Math.min(580, window.innerWidth - 100);
  boardSize.value = Math.max(360, size);
  const totalWidth = boardSize.value + numPadWidth;
  const dpr = window.devicePixelRatio || 1;
  canvas.width = totalWidth * dpr;
  canvas.height = boardSize.value * dpr;
  canvas.style.width = `${totalWidth}px`;
  canvas.style.height = `${boardSize.value}px`;
  const ctx = canvas.getContext("2d");
  if (ctx) {
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.scale(dpr, dpr);
  }
  renderBoard();
}

function renderBoard() {
  const canvas = canvasRef.value;
  if (!canvas) return;
  const ctx = canvas.getContext("2d");
  if (!ctx) return;

  const size = boardSize.value;
  const totalWidth = size + numPadWidth;
  const cellSize = size / GRID_SIZE;

  // 如果计时未在流动，显示暂停遮罩
  if (!timerRunning.value) {
    ctx.clearRect(0, 0, totalWidth, size);

    // 红色半透明遮罩
    ctx.fillStyle = "rgba(220, 53, 69, 0.15)";
    ctx.fillRect(0, 0, size, size);

    // 绘制暂停图标和文字
    ctx.fillStyle = "rgba(220, 53, 69, 0.8)";
    ctx.font = `bold ${Math.floor(size * 0.08)}px 'Segoe UI', sans-serif`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText("⏸ 已暂停", size / 2, size / 2);

    ctx.font = `${Math.floor(size * 0.035)}px 'Segoe UI', sans-serif`;
    ctx.fillStyle = "rgba(220, 53, 69, 0.6)";
    ctx.fillText("点击「继续」恢复游戏", size / 2, size / 2 + size * 0.08);

    return;
  }

  // 清空整个画布
  ctx.clearRect(0, 0, totalWidth, size);

  // 绘制数独区域背景
  ctx.fillStyle = "#fdfaf3";
  ctx.fillRect(0, 0, size, size);

  // 背景色
  drawCageBackgrounds(ctx, cellSize);
  drawConflicts(ctx, cellSize);
  drawSelection(ctx, cellSize);
  drawHighlightedNumbers(ctx, cellSize);

  // 线条
  drawGridLines(ctx, cellSize);
  drawCageBorders(ctx, cellSize);

  // 内容
  drawCageSums(ctx, cellSize);
  drawValues(ctx, cellSize);
  drawInequalities(ctx, cellSize);

  // 波纹动画
  drawRipples(ctx, cellSize);

  // 绘制数字按钮区域
  drawNumberPad(ctx, size);
}

// 绘制数字按钮
function drawNumberPad(ctx: CanvasRenderingContext2D, boardSize: number) {
  const startX = boardSize + numPadOffset;
  const startY = getNumPadStartY(boardSize);

  for (let i = 0; i < 9; i++) {
    const n = i + 1;
    const x = startX;
    const y = startY + i * (numPadBtnSize + numPadBtnGap);
    const isSelected = selectedNumber.value === n;

    // 绘制圆形按钮背景
    ctx.beginPath();
    ctx.arc(
      x + numPadBtnSize / 2,
      y + numPadBtnSize / 2,
      numPadBtnSize / 2,
      0,
      Math.PI * 2
    );

    if (isSelected) {
      // 选中状态 - 橙色渐变
      const gradient = ctx.createRadialGradient(
        x + numPadBtnSize / 2,
        y + numPadBtnSize / 2,
        0,
        x + numPadBtnSize / 2,
        y + numPadBtnSize / 2,
        numPadBtnSize / 2
      );
      gradient.addColorStop(0, "#ffc107");
      gradient.addColorStop(1, "#ff9800");
      ctx.fillStyle = gradient;
    } else {
      // 默认状态 - 白色
      ctx.fillStyle = "#fff";
    }
    ctx.fill();

    // 绘制边框
    ctx.strokeStyle = isSelected ? "#e68a00" : "#cdd7ff";
    ctx.lineWidth = 2;
    ctx.stroke();

    // 绘制数字
    ctx.fillStyle = isSelected ? "#fff" : "#0f172a";
    ctx.font = `bold ${Math.floor(
      numPadBtnSize * 0.5
    )}px 'Segoe UI', sans-serif`;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(String(n), x + numPadBtnSize / 2, y + numPadBtnSize / 2 + 1);
  }

  // 绘制清除按钮（第10个）
  const clearX = startX;
  const clearY = startY + 9 * (numPadBtnSize + numPadBtnGap);
  const isClearSelected = clearSelected.value;

  ctx.beginPath();
  ctx.arc(
    clearX + numPadBtnSize / 2,
    clearY + numPadBtnSize / 2,
    numPadBtnSize / 2,
    0,
    Math.PI * 2
  );

  if (isClearSelected) {
    // 选中状态 - 红色渐变
    const gradient = ctx.createRadialGradient(
      clearX + numPadBtnSize / 2,
      clearY + numPadBtnSize / 2,
      0,
      clearX + numPadBtnSize / 2,
      clearY + numPadBtnSize / 2,
      numPadBtnSize / 2
    );
    gradient.addColorStop(0, "#ef5350");
    gradient.addColorStop(1, "#d32f2f");
    ctx.fillStyle = gradient;
  } else {
    ctx.fillStyle = "#fff";
  }
  ctx.fill();

  ctx.strokeStyle = isClearSelected ? "#b71c1c" : "#cdd7ff";
  ctx.lineWidth = 2;
  ctx.stroke();

  // 绘制X符号
  ctx.fillStyle = isClearSelected ? "#fff" : "#d32f2f";
  ctx.font = `bold ${Math.floor(numPadBtnSize * 0.5)}px 'Segoe UI', sans-serif`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText("✕", clearX + numPadBtnSize / 2, clearY + numPadBtnSize / 2 + 1);
}

function drawCageBackgrounds(ctx: CanvasRenderingContext2D, cellSize: number) {
  puzzle.value.cages.forEach((cage) => {
    ctx.fillStyle = cage.color || `rgba(200, 200, 200, 0.2)`;
    cage.cells.forEach(({ row, col }) => {
      ctx.fillRect(col * cellSize, row * cellSize, cellSize, cellSize);
    });
  });
}

function drawConflicts(ctx: CanvasRenderingContext2D, cellSize: number) {
  ctx.fillStyle = "rgba(210, 40, 40, 0.18)";
  conflictCells.value.forEach((key) => {
    const [r, c] = key.split("-").map(Number);
    ctx.fillRect(
      (c as number) * cellSize,
      (r as number) * cellSize,
      cellSize,
      cellSize
    );
  });
}

function drawSelection(ctx: CanvasRenderingContext2D, cellSize: number) {
  if (!selectedCell.value) return;
  const { row, col } = selectedCell.value;
  ctx.fillStyle = "rgba(60, 120, 255, 0.25)";
  ctx.fillRect(col * cellSize, row * cellSize, cellSize, cellSize);
}

function drawHighlightedNumbers(
  ctx: CanvasRenderingContext2D,
  cellSize: number
) {
  if (selectedNumber.value === null) return;

  ctx.fillStyle = "rgba(255, 200, 50, 0.35)";
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      if ((grid.value[r] as number[])[c] === selectedNumber.value) {
        ctx.fillRect(c * cellSize, r * cellSize, cellSize, cellSize);
      }
    }
  }
}

function drawRipples(ctx: CanvasRenderingContext2D, cellSize: number) {
  const now = performance.now();

  const bandWidth = 1;

  ripples.value.forEach((ripple) => {
    const elapsed = now - ripple.startTime;
    const progress = elapsed / ripple.duration;
    if (progress >= 1) return;

    const maxCellDistance = ripple.cells.reduce((acc, cell) => {
      const d = Math.hypot(cell.row - ripple.row, cell.col - ripple.col);
      return Math.max(acc, d);
    }, 0);

    const waveDistance = progress * (maxCellDistance + 1);
    const allowed = new Set(ripple.cells.map((c) => `${c.row}-${c.col}`));

    ripple.cells.forEach((cell) => {
      const dist = Math.hypot(cell.row - ripple.row, cell.col - ripple.col);
      const diff = Math.abs(dist - waveDistance);
      const alpha = Math.max(0, 0.35 * (1 - diff / bandWidth));

      if (alpha > 0 && allowed.has(`${cell.row}-${cell.col}`)) {
        ctx.fillStyle = `rgba(120, 200, 255, ${alpha})`;
        ctx.fillRect(
          cell.col * cellSize,
          cell.row * cellSize,
          cellSize,
          cellSize
        );
      }
    });
  });
}

function drawGridLines(ctx: CanvasRenderingContext2D, cellSize: number) {
  // 细线
  ctx.strokeStyle = "#bbb";
  ctx.lineWidth = 0.8;
  for (let i = 1; i < GRID_SIZE; i++) {
    ctx.beginPath();
    ctx.moveTo(i * cellSize, 0);
    ctx.lineTo(i * cellSize, GRID_SIZE * cellSize);
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(0, i * cellSize);
    ctx.lineTo(GRID_SIZE * cellSize, i * cellSize);
    ctx.stroke();
  }

  // 粗线
  ctx.strokeStyle = "#1f1f1f";
  ctx.lineWidth = 2.6;
  for (let i = 0; i <= GRID_SIZE; i += BOX_SIZE) {
    ctx.beginPath();
    ctx.moveTo(i * cellSize, 0);
    ctx.lineTo(i * cellSize, GRID_SIZE * cellSize);
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(0, i * cellSize);
    ctx.lineTo(GRID_SIZE * cellSize, i * cellSize);
    ctx.stroke();
  }
}

function drawCageBorders(ctx: CanvasRenderingContext2D, cellSize: number) {
  const offset = 3;
  ctx.strokeStyle = "#666";
  ctx.lineWidth = 1.5;
  ctx.setLineDash([5, 4]);

  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      const x = c * cellSize;
      const y = r * cellSize;
      const cage = getCageForCell(puzzle.value.cages, r, c);

      if (!cage) continue;

      // 检查相邻格子
      const hasNeighbor = (nr: number, nc: number) =>
        nr >= 0 &&
        nr < GRID_SIZE &&
        nc >= 0 &&
        nc < GRID_SIZE &&
        isSameCageBorder(puzzle.value.cages, r, c, nr, nc);

      if (!hasNeighbor(r - 1, c)) {
        ctx.beginPath();
        ctx.moveTo(x + offset, y + offset);
        ctx.lineTo(x + cellSize - offset, y + offset);
        ctx.stroke();
      }

      if (!hasNeighbor(r + 1, c)) {
        ctx.beginPath();
        ctx.moveTo(x + offset, y + cellSize - offset);
        ctx.lineTo(x + cellSize - offset, y + cellSize - offset);
        ctx.stroke();
      }

      if (!hasNeighbor(r, c - 1)) {
        ctx.beginPath();
        ctx.moveTo(x + offset, y + offset);
        ctx.lineTo(x + offset, y + cellSize - offset);
        ctx.stroke();
      }

      if (!hasNeighbor(r, c + 1)) {
        ctx.beginPath();
        ctx.moveTo(x + cellSize - offset, y + offset);
        ctx.lineTo(x + cellSize - offset, y + cellSize - offset);
        ctx.stroke();
      }
    }
  }

  ctx.setLineDash([]);
}

function drawCageSums(ctx: CanvasRenderingContext2D, cellSize: number) {
  ctx.fillStyle = "#333";
  ctx.font = `${Math.floor(cellSize * 0.22)}px Arial`;
  ctx.textAlign = "left";
  ctx.textBaseline = "top";

  puzzle.value.cages.forEach((cage) => {
    const topLeft = getCageTopLeft(cage);
    const x = topLeft.col * cellSize + cellSize * 0.08;
    const y = topLeft.row * cellSize + cellSize * 0.06;
    ctx.fillText(String(cage.sum), x, y);
  });
}

function drawValues(ctx: CanvasRenderingContext2D, cellSize: number) {
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";

  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      const val = (grid.value[r] as number[])[c];
      if (val === 0) continue;

      const isConflict = conflictCells.value.has(`${r}-${c}`);
      const isGiven = givenCells.value.has(`${r}-${c}`);

      ctx.fillStyle = isConflict ? "#c00000" : isGiven ? "#111" : "#0a4a9f";
      ctx.font = `${Math.floor(cellSize * 0.56)}px 'Arial Black'`;
      ctx.fillText(
        String(val),
        c * cellSize + cellSize / 2,
        r * cellSize + cellSize / 2 + 1
      );
    }
  }
}

function drawInequalities(ctx: CanvasRenderingContext2D, cellSize: number) {
  ctx.fillStyle = "#a02b2b";
  ctx.font = `${Math.floor(cellSize * 0.4)}px 'Fira Sans'`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";

  puzzle.value.inequalities.forEach((ineq) => {
    const { cell1, cell2, type } = ineq;
    if (cell1.row === cell2.row) {
      // 水平
      const r = cell1.row;
      const c = Math.min(cell1.col, cell2.col);
      const x = (c + 1) * cellSize;
      const y = r * cellSize + cellSize / 2;
      const text = cell1.col < cell2.col ? type : type === ">" ? "<" : ">";
      ctx.fillText(text, x, y);
    } else if (cell1.col === cell2.col) {
      // 垂直
      const c = cell1.col;
      const r = Math.min(cell1.row, cell2.row);
      const x = c * cellSize + cellSize / 2;
      const y = (r + 1) * cellSize;
      const text = cell1.row < cell2.row ? type : type === ">" ? "<" : ">";

      ctx.save();
      ctx.translate(x, y);
      ctx.rotate(Math.PI / 2);
      ctx.fillText(text, 0, 0);
      ctx.restore();
    }
  });
}
</script>

<template>
  <div class="page">
    <div class="layout">
      <div class="board-column">
        <div class="hero">
          <h1>数独</h1>
        </div>

        <div class="board-card">
          <canvas
            ref="canvasRef"
            class="board-canvas"
            @click="handleCanvasClick"
          ></canvas>
        </div>
      </div>

      <div class="side-column">
        <div class="panel timer-panel">
          <div class="timer-heading">
            <span class="timer-icon">⏱</span>
            <span class="timer-value">{{ formattedTime }}</span>
          </div>
          <div class="timer-actions">
            <button
              class="btn ghost"
              @click="toggleTimer"
              :disabled="!wasmReady"
              :title="timerRunning ? '暂停' : '继续'"
            >
              {{ timerRunning ? "暂停" : "继续" }}
            </button>
            <button
              class="btn ghost"
              @click="resetTimer"
              :disabled="!wasmReady"
            >
              重置计时
            </button>
          </div>
        </div>

        <div class="panel actions-panel">
          <div class="button-row">
            <div class="generate-group">
              <button
                class="btn primary"
                @click="generate"
                :disabled="!wasmReady || isLoading"
              >
                生成新盘
              </button>
              <button
                class="btn icon"
                @click="showGeneratePanel = !showGeneratePanel"
                :disabled="isLoading"
                aria-label="生成设置"
              >
                ▾
              </button>
            </div>
            <button
              class="btn ghost"
              @click="solve"
              :disabled="!wasmReady || isLoading"
            >
              求解
            </button>
            <button class="btn ghost" @click="reset" :disabled="isLoading">
              重置
            </button>
            <button
              class="btn ghost"
              @click="setCellValue(null)"
              :disabled="isLoading"
            >
              清除
            </button>
          </div>

          <div v-if="showGeneratePanel" class="dropdown-panel">
            <div class="param-row">
              <label>谜题类型</label>
              <select v-model="puzzleType">
                <option value="standard">标准数独</option>
                <option value="killer">杀手数独</option>
                <option value="inequality">不等式数独</option>
                <option value="mixed">混合模式</option>
              </select>
            </div>
            <div
              class="param-row"
              v-if="puzzleType === 'killer' || puzzleType === 'mixed'"
            >
              <label>笼子数量</label>
              <div class="range-inputs">
                <input
                  type="number"
                  v-model.number="minCages"
                  min="0"
                  max="30"
                  placeholder="最小"
                />
                <span>~</span>
                <input
                  type="number"
                  v-model.number="maxCages"
                  min="0"
                  max="30"
                  placeholder="最大"
                />
              </div>
            </div>
            <div
              class="param-row"
              v-if="puzzleType === 'inequality' || puzzleType === 'mixed'"
            >
              <label>不等式数量</label>
              <div class="range-inputs">
                <input
                  type="number"
                  v-model.number="minInequalities"
                  min="0"
                  max="40"
                  placeholder="最小"
                />
                <span>~</span>
                <input
                  type="number"
                  v-model.number="maxInequalities"
                  min="0"
                  max="40"
                  placeholder="最大"
                />
              </div>
            </div>
            <div class="param-row checkbox-row">
              <label>
                <input type="checkbox" v-model="fillComplete" />
                填满（无空格）
              </label>
            </div>
            <div class="param-row checkbox-row">
              <label>
                <input type="checkbox" v-model="ensureUnique" />
                保证唯一解
              </label>
            </div>
            <button
              class="btn primary full"
              @click="generate"
              :disabled="!wasmReady || isLoading"
            >
              应用并生成
            </button>
          </div>
        </div>
      </div>
    </div>

    <div
      v-if="showVictoryModal"
      class="victory-modal-overlay"
      @click="showVictoryModal = false"
    >
      <div class="victory-modal" @click.stop>
        <div class="victory-icon">🎉</div>
        <h2>恭喜完成！</h2>
        <p class="victory-time">用时: {{ formattedTime }}</p>
        <div class="victory-buttons">
          <button class="btn primary" @click="generate">新游戏</button>
          <button class="btn ghost" @click="showVictoryModal = false">
            关闭
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.page {
  min-height: 100vh;
  background: radial-gradient(
    circle at 20% 20%,
    #f7fbff 0%,
    #eaf1ff 35%,
    #f5f7fb 70%
  );
  padding: 28px;
  display: flex;
  flex-direction: column;
  gap: 18px;
  font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif;
  color: #0f172a;
}

.layout {
  display: grid;
  grid-template-columns: minmax(320px, 640px) minmax(260px, 380px);
  gap: 18px;
  align-items: start;
}

.board-column {
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.hero {
  padding: 12px 16px;
  background: linear-gradient(120deg, #1a237e 0%, #3949ab 50%, #5c6bc0 100%);
  border-radius: 14px;
  color: #fff;
  box-shadow: 0 12px 32px rgba(26, 35, 126, 0.2);
  text-align: center;
}

h1 {
  margin: 0;
  font-size: 24px;
  font-weight: 700;
  letter-spacing: 2px;
}

.board-card {
  background: #ffffff;
  border-radius: 16px;
  padding: 12px;
  box-shadow: 0 16px 40px rgba(15, 23, 42, 0.08);
  display: flex;
  justify-content: center;
}

.board-canvas {
  border: 2px solid #0f172a;
  background: #fdfaf3;
  max-width: 100%;
  box-shadow: 0 12px 28px rgba(0, 0, 0, 0.12);
  cursor: pointer;
  border-radius: 10px;
}

.side-column {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.panel {
  background: #ffffff;
  border-radius: 14px;
  padding: 14px;
  box-shadow: 0 12px 32px rgba(15, 23, 42, 0.06);
  border: 1px solid #e6ecff;
}

.timer-panel {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
}

.timer-heading {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 22px;
  font-weight: 700;
  color: #1a237e;
}

.timer-icon {
  font-size: 22px;
}

.timer-value {
  font-family: "Consolas", "Courier New", monospace;
  min-width: 70px;
  text-align: center;
}

.timer-actions {
  display: flex;
  gap: 8px;
}

.actions-panel .button-row {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
  align-items: center;
}

.generate-group {
  display: flex;
  border-radius: 10px;
  overflow: hidden;
  box-shadow: 0 8px 18px rgba(15, 23, 42, 0.08);
}

.dropdown-panel {
  margin-top: 10px;
  padding: 12px;
  border-radius: 12px;
  background: linear-gradient(150deg, #f5f7ff 0%, #eef3ff 100%);
  border: 1px solid #dbe5ff;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.param-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.param-row label {
  font-size: 14px;
  color: #0f172a;
  white-space: nowrap;
}

.param-row select,
.range-inputs input {
  flex: 1;
  padding: 8px 10px;
  border-radius: 10px;
  border: 1px solid #c8d6ff;
  background: #fff;
  font-size: 14px;
  color: #0f172a;
}

.range-inputs {
  display: flex;
  align-items: center;
  gap: 6px;
}

.range-inputs span {
  color: #6b7280;
}

.checkbox-row {
  justify-content: flex-start;
}

.checkbox-row label {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  user-select: none;
}

.checkbox-row input[type="checkbox"] {
  width: 18px;
  height: 18px;
  accent-color: #3b5bfd;
  cursor: pointer;
}

.btn {
  border-radius: 10px;
  border: 1px solid #cdd7ff;
  background: linear-gradient(#ffffff, #f5f7ff);
  font-size: 14px;
  font-weight: 700;
  color: #0f172a;
  cursor: pointer;
  transition: transform 0.08s ease, box-shadow 0.12s ease,
    border-color 0.12s ease;
  box-shadow: 0 4px 10px rgba(15, 23, 42, 0.08);
  padding: 10px 14px;
}

.btn:hover:not(:disabled) {
  transform: translateY(-1px);
  border-color: #8ea2ff;
  box-shadow: 0 8px 18px rgba(15, 23, 42, 0.12);
}

.btn:disabled {
  opacity: 0.55;
  cursor: not-allowed;
}

.btn.primary {
  background: linear-gradient(120deg, #3b5bfd 0%, #2d4de6 100%);
  color: #fff;
  border-color: #2746d5;
  min-width: 118px;
}

.btn.ghost {
  background: #f8f9fe;
  color: #1a237e;
}

.btn.icon {
  width: 38px;
  min-width: 38px;
  padding: 0;
  font-size: 16px;
}

.btn.full {
  width: 100%;
}

.victory-modal-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.45);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  animation: fadeIn 0.3s ease;
}

.victory-modal {
  background: linear-gradient(135deg, #ffffff 0%, #f2f6ff 100%);
  border-radius: 20px;
  padding: 32px 40px;
  text-align: center;
  box-shadow: 0 24px 60px rgba(0, 0, 0, 0.28);
  animation: scaleIn 0.4s ease;
}

.victory-icon {
  font-size: 64px;
  margin-bottom: 16px;
  animation: bounce 1s ease infinite;
}

.victory-modal h2 {
  margin: 0 0 12px 0;
  font-size: 28px;
  color: #1a237e;
}

.victory-time {
  font-size: 20px;
  color: #475569;
  margin-bottom: 24px;
  font-family: "Consolas", "Courier New", monospace;
}

.victory-buttons {
  display: flex;
  gap: 12px;
  justify-content: center;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

@keyframes scaleIn {
  from {
    transform: scale(0.86);
    opacity: 0;
  }
  to {
    transform: scale(1);
    opacity: 1;
  }
}

@keyframes bounce {
  0%,
  100% {
    transform: translateY(0);
  }
  50% {
    transform: translateY(-10px);
  }
}

@media (max-width: 960px) {
  .layout {
    grid-template-columns: 1fr;
  }

  .side-column {
    flex-direction: row;
    flex-wrap: wrap;
  }

  .panel {
    flex: 1 1 300px;
  }
}

@media (prefers-color-scheme: dark) {
  .page {
    background: radial-gradient(
      circle at 20% 20%,
      #0b1021 0%,
      #0f172a 60%,
      #0b1226 100%
    );
    color: #e2e8f0;
  }

  .hero {
    background: linear-gradient(120deg, #111827 0%, #1f2937 100%);
    box-shadow: 0 16px 40px rgba(0, 0, 0, 0.35);
  }

  .board-card,
  .panel {
    background: #0f172a;
    border-color: #1f2a44;
    box-shadow: 0 14px 36px rgba(0, 0, 0, 0.4);
  }

  .board-canvas {
    background: #111827;
    border-color: #e5e7eb;
  }

  .btn {
    background: linear-gradient(#111827, #0b1222);
    color: #e2e8f0;
    border-color: #233659;
  }

  .btn.primary {
    background: linear-gradient(120deg, #3154ff 0%, #1f3dcf 100%);
    border-color: #1b35b5;
  }

  .btn.ghost {
    background: #151c2f;
  }

  .dropdown-panel {
    background: linear-gradient(150deg, #151c2f 0%, #0f172a 100%);
    border-color: #1f2a44;
  }

  .param-row select,
  .range-inputs input {
    background: #0b1222;
    border-color: #233659;
    color: #e2e8f0;
  }

  .btn.num.active {
    background: linear-gradient(120deg, #ffa726 0%, #fb8c00 100%);
  }

  .victory-modal {
    background: linear-gradient(135deg, #0f172a 0%, #0b1222 100%);
  }

  .victory-modal h2 {
    color: #a5b4fc;
  }

  .victory-time {
    color: #cbd5e1;
  }
}
</style>
