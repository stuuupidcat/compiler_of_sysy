#include "AST.h"

//当前临时标号的个数。
int temp_sign_num = 0;

//当前基本块的个数。
int basic_block_num = -1;

//unordered_map的集合，下标为当前的block。
std::vector<std::unordered_map<Value, ValueData>> blocks_values;

//unordered_map中key的集合，下标为当前的block。
std::vector<std::vector<Value>> blocks_insts;

//函数的个数，搭配符号表使用。
int function_num = -1;
//符号表。
std::vector<std::unordered_map<std::string, VariableInfo>> symbol_table; 

Value InsertValuedata(ValueData valuedata) {
    Value value = random();
    blocks_values[basic_block_num].insert(std::make_pair(value, valuedata));
    blocks_insts[basic_block_num].push_back(value);
    return value;
}


void PrintInstruction() {
    for (auto inst: blocks_insts[basic_block_num]) {
        auto vd = blocks_values[basic_block_num][inst];
        if (vd.inst_type != "number" && vd.inst_type != "lval")
            std::cout << "  " <<vd.format();
    }
}

VariableInfo::VariableInfo(Value value_, bool is_const_, std::string name_) {
    value = value_;
    alloc_name = name_;
    is_const_variable = is_const_;
}


std::string ValueData::format() {
    std::string res;
    ValueData lhs_vd, rhs_vd;
    VariableInfo vi;

    //指令类型为number并不需要输出，将number返回即可。
    if (inst_type == "number") {
        res = std::to_string((long)lhs);
        return res;
    }
    
    //寻找对应的ValueData。
    if (lhs != 0) {
        lhs_vd = (*blocks_values[basic_block_num].find(lhs)).second;
    }
    if (rhs != 0) {
        rhs_vd = (*blocks_values[basic_block_num].find(rhs)).second;
    }
    if (variable_name != "") {
        vi = (*symbol_table[function_num].find(variable_name)).second;
    }


    //运算符为一元操作, lhs为0。
    if (inst_type.find("single") != std::string::npos) {
        res = "%" + std::to_string(no) + " = " + inst_type.substr(6) + " 0, ";
        if (rhs_vd.inst_type == "number" || rhs_vd.inst_type == "lval") {
            res += rhs_vd.format() + "\n";
        }
        else {
            res += "%" + std::to_string(rhs_vd.no) + "\n";
        }
    }

    //指令类型为lval，是曾经一个表达式的值。
    else if (inst_type == "lval") {
        if (lhs_vd.inst_type == "number" || lhs_vd.inst_type == "lval") {
            res = lhs_vd.format();
        }
        else {
            res = "%" + std::to_string(lhs_vd.no);
        }
    }

    //运算符为return操作。
    else if (inst_type == "return") {
        res = "ret ";
        if (lhs_vd.inst_type == "number" || lhs_vd.inst_type == "lval") {
            res += lhs_vd.format();
        }
        else {
            res += "%" + std::to_string(lhs_vd.no);
        }
        res += "\n";
    }
    //运算符为二元操作。
    else if (
        inst_type == "add" ||
        inst_type == "sub" ||
        inst_type == "mul" ||
        inst_type == "div" ||
        inst_type == "mod" ||
        inst_type == "le" ||
        inst_type == "ge" ||
        inst_type == "lt" ||
        inst_type == "gt" ||
        inst_type == "eq" ||
        inst_type == "ne" ||
        inst_type == "and" ||
        inst_type == "or"
    ) { 
        res = "%" + std::to_string(no) + " = " + inst_type + " ";
        //处理左表达式
        if (lhs_vd.inst_type == "number" || lhs_vd.inst_type == "lval") {
            res += lhs_vd.format() + ",";
        }
        else {
            res += "%" + std::to_string(lhs_vd.no) + ", ";
        }
        //处理右表达式
        if (rhs_vd.inst_type == "number" || rhs_vd.inst_type == "lval") {
            res += rhs_vd.format() + "\n";
        }
        else {
            res += "%" + std::to_string(rhs_vd.no) + "\n";
        }
    }
    //指令类型为alloc
    else if (inst_type == "alloc") {
        //定义且赋值
        res = "@" + variable_name + " = alloc i32\n";
    }
    //指令为变量的赋值
    else if(inst_type == "store") {
        res = "store ";
        if (lhs_vd.inst_type == "number" || lhs_vd.inst_type == "lval") {
            res += lhs_vd.format() + ",";
        }
        else {
            res += "%" + std::to_string(lhs_vd.no) + ", ";
        }
        res += "@" + variable_name + "\n";
    }
    //载入一个变量
    else if (inst_type == "load") {
        res = "%" + std::to_string(no) + " = load @" + variable_name + "\n";
    }

    return res;
}

ValueData AllocateValueData(std::string inst_type_, Value lhs_, Value rhs_, std::string name_="") {
    ValueData vd = {temp_sign_num, inst_type_, lhs_, rhs_, name_};
    temp_sign_num++;
    return vd;
}

ValueData::ValueData(int no_, std::string inst_type_, Value lhs_, Value rhs_, std::string variable_name_ = "") {
    no = no_;
    inst_type = inst_type_;
    lhs = lhs_;
    rhs = rhs_;
    variable_name = variable_name_;
}


Value CompUnitAST::DumpKoopa()  {
    blocks_values.resize(100);
    blocks_insts.resize(100);
    symbol_table.resize(100);

    function_num++;
    func_def -> DumpKoopa();
    return 0;
}

Value FuncDefAST::DumpKoopa()   {
    std::cout << "fun ";
    std::cout << "@" << ident << "(): ";

    func_type -> DumpKoopa();
    block -> DumpKoopa();
    return 0;
}

Value FuncTypeAST::DumpKoopa()   {
    std::cout << "i32 ";
    return 0;
}

//wrong code.
Value BlockAST::DumpKoopa()   {
    std::cout << "{" << std::endl;
    std::cout << "%entry:" << std::endl;
    basic_block_num++;
    for (auto &blockitem:blockitems) {
        blockitem->DumpKoopa();
    }
    PrintInstruction();
    std::cout << '}' << std::endl;
    return 0;
}


Value StmtAST::DumpKoopa() {
    if (mode == 0) {
        return 0;
    }
    else if (mode == 1) {
        //Value allocated = Allocate(&exp);
        Value lhs_value = exp->DumpKoopa();
        Value rhs_value = 0;
        ValueData vd = AllocateValueData("return", lhs_value, rhs_value);
        Value this_value = InsertValuedata(vd);
        return this_value;
    }
    else if (mode == 2) { //lval(ident) = exp;
        Value lhs_value = exp->DumpKoopa();
        Value rhs_value = 0;
        //不需要分配百分号的值。
        ValueData vd = ValueData(-1, "store", lhs_value, rhs_value, lval->ident);
        //但是需要改动符号表中的值。
        auto vi = symbol_table[function_num].find(lval->ident);
        (*vi).second.value = lhs_value;
        
        Value this_value = InsertValuedata(vd);
        return this_value;
    }
    return 0;
}


Value NumberAST::DumpKoopa() {
    Value lhs_value = (Value)(long)num;
    Value rhs_value = 0;

    //不需要分配新的值。
    ValueData vd = ValueData(-1, "number", lhs_value, rhs_value);
    Value this_value = InsertValuedata(vd);
    return this_value;
}


Value ExpAST::DumpKoopa()  {
    return lorexp -> DumpKoopa();
}


Value UnaryExpAST::DumpKoopa() {
    if (mode == 0) {
        return primaryexp->DumpKoopa();
    }
    else if (mode == 1) {//?
        std::string ops[3] = {"singleadd", "singlesub", "singleeq"};

        Value lhs_value = 0;
        Value rhs_value = unaryexp->DumpKoopa();


        ValueData vd = AllocateValueData(ops[unaryop->mode], lhs_value, rhs_value);
        Value this_value = InsertValuedata(vd);
        return this_value;
    }
    //never reached.
    return 0;
}


Value PrimaryExpAST::DumpKoopa()  {
    if (mode == 0) {
        return exp->DumpKoopa();
    }
    else if (mode == 1) {
        return lval->DumpKoopa();
    }
    else if (mode == 2) {
        return number->DumpKoopa();
    }
    //never reached;
    return 0;
}


Value UnaryOpAST::DumpKoopa()  {
    return 0;
}


Value MulExpAST::DumpKoopa()  {
    if (mode == 0) {
        return unaryexp -> DumpKoopa();
    }
    else {
        std::string ops[4] = {"", "mul", "div", "mod"};

        Value lhs_value = mulexp->DumpKoopa();
        Value rhs_value = unaryexp->DumpKoopa();
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = InsertValuedata(vd);
        return this_value;
    }
}


Value AddExpAST::DumpKoopa()  {
    if (mode == 0) {
        return mulexp -> DumpKoopa();
    }
    else {
        std::string ops[4] = {"", "add", "sub"};
        Value lhs_value = addexp->DumpKoopa();
        Value rhs_value = mulexp->DumpKoopa();
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = InsertValuedata(vd);
        return this_value;
    }
}

Value RelExpAST::DumpKoopa() {
    if (mode == 0) {
        return addexp->DumpKoopa();
    }
    else {
        std::string ops[5] = {"", "lt", "gt", "le", "ge"};
        Value lhs_value = relexp->DumpKoopa();
        Value rhs_value = addexp->DumpKoopa();
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = InsertValuedata(vd);
        return this_value;        
    }
}

Value EqExpAST::DumpKoopa() {
    if (mode == 0) {
        return relexp->DumpKoopa();
    }
    else {
        std::string ops[5] = {"", "eq", "ne"};
        Value lhs_value = eqexp->DumpKoopa();
        Value rhs_value = relexp->DumpKoopa();
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = InsertValuedata(vd);
        return this_value;        
    }
}

Value LAndExpAST::DumpKoopa() {
    if (mode == 0) {
        return eqexp->DumpKoopa();
    }
    else {
        Value lhs_value = landexp->DumpKoopa();
        Value rhs_value = eqexp->DumpKoopa();
        
        //按位与或实现逻辑与或。
        //%n = eq lhs, 0;    -> vd_l
        //%n+1 = eq rhs, 0;  -> vd_r
        //%n+2 = and %n, %n+1;

        //为数字0, vd_l, vd_r分配一个数据结构。采用随机数作为value.
        ValueData vd_zero = AllocateValueData("number", 0, 0);
        Value vd_zero_value = InsertValuedata(vd_zero);
        ValueData vd_l = AllocateValueData("ne", lhs_value, vd_zero_value);
        Value vd_l_value = InsertValuedata(vd_l);
        ValueData vd_r = AllocateValueData("ne", rhs_value, vd_zero_value);
        Value vd_r_value = InsertValuedata(vd_r);
        ValueData vd = AllocateValueData("and", vd_l_value, vd_r_value);
        Value this_value = InsertValuedata(vd);
        return this_value;        
    }
}


Value LOrExpAST::DumpKoopa() {
    if (mode == 0) {
        return landexp->DumpKoopa();
    }
    else {
        Value lhs_value = lorexp->DumpKoopa();
        Value rhs_value = landexp->DumpKoopa();

         //按位与或实现逻辑与或。
        //%n = eq lhs, 0;    -> vd_l
        //%n+1 = eq rhs, 0;  -> vd_r
        //%n+2 = or %n, %n+1;

        //为数字0, vd_l, vd_r分配一个数据结构。采用随机数作为value.
        ValueData vd_zero = AllocateValueData("number", 0, 0);
        Value vd_zero_value = InsertValuedata(vd_zero);
        ValueData vd_l = AllocateValueData("ne", lhs_value, vd_zero_value);
        Value vd_l_value = InsertValuedata(vd_l);
        ValueData vd_r = AllocateValueData("ne", rhs_value, vd_zero_value);
        Value vd_r_value = InsertValuedata(vd_r);
        ValueData vd = AllocateValueData("or", vd_l_value, vd_r_value);
        Value this_value = InsertValuedata(vd);
        return this_value;        
    }
}

/////////////////////////////////////////////
Value DeclAST::DumpKoopa() {
    if (mode == 0)
        return constdecl->DumpKoopa();
    else 
        return vardecl->DumpKoopa();
}

Value ConstDeclAST::DumpKoopa() {
    for (auto& constdef: constdefs) {
        constdef->DumpKoopa();
    }
    return 0;
}

Value VarDeclAST::DumpKoopa() {
    for (auto& vardef: vardefs) {
        vardef->DumpKoopa();
    }
    return 0;
}

Value ConstDefAST::DumpKoopa() {
    //exp_value是一个表达式对应的<Value, ValueData>的键值。
    Value exp_value = constinitval->DumpKoopa();
    //将符号插入到符号表中。
    symbol_table[function_num].insert(std::make_pair(ident,
                                                     VariableInfo(exp_value, true, ident)));
    return 0;
}

Value VarDefAST::DumpKoopa() {
    //分成三个指令
    //alloc
    //计算exp的值
    //store
    ValueData vd_alloc = ValueData(-1, "alloc", 0, 0, ident);
    InsertValuedata(vd_alloc);
    if (mode == 0) { //无赋值
        symbol_table[function_num].insert(std::make_pair(ident,
                                                     VariableInfo(0, false, ident)));
    }
    else {
        Value lhs = initval->DumpKoopa();
        ValueData vd_store = ValueData(-1, "store", lhs, 0, ident);
        InsertValuedata(vd_store);
        symbol_table[function_num].insert(std::make_pair(ident,
                                                     VariableInfo(lhs, false, ident)));
    }
    return 0;
}

Value ConstInitValAST::DumpKoopa() {
    return constexp->DumpKoopa();
}

Value BlockItemAST::DumpKoopa() {
    if (mode == 0) {
        return decl->DumpKoopa();
    }
    else if (mode == 1) {
        return stmt->DumpKoopa();
    }
    return 0;
}

Value LValAST::DumpKoopa() {
    
    //加入变量之后应如何改变。
    auto vi = symbol_table[function_num].find(ident);
    Value lhs_value = (*vi).second.value;
    Value rhs_value = 0;

    if (!(*vi).second.is_const_variable) {
        //变量
         ValueData vd = AllocateValueData("load", lhs_value, rhs_value, ident);
         Value this_value = InsertValuedata(vd);
         return this_value;   
    }

    else {
        //常量不需要分配新的值。
        //宛如number。需要新的数据结构但并不需要新的临时标号。
        ValueData vd = ValueData(-1, "lval", lhs_value, rhs_value);
        Value this_value = InsertValuedata(vd);
        return this_value;   
    }
    return 0;
}

Value ConstExpAST::DumpKoopa() {
    return exp->DumpKoopa();
}

Value InitValAST::DumpKoopa() {
    return exp->DumpKoopa();
}

