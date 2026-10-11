# 语义测试样例分类

样例目录先回答“应通过还是应报错”，再回答“测试哪个功能域”：

```text
cases/
├── valid/
│   ├── semantic_clean_output.cmm
│   └── <功能域>/
└── invalid/
    └── <功能域>/
```

两个结果目录下使用相同的功能域名称：

- `declarations/`：全局与局部声明、重复定义、初始化。
- `expressions/`：变量、赋值、算术与逻辑表达式。
- `functions/`：函数定义、返回值、参数与调用。
- `arrays/`：数组访问与下标检查。
- `control-flow/`：`if`、`while` 与嵌套作用域。
- `structures/`：结构体定义、字段与标签引用。

`valid/semantic_clean_output.cmm` 是不依赖单一功能域的无诊断冒烟样例。所有会自动执行的样例由上级的 `semantic_expected.tsv` 统一登记。
