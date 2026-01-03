import { ref, computed, watch } from 'vue'
import type { SudokuPuzzle, Cell } from '@/types/sudoku'

const GRID_SIZE = 9
const BOX_SIZE = 3

// ===================================
// 游戏状态
// ===================================
const puzzle = ref<SudokuPuzzle>(createEmptyPuzzle())
const grid = ref<number[][]>(createEmptyGrid())
const selectedCell = ref<Cell | null>(null)
// undefined: 未选择数字, null: 删除模式, 1-9: 填入数字
const selectedNumber = ref<number | null | undefined>(undefined)
const conflictCells = ref<Set<string>>(new Set())
const givenCells = ref<Set<string>>(new Set())

// 笔记功能
const notes = ref<Set<number>[][]>(createEmptyNotes())
const notesMode = ref(false)

// 计时器
const timerSeconds = ref(0)
const timerRunning = ref(false)
let timerInterval: number | null = null

// 胜利状态
const isVictory = ref(false)
const showVictoryModal = ref(false)

// 历史记录（撤销/重做）
interface HistoryState {
  grid: number[][]
  notes: number[][] // 存储为数组方便序列化
}
const history = ref<HistoryState[]>([])
const historyIndex = ref(-1)

function createEmptyPuzzle(): SudokuPuzzle {
  return {
    grid: createEmptyGrid(),
    cages: [],
    inequalities: []
  }
}

function createEmptyGrid(): number[][] {
  return Array.from({ length: GRID_SIZE }, () => Array(GRID_SIZE).fill(0))
}

function createEmptyNotes(): Set<number>[][] {
  return Array.from({ length: GRID_SIZE }, () => 
    Array.from({ length: GRID_SIZE }, () => new Set<number>())
  )
}

// 将笔记转换为数组（用于历史记录序列化）
function notesToArrays(notesGrid: Set<number>[][]): number[][] {
  return notesGrid.map(row => 
    row.map(cellNotes => {
      let bits = 0
      cellNotes.forEach(n => bits |= (1 << n))
      return bits
    })
  )
}

// 从数组恢复笔记
function arraysToNotes(arrays: number[][]): Set<number>[][] {
  return arrays.map(row => 
    row.map(bits => {
      const set = new Set<number>()
      for (let n = 1; n <= 9; n++) {
        if (bits & (1 << n)) set.add(n)
      }
      return set
    })
  )
}

// ===================================
// 冲突检测
// ===================================
function recomputeConflicts() {
  const conflicts = new Set<string>()

  const markDuplicates = (cells: Array<{ r: number; c: number; v: number }>) => {
    const byVal = new Map<number, Array<{ r: number; c: number }>>()
    cells.forEach(({ r, c, v }) => {
      if (v === 0) return
      if (!byVal.has(v)) byVal.set(v, [])
      byVal.get(v)!.push({ r, c })
    })
    byVal.forEach((list) => {
      if (list.length > 1) {
        list.forEach(({ r, c }) => conflicts.add(`${r}-${c}`))
      }
    })
  }

  // 行检测
  for (let r = 0; r < GRID_SIZE; r++) {
    const row = grid.value[r]!
    markDuplicates(row.map((v, c) => ({ r, c, v })))
  }

  // 列检测
  for (let c = 0; c < GRID_SIZE; c++) {
    const col = grid.value.map((row, r) => ({ r, c, v: row![c]! }))
    markDuplicates(col)
  }

  // 宫格检测
  for (let br = 0; br < BOX_SIZE; br++) {
    for (let bc = 0; bc < BOX_SIZE; bc++) {
      const box: Array<{ r: number; c: number; v: number }> = []
      for (let r = 0; r < BOX_SIZE; r++) {
        for (let c = 0; c < BOX_SIZE; c++) {
          const row = br * BOX_SIZE + r
          const col = bc * BOX_SIZE + c
          box.push({ r: row, c: col, v: grid.value[row]![col]! })
        }
      }
      markDuplicates(box)
    }
  }

  // 不等式检测（包括边界值冲突）
  puzzle.value.inequalities.forEach(({ cell1, cell2, type }) => {
    const v1 = grid.value[cell1.row]![cell1.col]!
    const v2 = grid.value[cell2.row]![cell2.col]!
    
    // 边界值检测：
    // - 如果是 ">"，cell1 的值不能是 1（1 不能大于任何数）
    // - 如果是 "<"，cell1 的值不能是 9（9 不能小于任何数）
    if (v1 !== 0) {
      if ((type === '>' && v1 === 1) || (type === '<' && v1 === 9)) {
        conflicts.add(`${cell1.row}-${cell1.col}`)
      }
    }
    if (v2 !== 0) {
      // 对于 cell2：
      // - 如果是 ">"，cell2 的值不能是 9（没有数能大于 9）
      // - 如果是 "<"，cell2 的值不能是 1（没有数能小于 1）
      if ((type === '>' && v2 === 9) || (type === '<' && v2 === 1)) {
        conflicts.add(`${cell2.row}-${cell2.col}`)
      }
    }
    
    // 实际值冲突检测
    if (v1 !== 0 && v2 !== 0) {
      const valid = type === '>' ? v1 > v2 : v1 < v2
      if (!valid) {
        conflicts.add(`${cell1.row}-${cell1.col}`)
        conflicts.add(`${cell2.row}-${cell2.col}`)
      }
    }
  })

  // 笼子检测（和 + 唯一性）
  puzzle.value.cages.forEach((cage) => {
    let sum = 0
    let filled = true
    const cageValues: Array<{ r: number; c: number; v: number }> = []
    
    cage.cells.forEach(({ row, col }) => {
      const v = grid.value[row]![col]!
      if (v === 0) {
        filled = false
      } else {
        cageValues.push({ r: row, c: col, v })
        sum += v
      }
    })
    
    // 笼子内重复数字检测
    const valueMap = new Map<number, Array<{ r: number; c: number }>>()
    cageValues.forEach(({ r, c, v }) => {
      if (!valueMap.has(v)) valueMap.set(v, [])
      valueMap.get(v)!.push({ r, c })
    })
    valueMap.forEach((cells) => {
      if (cells.length > 1) {
        cells.forEach(({ r, c }) => conflicts.add(`${r}-${c}`))
      }
    })
    
    // 笼子和检测
    if (sum > cage.sum || (filled && sum !== cage.sum)) {
      cageValues.forEach(({ r, c }) => conflicts.add(`${r}-${c}`))
    }
  })

  conflictCells.value = conflicts
}

// ===================================
// 胜利检测
// ===================================
const checkVictory = computed(() => {
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      if (grid.value[r]![c] === 0) return false
    }
  }
  return conflictCells.value.size === 0
})

watch(checkVictory, (won) => {
  if (won && !isVictory.value && timerRunning.value) {
    isVictory.value = true
    stopTimer()
    showVictoryModal.value = true
  }
})

// ===================================
// 计时器
// ===================================
function startTimer() {
  if (timerInterval) return
  timerRunning.value = true
  timerInterval = window.setInterval(() => {
    timerSeconds.value++
  }, 1000)
}

function stopTimer() {
  timerRunning.value = false
  if (timerInterval) {
    clearInterval(timerInterval)
    timerInterval = null
  }
}

function resetTimer() {
  stopTimer()
  timerSeconds.value = 0
  isVictory.value = false
}

function toggleTimer() {
  if (timerRunning.value) {
    stopTimer()
  } else {
    startTimer()
  }
}

const formattedTime = computed(() => {
  const mins = Math.floor(timerSeconds.value / 60)
  const secs = timerSeconds.value % 60
  return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`
})

// ===================================
// 单元格操作
// ===================================
function selectCell(row: number, col: number) {
  if (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE) return
  selectedCell.value = { row, col }
}

function setCellValue(value: number | null) {
  if (!selectedCell.value) return
  const { row, col } = selectedCell.value
  if (givenCells.value.has(`${row}-${col}`)) return

  if (notesMode.value) {
    // 笔记模式：切换候选数字
    if (value !== null && value >= 1 && value <= 9) {
      // 只有在单元格为空时才能添加笔记
      if (grid.value[row]![col] !== 0) return
      
      // 保存历史
      saveHistory()
      toggleNote(row, col, value)
    }
  } else {
    // 普通模式：填入数字
    const oldValue = grid.value[row]![col]!
    const newValue = value ?? 0
    
    // 如果值没有变化，不做任何操作
    if (oldValue === newValue) return
    
    // 保存历史
    saveHistory()
    
    grid.value[row]![col] = newValue
    // 填入数字时清除该单元格的笔记
    if (newValue !== 0) {
      notes.value[row]![col]!.clear()
      // 自动清除同行、同列、同宫的该数字笔记
      clearRelatedNotes(row, col, newValue)
    }
    recomputeConflicts()
  }
}

function toggleNote(row: number, col: number, num: number) {
  // 如果单元格已有数字，不能添加笔记
  if (grid.value[row]![col] !== 0) return
  
  const cellNotes = notes.value[row]![col]!
  if (cellNotes.has(num)) {
    cellNotes.delete(num)
  } else {
    cellNotes.add(num)
  }
}

function clearRelatedNotes(row: number, col: number, num: number) {
  // 清除同行笔记
  for (let c = 0; c < GRID_SIZE; c++) {
    notes.value[row]![c]!.delete(num)
  }
  // 清除同列笔记
  for (let r = 0; r < GRID_SIZE; r++) {
    notes.value[r]![col]!.delete(num)
  }
  // 清除同宫笔记
  const boxRow = Math.floor(row / BOX_SIZE) * BOX_SIZE
  const boxCol = Math.floor(col / BOX_SIZE) * BOX_SIZE
  for (let r = boxRow; r < boxRow + BOX_SIZE; r++) {
    for (let c = boxCol; c < boxCol + BOX_SIZE; c++) {
      notes.value[r]![c]!.delete(num)
    }
  }
}

function toggleNotesMode() {
  notesMode.value = !notesMode.value
}

function clearCellNotes(row: number, col: number) {
  notes.value[row]![col]!.clear()
}

function getCellNotes(row: number, col: number): Set<number> {
  return notes.value[row]![col]!
}

function clearCell() {
  if (!selectedCell.value) return
  const { row, col } = selectedCell.value
  if (givenCells.value.has(`${row}-${col}`)) return
  
  // 保存历史
  saveHistory()
  
  if (notesMode.value) {
    // 笔记模式：清除笔记
    clearCellNotes(row, col)
  } else {
    // 普通模式：清除数字
    grid.value[row]![col] = 0
    recomputeConflicts()
  }
}

// ===================================
// 历史记录
// ===================================
function saveHistory() {
  // 移除当前位置之后的历史
  history.value = history.value.slice(0, historyIndex.value + 1)
  // 添加当前状态（包括笔记）
  history.value.push({
    grid: grid.value.map(row => [...row]),
    notes: notesToArrays(notes.value)
  })
  historyIndex.value = history.value.length - 1
  
  // 限制历史数量
  if (history.value.length > 100) {
    history.value.shift()
    historyIndex.value--
  }
}

function undo() {
  if (historyIndex.value > 0) {
    historyIndex.value--
    const state = history.value[historyIndex.value]!
    grid.value = state.grid.map(row => [...row])
    notes.value = arraysToNotes(state.notes)
    recomputeConflicts()
  }
}

function redo() {
  if (historyIndex.value < history.value.length - 1) {
    historyIndex.value++
    const state = history.value[historyIndex.value]!
    grid.value = state.grid.map(row => [...row])
    notes.value = arraysToNotes(state.notes)
    recomputeConflicts()
  }
}

const canUndo = computed(() => historyIndex.value > 0)
const canRedo = computed(() => historyIndex.value < history.value.length - 1)

// ===================================
// 游戏操作
// ===================================
function loadPuzzle(puzzleData: SudokuPuzzle, resetTimerFlag: boolean = true) {
  puzzle.value = puzzleData
  grid.value = puzzleData.grid.map(row => [...row])
  selectedCell.value = null
  selectedNumber.value = undefined
  notesMode.value = false
  notes.value = createEmptyNotes()
  
  // 标记给定数字
  givenCells.value.clear()
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      if (puzzleData.grid[r]![c] !== 0) {
        givenCells.value.add(`${r}-${c}`)
      }
    }
  }
  
  // 重置历史
  history.value = [{
    grid: grid.value.map(row => [...row]),
    notes: notesToArrays(notes.value)
  }]
  historyIndex.value = 0
  
  recomputeConflicts()
  
  // 只有在需要时才重置计时器
  if (resetTimerFlag) {
    resetTimer()
    startTimer()
  }
  
  isVictory.value = false
  showVictoryModal.value = false
}

function resetGame() {
  grid.value = puzzle.value.grid.map(row => [...row])
  selectedCell.value = null
  selectedNumber.value = undefined
  notesMode.value = false
  notes.value = createEmptyNotes()
  
  // 重置历史
  history.value = [{
    grid: grid.value.map(row => [...row]),
    notes: notesToArrays(notes.value)
  }]
  historyIndex.value = 0
  
  recomputeConflicts()
  resetTimer()
  startTimer()
  isVictory.value = false
  showVictoryModal.value = false
}

function clearGame() {
  puzzle.value = createEmptyPuzzle()
  grid.value = createEmptyGrid()
  notes.value = createEmptyNotes()
  notesMode.value = false
  givenCells.value.clear()
  conflictCells.value.clear()
  selectedCell.value = null
  selectedNumber.value = undefined
  history.value = []
  historyIndex.value = -1
  resetTimer()
}

// ===================================
// 导出
// ===================================
export function useSudokuGame() {
  return {
    // 状态
    puzzle,
    grid,
    selectedCell,
    selectedNumber,
    conflictCells,
    givenCells,
    timerSeconds,
    timerRunning,
    formattedTime,
    isVictory,
    showVictoryModal,
    canUndo,
    canRedo,
    notes,
    notesMode,
    
    // 方法
    selectCell,
    setCellValue,
    clearCell,
    loadPuzzle,
    resetGame,
    clearGame,
    undo,
    redo,
    startTimer,
    stopTimer,
    toggleTimer,
    recomputeConflicts,
    toggleNotesMode,
    getCellNotes,
    
    // 常量
    GRID_SIZE,
    BOX_SIZE
  }
}
