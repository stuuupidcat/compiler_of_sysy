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
    * 利用if-else分支查看表达式的mode并确定优先级。x
    * dumpkoopa时将这个表达式的值压入栈中？x
    有关Sysy向Koopa IR转变的过程（算术表达式的部分），我目前的思路是将每一个非终结符号都赋予一个“%n”，即每一个非终结符号都有一个值。然而这种思路产生了诸多问题：1.由于在分配标号的过程是自上而下的，写出的koopair程序往往“%n”都是倒过来的，大数反而在前面。2.过程中会产生许多诸如“%3 = %2”这样的无聊式子。 所以想向您请教下较为正常的思路或麻烦您帮我指出我思路中的问题！
    * 利用如下思路。
```c++

using Value = int;

BasicBlock {
    std::vector<Value> insts;
    std::unordered_map<Value, ValueData> values;
}

using ValueData = std::variant<
    OrValueType,
    SubValueType
>

struct OrValueType {
    int no;
    std::string format() {
        return std::format("%{} = or {} {}", no, lhs..., rhs...)
    }
    Value lhs, rhs;
}

// a || b
Value dumpKoopa() {
    // this is AST 
    switch (mode) {
        // a
        case and_expr: {
            auto and_expr;
            //这个如果是一元的就可以直接返回子表达式的值。
            return and_expr.dumpKoopa();
        }
        case or_expr: {
            auto lhs;
            auto rhs;
            Value lhs_value = lhs->dumpKoopa();
            Value rhs_value = rhs->dumpKoopa();
            OrValueType vd = { "%1", lhs_value, rhs_value };
            Value this_value;
            bb.values.insert(this_value, vd);
            bb.insts.push_back(this_value);
            return this_value;
        }
    }
}

for (auto inst : bb.insts()) {
    auto vd = bb.values.find(...);
    cout << vd.format()
}
```

### lv3.3 
* 在and or语句前插入两条ne来用按位异或实现逻辑异或。

## lv4

### lv4.1
* 递归的卡巴斯基范式。
* 符号表如何处理？
    - 处理constdef的时候，将<变量名，对应表达式的id>插入到符号表中。
    - 有对应的lval的时候，在符号表中找到对应的变量名及其ID。同Number处理。
    - 将单独的分号作为一个stmt
    

### lv4.2:
* 将定义分为不同的alloc load及store的组合；
* vardef = alloc + exp + load
* 
* 每一个alloc load 是否需要分配百分号开头的符号？
- alloc不需要
- store不需要
- load需要，可是load什么时候有用呢 lval?

改一下value