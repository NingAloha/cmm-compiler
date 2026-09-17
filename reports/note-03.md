# Note 03：从表达式到完整程序结构（截至 `86e5362`）

> 对应提交：`86e536251ba2ec6247e938fd74e78b5ed535805b`  
> 阶段目标：把只能解析单个表达式的分析器，扩展为能够识别 C-- 程序、全局定义、函数、局部定义、语句和结构体的语法分析器。

## 1. 阶段变化：根节点不再是 `Exp`

Note 02 的根规则是：

```yacc
Program:
    Exp
    ;
```

因此输入只能是单独一条表达式，例如：

```c
a + b * c
```

真实的 C-- 源文件最外层应该是全局变量、结构体定义或函数定义，而不是裸表达式。本阶段将根规则改为：

```yacc
Program:
    ExtDefList
    ;

ExtDefList:
    ExtDef ExtDefList
    | /* empty */
    ;
```

`ExtDef` 是 external definition（外部定义）。右递归加空产生式表示：一个程序由零个或多个顶层定义组成。

```text
Program
└── ExtDefList
    ├── ExtDef        第一个顶层定义
    └── ExtDefList    剩余顶层定义；最终可以为空
```

## 2. 词法器的职责变化

为了让 Bison 能解析语句、定义和函数，`lexer.l` 不再只为表达式 token 执行 `return`，而是把所有当前文法需要的关键字和界符返回给 Bison：

```lex
"int"    { return TYPE; }
"float"  { return TYPE; }
"struct" { return STRUCT; }
"return" { return RETURN; }
"if"     { return IF; }
"else"   { return ELSE; }
"while"  { return WHILE; }

";" { return SEMI; }
"{" { return LC; }
"}" { return RC; }
```

此时 `TYPE` 只表示“这里是类型关键字”；`int` 或 `float` 的具体文本仍保留在 `yytext` 中，后续语义分析阶段会再使用它。

## 3. 三类顶层定义

```yacc
ExtDef:
    Specifier ExtDecList SEMI
    | Specifier SEMI
    | Specifier FunDec CompSt
    ;
```

三条候选分别对应：

| 形式 | 示例 |
| --- | --- |
| 全局变量定义 | `int count, values[10];` |
| 独立类型定义 | `struct Point { int x; };` |
| 函数定义 | `int main() { return 0; }` |

全局变量列表为：

```yacc
ExtDecList:
    VarDec
    | VarDec COMMA ExtDecList
    ;
```

注意：截至该提交，`ExtDecList` 不包含 `ASSIGNOP Exp`，所以允许 `int count;`，但暂不支持 `int count = 1;` 形式的全局初始化。这与局部定义的规则不同。

## 4. 函数和形参

```yacc
FunDec:
    ID LP VarList RP
    | ID LP RP
    ;

VarList:
    ParamDec COMMA VarList
    | ParamDec
    ;

ParamDec:
    Specifier VarDec
    ;
```

对应关系：

```c
int add(int left, int right) {
    return left + right;
}
```

```text
int                 → Specifier
add(int left, ...)  → FunDec
int left            → ParamDec
left                → VarDec
{ ... }             → CompSt
```

形参的类型和变量形状复用 `Specifier`、`VarDec`，避免为函数形参另写一套规则。

## 5. 语句块、局部定义和语句

函数体或嵌套块由 `CompSt` 表示：

```yacc
CompSt:
    LC DefList StmtList RC
    ;
```

它把块内内容分为两个区域：

```text
{
    DefList    局部定义，例如 int x = 1;
    StmtList   语句，例如 return x;
}
```

这个顺序来自实验文法：定义必须出现在语句之前。

### 5.1 定义相关规则

```yacc
DefList:
    Def DefList
    | /* empty */
    ;

Def:
    Specifier DecList SEMI
    ;

DecList:
    Dec
    | Dec COMMA DecList
    ;

Dec:
    VarDec
    | VarDec ASSIGNOP Exp
    ;
```

因此局部定义支持：

```c
int x = 1, y;
float ratio = 3.14;
```

`VarDec` 负责变量本身的形状：

```yacc
VarDec:
    ID
    | VarDec LB INT RB
    ;
```

它可递归识别：

```c
int values[10];
float matrix[3][4];
```

### 5.2 语句相关规则

```yacc
Stmt:
    Exp SEMI
    | RETURN Exp SEMI
    | CompSt
    | WHILE LP Exp RP Stmt
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    | IF LP Exp RP Stmt ELSE Stmt
    ;
```

最后位置使用 `Stmt`，而不是固定写成某一种语句，因此循环体和分支体既可以是一条普通语句，也可以是嵌套块：

```c
while (count) {
    count = count - 1;
}
```

## 6. `if-else` 的悬挂 else 问题

嵌套条件语句：

```c
if (rain)
    if (umbrella)
        go_out;
    else
        stay_home;
```

其中 `else` 应当属于最近的、尚未配对的内层 `if (umbrella)`。为明确这个规则，文法声明：

```yacc
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
```

后声明的 `ELSE` 优先级更高。无 `else` 的 `if` 规则再用：

```yacc
IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
```

给自己指定一个低于 `ELSE` 的优先级。于是 Bison 读到 `else` 时会继续匹配它，而不是过早结束内层 `if`。`LOWER_THAN_ELSE` 是仅供优先级使用的虚构名称，lexer 不会返回它。

## 7. 结构体类型

```yacc
Specifier:
    TYPE
    | STRUCT OptTag LC DefList RC
    | STRUCT Tag
    ;

Tag:
    ID
    ;

OptTag:
    ID
    | /* empty */
    ;
```

支持三种形状：

```c
struct Point { int x; } point;  // 具名结构体，并声明变量
struct { int x; } point;        // 匿名结构体
struct Point point;             // 引用已有名称
```

文法只判断形式正确。例如 `struct Point point;` 中 `Point` 是否真的定义过，是未来语义分析和符号表的责任。

## 8. 测试为什么要迁移到函数中

根规则从 `Program: Stmt` 变成 `Program: ExtDefList` 后，原先的测试文本：

```c
a + b;
```

不再是完整程序。测试被机械地包进最小函数：

```c
int main() {
    a + b;
}
```

无效样例也同样包裹，以确保它失败的原因仍是内部语法错误，而非缺失函数外层结构。

测试目录按语言构造分类：

```text
tests/parser/valid/expressions/
tests/parser/valid/statements/
tests/parser/valid/definitions/
tests/parser/valid/functions/
tests/parser/valid/programs/
```

Makefile 使用递归 `find` 搜集所有 `.cmm` 文件：

```make
VALID_TESTS := $(sort $(shell find tests/parser/valid -type f -name '*.cmm'))
INVALID_TESTS := $(sort $(shell find tests/parser/invalid -type f -name '*.cmm'))
```

因此新增分类目录后不需要再修改 Makefile。

## 9. 阶段边界

截至 `86e5362`，分析器已能接受实验文法规定的主要程序结构，但依然只做“识别”，不会保存结果。尚未实现：

- 语法树/抽象语法树（AST）的节点与边；
- `yylval` 传递 token 的文本和节点；
- AST 打印与释放；
- 符号表、类型检查和其他语义分析；
- 代码生成。

下一阶段将构造 AST，使成功解析的程序不再被立即丢弃。
