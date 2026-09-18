# C-- Compiler

基于 Flex、Bison 和 C 实现的 C-- 编译器课程项目。

## 当前阶段

**Stage 01：词法分析、语法分析与语法树输出。**

程序读取一个 C-- 源文件：无词法或语法错误时按先序遍历打印语法树；有错误时输出 A 类（词法）或 B 类（语法）错误信息，并以非零状态码结束。

当前实现覆盖基础文法，并实现八/十六进制整数、指数形式浮点数及 `//`、`/* ... */` 注释支持。

## 依赖

- C 编译器：`gcc`
- 词法分析器生成器：`flex`
- 语法分析器生成器：`bison`
- 构建工具：`make`

## 构建与运行

```bash
make
./build/parser path/to/source.cmm
```

运行全部测试：

```bash
make test
```

测试样例与判定规则见 [tests/README.md](tests/README.md)。

## 模块说明

| 模块 | 主要职责 | 说明文档 |
| --- | --- | --- |
| `src/frontend/lexer.l` | 将字符流转换为 token，处理数值、注释与词法错误 | [词法分析器](docs/stage-01/lexer.md) |
| `src/frontend/syntax.y` | 按文法归约、处理优先级和语法错误恢复 | [语法分析器](docs/stage-01/syntax.md) |
| `src/frontend/tree.c`、`tree.h` | 创建、连接、打印和释放语法树 | [语法树](docs/stage-01/tree.md) |
| `src/driver/main.c` | 打开输入文件、驱动解析并决定输出与退出码 | [语法分析器中的流程说明](docs/stage-01/syntax.md#1-token-的语义值是树结点) |
