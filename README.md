# C-- Compiler

基于 Flex、Bison 和 C 实现的 C-- 编译器课程项目。

## 当前阶段

**Stage 02（进行中）：已完成类型系统、符号表和必做语义检查，并已接入解析器。**

默认调用 `parser` 执行实验二语义检查：正确程序不输出，错误程序只输出对应的 `Error type` 信息。传入 `--tree` 时保留实验一行为，按先序遍历打印语法树。

当前实现覆盖基础文法，并实现八/十六进制整数、指数形式浮点数及 `//`、`/* ... */` 注释支持。

## 依赖

- C 编译器：`gcc`
- 词法分析器生成器：`flex`
- 语法分析器生成器：`bison`
- 构建工具：`make`

## 构建与运行

```bash
make
./parser path/to/source.cmm
./parser --tree path/to/source.cmm
```

运行测试：

```bash
make test-01  # 实验一：词法、语法与语法树测试
make test-02  # 实验二：类型、符号表与语义检查测试
make test     # 运行全部阶段测试
```

测试样例与判定规则见[实验一测试说明](tests/stage-01/README.md)和[实验二测试说明](tests/stage-02/README.md)。

## 模块说明

| 模块 | 主要职责 | 说明文档 |
| --- | --- | --- |
| `src/frontend/lexer.l` | 将字符流转换为 token，处理数值、注释与词法错误 | [词法分析器](docs/stage-01/lexer.md) |
| `src/frontend/syntax.y` | 按文法归约、处理优先级和语法错误恢复 | [语法分析器](docs/stage-01/syntax.md) |
| `src/frontend/tree.c`、`tree.h` | 创建、连接、打印和释放语法树 | [语法树](docs/stage-01/tree.md) |
| `src/driver/main.c` | 打开输入文件、驱动解析并决定输出与退出码 | [语法分析器中的流程说明](docs/stage-01/syntax.md#1-token-的语义值是树结点) |
| `src/semantic/type.c`、`type.h` | 表示基础类型、数组、结构体和函数类型，并判断类型等价 | [实验二测试](tests/stage-02/README.md) |
| `src/semantic/symbol.c`、`symbol.h` | 维护全局符号表，支持插入、查找、去重和清空 | [实验二测试](tests/stage-02/README.md) |
