# Contributing to Sudoku Solver

感谢您对本项目的关注！我们欢迎各种形式的贡献。

## 如何贡献

### 报告 Bug

如果您发现了 bug，请创建一个 Issue，包含以下信息：

1. **问题描述** - 简要描述您遇到的问题
2. **复现步骤** - 详细的复现步骤
3. **预期行为** - 您期望的正确行为
4. **实际行为** - 实际发生的行为
5. **环境信息** - 操作系统、编译器版本等

### 提交功能建议

如果您有新功能的想法：

1. 先搜索现有 Issues，确保没有重复
2. 创建一个新的 Issue，详细描述您的建议
3. 说明这个功能解决什么问题
4. 如果可能，提供实现思路

### 提交代码

1. **Fork 仓库** - 点击 GitHub 页面右上角的 Fork 按钮
2. **创建分支** - `git checkout -b feature/your-feature-name`
3. **编写代码** - 遵循项目的代码风格
4. **添加测试** - 为新功能添加测试用例
5. **提交更改** - `git commit -m "Add: your feature description"`
6. **推送分支** - `git push origin feature/your-feature-name`
7. **创建 PR** - 在 GitHub 上创建 Pull Request

## 代码风格

### C++ 代码规范

- 使用 C++17 标准
- 缩进使用 4 个空格
- 类名使用 PascalCase
- 函数和变量名使用 camelCase
- 常量使用 UPPER_SNAKE_CASE
- 添加适当的注释

### 示例

```cpp
/**
 * @brief 求解数独谜题
 * @param puzzle 输入的谜题
 * @param checkUniqueness 是否检查唯一性
 * @return 求解结果
 */
SudokuSolution solve(const SudokuPuzzle& puzzle, bool checkUniqueness = false);
```

## 测试

在提交 PR 之前，请确保：

1. 所有现有测试通过：
   ```bash
   cd build
   ctest -C Release --output-on-failure
   ```

2. 为新功能添加测试用例

3. 测试覆盖边界情况

## 文档

- 为公共 API 添加 Doxygen 风格的注释
- 更新 README.md 文档（如果需要）
- 添加使用示例

## 提交信息规范

提交信息应简洁明了，使用以下前缀：

- `Add:` - 添加新功能
- `Fix:` - 修复 bug
- `Update:` - 更新现有功能
- `Refactor:` - 代码重构
- `Docs:` - 文档更新
- `Test:` - 测试相关
- `Chore:` - 构建/工具相关

## 行为准则

请参阅 [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)

## 问题？

如果您有任何问题，欢迎在 Issues 中提问或通过 GitHub Discussions 讨论。

再次感谢您的贡献！
