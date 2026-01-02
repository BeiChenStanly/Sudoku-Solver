// WASM 模块包装器

import type { SolveResult, PuzzleType } from "../types/sudoku";

// WASM 模块类型定义
interface SudokuWasmModule {
  solvePuzzle(input: string, checkUniqueness: boolean): string;
  generatePuzzle(
    type: string,
    minCages: number,
    maxCages: number,
    minInequalities: number,
    maxInequalities: number,
    seed: number,
    includeSolution: boolean,
    fillAllCells: boolean,
    ensureUniqueSolution: boolean
  ): string;
  verifySolution(puzzle: string, solution: string): string;
  getVersion(): string;
}

// 全局 createSudokuModule 函数类型
declare global {
  interface Window {
    createSudokuModule: () => Promise<SudokuWasmModule>;
  }
}

let wasmModule: SudokuWasmModule | null = null;
let loadingPromise: Promise<SudokuWasmModule> | null = null;

// 初始化 WASM 模块
export async function initWasm(): Promise<void> {
  if (wasmModule) return;

  if (!loadingPromise) {
    loadingPromise = new Promise((resolve, reject) => {
      // 动态加载 WASM JS 文件
      const script = document.createElement("script");
      script.src = "/Sudoku-Solver/wasm/sudoku_wasm.js";
      script.onload = async () => {
        try {
          if (window.createSudokuModule) {
            const module = await window.createSudokuModule();
            wasmModule = module;
            resolve(module);
          } else {
            reject(new Error("createSudokuModule not found"));
          }
        } catch (err) {
          reject(err);
        }
      };
      script.onerror = () => reject(new Error("Failed to load WASM module"));
      document.head.appendChild(script);
    });
  }

  await loadingPromise;
}

// 检查 WASM 是否已加载
export function isWasmReady(): boolean {
  return wasmModule !== null;
}

// 求解数独
export async function solvePuzzle(
  puzzleStr: string,
  checkUniqueness: boolean = false
): Promise<SolveResult> {
  await initWasm();

  if (!wasmModule) {
    throw new Error("WASM module not initialized");
  }

  const resultJson = wasmModule.solvePuzzle(puzzleStr, checkUniqueness);
  return JSON.parse(resultJson) as SolveResult;
}

// 生成数独谜题
export async function generatePuzzle(
  type: PuzzleType = "mixed",
  options: {
    minCages?: number;
    maxCages?: number;
    minInequalities?: number;
    maxInequalities?: number;
    seed?: number;
    includeSolution?: boolean;
    fillAllCells?: boolean;
    ensureUniqueSolution?: boolean;
  } = {}
): Promise<string> {
  await initWasm();

  if (!wasmModule) {
    throw new Error("WASM module not initialized");
  }

  const {
    minCages = 12,
    maxCages = 18,
    minInequalities = 15,
    maxInequalities = 25,
    seed = 0,
    includeSolution = false,
    fillAllCells = false,
    ensureUniqueSolution = true,
  } = options;

  return wasmModule.generatePuzzle(
    type,
    minCages,
    maxCages,
    minInequalities,
    maxInequalities,
    seed,
    includeSolution,
    fillAllCells,
    ensureUniqueSolution
  );
}

// 验证解答
export async function verifySolution(
  puzzleStr: string,
  solution: number[][]
): Promise<boolean> {
  await initWasm();

  if (!wasmModule) {
    throw new Error("WASM module not initialized");
  }

  const solutionJson = JSON.stringify(solution);
  const resultJson = wasmModule.verifySolution(puzzleStr, solutionJson);
  const result = JSON.parse(resultJson);
  return result.valid === true;
}

// 获取版本信息
export async function getVersion(): Promise<{
  version: string;
  solver: string;
}> {
  await initWasm();

  if (!wasmModule) {
    throw new Error("WASM module not initialized");
  }

  return JSON.parse(wasmModule.getVersion());
}
