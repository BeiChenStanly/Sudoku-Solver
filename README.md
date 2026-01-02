# Sudoku Solver & Generator using MiniSat

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.14+-green.svg)](https://cmake.org/)

基于 MiniSat SAT 求解器的数独求解和生成程序，支持多种数独变体。

## ✨ 特性

- 🚀 **高性能 SAT 求解** - 使用 MiniSat 求解器，毫秒级求解
- 🔍 **唯一性检查** - 验证解是否唯一
- 🎮 **Web 前端** - 基于 Canvas 的交互式界面
- 📦 **多种数独类型** - 支持标准、杀手、不等式和混合数独

## 支持的数独类型

1. **标准数独 (Standard Sudoku)** - 经典 9x9 数独
2. **杀手数独 (Killer Sudoku)** - 带有笼子约束，每个笼子内数字之和等于指定值且不重复
3. **不等式数独 (Inequality Sudoku)** - 相邻格子间有大于/小于约束
4. **混合数独 (Mixed Sudoku)** - 杀手数独和不等式数独的组合

## 构建项目

### 前置条件

- CMake 3.14 或更高版本
- C++17 兼容的编译器 (MSVC, GCC, Clang)
- (可选) ZLIB 用于 MiniSat 的某些功能

### 构建步骤

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 构建
cmake --build . --config Release

# 运行测试
ctest -C Release --output-on-failure
```

### Windows (Visual Studio)

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
ctest -C Release --output-on-failure
```

## 使用方法

### 命令行求解

```bash
# 从文件求解
./sudoku_solve puzzle.txt

# 从字符串求解 (81字符)
./sudoku_solve --string "530070000600195000098000060800060003400803001700020006060000280000419005000080079"

# 求解并检查解是否唯一
./sudoku_solve puzzle.txt --unique

# 显示帮助
./sudoku_solve --help
```

### 求解选项

| 选项           | 说明           |
| -------------- | -------------- |
| `--unique, -u` | 检查解是否唯一 |

### 命令行生成

```bash
# 生成混合数独（默认）
./sudoku_solve --generate

# 生成指定类型
./sudoku_solve --generate --type killer
./sudoku_solve --generate --type inequality
./sudoku_solve --generate --type mixed
./sudoku_solve --generate --type standard

# 自定义参数
./sudoku_solve --generate --type mixed --cages 10 15 --ineq 10 15

# 设置随机种子（可重现）
./sudoku_solve --generate --seed 12345

# 输出到文件
./sudoku_solve --generate --output puzzle.txt

# 同时输出解答
./sudoku_solve --generate --with-solution

# 完整示例
./sudoku_solve --generate --type mixed --cages 12 18 --ineq 10 15 --seed 42 --with-solution --output puzzle.txt
```

### 生成选项

| 选项                   | 说明                                          | 默认值 |
| ---------------------- | --------------------------------------------- | ------ |
| `--type <TYPE>`        | 谜题类型: standard, killer, inequality, mixed | mixed  |
| `--cages <MIN> <MAX>`  | 笼子数量范围                                  | 10 20  |
| `--ineq <MIN> <MAX>`   | 不等式数量范围                                | 10 20  |
| `--givens <MIN> <MAX>` | 给定值数量范围                                | 0 10   |
| `--seed <N>`           | 随机种子（用于重现）                          | 随机   |
| `--output <FILE>`      | 输出文件（默认输出到标准输出）                | stdout |
| `--with-solution`      | 包含解答                                      | 否     |
| `--fill-all`           | 笼子覆盖所有格子（用于 killer/mixed）         | 否     |
| `--no-unique`          | 不确保唯一解（生成更快）                      | 否     |

### 输入格式

#### 1. 简单格式 (标准数独)

81 个字符，使用 `.` 或 `0` 表示空格：

```
530070000600195000098000060800060003400803001700020006060000280000419005000080079
```

或者 9 行格式：

```
5 3 . . 7 . . . .
6 . . 1 9 5 . . .
. 9 8 . . . . 6 .
8 . . . 6 . . . 3
4 . . 8 . 3 . . 1
7 . . . 2 . . . 6
. 6 . . . . 2 8 .
. . . 4 1 9 . . 5
. . . . 8 . . 7 9
```

#### 2. 自定义格式 (支持所有类型)

```
GRID
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0 0

CAGES
10 0 0 0 1
15 0 2 0 3 0 4

INEQUALITIES
0 0 > 0 1
0 1 < 0 2
```

- `CAGES` 部分：`sum r1 c1 r2 c2 ...` (目标和 + 单元格坐标对)
- `INEQUALITIES` 部分：`r1 c1 > r2 c2` 或 `r1 c1 < r2 c2`
- 坐标从 0 开始

## 编程接口

```cpp
#include "SudokuSolver.h"
#include "SudokuParser.h"
#include "SudokuGenerator.h"

using namespace sudoku;

// ========== 求解谜题 ==========

// 创建求解器
SudokuSolver solver;

// 从字符串解析谜题
SudokuPuzzle puzzle = SudokuParser::parseFromString(puzzleString);

// 或者手动创建谜题
SudokuPuzzle puzzle;
puzzle.setCell(0, 0, 5);  // 设置给定值
puzzle.addCage(Cage({{0, 1}, {0, 2}}, 10));  // 添加笼子
puzzle.addInequality(InequalityConstraint(
    Cell(0, 0), Cell(0, 1), InequalityType::GREATER_THAN));

// 求解
SudokuSolution solution = solver.solve(puzzle);

if (solution.solved) {
    // 使用 solution.grid[row][col] 获取结果
    std::cout << SudokuParser::toString(solution);
}

// 验证解答
bool valid = SudokuSolver::verifySolution(puzzle, solution);

// ========== 生成谜题 ==========

// 创建生成器
SudokuGenerator generator;

// 配置生成参数
GeneratorConfig config;
config.type = SudokuType::KILLER_INEQUALITY;  // 混合数独
config.minCages = 10;
config.maxCages = 15;
config.minInequalities = 10;
config.maxInequalities = 15;
config.seed = 12345;  // 可选：设置随机种子

// 生成谜题和解答
SudokuSolution solution;
SudokuPuzzle puzzle = generator.generateWithSolution(config, solution);

// 输出为自定义格式
std::string output = generator.toCustomFormat(puzzle);
std::string outputWithSolution = generator.toCustomFormatWithSolution(puzzle, solution);
```

## 项目结构

```
Sudoku Solver/
├── CMakeLists.txt             # 主构建配置
├── README.md                  # 本文件
├── LICENSE                    # MIT 许可证
├── CONTRIBUTING.md            # 贡献指南
├── CODE_OF_CONDUCT.md         # 行为准则
├── src/
│   ├── SudokuTypes.h          # 数据类型定义
│   ├── SudokuSolver.h/cpp     # 高层求解接口
│   ├── SudokuEncoder.h/cpp    # SAT 编码器
│   ├── SudokuParser.h/cpp     # 输入解析器
│   ├── SudokuGenerator.h/cpp  # 谜题生成器
│   └── main.cpp               # 命令行入口
├── tests/
│   ├── test_main.cpp
│   ├── test_standard_sudoku.cpp
│   ├── test_killer_sudoku.cpp
│   ├── test_inequality_sudoku.cpp
│   ├── test_mixed_sudoku.cpp
│   ├── test_generator.cpp
│   └── test_uniqueness.cpp    # 唯一性测试
├── examples/
│   ├── standard_easy.txt
│   ├── standard_hard.txt
│   ├── killer_sudoku.txt
│   ├── inequality_sudoku.txt
│   └── mixed_sudoku.txt
└── minisat/                   # MiniSat SAT 求解器
```

## SAT 编码说明

### 变量定义

对于每个单元格 (r, c) 和每个可能的值 v (1-9)，定义布尔变量 x(r,c,v)，表示"单元格 (r,c) 的值为 v"。

### 基本约束

1. **每个单元格恰好一个值**：对于每个 (r,c)，恰好一个 x(r,c,v) 为真
2. **行约束**：每行每个数字恰好出现一次
3. **列约束**：每列每个数字恰好出现一次
4. **宫格约束**：每个 3x3 宫格每个数字恰好出现一次

### 杀手数独约束

1. **笼子唯一性**：笼子内每个数字最多出现一次
2. **笼子和**：笼子内数字之和等于目标值（通过枚举有效组合编码）

### 不等式约束

对于 cell1 > cell2：禁止所有 (v1, v2) 使得 v1 ≤ v2

## 🤝 贡献

欢迎贡献！请查看 [CONTRIBUTING.md](CONTRIBUTING.md) 了解详情。

1. Fork 本仓库
2. 创建您的特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交您的更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开一个 Pull Request

## 📄 许可证

本项目使用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

MiniSat 使用其原始许可证。

## 🙏 致谢

- [MiniSat](http://minisat.se/) - 高效的 SAT 求解器
- [Google Test](https://github.com/google/googletest) - C++ 测试框架
