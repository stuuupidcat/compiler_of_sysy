## level 1

- 教程中将Number设成了int_val,不知如何处理，改为BaseAST好处理一些。
- 如何安装gdb，进而在vscode中逐行调试？
    * 安装g++_9，g++
    * 安装gmp
- 不能在h文件中定义函数

## level 3

### level 3.1

- 考虑各个表达式的优先级，谨慎调整dumpkoopa位置；```void UnaryExpAST::DumpKoopa()```