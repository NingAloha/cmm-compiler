# Note 01：词法分析器（截至 `71c78bb`）

> 对应提交：`71c78bb59fc0804d2bed84854bf319321fe5eeb1`  
> 阶段目标：用 Flex 将 C-- 源代码中的字符流切分为 token，并输出 token 或词法错误。

## 1. 这一阶段完成了什么

词法分析器读取一个 `.cmm` 文件，把其中的字符按词法规则识别为：关键字、标识符、整数、浮点数、运算符、界符等 token。例如：

```c
int answer = 42;
```

会依次识别出：

```text
TYPE: int
ID: answer
ASSIGNOP
INT: 42
SEMI
```

这一阶段尚未判断这串 token 是否符合程序的语法；例如 token 的排列是否能构成一条合法语句，是下一阶段语法分析器的工作。

## 2. 文件分工

```text
src/frontend/lexer.l  Flex 规则：描述“什么字符算什么 token”
src/driver/main.c     打开输入文件，并启动词法分析
Makefile              生成、编译、测试词法分析器
tests/lexer/          词法分析的有效与无效样例
```

构建链路是：

```text
lexer.l --flex--> build/lexer.c --gcc + main.c--> build/lexer
```

Flex 负责把声明式的规则翻译成 C 代码；最终运行的仍是编译出来的 C 程序。

## 3. `lexer.l` 的三段结构

Flex 文件由两个 `%%` 分隔为三部分：

```lex
定义区
%%
规则区
%%
用户 C 代码区
```

本项目的主要内容在前两段。

### 3.1 定义区：选项、状态和正则表达式

```lex
%option noyywrap yylineno noinput nounput
%x COMMENT
```

- `noyywrap`：输入结束时不需要自行实现 `yywrap()`。
- `yylineno`：Flex 维护当前行号，换行时自动增加 `yylineno`。
- `noinput`、`nounput`：不生成未使用的辅助函数，避免编译器警告。
- `%x COMMENT`：声明独占起始状态 `COMMENT`，用于扫描块注释内部。

接着定义可复用的正则表达式：

```lex
DIGIT [0-9]
ID [A-Za-z_][A-Za-z0-9_]*
FLOAT {DIGIT}+\.{DIGIT}+
```

- `ID` 的首字符只能是字母或下划线，之后才可以出现数字。
- `FLOAT` 要求小数点两侧至少各有一个数字，因此 `3.14` 是浮点数，`.5` 和 `3.` 不是。
- 花括号表示引用此前定义的模式，例如 `{DIGIT}+`。

`%{ ... %}` 中的 C 代码会原样放入生成的 `lexer.c`。这里保存了 `comment_start_line`，以便报告未闭合块注释的起始行。

### 3.2 规则区：模式匹配后执行动作

规则的一般形式是：

```lex
模式    { 匹配成功后执行的 C 动作 }
```

例如：

```lex
"return" { printf("RETURN\n"); }
{DIGIT}+ { printf("INT: %s\n", yytext); }
```

其中 `yytext` 是本次匹配到的原始文本。

Flex 的两个核心匹配原则是：

1. 优先匹配最长的文本。
2. 若两个规则匹配长度相同，采用文件中更靠前的规则。

因此本项目中特意保留了以下顺序：

- 关键字在 `{ID}` 之前：`int` 应识别为 `TYPE`，而不是普通 `ID`。
- `>=`、`<=`、`==`、`!=` 在 `>`、`<`、`=`、`!` 之前。
- `&&`、`||` 在单字符运算符之前。
- `FLOAT` 在 `{DIGIT}+` 之前；虽然最长匹配已能区分 `3.14` 与 `3`，这个顺序仍清楚表达了“先判断浮点数”的意图。

### 3.3 空白、注释和兜底错误

空格、Tab、回车和换行不产生 token：

```lex
[ \t\r]+ { /* 忽略 */ }
\n       { /* yylineno 自动更新 */ }
```

单行注释通过下面规则直接跳过：

```lex
"//".* { /* 忽略至本行结尾 */ }
```

块注释需要跨行，因此使用状态切换：

```text
INITIAL --遇到 /*--> COMMENT --遇到 */--> INITIAL
```

对应的关键操作是：

```lex
"/*"              { comment_start_line = yylineno; BEGIN(COMMENT); }
<COMMENT>"*/"     { BEGIN(INITIAL); }
<COMMENT><<EOF>>  { 报告 Unterminated comment; }
```

在 `COMMENT` 状态中，换行和普通字符都被忽略；只有 `*/` 才能退出。`<COMMENT><<EOF>>` 专门处理文件结束前仍未遇到 `*/` 的情况。

最后的单独 `.` 是兜底规则：前面的规则都没有匹配到时，报告未知字符。

```lex
. {
    fprintf(stderr,
        "Error type A at Line %d: Mysterious character \"%s\".\n",
        yylineno, yytext);
}
```

注意：在 Flex 中，`.` 匹配除换行外的任意一个字符；换行已经由前面的 `\n` 规则单独处理。

## 4. `main.c` 如何启动 Flex

驱动程序的流程为：

```text
检查命令行参数
    ↓
fopen 打开 source.cmm
    ↓
把文件指针赋给 Flex 提供的 yyin
    ↓
调用 yylex() 开始扫描
    ↓
关闭文件
```

`yyin` 和 `yylex()` 都由 Flex 生成的代码提供：

- `yyin`：当前词法分析器读取的输入流；
- `yylex()`：持续匹配规则、执行动作，直至文件结束。

因此命令：

```bash
./build/lexer tests/lexer/valid/basic.cmm
```

会让词法分析器读取指定样例，而不是从标准输入读取。

## 5. Makefile 的依赖关系

主要目标如下：

```make
$(BUILD_DIR):
	mkdir -p $@

$(LEXER_SOURCE): $(LEXER_SPEC) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

$(TARGET): $(LEXER_SOURCE) $(DRIVER_SOURCE)
	$(CC) $(CFLAGS) -o $@ $^
```

- `$@`：当前规则的目标，例如 `build/lexer.c`。
- `$<`：第一个普通依赖，例如 `src/frontend/lexer.l`。
- `$^`：全部普通依赖，例如生成的 `lexer.c` 和 `main.c`。
- `| $(BUILD_DIR)`：顺序仅依赖。`build/` 必须先存在，但目录时间戳变化不应触发重新生成 `lexer.c`。

常用命令：

```bash
make        # 生成 build/lexer
make test   # 运行基础有效样例
make clean  # 删除 build/
```

## 6. 已覆盖的测试与错误

截至该提交，词法测试包含：

- `tests/lexer/valid/basic.cmm`：关键字、标识符、整数、浮点数、界符和单行注释；
- `tests/lexer/valid/block_comment.cmm`：正常结束的块注释；
- `tests/lexer/invalid/unknown_character.cmm`：未知字符，输出 `Error type A`；
- `tests/lexer/invalid/unterminated_comment.cmm`：未闭合块注释，报告其起始行。

## 7. 阶段边界

该版本的词法器以 `printf` 输出 token 名称，便于观察和测试。它还没有：

- 向语法分析器返回 token 编号；
- 设置 `yylval` 传递标识符或字面量的语义值；
- 构造语法树；
- 执行语义检查，例如变量是否已声明。

这些工作属于后续的 Bison 语法分析、抽象语法树和语义分析阶段。
