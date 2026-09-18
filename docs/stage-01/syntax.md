# 语法分析器：按产生式归约并构造语法树

对应源码：[src/frontend/syntax.y](../../src/frontend/syntax.y)。Bison 根据该文件生成 `build/syntax.tab.c`；生成的语法分析器在解析期间调用 Flex 提供的 `yylex()`。

## 1. token 的语义值是树结点

`%union` 只定义了一种语义值：

```c
%union { TreeNode *node; }
```

所有终结符用 `%token <node>` 声明，所有非终结符用 `%type <node>` 声明。因此每次归约的 `$$` 都是一棵子树，而 `$1`、`$2` 等是产生式右侧的子树或 token 结点。词法分析器已经为终结符创建结点；Bison 动作负责创建 `Program`、`Exp`、`Stmt` 等非终结符结点并连接孩子。

`Program` 是起始符号。它在归约 `Program → ExtDefList` 时创建根结点并写入全局 `syntax_tree_root`，供 `main.c` 在分析成功后打印。

## 2. 文法结构如何映射到实现

实现按实验文法分为几个区域：

- 顶层：`Program`、`ExtDefList`、`ExtDef`、`ExtDecList`，处理全局变量、仅有类型的声明、函数定义。
- 类型和声明：`Specifier`、`StructSpecifier`、`Tag`、`OptTag`、`VarDec`，处理基本类型、结构体、数组变量。
- 函数：`FunDec`、`VarList`、`ParamDec`，处理函数名和形参。
- 语句块与局部定义：`CompSt`、`DefList`、`Def`、`DecList`、`Dec`、`StmtList`、`Stmt`。
- 表达式：`Exp`、`Args`，处理常量、变量、调用、数组/成员访问、单目与二元运算。

以局部变量初始化为例，`Dec → VarDec ASSIGNOP Exp` 的动作依次把变量、赋值号和右值表达式加到 `Dec` 下。这让输出树保留了原始产生式的层次，而不只是一个抽象运算符树。

## 3. 空产生式为什么写成 `NULL`

`ExtDefList`、`DefList`、`StmtList` 和 `OptTag` 的空分支都令 `$$ = NULL`，而不是创建一个名为这些符号的空结点。`tree_add_child()` 收到空指针会直接返回。

这个选择直接对应实验输出要求：产生 ε 的语法单元无需打印。非空列表仍按右递归形式保留，例如 `DefList → Def DefList`；最末尾的空列表不会出现在树中。

## 4. 表达式优先级与结合性

`Exp` 使用直接左递归，歧义由 Bison 的优先级声明消除。Bison 中声明越靠后，优先级越高；当前顺序可以概括为：

| 从低到高 | 结合性 | 声明 |
| --- | --- | --- |
| 赋值 | 右结合 | `%right ASSIGNOP` |
| 逻辑或、逻辑与、关系 | 左结合 | `%left OR`、`%left AND`、`%left RELOP` |
| 加减、乘除 | 左结合 | `%left PLUS MINUS`、`%left STAR DIV` |
| 逻辑非、单目负号 | 右结合 | `%right NOT UMINUS` |
| 括号、调用、下标、成员访问 | 左结合 | `%left LP RP LB RB DOT` |

`MINUS Exp %prec UMINUS` 很关键：词法阶段只有一种 `MINUS`，但在语法阶段将前缀 `-x` 指定为 `UMINUS` 的更高优先级，从而区分 `-a * b` 与 `-(a * b)` 的归约方式。

关系运算符都使用同一 `RELOP` token，因此 `>`、`<`、`>=`、`<=`、`==`、`!=` 具有题目指定的同一优先级。

## 5. 悬挂 else 的处理

`if` 的两个产生式本身存在经典的 dangling-else 冲突：

```yacc
IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
IF LP Exp RP Stmt ELSE Stmt
```

随后用：

```yacc
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
```

令 `ELSE` 的优先级更高。结果是，当解析器既可归约无 `else` 的 `if`、又可移进 `ELSE` 时，它会选择移进，所以 `else` 总是绑定到最近的尚未匹配的 `if`。

## 6. 错误恢复策略

文法在典型同步位置插入了 `error`：

- `error SEMI`：顶层定义、局部定义、普通表达式语句；以分号同步。
- `ID LP error RP`、`LP error RP`：函数调用或括号表达式；以右括号同步。
- `VarDec LB error RB`、`Exp LB error RB`：数组定义和访问；以右方括号同步。
- `IF/WHILE LP error RP Stmt`、`LC DefList error RC`：控制语句与复合语句；以右括号或右花括号同步。

这些分支通常调用 `yyerrok`，清除 Bison 的恢复状态，以便继续报告之后不同行的错误。它们仍会创建一个不完整的内部结点，但最终不会打印：`main.c` 只有在解析返回 0、`lexical_error == 0`、`syntax_error == 0` 且根存在时才打印树。

`yyerror()` 统一输出 `Error type B at Line <line>: Syntax error.` 并设置 `syntax_error`。其中有一个有意的去重条件：若先前已经有词法错误（`lexical_error != 0`），`yyerror()` 直接返回，不再追加 B 类错误。它避免同一问题产生连锁 A/B 报告，但也意味着“一个 A 错误后再出现独立 B 错误”不会得到 B 报告；若后续实验要求这种混合错误的完整报告，需要调整这一策略。

## 7. 与构建系统的连接

Makefile 先运行 Bison 生成 `syntax.tab.c` 和 `syntax.tab.h`，再运行 Flex。`lexer.l` 包含生成的头文件以取得 token 编号和 `YYSTYPE` 定义。`syntax.y` 末尾包含生成的 `lex.yy.c`，所以最终编译命令只需要编译 `syntax.tab.c`、`tree.c` 和 `main.c`。
