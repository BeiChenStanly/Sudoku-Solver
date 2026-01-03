# 🧩 Sudoku Solver & Generator

[![Deploy to GitHub Pages](https://github.com/BeiChenStanly/Sudoku-Solver/actions/workflows/deploy.yml/badge.svg)](https://github.com/BeiChenStanly/Sudoku-Solver/actions/workflows/deploy.yml)
[![Tests](https://github.com/BeiChenStanly/Sudoku-Solver/actions/workflows/test.yml/badge.svg)](https://github.com/BeiChenStanly/Sudoku-Solver/actions/workflows/test.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

基于 MiniSat SAT 求解器的数独求解和生成程序，提供 Web 应用、桌面应用和命令行工具。

## ✨ 功能特点

- 🎮 **多种数独类型**：标准数独、杀手数独、不等式数独、混合数独
- ⚡ **SAT 求解器**：基于 MiniSat，快速求解任意难度谜题
- 🎯 **智能难度系统**：5 级难度，从新手到专家
- 📝 **笔记功能**：候选数字标记，辅助推理
- 🎨 **现代化 UI**：响应式设计，明暗主题切换
- ⏱️ **游戏计时**：记录解题时间，页面不可见时自动暂停
- 🖥️ **跨平台**：Web 应用 + 桌面应用 (Windows/macOS/Linux)

## 🚀 在线体验

访问 [**GitHub Pages**](https://beichenstanly.github.io/Sudoku-Solver/) 立即开始游戏！

## 📥 桌面应用下载

在 [Releases](https://github.com/BeiChenStanly/Sudoku-Solver/releases) 页面下载适合您系统的版本：

| 平台 | 格式 |
|------|------|
| Windows | MSI / NSIS 安装包 |
| macOS (Intel) | DMG |
| macOS (Apple Silicon) | DMG |
| Linux | DEB / AppImage |

## 🎮 数独类型说明

### 标准数独 (Standard Sudoku)

经典 9x9 数独，遵循基本规则：
- 每行包含 1-9 各一次
- 每列包含 1-9 各一次  
- 每个 3x3 宫格包含 1-9 各一次

### 杀手数独 (Killer Sudoku)

在标准规则基础上增加笼子约束：
- 标有数字的虚线区域称为"笼子"
- 笼子内的数字之和必须等于标注的数值
- 笼子内的数字不能重复

### 不等式数独 (Inequality Sudoku)

在标准规则基础上增加大小约束：
- 相邻单元格之间有 `<` 或 `>` 符号
- 符号指示两个格子的大小关系

### 混合数独 (Mixed Sudoku)

结合杀手数独和不等式数独的所有规则，是最具挑战性的变体。

## 🎯 难度系统

| 难度等级 | 约束移除比例 | 适合人群 |
|---------|------------|---------|
| 新手 (Beginner) | 45% | 初学者 |
| 简单 (Easy) | 55% | 休闲玩家 |
| 中等 (Medium) | 65% | 有经验玩家 |
| 困难 (Hard) | 75% | 高手 |
| 专家 (Expert) | 85% | 挑战极限 |

难度越高，移除的约束越多，需要更多的逻辑推理。

## 📝 游戏操作

### 基本操作

- **选择单元格**：点击棋盘上的格子
- **填入数字**：点击数字键盘或按键盘 1-9
- **清除数字**：点击删除按钮或按 Delete/Backspace/0
- **方向键**：使用箭头键移动选择

### 笔记功能

1. 点击"笔记关闭"按钮开启笔记模式
2. 在笔记模式下点击数字，会在当前格子标记/取消候选数字
3. 填入正式数字时会自动清除相关笔记

### 快捷键

| 按键 | 功能 |
|------|------|
| `1-9` | 填入数字 |
| `0` / `Delete` / `Backspace` | 清除单元格 |
| `↑↓←→` | 移动选择 |

## 🛠️ 本地开发

### 前置条件

- [Node.js](https://nodejs.org/) 18+
- [CMake](https://cmake.org/) 3.14+
- [Emscripten](https://emscripten.org/) (用于 WASM 构建)
- C++17 兼容编译器

### 安装依赖

```bash
# 克隆仓库
git clone https://github.com/BeiChenStanly/Sudoku-Solver.git
cd Sudoku-Solver

# 安装 Emscripten (如果尚未安装)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ..
```

### 构建 WebAssembly

```bash
# 确保 MiniSat 已设置
cp System.cc minisat/minisat/utils/System.cc
cp SolverTypes.h minisat/minisat/core/SolverTypes.h
cp Options.h minisat/minisat/utils/Options.h

# 构建 WASM
mkdir -p build-wasm && cd build-wasm
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake cmake --build . --config Release
cd ..

# 复制 WASM 文件到前端
mkdir -p frontend/public/wasm
cp build-wasm/sudoku_wasm.js frontend/public/wasm/
cp build-wasm/sudoku_wasm.wasm frontend/public/wasm/
```

### 运行开发服务器

```bash
cd frontend
npm install
npm run dev
```

访问 http://localhost:1420 查看应用。

### 构建桌面应用 (Tauri)

```bash
# 安装 Rust (如果尚未安装)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# 构建桌面应用
cd frontend
npm run tauri build
```

构建产物位于 `frontend/src-tauri/target/release/bundle/`。

## 📦 命令行工具

### 求解谜题

```bash
# 从文件求解
./sudoku_solve puzzle.txt

# 从字符串求解 (81字符)
./sudoku_solve --string "530070000600195000098000060800060003400803001700020006060000280000419005000080079"
```

### 生成谜题

```bash
# 生成混合数独（默认）
./sudoku_solve --generate

# 生成指定类型
./sudoku_solve --generate --type killer
./sudoku_solve --generate --type inequality
./sudoku_solve --generate --type mixed

# 自定义参数
./sudoku_solve --generate --type mixed --cages 10 15 --ineq 10 15 --seed 42

# 输出到文件并包含解答
./sudoku_solve --generate --output puzzle.txt --with-solution
```

### 生成选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `--type <TYPE>` | 谜题类型: standard, killer, inequality, mixed | mixed |
| `--cages <MIN> <MAX>` | 笼子数量范围 | 10 20 |
| `--ineq <MIN> <MAX>` | 不等式数量范围 | 10 20 |
| `--givens <MIN> <MAX>` | 给定值数量范围 | 0 10 |
| `--seed <N>` | 随机种子（用于重现） | 随机 |
| `--output <FILE>` | 输出文件 | stdout |
| `--with-solution` | 包含解答 | 否 |

## 📁 输入格式

### 简单格式 (标准数独)

81 个字符，使用 `.` 或 `0` 表示空格：

```
530070000600195000098000060800060003400803001700020006060000280000419005000080079
```

### 自定义格式 (支持所有类型)

```
GRID
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0
...

CAGES
10 0 0 0 1
15 0 2 0 3 0 4

INEQUALITIES
0 0 > 0 1
0 1 < 0 2
```

## 🏗️ 项目结构

```
Sudoku-Solver/
├── src/                    # C++ 核心求解器
│   ├── SudokuSolver.*      # 求解接口
│   ├── SudokuEncoder.*     # SAT 编码器
│   ├── SudokuParser.*      # 输入解析器
│   ├── SudokuGenerator.*   # 谜题生成器
│   └── wasm_bindings.cpp   # WebAssembly 绑定
├── frontend/               # Vue 3 前端
│   ├── src/
│   │   ├── components/     # UI 组件
│   │   ├── composables/    # 组合式函数
│   │   ├── services/       # WASM 服务
│   │   └── types/          # TypeScript 类型
│   └── src-tauri/          # Tauri 桌面应用配置
├── minisat/                # MiniSat SAT 求解器
├── tests/                  # C++ 测试
├── examples/               # 示例谜题
└── .github/workflows/      # CI/CD 配置
```

## 🔬 技术实现

### SAT 编码

使用布尔可满足性问题 (SAT) 对数独进行编码：

- **变量**：对于每个单元格 (r, c) 和值 v，定义变量 x(r,c,v)
- **基本约束**：每格恰好一个值，行/列/宫格唯一性
- **杀手约束**：枚举有效数字组合，编码笼子和与唯一性
- **不等式约束**：禁止违反大小关系的值对

### 前端架构

- **Vue 3** + **Composition API**：响应式状态管理
- **Canvas 渲染**：高性能棋盘绘制
- **Web Worker**：后台运行 WASM 求解器，保持 UI 流畅
- **Tauri 2.0**：构建轻量级跨平台桌面应用

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！请查看 [CONTRIBUTING.md](CONTRIBUTING.md) 了解贡献指南。

## 📄 许可证

本项目使用 [MIT 许可证](LICENSE)。MiniSat 使用其原始许可证。

## 🙏 致谢

- [MiniSat](http://minisat.se/) - 高效的 SAT 求解器
- [Vue.js](https://vuejs.org/) - 渐进式 JavaScript 框架
- [Tauri](https://tauri.app/) - 构建桌面应用的工具包
- [Emscripten](https://emscripten.org/) - C++ 到 WebAssembly 编译器
