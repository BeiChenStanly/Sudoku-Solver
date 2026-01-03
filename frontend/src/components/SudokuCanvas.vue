<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount, watch, nextTick } from 'vue'
import { useSudokuGame } from '@/composables/useSudokuGame'
import type { Cell } from '@/types/sudoku'
import { getCageForCell, getCageTopLeft, isSameCageBorder } from '@/types/sudoku'
import { useTheme } from '@/composables/useTheme'

const {
  puzzle, grid, selectedCell, selectedNumber, conflictCells, givenCells,
  timerRunning, selectCell, setCellValue, GRID_SIZE, BOX_SIZE, notes, notesMode,
  isVictory
} = useSudokuGame()

const { isDark } = useTheme()

// Canvas 引用
const canvasRef = ref<HTMLCanvasElement | null>(null)
const containerRef = ref<HTMLDivElement | null>(null)
const boardSize = ref(450)

// 波纹动画
interface Ripple {
  row: number
  col: number
  startTime: number
  duration: number
  cells: Cell[]
}
const ripples = ref<Ripple[]>([])
let animationFrameId: number | null = null

// 调整尺寸
function resizeCanvas() {
  if (!containerRef.value || !canvasRef.value) return
  
  const containerWidth = containerRef.value.clientWidth
  const maxSize = Math.min(containerWidth - 20, 560)
  boardSize.value = Math.max(320, maxSize)
  
  const canvas = canvasRef.value
  const dpr = window.devicePixelRatio || 1
  canvas.width = boardSize.value * dpr
  canvas.height = boardSize.value * dpr
  canvas.style.width = `${boardSize.value}px`
  canvas.style.height = `${boardSize.value}px`
  
  const ctx = canvas.getContext('2d')
  if (ctx) {
    ctx.setTransform(1, 0, 0, 1, 0, 0)
    ctx.scale(dpr, dpr)
  }
  
  renderBoard()
}

// 检查完成的区域
function checkCompletions(row: number, col: number): Cell[] {
  const completed: Cell[] = []
  const addedKeys = new Set<string>()
  
  const addCell = (r: number, c: number) => {
    const key = `${r}-${c}`
    if (!addedKeys.has(key)) {
      addedKeys.add(key)
      completed.push({ row: r, col: c })
    }
  }
  
  // 检查行
  const rowCells = grid.value[row]!
  if (rowCells.every(v => v !== 0) && new Set(rowCells).size === 9) {
    for (let c = 0; c < GRID_SIZE; c++) addCell(row, c)
  }
  
  // 检查列
  const colVals = grid.value.map(r => r![col]!)
  if (colVals.every(v => v !== 0) && new Set(colVals).size === 9) {
    for (let r = 0; r < GRID_SIZE; r++) addCell(r, col)
  }
  
  // 检查宫
  const boxR = Math.floor(row / BOX_SIZE) * BOX_SIZE
  const boxC = Math.floor(col / BOX_SIZE) * BOX_SIZE
  const boxVals: number[] = []
  for (let r = boxR; r < boxR + BOX_SIZE; r++) {
    for (let c = boxC; c < boxC + BOX_SIZE; c++) {
      boxVals.push(grid.value[r]![c]!)
    }
  }
  if (boxVals.every(v => v !== 0) && new Set(boxVals).size === 9) {
    for (let r = boxR; r < boxR + BOX_SIZE; r++) {
      for (let c = boxC; c < boxC + BOX_SIZE; c++) {
        addCell(r, c)
      }
    }
  }
  
  // 检查笼子
  for (const cage of puzzle.value.cages) {
    // 检查当前格子是否在此笼子中
    const inCage = cage.cells.some(cell => cell.row === row && cell.col === col)
    if (!inCage) continue
    
    // 检查笼子是否完成（所有格子已填且和正确）
    let sum = 0
    let allFilled = true
    const cageVals: number[] = []
    
    for (const cell of cage.cells) {
      const val = grid.value[cell.row]![cell.col]!
      if (val === 0) {
        allFilled = false
        break
      }
      sum += val
      cageVals.push(val)
    }
    
    // 笼子完成条件：所有格子已填、和正确、无重复
    if (allFilled && sum === cage.sum && new Set(cageVals).size === cageVals.length) {
      for (const cell of cage.cells) {
        addCell(cell.row, cell.col)
      }
    }
  }
  
  return completed
}

// 波纹动画
function startRipple(row: number, col: number, cells: Cell[]) {
  const now = performance.now()
  ripples.value.push({ row, col, startTime: now, duration: 800, cells })
  
  if (ripples.value.length > 3) {
    ripples.value.shift()
  }
  
  if (!animationFrameId) {
    animateRipples()
  }
}

function animateRipples() {
  const now = performance.now()
  ripples.value = ripples.value.filter(r => now - r.startTime < r.duration)
  
  if (ripples.value.length > 0) {
    renderBoard()
    animationFrameId = requestAnimationFrame(animateRipples)
  } else {
    animationFrameId = null
  }
}

// 点击处理
function handleCanvasClick(event: MouseEvent) {
  const canvas = canvasRef.value
  if (!canvas || !timerRunning.value) return
  
  const rect = canvas.getBoundingClientRect()
  const x = ((event.clientX - rect.left) / rect.width) * boardSize.value
  const y = ((event.clientY - rect.top) / rect.height) * boardSize.value
  
  const cellSize = boardSize.value / GRID_SIZE
  const col = Math.floor(x / cellSize)
  const row = Math.floor(y / cellSize)
  
  if (col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE) {
    selectCell(row, col)
    
    // selectedNumber === null 表示删除模式
    // selectedNumber === undefined 表示未选择任何数字
    if (selectedNumber.value !== undefined) {
      const oldVal = grid.value[row]![col]!
      setCellValue(selectedNumber.value)
      
      if (selectedNumber.value !== null && selectedNumber.value !== oldVal) {
        const completedCells = checkCompletions(row, col)
        if (completedCells.length > 0) {
          startRipple(row, col, completedCells)
        }
      }
    }
    
    renderBoard()
  }
}

// 键盘处理
function handleKeydown(event: KeyboardEvent) {
  if (!selectedCell.value || !timerRunning.value) return
  
  const { row, col } = selectedCell.value
  
  if (event.key >= '1' && event.key <= '9') {
    const num = parseInt(event.key)
    const oldVal = grid.value[row]![col]!
    setCellValue(num)
    
    if (num !== oldVal) {
      const completedCells = checkCompletions(row, col)
      if (completedCells.length > 0) {
        startRipple(row, col, completedCells)
      }
    }
    renderBoard()
  } else if (event.key === '0' || event.key === 'Backspace' || event.key === 'Delete') {
    setCellValue(null)
    renderBoard()
  } else if (event.key === 'ArrowUp') {
    selectCell(row - 1, col)
    renderBoard()
  } else if (event.key === 'ArrowDown') {
    selectCell(row + 1, col)
    renderBoard()
  } else if (event.key === 'ArrowLeft') {
    selectCell(row, col - 1)
    renderBoard()
  } else if (event.key === 'ArrowRight') {
    selectCell(row, col + 1)
    renderBoard()
  }
}

// ===================================
// 渲染
// ===================================
function renderBoard() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return
  
  const size = boardSize.value
  const cellSize = size / GRID_SIZE
  
  // 清空画布
  ctx.clearRect(0, 0, size, size)
  
  // 暂停遮罩（胜利时不显示，保持棋盘可见）
  if (!timerRunning.value && !isVictory.value) {
    drawPausedOverlay(ctx, size)
    return
  }
  
  // 背景
  ctx.fillStyle = getComputedStyle(document.documentElement)
    .getPropertyValue('--board-bg').trim() || '#fffef9'
  ctx.fillRect(0, 0, size, size)
  
  // 绘制各层
  drawCageBackgrounds(ctx, cellSize)
  drawConflicts(ctx, cellSize)
  drawSelection(ctx, cellSize)
  drawHighlightedNumbers(ctx, cellSize)
  drawRipples(ctx, cellSize)
  drawGridLines(ctx, cellSize)
  drawCageBorders(ctx, cellSize)
  drawCageSums(ctx, cellSize)
  drawValues(ctx, cellSize)
  drawNotes(ctx, cellSize)
  drawInequalities(ctx, cellSize)
}

function drawPausedOverlay(ctx: CanvasRenderingContext2D, size: number) {
  const styles = getComputedStyle(document.documentElement)
  const bgColor = styles.getPropertyValue('--bg-tertiary').trim() || '#f3f4f6'
  
  ctx.fillStyle = bgColor
  ctx.fillRect(0, 0, size, size)
  
  ctx.fillStyle = styles.getPropertyValue('--text-tertiary').trim() || '#9ca3af'
  ctx.font = `bold ${Math.floor(size * 0.06)}px Inter, sans-serif`
  ctx.textAlign = 'center'
  ctx.textBaseline = 'middle'
  ctx.fillText('⏸ 游戏暂停', size / 2, size / 2 - 20)
  
  ctx.font = `${Math.floor(size * 0.03)}px Inter, sans-serif`
  ctx.fillText('点击「继续」恢复游戏', size / 2, size / 2 + 20)
}

function drawCageBackgrounds(ctx: CanvasRenderingContext2D, cellSize: number) {
  const colors = [
    'var(--cage-1)', 'var(--cage-2)', 'var(--cage-3)', 'var(--cage-4)',
    'var(--cage-5)', 'var(--cage-6)', 'var(--cage-7)', 'var(--cage-8)'
  ]
  
  puzzle.value.cages.forEach((cage, i) => {
    const colorVar = colors[i % colors.length]!
    ctx.fillStyle = getComputedStyle(document.documentElement)
      .getPropertyValue(colorVar.replace('var(', '').replace(')', '')).trim() 
      || 'rgba(200, 200, 200, 0.3)'
    
    cage.cells.forEach(({ row, col }) => {
      ctx.fillRect(col * cellSize, row * cellSize, cellSize, cellSize)
    })
  })
}

function drawConflicts(ctx: CanvasRenderingContext2D, cellSize: number) {
  const color = getComputedStyle(document.documentElement)
    .getPropertyValue('--cell-conflict').trim() || 'rgba(239, 68, 68, 0.2)'
  
  ctx.fillStyle = color
  conflictCells.value.forEach(key => {
    const [r, c] = key.split('-').map(Number) as [number, number]
    ctx.fillRect(c * cellSize, r * cellSize, cellSize, cellSize)
  })
}

function drawSelection(ctx: CanvasRenderingContext2D, cellSize: number) {
  if (!selectedCell.value) return
  
  const { row, col } = selectedCell.value
  const color = getComputedStyle(document.documentElement)
    .getPropertyValue('--cell-selected').trim() || 'rgba(99, 102, 241, 0.25)'
  
  ctx.fillStyle = color
  ctx.fillRect(col * cellSize, row * cellSize, cellSize, cellSize)
}

function drawHighlightedNumbers(ctx: CanvasRenderingContext2D, cellSize: number) {
  // undefined: 未选择，null: 删除模式，1-9: 选中数字
  // 只在选中具体数字时高亮
  if (selectedNumber.value === null || selectedNumber.value === undefined) return
  
  const color = getComputedStyle(document.documentElement)
    .getPropertyValue('--cell-highlight').trim() || 'rgba(245, 158, 11, 0.2)'
  
  ctx.fillStyle = color
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      if (grid.value[r]![c] === selectedNumber.value) {
        ctx.fillRect(c * cellSize, r * cellSize, cellSize, cellSize)
      }
    }
  }
}

function drawRipples(ctx: CanvasRenderingContext2D, cellSize: number) {
  const now = performance.now()
  
  ripples.value.forEach(ripple => {
    const elapsed = now - ripple.startTime
    const progress = elapsed / ripple.duration
    if (progress >= 1) return
    
    const maxDist = ripple.cells.reduce((acc, cell) => {
      const d = Math.hypot(cell.row - ripple.row, cell.col - ripple.col)
      return Math.max(acc, d)
    }, 0)
    
    const waveDist = progress * (maxDist + 1)
    
    ripple.cells.forEach(cell => {
      const dist = Math.hypot(cell.row - ripple.row, cell.col - ripple.col)
      const diff = Math.abs(dist - waveDist)
      const alpha = Math.max(0, 0.4 * (1 - diff))
      
      if (alpha > 0) {
        ctx.fillStyle = `rgba(99, 102, 241, ${alpha})`
        ctx.fillRect(cell.col * cellSize, cell.row * cellSize, cellSize, cellSize)
      }
    })
  })
}

function drawGridLines(ctx: CanvasRenderingContext2D, cellSize: number) {
  const styles = getComputedStyle(document.documentElement)
  const thinColor = styles.getPropertyValue('--board-line').trim() || '#cbd5e1'
  const boldColor = styles.getPropertyValue('--board-line-bold').trim() || '#334155'
  
  // 细线
  ctx.strokeStyle = thinColor
  ctx.lineWidth = 1
  for (let i = 1; i < GRID_SIZE; i++) {
    if (i % BOX_SIZE === 0) continue
    
    ctx.beginPath()
    ctx.moveTo(i * cellSize, 0)
    ctx.lineTo(i * cellSize, GRID_SIZE * cellSize)
    ctx.stroke()
    
    ctx.beginPath()
    ctx.moveTo(0, i * cellSize)
    ctx.lineTo(GRID_SIZE * cellSize, i * cellSize)
    ctx.stroke()
  }
  
  // 粗线
  ctx.strokeStyle = boldColor
  ctx.lineWidth = 2.5
  for (let i = 0; i <= GRID_SIZE; i += BOX_SIZE) {
    ctx.beginPath()
    ctx.moveTo(i * cellSize, 0)
    ctx.lineTo(i * cellSize, GRID_SIZE * cellSize)
    ctx.stroke()
    
    ctx.beginPath()
    ctx.moveTo(0, i * cellSize)
    ctx.lineTo(GRID_SIZE * cellSize, i * cellSize)
    ctx.stroke()
  }
}

function drawCageBorders(ctx: CanvasRenderingContext2D, cellSize: number) {
  const offset = 3
  ctx.strokeStyle = getComputedStyle(document.documentElement)
    .getPropertyValue('--text-tertiary').trim() || '#9ca3af'
  ctx.lineWidth = 1.5
  ctx.setLineDash([4, 3])
  
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      const x = c * cellSize
      const y = r * cellSize
      const cage = getCageForCell(puzzle.value.cages, r, c)
      
      if (!cage) continue
      
      const hasNeighbor = (nr: number, nc: number) =>
        nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE &&
        isSameCageBorder(puzzle.value.cages, r, c, nr, nc)
      
      if (!hasNeighbor(r - 1, c)) {
        ctx.beginPath()
        ctx.moveTo(x + offset, y + offset)
        ctx.lineTo(x + cellSize - offset, y + offset)
        ctx.stroke()
      }
      
      if (!hasNeighbor(r + 1, c)) {
        ctx.beginPath()
        ctx.moveTo(x + offset, y + cellSize - offset)
        ctx.lineTo(x + cellSize - offset, y + cellSize - offset)
        ctx.stroke()
      }
      
      if (!hasNeighbor(r, c - 1)) {
        ctx.beginPath()
        ctx.moveTo(x + offset, y + offset)
        ctx.lineTo(x + offset, y + cellSize - offset)
        ctx.stroke()
      }
      
      if (!hasNeighbor(r, c + 1)) {
        ctx.beginPath()
        ctx.moveTo(x + cellSize - offset, y + offset)
        ctx.lineTo(x + cellSize - offset, y + cellSize - offset)
        ctx.stroke()
      }
    }
  }
  
  ctx.setLineDash([])
}

function drawCageSums(ctx: CanvasRenderingContext2D, cellSize: number) {
  const color = getComputedStyle(document.documentElement)
    .getPropertyValue('--text-secondary').trim() || '#4b5563'
  
  ctx.fillStyle = color
  ctx.font = `600 ${Math.floor(cellSize * 0.22)}px Inter, sans-serif`
  ctx.textAlign = 'left'
  ctx.textBaseline = 'top'
  
  puzzle.value.cages.forEach(cage => {
    const topLeft = getCageTopLeft(cage)
    const x = topLeft.col * cellSize + cellSize * 0.08
    const y = topLeft.row * cellSize + cellSize * 0.06
    ctx.fillText(String(cage.sum), x, y)
  })
}

function drawValues(ctx: CanvasRenderingContext2D, cellSize: number) {
  const styles = getComputedStyle(document.documentElement)
  const givenColor = styles.getPropertyValue('--cell-given').trim() || '#1e293b'
  const userColor = styles.getPropertyValue('--cell-user').trim() || '#6366f1'
  const errorColor = styles.getPropertyValue('--cell-error').trim() || '#dc2626'
  
  ctx.textAlign = 'center'
  ctx.textBaseline = 'middle'
  
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      const val = grid.value[r]![c]!
      if (val === 0) continue
      
      const isConflict = conflictCells.value.has(`${r}-${c}`)
      const isGiven = givenCells.value.has(`${r}-${c}`)
      
      ctx.fillStyle = isConflict ? errorColor : (isGiven ? givenColor : userColor)
      ctx.font = `${isGiven ? '700' : '600'} ${Math.floor(cellSize * 0.55)}px Inter, sans-serif`
      ctx.fillText(String(val), c * cellSize + cellSize / 2, r * cellSize + cellSize / 2 + 2)
    }
  }
}

function drawNotes(ctx: CanvasRenderingContext2D, cellSize: number) {
  const styles = getComputedStyle(document.documentElement)
  const noteColor = styles.getPropertyValue('--text-tertiary').trim() || '#6b7280'
  const highlightColor = styles.getPropertyValue('--warning').trim() || '#f59e0b'
  
  const noteSize = cellSize / 3
  const fontSize = Math.floor(noteSize * 0.65)
  ctx.font = `500 ${fontSize}px Inter, sans-serif`
  ctx.textAlign = 'center'
  ctx.textBaseline = 'middle'
  
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      // 如果单元格有数字，不显示笔记
      if (grid.value[r]![c] !== 0) continue
      
      const cellNotes = notes.value[r]![c]
      if (!cellNotes || cellNotes.size === 0) continue
      
      const cellX = c * cellSize
      const cellY = r * cellSize
      
      // 在3x3小格中绘制笔记
      for (let n = 1; n <= 9; n++) {
        if (!cellNotes.has(n)) continue
        
        const noteRow = Math.floor((n - 1) / 3)
        const noteCol = (n - 1) % 3
        const noteX = cellX + noteCol * noteSize + noteSize / 2
        const noteY = cellY + noteRow * noteSize + noteSize / 2
        
        // 如果当前选中的数字与笔记相同，高亮显示
        ctx.fillStyle = selectedNumber.value === n ? highlightColor : noteColor
        ctx.fillText(String(n), noteX, noteY)
      }
    }
  }
}

function drawInequalities(ctx: CanvasRenderingContext2D, cellSize: number) {
  const styles = getComputedStyle(document.documentElement)
  const normalColor = styles.getPropertyValue('--text-secondary').trim() || '#64748b'
  const errorColor = styles.getPropertyValue('--error').trim() || '#ef4444'
  
  ctx.font = `600 ${Math.floor(cellSize * 0.35)}px Inter, sans-serif`
  ctx.textAlign = 'center'
  ctx.textBaseline = 'middle'
  
  puzzle.value.inequalities.forEach(ineq => {
    const { cell1, cell2, type } = ineq
    const v1 = grid.value[cell1.row]![cell1.col]!
    const v2 = grid.value[cell2.row]![cell2.col]!
    
    // 检查此不等式是否有冲突
    let hasConflict = false
    if (v1 !== 0 && v2 !== 0) {
      const valid = type === '>' ? v1 > v2 : v1 < v2
      hasConflict = !valid
    }
    // 边界值冲突
    if (v1 !== 0) {
      if ((type === '>' && v1 === 1) || (type === '<' && v1 === 9)) {
        hasConflict = true
      }
    }
    if (v2 !== 0) {
      if ((type === '>' && v2 === 9) || (type === '<' && v2 === 1)) {
        hasConflict = true
      }
    }
    
    ctx.fillStyle = hasConflict ? errorColor : normalColor
    
    if (cell1.row === cell2.row) {
      // 水平
      const r = cell1.row
      const c = Math.min(cell1.col, cell2.col)
      const x = (c + 1) * cellSize
      const y = r * cellSize + cellSize / 2
      const text = cell1.col < cell2.col ? type : (type === '>' ? '<' : '>')
      ctx.fillText(text, x, y)
    } else if (cell1.col === cell2.col) {
      // 垂直
      const c = cell1.col
      const r = Math.min(cell1.row, cell2.row)
      const x = c * cellSize + cellSize / 2
      const y = (r + 1) * cellSize
      const text = cell1.row < cell2.row ? type : (type === '>' ? '<' : '>')
      
      ctx.save()
      ctx.translate(x, y)
      ctx.rotate(Math.PI / 2)
      ctx.fillText(text, 0, 0)
      ctx.restore()
    }
  })
}

// 监听主题和数据变化
watch([isDark, grid, selectedCell, selectedNumber, conflictCells, timerRunning, notes, notesMode, isVictory], () => {
  nextTick(() => renderBoard())
}, { deep: true })

// 生命周期
onMounted(() => {
  resizeCanvas()
  window.addEventListener('resize', resizeCanvas)
  window.addEventListener('keydown', handleKeydown)
})

onBeforeUnmount(() => {
  window.removeEventListener('resize', resizeCanvas)
  window.removeEventListener('keydown', handleKeydown)
  if (animationFrameId) {
    cancelAnimationFrame(animationFrameId)
  }
})

// 暴露渲染方法供外部调用
defineExpose({ renderBoard, resizeCanvas })
</script>

<template>
  <div ref="containerRef" class="board-container">
    <canvas
      ref="canvasRef"
      class="sudoku-canvas"
      @click="handleCanvasClick"
    />
  </div>
</template>

<style scoped lang="scss">
.board-container {
  width: 100%;
  display: flex;
  justify-content: center;
  padding: 8px;
}

.sudoku-canvas {
  border-radius: var(--radius-md);
  box-shadow: var(--shadow-lg);
  cursor: pointer;
  transition: box-shadow var(--transition-fast);
  
  &:hover {
    box-shadow: var(--shadow-xl);
  }
}
</style>
