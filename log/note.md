## level 1

- 教程中将Number设成了int_val,不知如何处理，改为BaseAST好处理一些。
- 如何安装gdb，进而在vscode中逐行调试？
    * 安装g++_9，g++
    * 安装gmp
- 不能在h文件中定义函数

## level 3

### level 3.1

- 考虑各个表达式的优先级，谨慎调整dumpkoopa位置；```void UnaryExpAST::DumpKoopa()```
- 2022.3.12 要修改的地方：
    * AST中对number和temp_sign的处理；**用栈啊淦**
    * KoopaStr2Program中对二元运算符lhs和rhs的讨论。
### level 3.2
    * 利用if-else分支查看表达式的mode并确定优先级。