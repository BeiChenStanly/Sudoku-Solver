// Web Worker for WASM operations
// 在 Worker 中运行 WASM 以避免阻塞主线程

interface WorkerMessage {
  id: string
  type: 'solve' | 'generate' | 'init'
  payload?: any
}

interface WorkerResponse {
  id: string
  success: boolean
  data?: any
  error?: string
}

declare function createSudokuModule(options?: any): Promise<any>

let wasmModule: any = null
let initPromise: Promise<void> | null = null

// 获取基础路径
function getBasePath(): string {
  // 从 Worker 的 location 获取基础路径
  const workerUrl = self.location.href
  const url = new URL(workerUrl)
  // 提取 base path (如 /Sudoku-Solver/)
  const pathParts = url.pathname.split('/')
  // 找到 assets 之前的部分作为 base
  const assetsIndex = pathParts.indexOf('assets')
  if (assetsIndex > 0) {
    return pathParts.slice(0, assetsIndex).join('/') + '/'
  }
  // 开发模式下直接使用 origin
  return url.origin + '/Sudoku-Solver/'
}

// 初始化 WASM
async function initWasm(): Promise<void> {
  if (wasmModule) return
  if (initPromise) return initPromise

  initPromise = (async () => {
    try {
      const basePath = getBasePath()
      const wasmJsUrl = basePath + 'wasm/sudoku_wasm.js'
      const wasmBinaryUrl = basePath + 'wasm/sudoku_wasm.wasm'
      
      console.log('Loading WASM from:', wasmJsUrl)
      
      // 使用 importScripts 加载 WASM JS glue code
      // @ts-ignore - importScripts 只在 Worker 环境中可用
      importScripts(wasmJsUrl)
      
      // @ts-ignore - createSudokuModule 由上面的脚本定义
      if (typeof createSudokuModule === 'function') {
        // 配置 WASM 文件位置
        wasmModule = await createSudokuModule({
          locateFile: (path: string) => {
            if (path.endsWith('.wasm')) {
              return wasmBinaryUrl
            }
            return basePath + 'wasm/' + path
          }
        })
        console.log('WASM module loaded successfully')
      } else {
        throw new Error('createSudokuModule not found after loading script')
      }
    } catch (err) {
      console.error('WASM init error:', err)
      throw err
    }
  })()

  return initPromise
}

// 求解数独
async function solvePuzzle(puzzleStr: string, checkUniqueness: boolean) {
  await initWasm()
  if (!wasmModule) throw new Error('WASM not initialized')
  
  const resultJson = wasmModule.solvePuzzle(puzzleStr, checkUniqueness)
  return JSON.parse(resultJson)
}

// 生成数独
async function generatePuzzle(
  type: string,
  minCages: number,
  maxCages: number,
  minInequalities: number,
  maxInequalities: number,
  seed: number,
  includeSolution: boolean,
  fillAllCells: boolean,
  ensureUniqueSolution: boolean,
  difficulty: number
) {
  await initWasm()
  if (!wasmModule) throw new Error('WASM not initialized')
  
  return wasmModule.generatePuzzle(
    type,
    minCages,
    maxCages,
    minInequalities,
    maxInequalities,
    seed,
    includeSolution,
    fillAllCells,
    ensureUniqueSolution,
    difficulty
  )
}

// Worker 消息处理
self.onmessage = async (event: MessageEvent<WorkerMessage>) => {
  const { id, type, payload } = event.data
  let response: WorkerResponse

  try {
    switch (type) {
      case 'init':
        await initWasm()
        response = { id, success: true }
        break

      case 'solve':
        const solveResult = await solvePuzzle(
          payload.puzzleStr,
          payload.checkUniqueness || false
        )
        response = { id, success: true, data: solveResult }
        break

      case 'generate':
        const puzzleStr = await generatePuzzle(
          payload.type || 'mixed',
          payload.minCages || 12,
          payload.maxCages || 18,
          payload.minInequalities || 15,
          payload.maxInequalities || 25,
          payload.seed || 0,
          payload.includeSolution || false,
          payload.fillAllCells || false,
          payload.ensureUniqueSolution || true,
          payload.difficulty || 50
        )
        response = { id, success: true, data: puzzleStr }
        break

      default:
        response = { id, success: false, error: `Unknown message type: ${type}` }
    }
  } catch (error) {
    response = {
      id,
      success: false,
      error: error instanceof Error ? error.message : String(error)
    }
  }

  self.postMessage(response)
}
