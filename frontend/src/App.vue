<script setup lang="ts">
import SudokuBoard from "./components/SudokuBoard.vue";
</script>

<template>
  <div id="app">
    <SudokuBoard />
  </div>
</template>

<style>
* {
  box-sizing: border-box;
}

html,
body {
  margin: 0;
  padding: 0;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen,
    Ubuntu, Cantarell, sans-serif;
}

body {
  background: #fafafa;
}

@media (prefers-color-scheme: dark) {
  body {
    background: #0a0a0a;
  }
}

#app {
  width: 100%;
}
</style>
