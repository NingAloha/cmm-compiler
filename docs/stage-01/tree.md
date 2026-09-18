# 语法树：结点布局、连接方式与打印规则

对应源码：[src/frontend/tree.h](../../src/frontend/tree.h) 和 [src/frontend/tree.c](../../src/frontend/tree.c)。语法树模块不理解 C-- 的具体文法；它只提供通用的“创建结点、追加孩子、先序打印、释放内存”能力。

## 1. 结点结构

`TreeNode` 使用“第一个孩子—下一个兄弟”表示任意叉树：

```c
typedef struct TreeNode {
    char *name;
    char *text;
    int line;
    struct TreeNode *first_child;
    struct TreeNode *last_child;
    struct TreeNode *next_sibling;
} TreeNode;
```

各字段的语义如下：

| 字段 | 用途 |
| --- | --- |
| `name` | 结点类别，如 `Exp`、`ID`、`PLUS` |
| `text` | token 的附加文字；非终结符为 `NULL`，无附加文字的 token 为空串 |
| `line` | 非终结符的源代码行号，通常取产生式第一个孩子的行号 |
| `first_child` | 第一个孩子 |
| `next_sibling` | 同一父结点下的下一个孩子 |
| `last_child` | 最后一个孩子的缓存，用于高效追加 |

例如 `a + b` 的 `Exp` 结点有三个孩子：左 `Exp`、`PLUS` token、右 `Exp`；第一个孩子通过 `first_child` 找到，后两个依次通过 `next_sibling` 找到。

## 2. 创建结点与字符串所有权

`tree_new(name, text, line)` 分配一个 `TreeNode`，并通过内部的 `copy_string()` 分别复制 `name` 和 `text`。因此调用者可以传入 Flex 的 `yytext`：即使 Flex 后续重用其扫描缓冲区，树中保存的文字也不会被覆盖。

结点的三种常见创建方式决定了输出格式：

| 创建方式 | 示例 | 打印效果 |
| --- | --- | --- |
| `text == NULL` | Bison 创建的 `Exp`、`Stmt` | `Exp (line)` |
| `text == ""` | `LP`、`SEMI`、`STRUCT` | 只打印 `LP`、`SEMI`、`STRUCT` |
| `text` 为非空文字 | `ID`、`TYPE`、`INT`、`FLOAT` | `ID: name`、`INT: 42` |

这正好落实了实验要求：语法单元打印名称与行号，普通词法单元只打印名称，四类带值 token 额外打印值。

## 3. 为什么有 `last_child`

`tree_add_child(parent, child)` 的逻辑是：

1. 若父结点或孩子为空，直接返回；这使 ε 产生式的 `NULL` 可以安全传入。
2. 若当前没有孩子，让 `first_child = child`。
3. 否则执行 `last_child->next_sibling = child`。
4. 最后更新 `last_child = child`。

如果没有 `last_child`，每追加一个孩子都要从 `first_child` 沿兄弟链走到末尾。一个有 `k` 个孩子的产生式会产生 O(k²) 的连接开销；保存尾指针后，每次追加为 O(1)。虽然本实验的产生式右侧很短，这个设计依然让动作代码简单且可扩展。

## 4. 先序打印如何实现缩进

`tree_print(node, depth)` 的递归顺序为：

```text
打印当前结点
打印第一个孩子（depth + 1）
打印下一个兄弟（depth 不变）
```

也就是“根—孩子—兄弟”的先序遍历。每一层先打印两个空格，因此孩子相对父亲恰好缩进两个空格。`first_child` 和 `next_sibling` 的组合使这个过程不需要维护动态数组或额外栈。

例如 `Program` 的第一个孩子 `ExtDefList` 会以深度 1 打印；`ExtDefList` 的后续外部定义仍是其同层兄弟，深度不变。这与作业给出的树形输出一致。

## 5. 释放顺序与主程序的责任

`tree_free()` 递归释放：先释放第一个孩子子树，再释放下一个兄弟子树，最后释放本结点的 `name`、`text` 与结点本身。由于每个结点只由父结点或前一个兄弟指向一次，这个后序释放会覆盖整棵树且不重复释放。

`main.c` 无论解析成功还是失败都会调用 `tree_free(syntax_tree_root)`。成功时先打印再释放；失败时直接释放。因此错误恢复中临时构造的残缺子树也不会泄漏到一次正常运行之外。

## 6. 行号从哪里来

词法结点在 `lexer.l` 的 `make_token()` 中直接使用 `yylineno`。语法动作通常用 `$1->line` 作为新非终结符的行号，例如：

```yacc
$$ = tree_new("Stmt", NULL, $1->line);
```

这实现了“语法单元行号等于其生成的第一个词素的行号”。根结点是一个特殊情况：空输入时没有第一个孩子，于是 `Program` 回退使用当前 `yylineno`，通常为 1；非空输入则使用 `ExtDefList` 的行号。
