<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'
import { useTheme } from '@/composables/useTheme'
import { useSudokuGame } from '@/composables/useSudokuGame'

// GitHub 仓库链接
const GITHUB_REPO = 'https://github.com/BeiChenStanly/Sudoku-Solver'
import { sudokuWasm } from '@/services/sudokuWasm'
import { parsePuzzle, puzzleToString } from '@/types/sudoku'
import type { PuzzleType } from '@/types/sudoku'

import SudokuCanvas from '@/components/SudokuCanvas.vue'
import NumberPad from '@/components/NumberPad.vue'
import GameTimer from '@/components/GameTimer.vue'
import ControlPanel from '@/components/ControlPanel.vue'
import ThemeToggle from '@/components/ThemeToggle.vue'
import VictoryModal from '@/components/VictoryModal.vue'
import LoadingOverlay from '@/components/LoadingOverlay.vue'

const { initTheme } = useTheme()
const { 
  puzzle, loadPuzzle, resetGame, timerRunning, isVictory,
  undo, redo, canUndo, canRedo, startTimer, stopTimer
} = useSudokuGame()

// 页面可见性状态，用于自动暂停
const wasRunningBeforeHidden = ref(false)

const canvasRef = ref<InstanceType<typeof SudokuCanvas> | null>(null)
const isLoading = ref(false)
const loadingMessage = ref('')
const wasmReady = ref(false)
const errorMsg = ref('')

interface GenerateOptions {
  type: PuzzleType
  minCages: number
  maxCages: number
  minInequalities: number
  maxInequalities: number
  fillComplete: boolean
  ensureUnique: boolean
  difficulty?: number
}

// 初始化
onMounted(async () => {
  initTheme()
  
  // 监听页面可见性变化，实现自动暂停
  document.addEventListener('visibilitychange', handleVisibilityChange)
  
  try {
    loadingMessage.value = '正在加载引擎...'
    isLoading.value = true
    await sudokuWasm.init()
    wasmReady.value = true
    await generatePuzzle({
      type: 'mixed',
      minCages: 12,
      maxCages: 18,
      minInequalities: 15,
      maxInequalities: 25,
      fillComplete: false,
      ensureUnique: true
    })
  } catch (err) {
    errorMsg.value = `加载失败: ${err}`
    console.error(err)
  } finally {
    isLoading.value = false
  }
})

onBeforeUnmount(() => {
  sudokuWasm.destroy()
  document.removeEventListener('visibilitychange', handleVisibilityChange)
})

// 页面可见性变化处理 - 自动暂停/恢复
function handleVisibilityChange() {
  if (document.hidden) {
    // 页面变为不可见，记录状态并暂停
    if (timerRunning.value && !isVictory.value) {
      wasRunningBeforeHidden.value = true
      stopTimer()
    }
  } else {
    // 页面变为可见，恢复之前的状态
    if (wasRunningBeforeHidden.value && !isVictory.value) {
      wasRunningBeforeHidden.value = false
      startTimer()
    }
  }
}

// 生成谜题
async function generatePuzzle(options: GenerateOptions) {
  if (!wasmReady.value) return
  
  isLoading.value = true
  loadingMessage.value = '正在生成谜题...'
  errorMsg.value = ''
  
  try {
    const puzzleStr = await sudokuWasm.generate({
      type: options.type,
      minCages: options.minCages,
      maxCages: options.maxCages,
      minInequalities: options.minInequalities,
      maxInequalities: options.maxInequalities,
      fillAllCells: options.fillComplete,
      ensureUniqueSolution: options.ensureUnique,
      difficulty: options.difficulty ?? 50
    })
    
    const newPuzzle = parsePuzzle(puzzleStr)
    loadPuzzle(newPuzzle)
    
    // 触发重绘
    setTimeout(() => {
      canvasRef.value?.renderBoard()
    }, 50)
  } catch (err) {
    errorMsg.value = `生成失败: ${err}`
    console.error(err)
  } finally {
    isLoading.value = false
  }
}

// 求解
async function solvePuzzle() {
  if (!wasmReady.value) return
  
  isLoading.value = true
  loadingMessage.value = '正在求解...'
  errorMsg.value = ''
  
  try {
    const puzzleStr = puzzleToString(puzzle.value)
    const result = await sudokuWasm.solve(puzzleStr, false)
    
    if (result.solved && result.grid) {
      const solvedPuzzle = { ...puzzle.value, grid: result.grid }
      // 求解时不重置计时器
      loadPuzzle(solvedPuzzle, false)
      
      setTimeout(() => {
        canvasRef.value?.renderBoard()
      }, 50)
    } else {
      errorMsg.value = result.error || '无法求解此谜题'
    }
  } catch (err) {
    errorMsg.value = `求解失败: ${err}`
    console.error(err)
  } finally {
    isLoading.value = false
  }
}

// 重置
function handleReset() {
  resetGame()
  setTimeout(() => {
    canvasRef.value?.renderBoard()
  }, 50)
}

// 新游戏
function handleNewGame() {
  generatePuzzle({
    type: 'mixed',
    minCages: 12,
    maxCages: 18,
    minInequalities: 15,
    maxInequalities: 25,
    fillComplete: false,
    ensureUnique: true
  })
}
</script>

<template>
  <div class="app">
    <LoadingOverlay v-if="isLoading" :message="loadingMessage" />
    
    <header class="header">
      <div class="header-brand">
        <span class="header-icon">🧩</span>
        <h1 class="header-title">数独</h1>
      </div>
      <div class="header-actions">
        <a 
          :href="GITHUB_REPO"
          target="_blank"
          rel="noopener noreferrer"
          class="github-link"
          title="在 GitHub 上查看源代码"
        >
          <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
            <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/>
          </svg>
        </a>
        <ThemeToggle />
      </div>
    </header>
    
    <main class="main">
      <div class="game-container">
        <!-- 左侧：棋盘 -->
        <section class="board-section">
          <SudokuCanvas ref="canvasRef" />
        </section>
        
        <!-- 右侧：控制区 -->
        <aside class="control-section">
          <GameTimer />
          
          <div class="panel">
            <h3 class="panel-title">数字键盘</h3>
            <NumberPad @select="canvasRef?.renderBoard()" />
          </div>
          
          <div class="panel">
            <h3 class="panel-title">操作</h3>
            <ControlPanel 
              :is-loading="isLoading"
              :wasm-ready="wasmReady"
              :timer-running="timerRunning"
              @generate="generatePuzzle"
              @solve="solvePuzzle"
              @reset="handleReset"
            />
          </div>
          
          <div class="panel panel--actions">
            <button 
              class="action-btn"
              :disabled="!canUndo || !timerRunning"
              @click="undo(); canvasRef?.renderBoard()"
              title="撤销"
            >
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M3 7v6h6" />
                <path d="M21 17a9 9 0 0 0-9-9 9 9 0 0 0-6 2.3L3 13" />
              </svg>
              撤销
            </button>
            <button 
              class="action-btn"
              :disabled="!canRedo || !timerRunning"
              @click="redo(); canvasRef?.renderBoard()"
              title="重做"
            >
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M21 7v6h-6" />
                <path d="M3 17a9 9 0 0 1 9-9 9 9 0 0 1 6 2.3l3 2.7" />
              </svg>
              重做
            </button>
          </div>
          
          <div v-if="errorMsg" class="error-msg">
            {{ errorMsg }}
          </div>
        </aside>
      </div>
    </main>
    
    <VictoryModal @new-game="handleNewGame" />
  </div>
</template>

<style lang="scss">
@use '@/styles/variables';
@use '@/styles/global';
</style>

<style scoped lang="scss">
.app {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--bg-primary);
}

.header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 24px;
  background: var(--bg-secondary);
  border-bottom: 1px solid var(--border-light);
  
  @media (max-width: 640px) {
    padding: 12px 16px;
  }
}

.header-brand {
  display: flex;
  align-items: center;
  gap: 12px;
}

.header-icon {
  font-size: 28px;
}

.header-title {
  font-size: 24px;
  font-weight: 700;
  color: var(--text-primary);
  margin: 0;
  
  @media (max-width: 640px) {
    font-size: 20px;
  }
}

.header-actions {
  display: flex;
  align-items: center;
  gap: 12px;
}

.github-link {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  border-radius: var(--radius-md);
  color: var(--text-secondary);
  transition: all var(--transition-fast);
  
  &:hover {
    background: var(--bg-tertiary);
    color: var(--text-primary);
  }
  
  svg {
    display: block;
  }
}

.main {
  flex: 1;
  padding: 24px;
  
  @media (max-width: 640px) {
    padding: 16px;
  }
}

.game-container {
  max-width: 1100px;
  margin: 0 auto;
  display: grid;
  grid-template-columns: minmax(320px, 580px) minmax(280px, 400px);
  gap: 24px;
  align-items: start;
  
  @media (max-width: 960px) {
    grid-template-columns: 1fr;
    max-width: 600px;
  }
}

.board-section {
  background: var(--bg-secondary);
  border-radius: var(--radius-lg);
  border: 1px solid var(--border-light);
  box-shadow: var(--shadow-md);
  padding: 16px;
  
  @media (max-width: 640px) {
    padding: 12px;
  }
}

.control-section {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.panel {
  background: var(--bg-secondary);
  border-radius: var(--radius-lg);
  border: 1px solid var(--border-light);
  padding: 16px;
  
  &--actions {
    display: flex;
    gap: 12px;
  }
}

.panel-title {
  font-size: 13px;
  font-weight: 600;
  color: var(--text-tertiary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin: 0 0 12px 0;
}

.action-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 10px 16px;
  border: 1px solid var(--border-light);
  border-radius: var(--radius-md);
  background: var(--bg-secondary);
  color: var(--text-secondary);
  font-size: 13px;
  font-weight: 500;
  cursor: pointer;
  transition: all var(--transition-fast);
  
  &:hover:not(:disabled) {
    background: var(--bg-tertiary);
    color: var(--text-primary);
  }
  
  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }
}

.error-msg {
  padding: 12px 16px;
  background: rgba(239, 68, 68, 0.1);
  border: 1px solid rgba(239, 68, 68, 0.3);
  border-radius: var(--radius-md);
  color: var(--error);
  font-size: 13px;
}
</style>
