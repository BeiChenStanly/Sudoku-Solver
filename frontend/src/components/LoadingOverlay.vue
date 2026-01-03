<script setup lang="ts">
defineProps<{
  message?: string
  size?: 'sm' | 'md' | 'lg'
}>()
</script>

<template>
  <div class="loading-overlay">
    <div class="loading-content">
      <div class="spinner" :class="[`spinner--${size || 'md'}`]">
        <div class="spinner-grid">
          <div v-for="i in 9" :key="i" class="spinner-cell" :style="{ animationDelay: `${(i - 1) * 0.1}s` }"></div>
        </div>
      </div>
      <p v-if="message" class="loading-message">{{ message }}</p>
    </div>
  </div>
</template>

<style scoped lang="scss">
.loading-overlay {
  position: fixed;
  inset: 0;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(4px);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
  animation: fadeIn 0.2s ease;
}

.loading-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 20px;
}

.spinner {
  &--sm {
    --cell-size: 12px;
    --gap: 3px;
  }
  
  &--md {
    --cell-size: 18px;
    --gap: 4px;
  }
  
  &--lg {
    --cell-size: 24px;
    --gap: 5px;
  }
}

.spinner-grid {
  display: grid;
  grid-template-columns: repeat(3, var(--cell-size, 18px));
  gap: var(--gap, 4px);
}

.spinner-cell {
  width: var(--cell-size, 18px);
  height: var(--cell-size, 18px);
  background: var(--primary);
  border-radius: 3px;
  animation: pulse-cell 1.2s ease-in-out infinite;
  
  &:nth-child(1) { animation-delay: 0s; }
  &:nth-child(2) { animation-delay: 0.1s; }
  &:nth-child(3) { animation-delay: 0.2s; }
  &:nth-child(4) { animation-delay: 0.3s; }
  &:nth-child(5) { animation-delay: 0.4s; }
  &:nth-child(6) { animation-delay: 0.5s; }
  &:nth-child(7) { animation-delay: 0.6s; }
  &:nth-child(8) { animation-delay: 0.7s; }
  &:nth-child(9) { animation-delay: 0.8s; }
}

@keyframes pulse-cell {
  0%, 100% {
    opacity: 0.3;
    transform: scale(0.8);
  }
  50% {
    opacity: 1;
    transform: scale(1);
  }
}

.loading-message {
  color: #fff;
  font-size: 16px;
  font-weight: 500;
  text-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
}

@keyframes fadeIn {
  from { opacity: 0; }
  to { opacity: 1; }
}
</style>
