# Stage 01 测试用例

这些用例面向实验一的词法分析、语法分析和语法树输出。目录按预期结果分类：

- `valid/`：应以状态码 0 结束并打印语法树。
- `invalid/lexical/`：应以状态码非 0 结束，至少报告表中对应的 A 类错误。
- `invalid/syntax/`：应以状态码非 0 结束，至少报告表中对应的 B 类错误。

在项目根目录执行 `make test` 可运行全部样例。它只打印失败样例的文件名和原始输出，最后打印通过数与总数。预期退出码和用于判定的输出正则保存在 `expected.tsv`。

也可先执行 `make`，再逐个运行：

```bash
./build/parser tests/valid/01-minimal-function.cmm
./build/parser tests/invalid/lexical/02-invalid-octal.cmm
```

错误信息的说明文字可以不同；验收重点是错误类型与行号。对于含多个错误的文件，恢复策略可能影响后续错误的数量，因此表格只承诺至少出现的首个目标错误。

## 合法程序

| 文件 | 覆盖点 |
| --- | --- |
| `01-minimal-function.cmm` | 最小函数、`return`、整数常量 |
| `02-global-struct.cmm` | 全局变量、`int;`、具名结构体定义与引用、成员访问 |
| `03-expressions.cmm` | 算术、关系、逻辑、一元运算、`if-else`、`while` 和优先级 |
| `04-arrays-and-calls.cmm` | 形参数组、二维数组、函数调用、数组访问 |
| `05-optional-numbers.cmm` | 八进制、十六进制与指数浮点数 |
| `06-comments.cmm` | 单行注释、多行注释和跨行行号 |

`05-optional-numbers.cmm` 的语法树中应可观察到：`0123 → INT: 83`、`0x3F → INT: 63`、`1.05e-4 → FLOAT: 0.000105`。

## 词法错误

| 文件 | 首个目标输出 |
| --- | --- |
| `01-mysterious-character.cmm` | A 类错误，第 3 行（`~`） |
| `02-invalid-octal.cmm` | A 类错误，第 3 行（`09`） |
| `03-invalid-hexadecimal.cmm` | A 类错误，第 3 行（`0x3G`） |
| `04-invalid-exponent.cmm` | A 类错误，第 3 行（`1.05e`） |
| `05-invalid-fraction.cmm` | A 类错误，第 3 行（`.7`） |
| `06-unterminated-comment.cmm` | A 类错误，第 3 行（未结束块注释） |

## 语法错误

| 文件 | 首个目标输出 |
| --- | --- |
| `01-array-comma.cmm` | B 类错误，第 4 行（数组下标中误用逗号） |
| `02-missing-semi-before-else.cmm` | B 类错误，第 4 行（`else` 前缺少分号） |
| `03-bad-parameter-list.cmm` | B 类错误，第 1 行（形参列表末尾多余逗号） |
| `04-unclosed-parenthesis.cmm` | B 类错误，第 4 行（缺少右圆括号） |
| `05-nested-comment-remainder.cmm` | B 类错误，第 5 行（非嵌套注释最早结束后的多余 `*/`） |
