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
