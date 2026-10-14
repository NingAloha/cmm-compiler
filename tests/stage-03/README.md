# Stage 03 测试用例

这些测试覆盖 C-- 到文本 IR 的转换。目录结构沿用前两个阶段：
输入程序放在 `cases/`，预期结果通过清单登记；区别是本阶段的结果是完整
IR 文件，因此金标准输出放在 `expected/`。

## 当前测试层次

| 文件或目录 | 职责 |
| --- | --- |
| `test_ir.c` | 独立测试 IR 文本输出、临时变量和标签分配。 |
| `cases/valid/` | 已通过语义分析的 C-- 输入程序，按功能域组织。 |
| `expected/` | 与每个 C-- 输入逐字对应的预期 `.ir` 文件。 |
| `expected.tsv` | 映射 `源文件|预期 IR 文件`，供后续 IR 回归执行器读取。 |

在项目根目录运行：

```bash
make test-03
```

当前成功输出为：

```text
stage-03/ir: passed
```

当前七个样例覆盖整数立即数与运算符优先级、局部变量和赋值、函数参数
的逆序 `ARG`、短路条件与 `read/write`、一维数组的地址/解引用、`while`
循环与递归调用。
`tests/run_ir_tests.sh` 会读取 `expected.tsv`，以 `parser 源文件 目标.ir` 的
Stage 03 调用方式生成临时 IR，再与金标准逐字比较，并输出与前两阶段一致的：

```text
stage-03/cases: 7/7 passed
```
