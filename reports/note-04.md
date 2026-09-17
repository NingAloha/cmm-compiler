# Note 04：构造并输出语法树（当前工作区）

> 阶段目标：在已经能够识别完整 C-- 程序的基础上，为每一次语法规约构造语法树节点；解析成功后打印整棵树，并释放其占用的内存。

## 1. 为什么不能只判断“能否解析”

此前的 `yyparse()` 只能给出两种结果：输入符合文法，或输入不符合文法。它知道：

```c
a + b * c;
```

是合法的表达式语句，却没有把它的层次结构保存下来。

后续的语义分析需要检查变量是否声明、函数调用的参数是否匹配、表达式是否类型正确；这些工作都必须访问程序的结构。因此语法分析器除“接受或拒绝”外，还要产生一棵树：

```text
Exp
├── Exp
│   └── ID: a
├── PLUS
└── Exp
    ├── Exp
    │   └── ID: b
    ├── STAR
    └── Exp
        └── ID: c
```

这里 `b * c` 嵌套在右侧 `Exp` 中，直接记录了乘法高于加法的解析结果。

本实验的树保留了文法中的非终结符和终结符，所以更准确地说是**语法分析树**。它比通常会省略括号、分号等细节的抽象语法树（AST）更完整，也更适合与实验要求的输出逐行比对。

## 2. 树节点的数据结构

新增的 `src/frontend/tree.h` 定义了节点：

```c
typedef struct TreeNode {
    char *name;
    char *text;
    int line;
    struct TreeNode *first_child;
    struct TreeNode *next_sibling;
} TreeNode;
```

各字段的含义如下：

| 字段 | 作用 |
| --- | --- |
| `name` | 节点类型，如 `Exp`、`ID`、`PLUS` |
| `text` | token 的原始文本；非终结符为 `NULL` |
| `line` | 节点对应的源代码行号 |
| `first_child` | 第一个孩子 |
| `next_sibling` | 下一个兄弟 |

例如 token `answer` 会构成 `name = "ID"`、`text = "answer"` 的叶节点；非终结符 `Exp` 的 `text` 为 `NULL`，它的孩子则由文法右侧的符号组成。

树采用“第一个孩子 + 下一个兄弟”的表示法，而没有让节点保存可变长度数组。这样每个节点的结构固定，任意数量的孩子都能表示：

```text
ExtDef
  first_child ──→ Specifier ──→ FunDec ──→ CompSt
                  next_sibling  next_sibling
```

## 3. 创建、连接、打印和释放

`src/frontend/tree.c` 提供四个基本操作：

```c
TreeNode *tree_new(const char *name, const char *text, int line);
void tree_add_child(TreeNode *parent, TreeNode *child);
void tree_print(const TreeNode *node, int depth);
void tree_free(TreeNode *node);
```

### 3.1 创建节点时复制字符串

`tree_new()` 不直接保存传入字符串的地址，而是复制一份。例如 lexer 传来的 `yytext` 会在下一次扫描时复用；如果树节点只保存 `yytext` 指针，先前 token 的内容就可能被覆盖。复制后，每个节点都有独立且稳定的文本。

### 3.2 添加孩子保持文法顺序

`tree_add_child()` 的规则是：如果父节点还没有孩子，就设置 `first_child`；否则沿着 `next_sibling` 找到最后一个孩子，再将新节点接到末尾。

因此连续执行：

```c
tree_add_child(node, $1);
tree_add_child(node, $2);
tree_add_child(node, $3);
```

会让树中孩子的顺序与文法右侧 `$1 $2 $3` 完全一致。这一点很重要，因为语法树输出要求保留原程序的结构顺序。

### 3.3 缩进打印

`tree_print(node, depth)` 先输出 `depth` 层缩进，再递归打印当前节点的孩子，最后打印兄弟节点：

```text
打印当前节点
    ↓
递归打印 first_child，层数 + 1
    ↓
递归打印 next_sibling，层数不变
```

终结符分两类显示：

```text
ID: answer       有实际文本
INT: 42
TYPE: int
PLUS             只有 token 名称
LP
SEMI
```

非终结符则携带行号，例如 `Exp (2)`、`Stmt (5)`。

### 3.4 释放顺序

`tree_free()` 先递归释放孩子和兄弟，再释放当前节点保存的字符串和节点自身。这保证程序不再使用树后不会泄漏已分配的内存。

## 4. Flex 如何把 token 变成叶节点

`syntax.y` 使用 `%union` 声明 Bison 的语义值类型：

```yacc
%union {
    TreeNode *node;
}
```

随后每一种 token 都标记为 `<node>`：

```yacc
%token <node> INT FLOAT ID TYPE
%token <node> SEMI COMMA ASSIGNOP RELOP
```

这表示 lexer 返回 token 编号时，还要通过全局变量 `yylval` 附带一个 `TreeNode *`。

为避免每条 Flex 规则重复写同样的代码，`lexer.l` 定义了辅助函数：

```c
static int make_token(int token, const char *name, const char *text) {
    yylval.node = tree_new(name, text, yylineno);
    return token;
}
```

典型规则如下：

```lex
"int"       { return make_token(TYPE, "TYPE", yytext); }
{ID}        { return make_token(ID, "ID", yytext); }
{DIGIT}+    { return make_token(INT, "INT", yytext); }
"+"         { return make_token(PLUS, "PLUS", ""); }
```

`ID`、`INT`、`FLOAT`、`TYPE` 等有必要保留原始文本的 token 使用 `yytext`；`PLUS`、`SEMI`、`LP` 等只需显示名称的 token 使用空字符串 `""`。

## 5. Bison 归约时构造非终结符节点

声明区还需要标记所有非终结符的值也是 `<node>`：

```yacc
%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> FunDec VarList ParamDec
/* 其余非终结符同理 */
```

每条产生式在规约时执行动作。以加法为例：

```yacc
Exp:
    Exp PLUS Exp {
        $$ = tree_new("Exp", NULL, $1->line);
        tree_add_child($$, $1);
        tree_add_child($$, $2);
        tree_add_child($$, $3);
    }
    ;
```

这里：

- `$1`、`$2`、`$3` 是右侧三个符号已经构造好的节点；
- `$$` 是本次规约产生的左侧 `Exp` 节点；
- 行号采用第一个右侧符号的行号；
- 孩子严格按产生式右侧顺序加入。

单一子节点的规则也仍保留包装节点：

```yacc
VarDec:
    ID {
        $$ = tree_new("VarDec", NULL, $1->line);
        tree_add_child($$, $1);
    }
    ;
```

虽然可以直接把 `$1` 当作结果，但那样会丢失 `VarDec` 这一层文法结构，也无法得到实验要求的完整语法树。

空产生式没有实际节点，因此统一写作：

```yacc
| /* empty */ { $$ = NULL; }
```

父节点添加 `NULL` 孩子时会被忽略。这样空的 `DefList`、`StmtList`、`OptTag` 不会在输出中产生虚假的节点。

最外层 `Program` 节点除了保存整棵树外，还赋给全局变量：

```yacc
syntax_tree_root = $$;
```

这让 Bison 的归约动作与 `main.c` 之间有一个明确的交接点。

## 6. `main.c`：只在成功后输出

驱动程序声明：

```c
#include "tree.h"
extern TreeNode *syntax_tree_root;
```

解析流程变为：

```c
int result = yyparse();

if (result == 0 && syntax_tree_root != NULL) {
    tree_print(syntax_tree_root, 0);
}

tree_free(syntax_tree_root);
return result;
```

`yyparse()` 返回 `0` 表示语法分析成功。只有成功时才打印，避免语法错误的输入输出一棵不完整的树；随后无论成功或失败都尝试释放根节点，最后保持原来的退出状态码。

## 7. 构建和验证

语法树实现加入编译链接：

```make
TREE_SOURCE := src/frontend/tree.c

$(TARGET): $(PARSER_SOURCE) $(LEXER_SOURCE) $(TREE_SOURCE) $(DRIVER_SOURCE)
	$(CC) $(CFLAGS) -o $@ $(PARSER_SOURCE) $(TREE_SOURCE) $(DRIVER_SOURCE)
```

注意 `syntax.tab.c` 会包含生成的 `lex.yy.c`，所以最终链接时不应把 `lex.yy.c` 再编译一次，否则会产生重复定义。

截至本阶段，以下命令通过：

```bash
make clean && make test
./build/parser tests/parser/valid/expressions/plus_minus.cmm
```

本地 28 个有效样例均成功解析，`two_literals.cmm` 被正确拒绝；第二条命令会打印带缩进的语法树。

## 8. 当前边界

语法树已经完整记录了语法结构，但尚未进行语义检查。下列程序即使没有语法错误，也可能仍会被当前版本接受：

```c
int main() {
    unknown = 1;       // 未声明变量
    return "text";     // 本语言甚至尚未定义字符串类型
}
```

未来阶段需要结合符号表和类型系统检查这类问题。此外，词法错误发生后还应确保程序最终返回失败状态，避免词法错误输入被误判为解析成功。
