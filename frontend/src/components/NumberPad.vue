<script setup lang="ts">
import { computed } from 'vue'
import { useSudokuGame } from '@/composables/useSudokuGame'

const { selectedNumber, timerRunning, grid, GRID_SIZE, notesMode, toggleNotesMode } = useSudokuGame()

const emit = defineEmits<{
  (e: 'select', num: number | null | undefined): void
}>()

// 计算每个数字剩余数量
const numberCounts = computed(() => {
  const counts: Record<number, number> = {}
  for (let n = 1; n <= 9; n++) {
    counts[n] = 9
  }
  
  for (let r = 0; r < GRID_SIZE; r++) {
    for (let c = 0; c < GRID_SIZE; c++) {
      const val = grid.value[r]?.[c]
      if (val && val > 0 && counts[val] !== undefined) {
        counts[val]!--
      }
    }
  }
  
  return counts
})

function handleClick(num: number) {
  if (!timerRunning.value) return
  
  if (num === selectedNumber.value) {
    selectedNumber.value = undefined // 取消选中
  } else {
    selectedNumber.value = num
  }
  
  emit('select', selectedNumber.value)
}

function handleClearClick() {
  if (!timerRunning.value) return
  
  // 和数字按钮一样的行为：点击切换选中状态
  if (selectedNumber.value === null) {
    selectedNumber.value = undefined // 取消选中
  } else {
    selectedNumber.value = null // 选中删除模式
  }
  
  emit('select', selectedNumber.value)
}
</script>

<template>
  <div class="numpad-wrapper">
    <!-- 笔记模式切换 -->
    <button
      class="notes-toggle"
      :class="{ 'notes-toggle--active': notesMode }"
      :disabled="!timerRunning"
      @click="toggleNotesMode"
    >
      <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
        <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7" />
        <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z" />
      </svg>
      <span>{{ notesMode ? '笔记开启' : '笔记关闭' }}</span>
    </button>
    
    <div class="numpad">
      <button
        v-for="n in 9"
        :key="n"
        class="numpad-btn"
        :class="{ 
          'numpad-btn--active': selectedNumber === n,
          'numpad-btn--complete': (numberCounts[n] ?? 0) === 0,
          'numpad-btn--notes': notesMode
        }"
        :disabled="!timerRunning || (numberCounts[n] ?? 0) === 0"
        @click="handleClick(n)"
      >
        <span class="numpad-value">{{ n }}</span>
        <span v-if="(numberCounts[n] ?? 0) > 0" class="numpad-count">{{ numberCounts[n] }}</span>
      </button>
      
      <button
        class="numpad-btn numpad-btn--clear"
        :class="{ 'numpad-btn--active': selectedNumber === null }"
        :disabled="!timerRunning"
        @click="handleClearClick"
      >
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
          <path d="M18 6L6 18M6 6l12 12" />
        </svg>
      </button>
    </div>
  </div>
</template>

<style scoped lang="scss">
.numpad-wrapper {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.notes-toggle {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 10px 16px;
  border: 2px solid var(--border-light);
  border-radius: var(--radius-md);
  background: var(--bg-secondary);
  color: var(--text-secondary);
  font-weight: 600;
  font-size: 14px;
  cursor: pointer;
  transition: all var(--transition-fast);
  
  &:hover:not(:disabled) {
    border-color: var(--warning);
    background: var(--bg-tertiary);
    color: var(--warning);
  }
  
  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }
  
  &--active {
    border-color: var(--warning);
    background: var(--warning);
    color: var(--text-inverse);
    
    &:hover:not(:disabled) {
      background: var(--warning-hover, #d97706);
      color: var(--text-inverse);
    }
  }
}

.numpad {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 8px;
  
  @media (max-width: 640px) {
    grid-template-columns: repeat(5, 1fr);
    gap: 6px;
  }
}

.numpad-btn {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 2px;
  min-width: 48px;
  min-height: 56px;
  padding: 8px;
  border: 2px solid var(--border-light);
  border-radius: var(--radius-md);
  background: var(--bg-secondary);
  color: var(--text-primary);
  font-weight: 600;
  cursor: pointer;
  transition: all var(--transition-fast);
  
  &:hover:not(:disabled) {
    border-color: var(--primary);
    background: var(--bg-tertiary);
    transform: translateY(-2px);
    box-shadow: var(--shadow-md);
  }
  
  &:active:not(:disabled) {
    transform: translateY(0);
  }
  
  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }
  
  &--active {
    border-color: var(--primary);
    background: var(--primary);
    color: var(--text-inverse);
    box-shadow: var(--shadow-md);
    
    &:hover:not(:disabled) {
      background: var(--primary-hover);
    }
    
    .numpad-count {
      background: rgba(255, 255, 255, 0.3);
      color: var(--text-inverse);
    }
  }
  
  &--notes:not(&--active):not(:disabled) {
    border-style: dashed;
    border-color: var(--warning);
    
    &:hover {
      border-color: var(--warning);
      background: rgba(245, 158, 11, 0.1);
    }
  }
  
  &--complete {
    opacity: 0.35;
    border-style: dashed;
  }
  
  &--clear {
    background: var(--bg-tertiary);
    
    &:hover:not(:disabled) {
      background: var(--error);
      border-color: var(--error);
      color: var(--text-inverse);
    }
    
    &.numpad-btn--active {
      background: var(--error);
      border-color: var(--error);
      color: var(--text-inverse);
    }
  }
  
  @media (max-width: 640px) {
    min-width: 40px;
    min-height: 48px;
    padding: 6px;
  }
}

.numpad-value {
  font-size: 20px;
  line-height: 1;
  
  @media (max-width: 640px) {
    font-size: 18px;
  }
}

.numpad-count {
  font-size: 10px;
  font-weight: 500;
  padding: 1px 5px;
  border-radius: var(--radius-full);
  background: var(--bg-tertiary);
  color: var(--text-tertiary);
  
  @media (max-width: 640px) {
    font-size: 9px;
    padding: 1px 4px;
  }
}
</style>
