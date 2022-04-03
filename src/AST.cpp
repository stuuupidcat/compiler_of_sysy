#include "AST.h"

//当前临时标号的个数。
int temp_sign_num = 0;

//当前基本块的个数。
int basic_block_num = -1;

//函数的个数，搭配符号表使用。
int function_num = -1;

//label的个数;
int label_num = 0;

//unordered_map的集合，下标为当前的(function_num)
std::vector<std::unordered_map<Value, ValueData>> functions_values;

//unordered_map中key的集合，下标为当前的(function_num)
std::vector<std::vector<Value>> functions_insts;

//符号表。下标为当前的(function_num, block_num)
std::vector<std::vector<std::unordered_map<std::string, VariableInfo>>> symbol_table; 
//处理同名变量。下标为function_num
std::vector<std::unordered_map<std::string, int>> varname_cnt;

//循环的信息。
std::vector<LoopData> loop_data;
bool cur_loop_finished = false;

Value InsertValuedata(ValueData valuedata, Value allocated_val = 0) {
    if (allocated_val == 0) {
        Value value = random();
        functions_values[function_num].insert(std::make_pair(value, valuedata));
        functions_insts[function_num].push_back(value);
        return value;
    }
    else {
        functions_values[function_num].insert(std::make_pair(allocated_val, valuedata));
        functions_insts[function_num].push_back(allocated_val);
        return allocated_val;
    }
}
LoopData::LoopData(Value exp_label_value_, Value true_label_value_, Value jump_exp_label_value_, Value end_label_value_) {
    exp_label_value = exp_label_value_;
    true_label_value = true_label_value_;
    jump_exp_label_value = jump_exp_label_value_;
    end_label_value = end_label_value_;
}

void PrintInstruction() {
    int instruction_num = functions_insts[function_num].size();
    for (int i = 0; i < instruction_num; ++i) {
        auto vd = functions_values[function_num][functions_insts[function_num][i]];
        if (vd.inst_type == "return" || vd.inst_type == "break" || vd.inst_type == "continue") {     
            std::cout << "  "  << vd.format();    
            while (vd.inst_type != "label" && i < instruction_num) {
                i++;
                vd = functions_values[function_num][functions_insts[function_num][i]];
            }
            //改一点点就报错x
            i = i - 1;
            continue;
        }
        //要调到循环结尾的jump_exp_label
        //if (vd.inst_type == "break" || vd.inst_type == "continue") {
        //    std::cout << "  "  << vd.format();
        //    i++;
        //    auto temp = functions_values[function_num][functions_insts[function_num][i]];
        //    //vd的rhs是要跳转的地方，label的lhs是想要找到的东西。
        //    //有趣的是label没有lhs，而和他连在一起的jump有这个value
        //    while (temp.lhs != vd.rhs && i < instruction_num) {
        //        i++;
        //        temp = functions_values[function_num][functions_insts[function_num][i]];
        //    }
        //    continue;
        //}
        if (vd.inst_type == "label") {
            std::cout << vd.format();
        }
        else if (vd.inst_type != "number" && vd.inst_type != "lval")
            std::cout << "  " <<vd.format();
    }
}

VariableInfo::VariableInfo(Value value_, bool is_const_) {
    value = value_;
    is_const_variable = is_const_;
}


std::string ValueData::format() {
    std::string res;
    ValueData lhs_vd, rhs_vd, jump_cond_vd;
    VariableInfo vi;

    //指令类型为number并不需要输出，将number返回即可。
    if (inst_type == "number") {
        res = std::to_string((long)lhs);
        return res;
    }
    
    //寻找对应的ValueData。
    //向外寻找
    if (lhs != 0) {
        auto iter = functions_values[function_num].find(lhs);
        if (iter != functions_values[function_num].end()) 
            lhs_vd = (*iter).second;
    }
    if (rhs != 0) {
        auto iter = functions_values[function_num].find(rhs);
        if (iter != functions_values[function_num].end()) 
            rhs_vd = (*iter).second;
    }
    if (jump_cond != 0) {
        auto iter = functions_values[function_num].find(jump_cond);
        if (iter != functions_values[function_num].end()) 
            jump_cond_vd = (*iter).second;
    }
    if (variable_name != "") {
        for (int i = basic_block_num; i>=0; --i) {
            auto iter = symbol_table[function_num][i].find(variable_name);
            if (iter != symbol_table[function_num][i].end()) {
                vi = (*iter).second;
            }
        }
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
            res += lhs_vd.format() + ", ";
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
    else if (inst_type == "br") {
        if (jump_cond_vd.inst_type == "number" || jump_cond_vd.inst_type == "lval") {
            res = "br " + jump_cond_vd.format() + ", " + lhs_vd.variable_name + ", " + rhs_vd.variable_name + "\n";
        }
        else {
            res = "br %" + std::to_string(jump_cond_vd.no) + ", " + lhs_vd.variable_name + ", " + rhs_vd.variable_name + "\n";
        }
    }
    else if (inst_type == "jump" || inst_type == "break" || inst_type == "continue") {
        res = "jump " + lhs_vd.variable_name + "\n";
    }
    else if (inst_type == "label") {
        res = variable_name + ":\n";
    }
    return res;
}

ValueData AllocateValueData(std::string inst_type_, Value lhs_, Value rhs_, Value jump_cond_ = 0, std::string name_="") {
    ValueData vd = {temp_sign_num, inst_type_, lhs_, rhs_, jump_cond_, name_};
    temp_sign_num++;
    return vd;
}

ValueData::ValueData(int no_, std::string inst_type_, Value lhs_, Value rhs_, Value jump_cond_ = 0, std::string variable_name_ = "") {
    no = no_;
    inst_type = inst_type_;
    lhs = lhs_;
    rhs = rhs_;
    jump_cond = jump_cond_;
    variable_name = variable_name_;
}


Value CompUnitAST::DumpKoopa()  {
    functions_values.resize(100);
    functions_insts.resize(100);
    symbol_table.resize(100);
    for (auto &val: symbol_table)
        val.resize(100);
    varname_cnt.resize(100);

    function_num++;
    func_def -> DumpKoopa();
    return 0;
}

Value FuncDefAST::DumpKoopa()   {
    std::cout << "fun ";
    std::cout << "@" << ident << "(): ";

    func_type -> DumpKoopa();
    
    std::cout << "{" << std::endl;
    std::cout << "%entry:" << std::endl;
    block -> DumpKoopa();
    PrintInstruction();
    std::cout << '}' << std::endl;
    return 0;
}

Value FuncTypeAST::DumpKoopa()   {
    std::cout << "i32 ";
    return 0;
}

//
Value BlockAST::DumpKoopa()   {
    basic_block_num++;
    for (auto &blockitem:blockitems) {
        blockitem->DumpKoopa();
        
    }
    update();
    basic_block_num--;
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
        ValueData vd = ValueData(-1, "return", lhs_value, rhs_value);
        Value this_value = InsertValuedata(vd);
        return this_value;
    }
    else if (mode == 2) {
        ValueData vd = ValueData(-1, "return", 0, 0);
        Value this_value = InsertValuedata(vd);
        return this_value;
    }
    else if (mode == 3) { //lval = exp;
        //if lval是变量。
        //可是lval就是变量。
        auto vi = find_var_in_symbol_table(lval->ident);
        if (!(*vi).second.is_const_variable) {
            lval->ident_id = (*vi).first;
        }
        Value lhs_value = exp->DumpKoopa();
        Value rhs_value = 0;
        //不需要分配百分号的值。
        ValueData vd = ValueData(-1, "store", lhs_value, rhs_value, 0, lval->ident_id);
        //但是需要改动符号表中的值。
        change_varvalue_in_symbol_table(lval->ident_id, lhs_value);
        
        
        Value this_value = InsertValuedata(vd);
        return this_value;
    }
    else if (mode == 4) { // exp;
        //求值但被丢弃。
        exp->DumpKoopa();
    }
    else if (mode == 5) {
        return block->DumpKoopa();
    }
    else if (mode == 6) {
        return ifstmt->DumpKoopa();
    }
    else if (mode == 7) {
        return whilestmt->DumpKoopa();
    }
    else if (mode == 8) {
        if (!cur_loop_finished) {
            cur_loop_finished = true;
            ValueData break_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
            Value break_label_val = random();

            ValueData jump_vd1 = ValueData(-1, "jump", break_label_val, 0);
            InsertValuedata(jump_vd1);


            InsertValuedata(break_label_vd, break_label_val);

            //lhs是要跳的地方，rhs是要跳过接下来的语句，直到一个循环结束的标签。
            ValueData jump_vd2 = ValueData(-1, "break", loop_data[loop_data.size()-1].end_label_value, loop_data[loop_data.size()-1].jump_exp_label_value);
            InsertValuedata(jump_vd2);

            //loop_data.erase(loop_data.end()-1);
            loop_data.pop_back();
        }
        return 0;
    }
    else if (mode == 9) {
        if (!cur_loop_finished) {
            cur_loop_finished = true;
            ValueData ct_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
            Value ct_label_val = random();

            ValueData jump_vd1 = ValueData(-1, "jump", ct_label_val, 0);
            InsertValuedata(jump_vd1);

            InsertValuedata(ct_label_vd, ct_label_val);

            ValueData jump_vd2 = ValueData(-1, "continue", loop_data[loop_data.size()-1].exp_label_value, loop_data[loop_data.size()-1].jump_exp_label_value);
            InsertValuedata(jump_vd2);

            //不需要更新循环信息
            return 0;
        }
        
    }
    return 0;
}

//generate instruction
Value IfStmtAST::DumpKoopa() {
    //mode = 0; IF '(' exp ')' Stmt
    //mode = 1; IF '(' exp ')' Stmt ELSE Stmt
    if (mode == 0) {
        //exp
        //true_label
        //true_stmt
        //jump_end
        //end_label
        Value cond_value = exp->DumpKoopa();
        ValueData true_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
        Value true_label_val = random();
        ValueData end_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
        Value end_label_val = random();
        ValueData br_vd = ValueData(-1, "br", true_label_val, end_label_val, cond_value);
        InsertValuedata(br_vd);
        InsertValuedata(true_label_vd, true_label_val);
        stmt->DumpKoopa();
        ValueData jump_vd = ValueData(-1, "jump", end_label_val, 0);
        InsertValuedata(jump_vd);
        InsertValuedata(end_label_vd, end_label_val);
    }
    else if (mode == 1) {
        //exp
        //true_label
        //true_stmt
        //jump_end
        //false_label
        //false_stmt
        //jump_end
        //end_label
        Value cond_value = exp->DumpKoopa();
        ValueData true_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
        Value true_label_val = random();
        ValueData false_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
        Value false_label_val = random();
        ValueData end_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
        Value end_label_val = random();

        ValueData br_vd = ValueData(-1, "br", true_label_val, false_label_val, cond_value);
        InsertValuedata(br_vd);

        InsertValuedata(true_label_vd, true_label_val);
        
        stmt->DumpKoopa();
        
        ValueData jump_vd = ValueData(-1, "jump", end_label_val, 0);
        InsertValuedata(jump_vd);
        
        InsertValuedata(false_label_vd, false_label_val);
        
        elsestmt->DumpKoopa();
        
        ValueData jump_vd2 = ValueData(-1, "jump", end_label_val, 0);
        InsertValuedata(jump_vd2);
        
        InsertValuedata(end_label_vd, end_label_val);
    }
    return 0;
}

Value WhileStmtAST::DumpKoopa() {
    //照着上面if抄。
    //jump_exp_label
    //exp_label
    //exp
    //br 
    //true_label
    //true_stmt
    //jump jump_exp_label
    //jump_exp_label

    //jump exp_label
    //end_label
    cur_loop_finished = false;
    ValueData exp_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
    Value exp_label_val = random();
    ValueData true_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
    Value true_label_val = random();
    ValueData end_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
    Value end_label_val = random();
    ValueData jump_exp_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
    Value jump_exp_label_val = random();
    
    //更新循环信息。
    loop_data.push_back(LoopData(exp_label_val, true_label_val, jump_exp_label_val, end_label_val));
    
    ValueData jump_vd = ValueData(-1, "jump", exp_label_val, 0);
    InsertValuedata(jump_vd);
    InsertValuedata(exp_label_vd, exp_label_val);

    Value cond_value = exp->DumpKoopa();
    ValueData br_vd = ValueData(-1, "br", true_label_val, end_label_val, cond_value);
    InsertValuedata(br_vd);

    InsertValuedata(true_label_vd, true_label_val);

    stmt->DumpKoopa();

    //连在一起的！
    ValueData jump_vd1 = ValueData(-1, "jump", jump_exp_label_val, 0);
    InsertValuedata(jump_vd1);
    InsertValuedata(jump_exp_label_vd, jump_exp_label_val);

    ValueData jump_vd2 = ValueData(-1, "jump", exp_label_val, 0);
    InsertValuedata(jump_vd2);

    InsertValuedata(end_label_vd, end_label_val); 

    //恢复循环信息
    //loop_data.erase(loop_data.begin()+loop_data.size()-1);
    //?
    //loop_data.erase(loop_data.end()-1);
    loop_data.pop_back();
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
        //implement short-circuit logic and.
        /*int result = 0;
        if (lhs_value != 0) {
            result = rhs_value != 0;
        }*/

        //int result = 0;
        auto zero = new NumberAST();  
        zero->num = 0;

        auto vardef1 = new VarDefAST();
        vardef1->mode = 1;
        vardef1->ident = "__short_circuit_and_result";
        vardef1->initval = std::unique_ptr<BaseAST>(zero);
        
        auto vardecl = new VarDeclAST(); //是一个vardecl
        vardecl->vardefs.push_back(std::unique_ptr<BaseAST>(vardef1));

        //if (lhs_value != 0) 
        //    result = rhs_value != 0;
        //1
        auto one = new NumberAST();
        one->num = 1;

        //lhs_value == 1x
        //lhs_value != 0
        auto eqexp1 = new EqExpAST();
        eqexp1->mode = 2;
        eqexp1->eqexp = std::move(landexp);
        eqexp1->relexp = std::unique_ptr<BaseAST>(zero);

        //rhs_value != 0;
        auto eqexp2 = new EqExpAST();
        eqexp2->mode = 2;
        eqexp2->eqexp = std::move(eqexp);
        eqexp2->relexp = std::unique_ptr<BaseAST>(zero);

        //result = rhs_value != 0;
        auto lval = new LValAST();
        lval->ident = vardef1->ident;
        auto stmt = new StmtAST();
        stmt->mode = 3;
        stmt->lval = std::unique_ptr<BaseAST>(lval);
        stmt->exp = std::unique_ptr<BaseAST>(eqexp2);

        //if
        auto ifstmt = new IfStmtAST();
        ifstmt->mode = 0;
        ifstmt->exp = std::unique_ptr<BaseAST>(eqexp1);
        ifstmt->stmt = std::unique_ptr<BaseAST>(stmt);

        vardecl->DumpKoopa();
        ifstmt->DumpKoopa();

        //返回__short_circuit_result的值.
        auto vi = find_var_in_symbol_table(vardef1->ident);
        ident_id = (*vi).first;
        Value lhs_value = (*vi).second.value;
        Value rhs_value = 0;
    
        if (!(*vi).second.is_const_variable) {
            //变量
             ValueData vd = AllocateValueData("load", lhs_value, rhs_value, 0, ident_id);
             Value this_value = InsertValuedata(vd);
             return this_value;   
        }
        return 0;
        //按位与或实现逻辑与或。
        //%n = eq lhs, 0;    -> vd_l
        //%n+1 = eq rhs, 0;  -> vd_r
        //%n+2 = and %n, %n+1;
        //为数字0, vd_l, vd_r分配一个数据结构。采用随机数作为value.
        //Value lhs_value = landexp->DumpKoopa();
        //Value rhs_value = eqexp->DumpKoopa();
        //ValueData vd_zero = AllocateValueData("number", 0, 0);
        //Value vd_zero_value = InsertValuedata(vd_zero);
        //ValueData vd_l = AllocateValueData("ne", lhs_value, vd_zero_value);
        //Value vd_l_value = InsertValuedata(vd_l);
        //ValueData vd_r = AllocateValueData("ne", rhs_value, vd_zero_value);
        //Value vd_r_value = InsertValuedata(vd_r);
        //ValueData vd = AllocateValueData("and", vd_l_value, vd_r_value);
        //Value this_value = InsertValuedata(vd);
        //return this_value;        
    }
}


Value LOrExpAST::DumpKoopa() {
    //按位与或实现逻辑与或。
    //%n = eq lhs, 0;    -> vd_l
    //%n+1 = eq rhs, 0;  -> vd_r
    //%n+2 = or %n, %n+1;
    if (mode == 0) {
        return landexp->DumpKoopa();
    }
    else {
        //implement short-circuit logic or.
        /*int result = 1;
        if (lhs_value == 0) {
            result = rhs_value != 0;
        }*/

        //int result = 1;
        auto one = new NumberAST();  
        one->num = 1;

        auto vardef1 = new VarDefAST();
        vardef1->mode = 1;
        vardef1->ident = "__short_circuit_or_result";
        vardef1->initval = std::unique_ptr<BaseAST>(one);
        
        auto vardecl = new VarDeclAST(); //是一个vardecl
        vardecl->vardefs.push_back(std::unique_ptr<BaseAST>(vardef1));

        //if (lhs_value == 0)result = rhs_value != 0;
        //0
        auto zero = new NumberAST();
        zero->num = 0;

        //lhs_value == 0
        auto eqexp1 = new EqExpAST();
        eqexp1->mode = 1;
        eqexp1->eqexp = std::move(lorexp);
        eqexp1->relexp = std::unique_ptr<BaseAST>(zero);

        //rhs_value != 0;
        auto eqexp2 = new EqExpAST();
        eqexp2->mode = 2;
        eqexp2->eqexp = std::move(landexp);
        eqexp2->relexp = std::unique_ptr<BaseAST>(zero);

        //result = rhs_value != 0;
        auto lval = new LValAST();
        lval->ident = vardef1->ident;
        auto stmt = new StmtAST();
        stmt->mode = 3;
        stmt->lval = std::unique_ptr<BaseAST>(lval);
        stmt->exp = std::unique_ptr<BaseAST>(eqexp2);
        
        //if
        auto ifstmt = new IfStmtAST();
        ifstmt->mode = 0;
        ifstmt->exp = std::unique_ptr<BaseAST>(eqexp1);
        ifstmt->stmt = std::unique_ptr<BaseAST>(stmt);

        vardecl->DumpKoopa();
        ifstmt->DumpKoopa();
        
        //返回__short_circuit_result的值.
        auto vi = find_var_in_symbol_table(vardef1->ident);
        ident_id = (*vi).first;
        Value lhs_value = (*vi).second.value;
        Value rhs_value = 0;
    
        if (!(*vi).second.is_const_variable) {
            //变量
             ValueData vd = AllocateValueData("load", lhs_value, rhs_value, 0, ident_id);
             Value this_value = InsertValuedata(vd);
             return this_value;   
        }
        return 0;
        //Value lhs_value = lorexp->DumpKoopa();
        //Value rhs_value = landexp->DumpKoopa();
        //为数字0, vd_l, vd_r分配一个数据结构。采用随机数作为value.
        //ValueData vd_zero = AllocateValueData("number", 0, 0);
        //Value vd_zero_value = InsertValuedata(vd_zero);
        //ValueData vd_l = AllocateValueData("ne", lhs_value, vd_zero_value);
        //Value vd_l_value = InsertValuedata(vd_l);
        //ValueData vd_r = AllocateValueData("ne", rhs_value, vd_zero_value);
        //Value vd_r_value = InsertValuedata(vd_r);
        //ValueData vd = AllocateValueData("or", vd_l_value, vd_r_value);
        //Value this_value = InsertValuedata(vd);
        //return this_value;        
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
    //处理同名变量
    auto iter = varname_cnt[function_num].find(ident);
    if (iter == varname_cnt[function_num].end()) {
        varname_cnt[function_num].insert(std::make_pair(ident, 1));
        ident_id = ident +'_' + std::to_string(1);
    }
    else {
        varname_cnt[function_num][ident]++;
        ident_id = ident + '_' + std::to_string(varname_cnt[function_num][ident]);
    }
    //将符号插入到符号表中。
    symbol_table[function_num][basic_block_num].insert(std::make_pair(ident_id,
                                                     VariableInfo(exp_value, true)));
    return 0;
}

Value VarDefAST::DumpKoopa() {
    //分成三个指令
    //alloc
    //计算exp的值
    //store
    
    //处理同名变量
    auto iter = varname_cnt[function_num].find(ident);
    if (iter == varname_cnt[function_num].end()) {
        varname_cnt[function_num].insert(std::make_pair(ident, 1));
        ident_id = ident +'_' + std::to_string(1);
    }
    else {
        varname_cnt[function_num][ident]++;
        ident_id = ident + '_' + std::to_string(varname_cnt[function_num][ident]);
    }



    ValueData vd_alloc = ValueData(-1, "alloc", 0, 0, 0,ident_id);
    InsertValuedata(vd_alloc);
    if (mode == 0) { //无赋值
        symbol_table[function_num][basic_block_num].insert(std::make_pair(ident_id,
                                                     VariableInfo(0, false)));
    }
    else {
        Value lhs = initval->DumpKoopa();
        ValueData vd_store = ValueData(-1, "store", lhs, 0, 0,ident_id);
        InsertValuedata(vd_store);
        symbol_table[function_num][basic_block_num].insert(std::make_pair(ident_id,
                                                     VariableInfo(lhs, false)));
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
    auto vi = find_var_in_symbol_table(ident);
    ident_id = (*vi).first;
    Value lhs_value = (*vi).second.value;
    Value rhs_value = 0;

    if (!(*vi).second.is_const_variable) {
        //变量
         ValueData vd = AllocateValueData("load", lhs_value, rhs_value, 0, (*vi).first);
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

std::unordered_map<std::string, VariableInfo>::iterator find_var_in_symbol_table(std::string& ident) {
    std::unordered_map<std::string, VariableInfo>::iterator it;
    int num = varname_cnt[function_num][ident];
    //倒着查。
    //最近的定义标号较大。
    for (int i = num; i >= 1; --i) {
        for (int j = basic_block_num; j >= 0; --j) {
            std::string s = ident + "_" + std::to_string(i);
            auto vi = symbol_table[function_num][j].find(s);
            if (vi != symbol_table[function_num][j].end())
                return vi;
        }
    }
    //never reached
    return it;
}

void change_varvalue_in_symbol_table(std::string &var_name, Value cur_value) {
    for (int j = basic_block_num; j >= 0; --j) {
        auto vi = symbol_table[function_num][j].find(var_name);
        if (vi != symbol_table[function_num][j].end()) {
            symbol_table[function_num][j][var_name].value = cur_value;
        }
    }
}

void update() {
    symbol_table[function_num].erase(symbol_table[function_num].begin() + basic_block_num);
}