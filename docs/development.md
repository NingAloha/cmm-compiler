# 开发与测试指南

## 提交改动前检查

在仓库根目录运行：

```sh
make check-format
make test
make test-sanitize
```

`make check-format` 使用 `clang-format` 和 `shfmt`，不会改写文件。需要自动修正时运行 `make format`，再检查差异。课程构建不依赖这两个工具。

`make test-sanitize` 需要支持 AddressSanitizer 和 UndefinedBehaviorSanitizer 的编译器及运行时，默认将产物放在 `build/sanitize/`，不覆盖普通构建。若有报告，先检查第一个错误及其源码位置。

## 添加回归测试

1. 每个用例聚焦一个语言行为或缺陷；先确认它能在修复前失败。
2. 将最小 C-- 输入加入对应阶段的 `cases/`，并在该阶段清单登记。各阶段判定规则见 [Stage 01](../tests/stage-01/README.md)、[Stage 02](../tests/stage-02/README.md) 和 [Stage 03](../tests/stage-03/README.md)。
3. 人工核对预期结果。Stage 03 的 IR 文件逐字比较，不要用待测编译器的输出直接覆盖预期文件。拒绝用例还应检查没有留下输出文件。
4. 先运行对应阶段的测试，再运行 `make test`。

能直接通过 C 接口验证的边界条件，优先放入模块单元测试。跨越解析、语义分析和 IR 生成的行为使用端到端用例。现有执行器足以支持这两类测试，不需要安装 Unity、CMocka、lit 或 FileCheck。

## 编写接口注释

沿用仓库的 LLVM 风格和两空格缩进。在公开头文件里用 Doxygen 文档块记录调用方需要知道的契约，例如资源归谁释放、返回值的含义、失败后是否会留下部分输出。实现中的注释解释原因或约束，不复述代码。
