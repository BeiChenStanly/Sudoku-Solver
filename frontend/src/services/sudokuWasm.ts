// WASM Service - 封装 Worker 调用
import type { SolveResult, PuzzleType } from '@/types/sudoku'

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

class SudokuWasmService {
  private worker: Worker | null = null
  private pendingRequests = new Map<string, {
    resolve: (value: any) => void
    reject: (reason: any) => void
  }>()
  private initPromise: Promise<void> | null = null
  private isReady = false

  // 生成唯一 ID
  private generateId(): string {
    return `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`
  }

  // 初始化 Worker
  async init(): Promise<void> {
    if (this.isReady) return
    if (this.initPromise) return this.initPromise

    this.initPromise = new Promise((resolve, reject) => {
      try {
        // 创建 Worker - 使用 classic 类型以支持 importScripts
        this.worker = new Worker(
          new URL('../workers/sudoku.worker.ts', import.meta.url),
          { type: 'classic' }
        )

        this.worker.onmessage = (event: MessageEvent<WorkerResponse>) => {
          const { id, success, data, error } = event.data
          const pending = this.pendingRequests.get(id)
          
          if (pending) {
            this.pendingRequests.delete(id)
            if (success) {
              pending.resolve(data)
            } else {
              pending.reject(new Error(error || 'Unknown error'))
            }
          }
        }

        this.worker.onerror = (error) => {
          console.error('Worker error:', error)
          reject(error)
        }

        // 发送初始化消息
        const id = this.generateId()
        this.pendingRequests.set(id, {
          resolve: () => {
            this.isReady = true
            resolve()
          },
          reject
        })
        
        this.worker.postMessage({ id, type: 'init' } as WorkerMessage)
      } catch (err) {
        reject(err)
      }
    })

    return this.initPromise
  }

  // 发送消息到 Worker
  private sendMessage<T>(type: string, payload?: any): Promise<T> {
    return new Promise((resolve, reject) => {
      if (!this.worker) {
        reject(new Error('Worker not initialized'))
        return
      }

      const id = this.generateId()
      this.pendingRequests.set(id, { resolve, reject })
      
      this.worker.postMessage({ id, type, payload } as WorkerMessage)
    })
  }

  // 求解数独
  async solve(puzzleStr: string, checkUniqueness = false): Promise<SolveResult> {
    await this.init()
    return this.sendMessage('solve', { puzzleStr, checkUniqueness })
  }

  // 生成数独
  async generate(options: {
    type?: PuzzleType
    minCages?: number
    maxCages?: number
    minInequalities?: number
    maxInequalities?: number
    seed?: number
    includeSolution?: boolean
    fillAllCells?: boolean
    ensureUniqueSolution?: boolean
    difficulty?: number
  } = {}): Promise<string> {
    await this.init()
    return this.sendMessage('generate', options)
  }

  // 检查是否就绪
  get ready(): boolean {
    return this.isReady
  }

  // 销毁 Worker
  destroy() {
    if (this.worker) {
      this.worker.terminate()
      this.worker = null
      this.isReady = false
      this.initPromise = null
      this.pendingRequests.clear()
    }
  }
}

// 单例导出
export const sudokuWasm = new SudokuWasmService()
