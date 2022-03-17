

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