# Note 02：从词法 token 到表达式语法分析（截至 `5c79623`）

> 对应提交：`5c796234022d1ca09acd497c66af460198c11221`  
> 阶段目标：接入 Bison，使 Flex 返回 token 给语法分析器，并识别完整的表达式结构与运算符优先级。

## 1. 与 Note 01 相比，发生了什么变化

Note 01 的词法器在匹配到规则后直接 `printf`，例如打印 `INT: 42`。这能证明 token 被正确识别，却不能判断 token 的排列是否合法。

本阶段加入了 Bison：

```text
源文件
  ↓
Flex: yylex() 逐个返回 token
  ↓
Bison: yyparse() 按文法组合 token
  ↓
接受完整表达式，或报告 Error type B
```

例如：

```c
a + b * c
```

词法层只知道它是 `ID PLUS ID STAR ID`；语法层进一步根据优先级把它理解为：

```text
a + (b * c)
```

截至该提交，分析器只验证表达式是否符合文法，不构造抽象语法树（AST），也不进行类型或变量声明检查。

## 2. Bison 生成的文件与 Flex 的衔接

新增的 `src/frontend/syntax.y` 是 Bison 文法文件。Makefile 中的命令：

```make
$(BISON) -d -v -o build/syntax.c src/frontend/syntax.y
```

会生成：

```text
build/syntax.c       Bison 生成的语法分析器实现
build/syntax.h       token 编号的 C 头文件
build/syntax.output  文法状态与分析报告，供排查冲突使用
```

`-d` 的关键作用是生成 `syntax.h`。词法器在自己的 C 代码区中包含它：

```c
#include "syntax.h"
```

于是 `lexer.l` 可以写：

```lex
{DIGIT}+ { return INT; }
"+"       { return PLUS; }
```

其中 `INT`、`PLUS` 不是 C 保留字，而是 Bison 根据 `%token` 声明生成的整数常量。Flex 返回这些编号，Bison 就知道读到了哪一种 token。

Makefile 中让 `lexer.c` 依赖 `syntax.c`：

```make
$(LEXER_SOURCE): $(LEXER_SPEC) $(PARSER_SOURCE) | $(BUILD_DIR)
```

原因是生成 `syntax.c` 时也会生成 `syntax.h`；必须先有这个头文件，Flex 生成的 C 代码才能正常编译。

## 3. `main.c` 的入口变化

词法器阶段的入口是：

```c
yylex();
```

本阶段改为：

```c
int parse_result = yyparse();
return parse_result;
```

`yyparse()` 由 Bison 生成。它需要下一个 token 时，会调用 Flex 提供的 `yylex()`；因此调用关系是：

```text
main → yyparse → yylex
```

`yyin` 仍由 `main` 指向用户传入的 `.cmm` 文件。`yyparse()` 成功时通常返回 `0`，发生语法错误时返回非零值，这也成为测试目标判断成功或失败的依据。

## 4. `syntax.y` 的基本结构

和 Flex 文件类似，Bison 文件也被两个 `%%` 切成三段：

```yacc
%{ C 声明区 %}
token / 优先级 / 起始符号声明区
%%
文法规则区
%%
用户 C 代码区
```

### 4.1 声明区

```yacc
%{
#include <stdio.h>

int yylex(void);
extern int yylineno;
void yyerror(const char *message);
%}
```

- `yylex()`：告诉 Bison，取下一个 token 的函数由 Flex 提供。
- `yylineno`：读取 Flex 自动维护的当前行号。
- `yyerror()`：Bison 检测到语法错误时调用的函数。

### 4.2 `%token`：Bison 认识哪些 token

```yacc
%token INT FLOAT ID TYPE
%token SEMI COMMA ASSIGNOP RELOP
%token PLUS MINUS STAR DIV AND OR DOT NOT
%token LP RP LB RB LC RC
%token STRUCT RETURN IF ELSE WHILE
```

这些名称是语言的 token 表。此时表达式文法实际使用的是 `INT`、`FLOAT`、`ID`、运算符、括号、方括号和点；其余 token 先声明，为后续语句、定义和结构体规则预留。

同样地，词法器在这个版本中只对表达式所需 token 执行 `return`。例如 `";"` 仍是打印 `SEMI`，因为当前的根规则不是语句，尚不需要分号。

### 4.3 起始符号

```yacc
%start Program
```

这指定整个输入必须从 `Program` 开始解析。本阶段的规则是：

```yacc
Program:
    Exp
    ;
```

所以一个测试文件暂时只应放一条表达式，例如 `a + b * c`，而不能写成完整函数、更不能在结尾写 `;`。

## 5. 表达式文法

核心规则为：

```yacc
Exp:
    ID
    | INT
    | FLOAT
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp %prec UMINUS
    | NOT Exp
    | Exp RELOP Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp ASSIGNOP Exp
    | ID LP RP
    | ID LP Args RP
    | Exp LB Exp RB
    | Exp DOT ID
    ;
```

可以按形状分为几类：

| 形状 | 例子 | 含义 |
| --- | --- | --- |
| 基础表达式 | `name`、`42`、`3.14` | 标识符或字面量 |
| 二元表达式 | `a + b`、`a < b`、`a && b` | 两个表达式之间有运算符 |
| 括号 | `(a + b)` | 改变默认优先级 |
| 一元表达式 | `-x`、`!flag` | 单个操作数前的运算符 |
| 函数调用 | `f()`、`f(a, b)` | 函数名后接参数列表 |
| 数组访问 | `a[i]` | 表达式后接下标 |
| 结构体成员 | `p.x` | 表达式后接 `.` 和成员名 |

`Args` 使用递归列表表示一个或多个参数：

```yacc
Args:
    Exp COMMA Args
    | Exp
    ;
```

例如 `f(a, b, c)` 的右侧部分会反复匹配 `Exp COMMA Args`，直到最后的 `c` 匹配 `Exp`。

## 6. 为什么需要优先级声明

仅有 `Exp: Exp PLUS Exp` 这类规则时，`a - b - c` 与 `a + b * c` 都有不止一种可能的归约方式。Bison 需要额外信息来消除歧义。

```yacc
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS
%left LP RP LB RB DOT
```

声明从上到下，优先级逐步升高。因此：

```text
赋值 =
  < ||
  < &&
  < 比较运算符
  < + -
  < * /
  < ! 和一元 -
  < 调用、下标、成员访问
```

- `%left`：同一级运算符从左结合，如 `a - b - c` 解析为 `(a - b) - c`。
- `%right`：同一级运算符从右结合，如 `a = b = c` 解析为 `a = (b = c)`。
- `UMINUS`：虚构的优先级名字，不由 lexer 返回。`MINUS` 在二元减法中属于 `PLUS MINUS` 级，而规则 `MINUS Exp %prec UMINUS` 用 `%prec` 手动赋予一元负号更高的优先级。

括号不是普通二元运算符；`LP Exp RP` 直接把内部的 `Exp` 组合成一个整体，因此 `(a + b) * c` 会先完成括号中的表达式。

## 7. 错误处理

词法器无法识别字符时仍输出词法错误：

```text
Error type A at Line <行号>: Mysterious character "<字符>".
```

Bison 无法依据当前文法继续解析时，会调用：

```c
void yyerror(const char *message) {
    fprintf(stderr, "Error type B at Line %d: %s\n",
        yylineno, message);
}
```

例如两个字面量直接并列：

```c
1 2
```

中间没有运算符，不能匹配 `Exp` 的任何组合形式，因此会报告 `Error type B`。

## 8. 自动化测试的变化

Makefile 的 `test` 目标不再只运行一个样例，而是收集：

```make
VALID_TESTS := $(sort $(wildcard tests/parser/valid/*.cmm))
INVALID_TESTS := $(sort $(wildcard tests/parser/invalid/*.cmm))
```

随后分别循环：

- 有效样例：程序退出状态为 `0`，打印 `PASS`；
- 无效样例：程序退出状态非 `0`，打印 `PASS (rejected)`；
- 任一结果与预期相反：立即 `exit 1`，使 `make test` 失败。

无效样例的标准输出与错误输出被重定向到 `/dev/null`，这样测试结果只展示通过/失败，不重复展示预期内的错误信息。

本阶段已经覆盖：字面量、四则运算优先级、括号、一元运算、关系/逻辑/赋值表达式、函数调用与参数、数组访问、成员访问，以及 `two_literals.cmm` 的拒绝行为。

## 9. 阶段边界

截至 `5c79623`，项目已经是“表达式识别器”，但还不是完整 C-- 程序解析器。尚未支持：

- 表达式结尾的分号与表达式语句；
- `return`、`if`、`else`、`while` 等语句；
- 语句块、变量定义、函数定义和全局定义；
- AST、语义值 `yylval`、符号表、类型检查。

下一阶段会先把表达式放入语句结构中，再逐步扩展到完整程序。
