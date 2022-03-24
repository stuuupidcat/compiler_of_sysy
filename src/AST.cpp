#include "AST.h"

//当前临时标号的个数。
int temp_sign_num = 0;

//当前基本块的个数。
int basic_block_num = -1;

//unordered_map的集合，下标为当前的block。
std::vector<std::unordered_map<Value, ValueData, ValueHash, ValueEqual>> blocks_values;

//unordered_map中key的集合，下标为当前的block。
std::vector<std::vector<Value>> blocks_insts;

//函数的个数，搭配符号表使用。
int function_num = -1;
//符号表。
std::vector<std::unordered_map<std::string, VariableInfo>> symbol_table; 

void InsertKVToMap(Value value, ValueData valuedata) {
    blocks_values[basic_block_num].insert(std::make_pair(value, valuedata));
    blocks_insts[basic_block_num].push_back(value);
}

size_t ValueHash::operator() (const Value& value) const noexcept
{
     std::hash<long> myhash;
     return myhash(long(value));
}


size_t ValueEqual::operator() (const Value& val_1, const Value& val_2) const noexcept
{
    return ((long)val_1 == (long)val_2);
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
    if (lhs != nullptr) {
        lhs_vd = (*blocks_values[basic_block_num].find(lhs)).second;
    }
    if (rhs != nullptr) {
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


Value CompUnitAST::DumpKoopa(Value self)  {
    blocks_values.resize(100);
    blocks_insts.resize(100);
    symbol_table.resize(100);

    function_num++;
    func_def -> DumpKoopa(nullptr);
    return self;
}

Value FuncDefAST::DumpKoopa(Value self)   {
    std::cout << "fun ";
    std::cout << "@" << ident << "(): ";

    func_type -> DumpKoopa(nullptr);
    block -> DumpKoopa(nullptr);
    return self;
}

Value FuncTypeAST::DumpKoopa(Value self)   {
    std::cout << "i32 ";
    return self;
}

//wrong code.
Value BlockAST::DumpKoopa(Value self)   {
    std::cout << "{" << std::endl;
    std::cout << "%entry:" << std::endl;
    basic_block_num++;
    for (auto &blockitem:blockitems) {
        blockitem->DumpKoopa(&blockitem);
    }
    PrintInstruction();
    std::cout << '}' << std::endl;
    return self;
}


Value StmtAST::DumpKoopa(Value self) {
    if (mode == 0) {
        return self;
    }
    else if (mode == 1) {
        //Value allocated = Allocate(&exp);
        Value lhs_value = exp->DumpKoopa(&exp);
        Value rhs_value = nullptr;
        ValueData vd = AllocateValueData("return", lhs_value, rhs_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;
    }
    else if (mode == 2) { //lval(ident) = exp;
        Value lhs_value = exp->DumpKoopa(&exp);
        Value rhs_value = nullptr;
        //不需要分配百分号的值。
        ValueData vd = ValueData(-1, "store", lhs_value, rhs_value, lval->ident);
        //但是需要改动符号表中的值。
        auto vi = symbol_table[function_num].find(lval->ident);
        (*vi).second.value = lhs_value;
        
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;
    }
    return self;
}


Value NumberAST::DumpKoopa(Value self) {
    Value lhs_value = (Value)(long)num;
    Value rhs_value = nullptr;

    //不需要分配新的值。
    ValueData vd = ValueData(-1, "number", lhs_value, rhs_value);
    Value this_value = self;
    InsertKVToMap(this_value, vd);
    return this_value;
}


Value ExpAST::DumpKoopa(Value self)  {
    return lorexp -> DumpKoopa(&lorexp);
}


Value UnaryExpAST::DumpKoopa(Value self) {
    if (mode == 0) {
        return primaryexp->DumpKoopa(&primaryexp);
    }
    else if (mode == 1) {//?
        std::string ops[3] = {"singleadd", "singlesub", "singleeq"};

        Value lhs_value = nullptr;
        Value rhs_value = unaryexp->DumpKoopa(&unaryexp);


        ValueData vd = AllocateValueData(ops[unaryop->mode], lhs_value, rhs_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;
    }
    //never reached.
    return self;
}


Value PrimaryExpAST::DumpKoopa(Value self)  {
    if (mode == 0) {
        return exp->DumpKoopa(&exp);
    }
    else if (mode == 1) {
        return lval->DumpKoopa(&lval);
    }
    else if (mode == 2) {
        return number->DumpKoopa(&number);
    }
    //never reached;
    return self;
}


Value UnaryOpAST::DumpKoopa(Value self)  {
    return self;
}


Value MulExpAST::DumpKoopa(Value self)  {
    if (mode == 0) {
        return unaryexp -> DumpKoopa(&unaryexp);
    }
    else {
        std::string ops[4] = {"", "mul", "div", "mod"};

        Value lhs_value = mulexp->DumpKoopa(&mulexp);
        Value rhs_value = unaryexp->DumpKoopa(&unaryexp);
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;
    }
}


Value AddExpAST::DumpKoopa(Value self)  {
    if (mode == 0) {
        return mulexp -> DumpKoopa(&mulexp);
    }
    else {
        std::string ops[4] = {"", "add", "sub"};
        Value lhs_value = addexp->DumpKoopa(&addexp);
        Value rhs_value = mulexp->DumpKoopa(&mulexp);
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;
    }
}

Value RelExpAST::DumpKoopa(Value self) {
    if (mode == 0) {
        return addexp->DumpKoopa(&addexp);
    }
    else {
        std::string ops[5] = {"", "lt", "gt", "le", "ge"};
        Value lhs_value = relexp->DumpKoopa(&relexp);
        Value rhs_value = addexp->DumpKoopa(&addexp);
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;        
    }
}

Value EqExpAST::DumpKoopa(Value self) {
    if (mode == 0) {
        return relexp->DumpKoopa(&relexp);
    }
    else {
        std::string ops[5] = {"", "eq", "ne"};
        Value lhs_value = eqexp->DumpKoopa(&eqexp);
        Value rhs_value = relexp->DumpKoopa(&relexp);
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;        
    }
}

Value LAndExpAST::DumpKoopa(Value self) {
    if (mode == 0) {
        return eqexp->DumpKoopa(&eqexp);
    }
    else {
        Value lhs_value = landexp->DumpKoopa(&landexp);
        Value rhs_value = eqexp->DumpKoopa(&eqexp);
        
        //按位与或实现逻辑与或。
        //%n = eq lhs, 0;    -> vd_l
        //%n+1 = eq rhs, 0;  -> vd_r
        //%n+2 = and %n, %n+1;

        //为数字0, vd_l, vd_r分配一个数据结构。采用随机数作为value.
        ValueData vd_zero = AllocateValueData("number", 0, 0);
        Value vd_zero_value  = (std::unique_ptr<BaseAST>*)random();
        ValueData vd_l = AllocateValueData("ne", lhs_value, vd_zero_value);
        Value vd_l_value = (std::unique_ptr<BaseAST>*)random();
        ValueData vd_r = AllocateValueData("ne", rhs_value, vd_zero_value);
        Value vd_r_value = (std::unique_ptr<BaseAST>*)random();
        InsertKVToMap(vd_zero_value, vd_zero);
        InsertKVToMap(vd_l_value, vd_l);
        InsertKVToMap(vd_r_value, vd_r);
        ValueData vd = AllocateValueData("and", vd_l_value, vd_r_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;        
    }
}


Value LOrExpAST::DumpKoopa(Value self) {
    if (mode == 0) {
        return landexp->DumpKoopa(&landexp);
    }
    else {
        Value lhs_value = lorexp->DumpKoopa(&lorexp);
        Value rhs_value = landexp->DumpKoopa(&landexp);

         //按位与或实现逻辑与或。
        //%n = eq lhs, 0;    -> vd_l
        //%n+1 = eq rhs, 0;  -> vd_r
        //%n+2 = or %n, %n+1;

        //为数字0, vd_l, vd_r分配一个数据结构。采用随机数作为value.
        ValueData vd_zero = AllocateValueData("number", 0, 0);
        Value vd_zero_value  = (std::unique_ptr<BaseAST>*)random();
        ValueData vd_l = AllocateValueData("ne", lhs_value, vd_zero_value);
        Value vd_l_value = (std::unique_ptr<BaseAST>*)random();
        ValueData vd_r = AllocateValueData("ne", rhs_value, vd_zero_value);
        Value vd_r_value = (std::unique_ptr<BaseAST>*)random();
        InsertKVToMap(vd_zero_value, vd_zero);
        InsertKVToMap(vd_l_value, vd_l);
        InsertKVToMap(vd_r_value, vd_r);
        ValueData vd = AllocateValueData("or", vd_l_value, vd_r_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;        
    }
}

/////////////////////////////////////////////
Value DeclAST::DumpKoopa(Value self) {
    if (mode == 0)
        return constdecl->DumpKoopa(&constdecl);
    else 
        return vardecl->DumpKoopa(&vardecl);
}

Value ConstDeclAST::DumpKoopa(Value self) {
    for (auto& constdef: constdefs) {
        constdef->DumpKoopa(&constdef);
    }
    return self;
}

Value VarDeclAST::DumpKoopa(Value self) {
    for (auto& vardef: vardefs) {
        vardef->DumpKoopa(&vardef);
    }
    return self;
}

Value ConstDefAST::DumpKoopa(Value self) {
    //exp_value是一个表达式对应的<Value, ValueData>的键值。
    Value exp_value = constinitval->DumpKoopa(&constinitval);
    //将符号插入到符号表中。
    symbol_table[function_num].insert(std::make_pair(ident,
                                                     VariableInfo(exp_value, true, ident)));
    return self;
}

Value VarDefAST::DumpKoopa(Value self) {
    //分成三个指令
    //alloc
    //计算exp的值
    //store
    ValueData vd_alloc = ValueData(-1, "alloc", nullptr, nullptr, ident);
    Value vd_alloc_value = (std::unique_ptr<BaseAST>*)random();
    InsertKVToMap(vd_alloc_value, vd_alloc);
    if (mode == 0) { //无赋值
        symbol_table[function_num].insert(std::make_pair(ident,
                                                     VariableInfo(nullptr, false, ident)));
    }
    else {
        Value lhs = initval->DumpKoopa(&initval);
        ValueData vd_store = ValueData(-1, "store", lhs, nullptr, ident);
        Value vd_store_value = (std::unique_ptr<BaseAST>*)random();
        InsertKVToMap(vd_store_value, vd_store);
        symbol_table[function_num].insert(std::make_pair(ident,
                                                     VariableInfo(lhs, false, ident)));
    }
    return self;
}

Value ConstInitValAST::DumpKoopa(Value self) {
    return constexp->DumpKoopa(&constexp);
}

Value BlockItemAST::DumpKoopa(Value self) {
    if (mode == 0) {
        return decl->DumpKoopa(&decl);
    }
    else if (mode == 1) {
        return stmt->DumpKoopa(&stmt);
    }
    return self;
}

Value LValAST::DumpKoopa(Value self) {
    
    //加入变量之后应如何改变。
    auto vi = symbol_table[function_num].find(ident);
    Value lhs_value = (*vi).second.value;
    Value rhs_value = nullptr;

    if (!(*vi).second.is_const_variable) {
        //变量
         ValueData vd = AllocateValueData("load", lhs_value, rhs_value, ident);
         Value this_value = self;
         InsertKVToMap(this_value, vd);
         return this_value;
    }

    else {
        //常量不需要分配新的值。
        //宛如number。需要新的数据结构但并不需要新的临时标号。
        ValueData vd = ValueData(-1, "lval", lhs_value, rhs_value);
        Value this_value = self;
        InsertKVToMap(this_value, vd);
        return this_value;
    }
    return self;
}

Value ConstExpAST::DumpKoopa(Value self) {
    return exp->DumpKoopa(&exp);
}

Value InitValAST::DumpKoopa(Value self) {
    return exp->DumpKoopa(&exp);
}

