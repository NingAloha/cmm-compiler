# 符号表：名字、类型和作用域的映射

对应源码：[src/semantic/symbol.h](../../src/semantic/symbol.h) 与 [src/semantic/symbol.c](../../src/semantic/symbol.c)。符号表将源程序中的名字映射到其种类与 `Type *`，是变量引用、函数调用和结构体标签查询的共同入口。

## 1. 符号结点

每个 `Symbol` 保存：

| 字段 | 含义 |
| --- | --- |
| `name` | 复制后的标识符文本 |
| `kind` | `SYMBOL_VARIABLE`、`SYMBOL_FUNCTION` 或 `SYMBOL_STRUCT` |
| `type` | 变量类型、函数类型或结构体类型 |
| `line` | 首次定义所在行 |
| `scope_depth` | 定义时的作用域深度 |
| `next` | 下一个符号结点 |

`SymbolTable` 用链表头指针和当前 `scope_depth` 表示环境。类型对象的生命周期不由符号表释放：基础类型是共享对象，复合类型可能又被多个符号引用；表清空时只释放符号名称和符号结点。

## 2. 插入和查询

`symbol_table_insert()` 在当前作用域中先调用 `symbol_table_find_current()` 检查重名；若重名则返回 `0`，不会覆盖原有定义。成功插入时复制名称并追加到链表末尾。

两种查询服务于不同语义规则：

- `symbol_table_find_current()` 只检查 `scope_depth` 等于当前深度的结点，用于重复定义判断；
- `symbol_table_find()` 在所有仍然存活的结点中返回最后出现的同名结点，用于普通引用。内层作用域的定义在链表中更靠后，因此自然遮蔽外层定义。

名称比较一律使用 `strcmp(...) == 0`，而不是比较字符串指针。

## 3. 作用域管理

`symbol_table_enter_scope()` 增加深度。`symbol_table_leave_scope()` 遍历链表并删除离开层级的全部符号，再降低深度。函数定义分析在插入函数自身后进入函数作用域并插入形参；嵌套复合语句同样进入和离开作用域。

因此当前实现支持选做 2.2 所需的局部变量遮蔽：内层块可定义同名变量，离开该块后该定义消失；不同函数中的局部变量也可同名。结构体成员不放入此表，而存入结构体类型自身的 `Field` 链表，符合成员名不与普通变量混用的规则。

## 4. 生命周期

一次 `semantic_analyze()` 开始时调用 `symbol_table_init()`，结束时调用 `symbol_table_clear()`。这保证多次分析不会复用上次程序的名字，也避免遗留的变量、函数或结构体符号影响下一次分析。
