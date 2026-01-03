<script setup lang="ts">
import { ref, computed } from 'vue'
import type { PuzzleType } from '@/types/sudoku'

const props = defineProps<{
  isLoading: boolean
  wasmReady: boolean
  timerRunning: boolean
}>()

const emit = defineEmits<{
  (e: 'generate', options: GenerateOptions): void
  (e: 'solve'): void
  (e: 'reset'): void
}>()

interface GenerateOptions {
  type: PuzzleType
  minCages: number
  maxCages: number
  minInequalities: number
  maxInequalities: number
  fillComplete: boolean
  ensureUnique: boolean
  difficulty: number
}

// 难度等级
type Difficulty = 'easy' | 'medium' | 'hard' | 'expert' | 'custom'

const showOptions = ref(false)
const puzzleType = ref<PuzzleType>('mixed')
const difficulty = ref<Difficulty>('medium')
const minCages = ref(12)
const maxCages = ref(18)
const minInequalities = ref(15)
const maxInequalities = ref(25)
const fillComplete = ref(false)
const ensureUnique = ref(true)

// 难度对应的移除比例 (0-100)
const difficultyLevels = {
  easy: 30,      // 移除 30% 的约束
  medium: 50,    // 移除 50% 的约束
  hard: 70,      // 移除 70% 的约束
  expert: 85,    // 移除 85% 的约束
  custom: 50     // 自定义时的默认值
}

const difficultyOptions: Difficulty[] = ['easy', 'medium', 'hard', 'expert']
const difficultyLabels: Record<Difficulty, string> = {
  easy: '简单',
  medium: '中等',
  hard: '困难',
  expert: '专家',
  custom: '自定义'
}

const customDifficulty = ref(50)

const effectiveDifficulty = computed(() => {
  return difficulty.value === 'custom' ? customDifficulty.value : difficultyLevels[difficulty.value]
})

function handleGenerate() {
  emit('generate', {
    type: puzzleType.value,
    minCages: minCages.value,
    maxCages: maxCages.value,
    minInequalities: minInequalities.value,
    maxInequalities: maxInequalities.value,
    fillComplete: fillComplete.value,
    ensureUnique: ensureUnique.value,
    difficulty: effectiveDifficulty.value
  })
  showOptions.value = false
}
</script>

<template>
  <div class="control-panel">
    <div class="control-row">
      <div class="btn-group">
        <button
          class="btn btn--primary"
          :disabled="!wasmReady || isLoading"
          @click="handleGenerate"
        >
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M12 3v3m0 12v3M3 12h3m12 0h3M5.6 5.6l2.1 2.1m8.6 8.6l2.1 2.1M5.6 18.4l2.1-2.1m8.6-8.6l2.1-2.1" />
          </svg>
          新游戏
        </button>
        <button
          class="btn btn--icon"
          :disabled="isLoading"
          @click="showOptions = !showOptions"
          :title="showOptions ? '收起选项' : '展开选项'"
        >
          <svg 
            width="16" 
            height="16" 
            viewBox="0 0 24 24" 
            fill="none" 
            stroke="currentColor" 
            stroke-width="2"
            :style="{ transform: showOptions ? 'rotate(180deg)' : 'none' }"
          >
            <polyline points="6 9 12 15 18 9" />
          </svg>
        </button>
      </div>
      
      <button
        class="btn btn--secondary"
        :disabled="!wasmReady || isLoading || !timerRunning"
        @click="$emit('solve')"
      >
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M9.5 2A2.5 2.5 0 0 1 12 4.5v15a2.5 2.5 0 0 1-4.96.44 2.5 2.5 0 0 1-2.96-3.08 3 3 0 0 1-.34-5.58 2.5 2.5 0 0 1 1.32-4.24 2.5 2.5 0 0 1 4.44-1.54" />
          <path d="M14.5 2A2.5 2.5 0 0 0 12 4.5v15a2.5 2.5 0 0 0 4.96.44 2.5 2.5 0 0 0 2.96-3.08 3 3 0 0 0 .34-5.58 2.5 2.5 0 0 0-1.32-4.24 2.5 2.5 0 0 0-4.44-1.54" />
        </svg>
        求解
      </button>
      
      <button
        class="btn btn--ghost"
        :disabled="isLoading || !timerRunning"
        @click="$emit('reset')"
      >
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8" />
          <path d="M3 3v5h5" />
        </svg>
        重置
      </button>
    </div>
    
    <Transition name="slide">
      <div v-if="showOptions" class="options-panel">
        <!-- 提示信息 -->
        <div class="options-hint">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <circle cx="12" cy="12" r="10"/>
            <path d="M12 16v-4M12 8h.01"/>
          </svg>
          <span>以下为初始约束设置。生成引擎会基于难度自动减少约束数量，以创建可玩谜题。</span>
        </div>
        
        <!-- 难度选择 -->
        <div class="option-row">
          <label class="option-label">难度等级</label>
          <div class="difficulty-selector">
            <button 
              v-for="d in difficultyOptions"
              :key="d"
              :class="['diff-btn', { active: difficulty === d }]"
              @click="difficulty = d"
            >
              {{ difficultyLabels[d] }}
            </button>
            <button 
              :class="['diff-btn', { active: difficulty === 'custom' }]"
              @click="difficulty = 'custom'"
            >
              自定义
            </button>
          </div>
        </div>
        
        <!-- 自定义难度滑块 -->
        <div v-if="difficulty === 'custom'" class="option-row">
          <label class="option-label">约束移除比例</label>
          <div class="slider-container">
            <input 
              type="range" 
              v-model.number="customDifficulty" 
              min="10" 
              max="95" 
              class="slider"
            />
            <span class="slider-value">{{ customDifficulty }}%</span>
          </div>
          <div class="slider-hint">更高 = 更难（移除更多约束）</div>
        </div>
        
        <div class="option-row">
          <label class="option-label">谜题类型</label>
          <select v-model="puzzleType" class="select">
            <option value="standard">标准数独</option>
            <option value="killer">杀手数独</option>
            <option value="inequality">不等式数独</option>
            <option value="mixed">混合模式</option>
          </select>
        </div>
        
        <div v-if="puzzleType === 'killer' || puzzleType === 'mixed'" class="option-row">
          <label class="option-label">初始笼子数量</label>
          <div class="range-input">
            <input type="number" v-model.number="minCages" min="0" max="30" class="input input--sm" />
            <span class="range-sep">~</span>
            <input type="number" v-model.number="maxCages" min="0" max="30" class="input input--sm" />
          </div>
        </div>
        
        <div v-if="puzzleType === 'inequality' || puzzleType === 'mixed'" class="option-row">
          <label class="option-label">初始不等式数量</label>
          <div class="range-input">
            <input type="number" v-model.number="minInequalities" min="0" max="40" class="input input--sm" />
            <span class="range-sep">~</span>
            <input type="number" v-model.number="maxInequalities" min="0" max="40" class="input input--sm" />
          </div>
        </div>
        
        <div class="option-row option-row--checkbox">
          <label class="checkbox-label">
            <input type="checkbox" v-model="fillComplete" />
            <span>填满（无空格）</span>
          </label>
        </div>
        
        <div class="option-row option-row--checkbox">
          <label class="checkbox-label">
            <input type="checkbox" v-model="ensureUnique" />
            <span>保证唯一解</span>
          </label>
        </div>
        
        <button 
          class="btn btn--accent btn--full" 
          :disabled="isLoading"
          @click="handleGenerate"
        >
          应用并生成
        </button>
      </div>
    </Transition>
  </div>
</template>

<style scoped lang="scss">
.control-panel {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.control-row {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.btn-group {
  display: flex;
  
  .btn:first-child {
    border-top-right-radius: 0;
    border-bottom-right-radius: 0;
  }
  
  .btn:last-child {
    border-top-left-radius: 0;
    border-bottom-left-radius: 0;
    margin-left: -1px;
  }
}

.btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 10px 16px;
  font-size: 13px;
  font-weight: 600;
  border-radius: var(--radius-md);
  border: 1px solid var(--border-light);
  cursor: pointer;
  transition: all var(--transition-fast);
  white-space: nowrap;
  
  svg {
    transition: transform var(--transition-fast);
  }
  
  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
  
  &--primary {
    background: var(--primary);
    color: var(--text-inverse);
    border-color: var(--primary);
    
    &:hover:not(:disabled) {
      background: var(--primary-hover);
    }
  }
  
  &--secondary {
    background: var(--bg-secondary);
    color: var(--text-primary);
    
    &:hover:not(:disabled) {
      background: var(--bg-tertiary);
    }
  }
  
  &--ghost {
    background: transparent;
    color: var(--text-secondary);
    border-color: transparent;
    
    &:hover:not(:disabled) {
      background: var(--bg-tertiary);
      color: var(--text-primary);
    }
  }
  
  &--accent {
    background: var(--accent);
    color: var(--text-inverse);
    border-color: var(--accent);
    
    &:hover:not(:disabled) {
      background: var(--accent-hover);
    }
  }
  
  &--icon {
    padding: 10px;
    min-width: 40px;
  }
  
  &--full {
    width: 100%;
    margin-top: 8px;
  }
}

.options-panel {
  padding: 16px;
  background: var(--bg-tertiary);
  border-radius: var(--radius-lg);
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.options-hint {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  padding: 10px 12px;
  background: var(--primary);
  background: linear-gradient(135deg, rgba(var(--primary-rgb), 0.1), rgba(var(--accent-rgb), 0.1));
  border-radius: var(--radius-md);
  font-size: 12px;
  color: var(--text-secondary);
  line-height: 1.5;
  
  svg {
    flex-shrink: 0;
    margin-top: 1px;
    color: var(--primary);
  }
}

.difficulty-selector {
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}

.diff-btn {
  padding: 6px 12px;
  font-size: 12px;
  font-weight: 500;
  border: 1px solid var(--border-light);
  border-radius: var(--radius-sm);
  background: var(--bg-secondary);
  color: var(--text-secondary);
  cursor: pointer;
  transition: all var(--transition-fast);
  
  &:hover {
    background: var(--bg-tertiary);
    color: var(--text-primary);
  }
  
  &.active {
    background: var(--primary);
    color: var(--text-inverse);
    border-color: var(--primary);
  }
}

.slider-container {
  display: flex;
  align-items: center;
  gap: 12px;
  flex: 1;
  max-width: 200px;
}

.slider {
  flex: 1;
  height: 6px;
  border-radius: 3px;
  background: var(--bg-secondary);
  appearance: none;
  cursor: pointer;
  
  &::-webkit-slider-thumb {
    appearance: none;
    width: 16px;
    height: 16px;
    border-radius: 50%;
    background: var(--primary);
    cursor: pointer;
    transition: transform var(--transition-fast);
    
    &:hover {
      transform: scale(1.1);
    }
  }
  
  &::-moz-range-thumb {
    width: 16px;
    height: 16px;
    border: none;
    border-radius: 50%;
    background: var(--primary);
    cursor: pointer;
  }
}

.slider-value {
  font-size: 12px;
  font-weight: 600;
  color: var(--primary);
  min-width: 40px;
  text-align: right;
}

.slider-hint {
  font-size: 11px;
  color: var(--text-tertiary);
  margin-top: -4px;
}

.option-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  flex-wrap: wrap;
  
  &--checkbox {
    justify-content: flex-start;
  }
}

.option-label {
  font-size: 13px;
  font-weight: 500;
  color: var(--text-secondary);
  white-space: nowrap;
}

.select {
  flex: 1;
  max-width: 160px;
  padding: 8px 32px 8px 12px;
  font-size: 13px;
  border-radius: var(--radius-md);
  border: 1px solid var(--border-light);
  background: var(--bg-secondary);
  color: var(--text-primary);
  cursor: pointer;
  appearance: none;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' fill='none' viewBox='0 0 24 24' stroke='%239ca3af'%3E%3Cpath stroke-linecap='round' stroke-linejoin='round' stroke-width='2' d='M19 9l-7 7-7-7'%3E%3C/path%3E%3C/svg%3E");
  background-repeat: no-repeat;
  background-position: right 8px center;
  background-size: 14px;
}

.range-input {
  display: flex;
  align-items: center;
  gap: 8px;
}

.range-sep {
  color: var(--text-tertiary);
}

.input {
  padding: 8px 10px;
  font-size: 13px;
  border-radius: var(--radius-md);
  border: 1px solid var(--border-light);
  background: var(--bg-secondary);
  color: var(--text-primary);
  
  &--sm {
    width: 60px;
    text-align: center;
  }
}

.checkbox-label {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  font-size: 13px;
  color: var(--text-primary);
  
  input[type="checkbox"] {
    width: 16px;
    height: 16px;
    accent-color: var(--primary);
    cursor: pointer;
  }
}

// 过渡动画
.slide-enter-active,
.slide-leave-active {
  transition: all var(--transition-normal);
  overflow: hidden;
}

.slide-enter-from,
.slide-leave-to {
  opacity: 0;
  max-height: 0;
  padding-top: 0;
  padding-bottom: 0;
}

.slide-enter-to,
.slide-leave-from {
  max-height: 400px;
}
</style>
