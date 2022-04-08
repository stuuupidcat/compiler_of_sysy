#include "AST.h"

//当前临时标号的个数。
std::vector<int> temp_sign_num;


//函数的个数，搭配符号表使用。
int scope_num = 0;

//当前基本块的个数。
std::vector<int> basic_block_num;

//label的个数;
int label_num = 0;

std::unordered_map<Value, ValueData> block_insts;
std::unordered_map<Value, ValueData> all_insts;
std::vector<Value> block_values;
std::vector<Value> all_values;


//符号表。下标为当前的(scope_num, block_num)
std::vector<std::vector<std::unordered_map<std::string, SymbolInfo>>> symbol_table; 
//处理同名变量。下标为scope_num
std::vector<std::unordered_map<std::string, int>> symbolname_cnt;

//循环的信息。
std::vector<LoopData> loop_data;
std::vector<std::vector<bool>> loop_broken_or_continued;

//编译期求值
bool print_ins = true;
//需要输出一个标签
bool need_label = false;

void InsertValueDataToBlock(ValueData valuedata, Value allocated_val) {
    if (print_ins) {
        block_insts.insert(std::make_pair(allocated_val, valuedata));
        block_values.push_back(allocated_val);
        all_insts.insert(std::make_pair(allocated_val, valuedata));
        all_values.push_back(allocated_val);
    }
}

//To block之前必须toall
Value InsertValueDataToAll(ValueData valuedata) {
    Value val = random();
    all_insts.insert(std::make_pair(val, valuedata));
    all_values.push_back(val);
    return val;
}
LoopData::LoopData(Value exp_label_value_, Value true_label_value_, Value jump_exp_label_value_, Value end_label_value_) {
    exp_label_value = exp_label_value_;
    true_label_value = true_label_value_;
    jump_exp_label_value = jump_exp_label_value_;
    end_label_value = end_label_value_;
}

void PrintInstruction() {
    int instruction_num = block_values.size();
    int i = 0;
    if (need_label) {
        for (; i < instruction_num; ++i) {
            auto vd = block_insts[block_values[i]];
            if (vd.inst_type == "label") {
                //std::cout << vd.format() << std::endl;
                need_label = false;
                break;
            }
        }
    }
    
    for (; i < instruction_num; ++i) {
        auto vd = block_insts[block_values[i]];
        if (vd.inst_type == "return" || vd.inst_type == "break" || vd.inst_type == "continue") {     
            std::cout << "  "  << vd.format();    
            //while (vd.inst_type != "label" && i < instruction_num) {
            //    i++;
            //    vd = block_insts[block_values[i]];
            //}
            //改一点点就报错x
            //i = i - 1;
            //continue;
            //return;
            need_label = true;
            break;
        }
        //要调到循环结尾的jump_exp_label 不用
        if (vd.inst_type == "label") {
            std::cout << vd.format();
        }
        else if (vd.inst_type == "globalalloc") {
            std::cout << vd.format();
        }
        else if (vd.inst_type != "number" && vd.inst_type != "lval")
            std::cout << "  " <<vd.format();
    }
    block_insts.erase(block_insts.begin(), block_insts.end());
    block_values.erase(block_values.begin(), block_values.end());
}

SymbolInfo::SymbolInfo(Value value_, int exp_val_, bool is_const_, bool is_func_, bool is_void_) {
    value = value_;
    exp_val = exp_val_;
    is_const_variable = is_const_;
    is_function = is_func_;
    is_void_function = is_void_;
}


std::string ValueData::format() {
    std::string res;
    ValueData lhs_vd, rhs_vd, jump_cond_vd;
    SymbolInfo vi;

    //指令类型为number并不需要输出，将number返回即可。
    //变量也是number，但要返回name
    if (inst_type == "number") {
        if (symbol_name != "") {
            return "@" + symbol_name;
        }
        else {
            res = std::to_string((long)lhs);
            return res;
        }
    }
    
    //寻找对应的ValueData。
    //向外寻找
    if (lhs != 0) {
        lhs_vd = all_insts[lhs];
    }
    if (rhs != 0) {
        rhs_vd = all_insts[rhs];
    }
    if (jump_cond != 0) {
        jump_cond_vd = all_insts[jump_cond];
    }
    if (symbol_name != "") {
        for (int i = basic_block_num[scope_num]; i>=0; --i) {
            auto iter = symbol_table[scope_num][i].find(symbol_name);
            if (iter != symbol_table[scope_num][i].end()) {
                vi = (*iter).second;
            }
        }

        auto iter = symbol_table[0][0].find(symbol_name);
        if (iter != symbol_table[0][0].end()) {
            vi = (*iter).second;
        }
    }
    //if (inst_type == "call") {
    //    auto iter = symbol_table[0][0].find(symbol_name);
    //    if (iter != symbol_table[0][0].end()) {
    //        vi = (*iter).second;
    //    }
    //}

    /////////////////////////////////////////////////////////////////////////////////
    if (inst_type == "call") {
        if (no == -1) {
            res = "call @" + symbol_name + "(";
        }
        else {
            res = "%" + std::to_string(no) + " = " + "call @" + symbol_name + "(";
        }
        int cnt = 0;
        for (auto val: parameters) {
            if (cnt >= 1) {res += ", ";}
            ValueData param_vd = all_insts[val];
            if (param_vd.inst_type == "number" || param_vd.inst_type == "lval") {
                res += param_vd.format();
            }
            else {
                res += "%" + std::to_string(param_vd.no);
            }
            cnt++;
        }
        res += ")\n";
    }

    //运算符为一元操作, lhs为0。
    else if (inst_type.find("single") != std::string::npos) {
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
            res = std::to_string(vi.exp_val);
        }
    }

    //运算符为return操作。
    else if (inst_type == "return") {
        res = "ret ";
        if (lhs != 0) {
            if (lhs_vd.inst_type == "number" || lhs_vd.inst_type == "lval") {
                res += lhs_vd.format();
            }
            else {
                res += "%" + std::to_string(lhs_vd.no);
            }
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
        res = "@" + symbol_name + " = alloc i32\n";
    }
    else if (inst_type == "globalalloc") {
        res = "global @" + symbol_name + " = alloc i32, " + std::to_string(initializer) + "\n";
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
        res += "@" + symbol_name + "\n";
    }
    //载入一个变量
    else if (inst_type == "load") {
        res = "%" + std::to_string(no) + " = load @" + symbol_name + "\n";
    }
    else if (inst_type == "br") {
        if (jump_cond_vd.inst_type == "number" || jump_cond_vd.inst_type == "lval") {
            res = "br " + jump_cond_vd.format() + ", " + lhs_vd.symbol_name + ", " + rhs_vd.symbol_name + "\n";
        }
        else {
            res = "br %" + std::to_string(jump_cond_vd.no) + ", " + lhs_vd.symbol_name + ", " + rhs_vd.symbol_name + "\n";
        }
    }
    else if (inst_type == "jump" || inst_type == "break" || inst_type == "continue") {
        res = "jump " + lhs_vd.symbol_name + "\n";
    }
    else if (inst_type == "label") {
        res = symbol_name + ":\n";
    }
    return res;
}

ValueData AllocateValueData(std::string inst_type_, Value lhs_, Value rhs_, Value jump_cond_ = 0, std::string name_="", int initializer_=0) {
    ValueData vd = {temp_sign_num[scope_num], inst_type_, lhs_, rhs_, jump_cond_, name_, initializer_};
    if (print_ins)
        temp_sign_num[scope_num]++;
    return vd;
}

ValueData::ValueData(int no_, std::string inst_type_, Value lhs_, Value rhs_, Value jump_cond_ = 0, std::string symbol_name_ = "", int initializer_ = 0) {
    no = no_;
    inst_type = inst_type_;
    lhs = lhs_;
    rhs = rhs_;
    jump_cond = jump_cond_;
    symbol_name = symbol_name_;
    initializer = initializer_;
}


Value CompUnitAST::DumpKoopa()  {
    std::cout << "decl @getint(): i32\n";
    std::cout << "decl @getch(): i32\n";
    std::cout << "decl @getarray(*i32): i32\n";
    std::cout << "decl @putint(i32)\n";
    std::cout << "decl @putch(i32)\n";
    std::cout << "decl @putarray(i32, *i32)\n";
    std::cout << "decl @starttime()\n";
    std::cout << "decl @stoptime()\n\n";



    symbol_table.resize(100);
    for (auto &val: symbol_table)
        val.resize(100);

    loop_broken_or_continued.resize(100);
    for (auto &val: loop_broken_or_continued)
        val.resize(100);
    symbolname_cnt.resize(100);

    basic_block_num.resize(100);
    for (auto &val: basic_block_num)
        val = 0;
    
    temp_sign_num.resize(100);
    for (auto &val: temp_sign_num)
        val = 0;
    
    for (auto& decl: decls) {
        print_ins = false;
        decl->DumpKoopa();
    }
    print_ins = true;
    PrintInstruction();
    //函数名字插入到符号表中。
    for (auto &func: funcdefs) {
        if (func->ident == "main") {
            func->ident_id = func->ident;
        } else {
            func->ident_id = func->ident + '_' + "function";
        }
        int bb_num = basic_block_num[scope_num];

        //表明是否是函数，如果是是否为void
        if (func->mode == 0 || func->mode == 1) {
            symbol_table[scope_num][bb_num].insert(std::make_pair(func->ident_id,
                                                     SymbolInfo(0, 0, false, true, true)));
        }

        else {
            symbol_table[scope_num][bb_num].insert(std::make_pair(func->ident_id,
                                                     SymbolInfo(0, 0, false, true, false)));
        }
        
    }
    for (auto& func: funcdefs) {
        scope_num++;
        func->DumpKoopa();
    }
    return 0;
}

Value FuncDefAST::DumpKoopa()   {
    std::cout << "fun ";
    std::cout << "@" << ident_id;

    //函数参数。
    std::cout << '(';
    if (mode == 1 || mode == 3) {
        funcfparams->DumpKoopa();
    }
    std::cout << ')';

    //void
    if (mode == 0 || mode == 1) {

    }
    //int
    else {  
        std::cout << ": i32";
    } 
    
    std::cout << " {" << std::endl;
    std::cout << "%entry:" << std::endl;
    need_label = false;
    for (auto& vardecl: vardecls) {
        vardecl->DumpKoopa();
    }
    PrintInstruction();

    block -> DumpKoopa();
    ValueData vd = ValueData(-1, "return", 0, 0, 0, "", 0);
    Value val = InsertValueDataToAll(vd);
    InsertValueDataToBlock(vd, val);
    PrintInstruction();
    std::cout << '}' << std::endl << std::endl;
    return 0;
}

//
Value BlockAST::DumpKoopa()   {
    enter_block();
    int len = blockitems.size();
    for (int i = 0; i < len; ++i) {
        blockitems[i]->DumpKoopa();
    }
    leave_block();
    return 0;
}

Value FuncFParamsAST::DumpKoopa()   {
    int cnt = 0;
    for (auto &param:funcfparams) {
        if (cnt != 0) {
            std::cout << ", ";
        }
        param->DumpKoopa();
        cnt++;

    }
    return 0;
}

Value FuncFParamAST::DumpKoopa()   {
    //将符号插入到符号表中。
    //不用 本质上是数字
                
    std::cout << "@" << ident << " :i32";
    return 0;
}

Value FuncRParamsAST::DumpKoopa()   {
    std::vector<Value> res;
    for (auto &exp: exps) {
        exp->DumpKoopa();
    }
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
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
    }
    else if (mode == 2) {
        ValueData vd = ValueData(-1, "return", 0, 0);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
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
        
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
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
        if (!loop_broken_or_continued[scope_num][basic_block_num[scope_num]]) {
            loop_broken_or_continued[scope_num][basic_block_num[scope_num]] = true;
            ValueData break_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
            Value break_label_val = InsertValueDataToAll(break_label_vd);

            ValueData jump_vd1 = ValueData(-1, "jump", break_label_val, 0);
            Value jump_vd1_val = InsertValueDataToAll(jump_vd1);

            InsertValueDataToBlock(jump_vd1, jump_vd1_val);
            InsertValueDataToBlock(break_label_vd, break_label_val);

            //lhs是要跳的地方，rhs是要跳过接下来的语句，直到一个循环结束的标签。
            ValueData jump_vd2 = ValueData(-1, "break", loop_data[loop_data.size()-1].end_label_value, loop_data[loop_data.size()-1].jump_exp_label_value);
            Value jump_vd2_val = InsertValueDataToAll(jump_vd2);
            InsertValueDataToBlock(jump_vd2, jump_vd2_val);

            //loop_data.erase(loop_data.end()-1);
            
            //test
            //loop_data.pop_back();
        }
        return 0;
    }
    else if (mode == 9) {
        if (!loop_broken_or_continued[scope_num][basic_block_num[scope_num]]) {
            loop_broken_or_continued[scope_num][basic_block_num[scope_num]] = true;
            ValueData ct_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
            Value ct_label_val = InsertValueDataToAll(ct_label_vd);

            ValueData jump_vd1 = ValueData(-1, "jump", ct_label_val, 0);
            Value jump_vd1_val = InsertValueDataToAll(jump_vd1);
            InsertValueDataToBlock(jump_vd1, jump_vd1_val);

            InsertValueDataToBlock(ct_label_vd, ct_label_val);

            ValueData jump_vd2 = ValueData(-1, "continue", loop_data[loop_data.size()-1].exp_label_value, loop_data[loop_data.size()-1].jump_exp_label_value);
            Value jump_vd2_val = InsertValueDataToAll(jump_vd2);
            InsertValueDataToBlock(jump_vd2, jump_vd2_val);

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
        Value true_label_val = InsertValueDataToAll(true_label_vd);
        ValueData end_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
        Value end_label_val = InsertValueDataToAll(end_label_vd);
        ValueData br_vd = ValueData(-1, "br", true_label_val, end_label_val, cond_value);
        Value br_val = InsertValueDataToAll(br_vd);
        InsertValueDataToBlock(br_vd, br_val);
        InsertValueDataToBlock(true_label_vd, true_label_val);
        
        if (stmt->mode != 5) {
            enter_block();
            stmt->DumpKoopa();
            leave_block();
        }
        else {
            stmt->DumpKoopa();
        }
        ValueData jump_vd = ValueData(-1, "jump", end_label_val, 0);
        Value jump_val = InsertValueDataToAll(jump_vd);
        InsertValueDataToBlock(jump_vd, jump_val);
        InsertValueDataToBlock(end_label_vd, end_label_val);
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
        Value true_label_val = InsertValueDataToAll(true_label_vd);
        ValueData false_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
        Value false_label_val = InsertValueDataToAll(false_label_vd);
        ValueData end_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
        Value end_label_val = InsertValueDataToAll(end_label_vd);

        ValueData br_vd = ValueData(-1, "br", true_label_val, false_label_val, cond_value);
        Value br_vd_val = InsertValueDataToAll(br_vd);
        InsertValueDataToBlock(br_vd, br_vd_val);

        InsertValueDataToBlock(true_label_vd, true_label_val);
        
        if (stmt->mode != 5) {
            enter_block();
            stmt->DumpKoopa();
            leave_block();
        }
        else {
            stmt->DumpKoopa();
        }

        ValueData jump_vd = ValueData(-1, "jump", end_label_val, 0);
        Value jump_vd_val = InsertValueDataToAll(jump_vd);
        InsertValueDataToBlock(jump_vd, jump_vd_val);
        
        InsertValueDataToBlock(false_label_vd, false_label_val);
        
        if (elsestmt->mode != 5) {
            enter_block();
            elsestmt->DumpKoopa();
            leave_block();
        }
        else {
            elsestmt->DumpKoopa();
        }

        ValueData jump_vd2 = ValueData(-1, "jump", end_label_val, 0);
        Value jump_vd2_val = InsertValueDataToAll(jump_vd2);
        InsertValueDataToBlock(jump_vd2, jump_vd2_val);
        
        InsertValueDataToBlock(end_label_vd, end_label_val);
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
    
    ValueData exp_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
    Value exp_label_val = InsertValueDataToAll(exp_label_vd);
    ValueData true_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
    Value true_label_val = InsertValueDataToAll(true_label_vd);
    ValueData jump_exp_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
    Value jump_exp_label_val = InsertValueDataToAll(jump_exp_label_vd);
    ValueData end_label_vd = ValueData(-1, "label", 0, 0, 0, "%L"+std::to_string(label_num++));
    Value end_label_val = InsertValueDataToAll(end_label_vd);
    
    
    //更新循环信息。
    loop_data.push_back(LoopData(exp_label_val, true_label_val, jump_exp_label_val, end_label_val));
    
    ValueData jump_vd = ValueData(-1, "jump", exp_label_val, 0);
    Value jump_vd_val = InsertValueDataToAll(jump_vd);
    InsertValueDataToBlock(jump_vd, jump_vd_val);
    InsertValueDataToBlock(exp_label_vd, exp_label_val);

    Value cond_value = exp->DumpKoopa();
    ValueData br_vd = ValueData(-1, "br", true_label_val, end_label_val, cond_value);
    Value br_vd_val = InsertValueDataToAll(br_vd);
    InsertValueDataToBlock(br_vd, br_vd_val);

    InsertValueDataToBlock(true_label_vd, true_label_val);

    if (stmt->mode != 5) {
        enter_block();
        stmt->DumpKoopa();
        leave_block();
    }
    else {
        stmt->DumpKoopa();
    }

    //连在一起的！
    ValueData jump_vd1 = ValueData(-1, "jump", jump_exp_label_val, 0);
    Value jump_vd1_val = InsertValueDataToAll(jump_vd1);
    InsertValueDataToBlock(jump_vd1, jump_vd1_val);

    InsertValueDataToBlock(jump_exp_label_vd, jump_exp_label_val);

    ValueData jump_vd2 = ValueData(-1, "jump", exp_label_val, 0);
    Value jump_vd2_val = InsertValueDataToAll(jump_vd2);
    InsertValueDataToBlock(jump_vd2, jump_vd2_val);

    InsertValueDataToBlock(end_label_vd, end_label_val); 

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

    exp_val = num;
    //不需要分配新的值。
    ValueData vd = ValueData(-1, "number", lhs_value, rhs_value, 0, ident);
    Value val = InsertValueDataToAll(vd);
    InsertValueDataToBlock(vd, val);
    return val;
}


Value ExpAST::DumpKoopa()  {
    auto val = lorexp -> DumpKoopa();
    exp_val = lorexp->exp_val;
    return val;
}


Value UnaryExpAST::DumpKoopa() {
    if (mode == 0) {
        auto val = primaryexp->DumpKoopa();
        exp_val = primaryexp->exp_val;
        return val;
    }
    else if (mode == 1) {//?
        std::string ops[3] = {"singleadd", "singlesub", "singleeq"};

        Value lhs_value = 0;
        Value rhs_value = unaryexp->DumpKoopa();

        if (unaryop->mode == 0) {
            exp_val = unaryexp->exp_val;
        }
        else if (unaryop->mode == 1) {
            exp_val = -(unaryexp->exp_val);
        }
        else if (unaryop->mode == 2) {
            exp_val = !(unaryexp->exp_val);
            //std::cout << "here!" << exp_val;
        }

        ValueData vd = AllocateValueData(ops[unaryop->mode], lhs_value, rhs_value);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
    }
    else if (mode == 2) {
        if (ident == "getint" || ident == "getch") {
            ValueData vd = AllocateValueData("call", 0, 0, 0, ident);
            Value val = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, val);
            return val;
        }
        if (ident == "starttime" || ident == "stoptime") {
            ValueData vd = ValueData(-1, "call", 0, 0, 0, ident);
            Value val = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, val);
            return val;
        }


        std::string func_name = ident + "_function";
        SymbolInfo symbol_info = symbol_table[0][0][func_name];
        ValueData vd;
        if (symbol_info.is_void_function) {
            vd = ValueData(-1, "call", 0, 0, 0, func_name);
        }
        else {
            vd = AllocateValueData("call", 0, 0, 0, func_name);
        }
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
    }
    else if (mode == 3) {
        if (ident == "getarray") {
            std::vector<Value> temp_vec;
            for (auto& exp: funcrparams->exps) {
                temp_vec.push_back(exp->DumpKoopa());
            }
            ValueData vd = ValueData(-1, "call", 0, 0, 0, ident);
            vd.parameters = temp_vec;
            Value val = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, val);
            return val;
        }

        if (ident == "putint" || ident == "putch" || ident == "putarray") {
            std::vector<Value> temp_vec;
            for (auto& exp: funcrparams->exps) {
                temp_vec.push_back(exp->DumpKoopa());
            }
            ValueData vd = ValueData(-1, "call", 0, 0, 0, ident);
            vd.parameters = temp_vec;
            Value val = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, val);
            return val;
        }

        std::string func_name = ident + "_function";
        SymbolInfo symbol_info = symbol_table[0][0][func_name];
        ValueData vd;
        //vd.parameters = funcrparams->DumpKoopa();
        std::vector<Value> temp_vec;
        for (auto& exp: funcrparams->exps) {
            temp_vec.push_back(exp->DumpKoopa());
        }
        if (symbol_info.is_void_function) {
            vd = ValueData(-1, "call", 0, 0, 0, func_name);
        }
        else {
            vd = AllocateValueData("call", 0, 0, 0, func_name);
        }
        vd.parameters = temp_vec;
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
    }
    //never reached.
    
    return 0;
}


Value PrimaryExpAST::DumpKoopa()  {
    Value val = 0;
    if (mode == 0) {
        val = exp->DumpKoopa();
        exp_val = exp->exp_val;
    }
    else if (mode == 1) {
        val = lval->DumpKoopa();
        exp_val = lval->exp_val;
    }
    else if (mode == 2) {
        val = number->DumpKoopa();
        exp_val = number->exp_val;
    }

    return val;
}


Value UnaryOpAST::DumpKoopa()  {
    return 0;
}


Value MulExpAST::DumpKoopa()  {
    if (mode == 0) {
        auto val = unaryexp->DumpKoopa();
        exp_val = unaryexp->exp_val;
        return val;
    }
    else {
        std::string ops[4] = {"", "mul", "div", "mod"};

        Value lhs_value = mulexp->DumpKoopa();
        Value rhs_value = unaryexp->DumpKoopa();

        if (mode == 1) {
            exp_val = mulexp->exp_val * unaryexp->exp_val;
        }
        else if (mode == 2) {
            if (exp_val != 0) {
                exp_val = mulexp->exp_val / unaryexp->exp_val;
            }
            else {
                exp_val = 0;
            }
            //exp_val = mulexp->exp_val / unaryexp->exp_val;
        }
        else if (mode == 3) {
            if (exp_val != 0) {
                exp_val = mulexp->exp_val % unaryexp->exp_val;
            }
            else {
                exp_val = 0;
            }
            //exp_val = mulexp->exp_val % unaryexp->exp_val;
        }

        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
    }
}


Value AddExpAST::DumpKoopa()  {
    if (mode == 0) {
        auto val = mulexp -> DumpKoopa();
        exp_val = mulexp->exp_val;
        return val;
    }
    else {
        std::string ops[4] = {"", "add", "sub"};
        Value lhs_value = addexp->DumpKoopa();
        Value rhs_value = mulexp->DumpKoopa();
        if (mode == 1) {
            exp_val = addexp->exp_val + mulexp->exp_val;
        }
        else {
            exp_val = addexp->exp_val - mulexp->exp_val;
        }
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
    }
}

Value RelExpAST::DumpKoopa() {
    if (mode == 0) {
        auto val = addexp->DumpKoopa();
        exp_val = addexp->exp_val;
        return val;
    }
    else {
        std::string ops[5] = {"", "lt", "gt", "le", "ge"};
        Value lhs_value = relexp->DumpKoopa();
        Value rhs_value = addexp->DumpKoopa();

        if (mode == 1) {
            exp_val = (relexp->exp_val < addexp->exp_val);
        }
        else if (mode == 2) {
            exp_val = (relexp->exp_val > addexp->exp_val);
        }
        else if (mode == 3) {
            exp_val = (relexp->exp_val <= addexp->exp_val);
        }
        else if (mode == 4) {
            exp_val = (relexp->exp_val >= addexp->exp_val);
        }

        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;  
    }
}

Value EqExpAST::DumpKoopa() {
    if (mode == 0) {
       auto val = relexp->DumpKoopa();
       exp_val = relexp->exp_val;
       return val;
    }
    else {
        std::string ops[5] = {"", "eq", "ne"};
        Value lhs_value = eqexp->DumpKoopa();
        Value rhs_value = relexp->DumpKoopa();
        
        if (mode == 1) {
            exp_val = (eqexp->exp_val == relexp->exp_val);
        }
        else {
            exp_val = (eqexp->exp_val != relexp->exp_val);
        }

        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;       
    }
}

Value LAndExpAST::DumpKoopa() {
    if (mode == 0) {
        auto val = eqexp->DumpKoopa();
        exp_val = eqexp->exp_val;
        return val;
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

        //short circuit logic and.
        exp_val = 0;
        if (eqexp1->eqexp->exp_val != 0) {
            exp_val = eqexp2->eqexp->exp_val != 0;
        }

        //返回__short_circuit_result的值.
        auto vi = find_var_in_symbol_table(vardef1->ident);
        ident_id = (*vi).first;
        Value lhs_value = (*vi).second.value;
        Value rhs_value = 0;
    
        if (!(*vi).second.is_const_variable) {
            //变量
            ValueData vd = AllocateValueData("load", lhs_value, rhs_value, 0, ident_id);
            Value val = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, val);
            return val;
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
        //Value vd_zero_value = InsertValueDataToBlock(vd_zero);
        //ValueData vd_l = AllocateValueData("ne", lhs_value, vd_zero_value);
        //Value vd_l_value = InsertValueDataToBlock(vd_l);
        //ValueData vd_r = AllocateValueData("ne", rhs_value, vd_zero_value);
        //Value vd_r_value = InsertValueDataToBlock(vd_r);
        //ValueData vd = AllocateValueData("and", vd_l_value, vd_r_value);
        //Value this_value = InsertValueDataToBlock(vd);
        //return this_value;        
    }
}


Value LOrExpAST::DumpKoopa() {
    //按位与或实现逻辑与或。
    //%n = eq lhs, 0;    -> vd_l
    //%n+1 = eq rhs, 0;  -> vd_r
    //%n+2 = or %n, %n+1;
    if (mode == 0) {
        auto val = landexp->DumpKoopa();
        exp_val = landexp->exp_val;
        return val;
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


        //short circuit logic or.
        exp_val = 1;
        if (eqexp1->eqexp->exp_val == 0) {
            exp_val = eqexp2->eqexp->exp_val != 0;
        }
        
        //返回__short_circuit_result的值.
        auto vi = find_var_in_symbol_table(vardef1->ident);
        ident_id = (*vi).first;
        Value lhs_value = (*vi).second.value;
        Value rhs_value = 0;
    
        if (!(*vi).second.is_const_variable) {
            //变量
            ValueData vd = AllocateValueData("load", lhs_value, rhs_value, 0, ident_id);
            Value val = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, val);
            return val;   
        }
        return 0;
        //Value lhs_value = lorexp->DumpKoopa();
        //Value rhs_value = landexp->DumpKoopa();
        //为数字0, vd_l, vd_r分配一个数据结构。采用随机数作为value.
        //ValueData vd_zero = AllocateValueData("number", 0, 0);
        //Value vd_zero_value = InsertValueDataToBlock(vd_zero);
        //ValueData vd_l = AllocateValueData("ne", lhs_value, vd_zero_value);
        //Value vd_l_value = InsertValueDataToBlock(vd_l);
        //ValueData vd_r = AllocateValueData("ne", rhs_value, vd_zero_value);
        //Value vd_r_value = InsertValueDataToBlock(vd_r);
        //ValueData vd = AllocateValueData("or", vd_l_value, vd_r_value);
        //Value this_value = InsertValueDataToBlock(vd);
        //return this_value;        
    }
}

/////////////////////////////////////////////
Value DeclAST::DumpKoopa() {
    Value val = 0;
    if (mode == 0)
        val = constdecl->DumpKoopa();
    else 
        val = vardecl->DumpKoopa();
    return val;
}

Value ConstDeclAST::DumpKoopa() {
    for (auto& constdef: constdefs) {
        //print_ins = false;
        constdef->DumpKoopa();
    }
    //print_ins = true;
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
    //处理局部同名变量
    if (scope_num != 0) {
        auto iter = symbolname_cnt[scope_num].find(ident);
        if (iter == symbolname_cnt[scope_num].end()) {
            symbolname_cnt[scope_num].insert(std::make_pair(ident, 1));
            ident_id = ident +'_' + std::to_string(1);
        }
        else {
            symbolname_cnt[scope_num][ident]++;
            ident_id = ident + '_' + std::to_string(symbolname_cnt[scope_num][ident]);
        }
    }
    else {
        ident_id = ident + '_' + "global";
    }
   
    //将符号插入到符号表中。
    symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                     SymbolInfo(exp_value, constinitval->exp_val, true, false, false)));
    return 0;
}

Value VarDefAST::DumpKoopa() {
    //分成三个指令
    //alloc
    //计算exp的值
    //store
    //处理同名变量
    if (scope_num != 0) {
        auto iter = symbolname_cnt[scope_num].find(ident);
        if (iter == symbolname_cnt[scope_num].end()) {
            symbolname_cnt[scope_num].insert(std::make_pair(ident, 1));
            ident_id = ident +'_' + std::to_string(1);
        }
        else {
            symbolname_cnt[scope_num][ident]++;
            ident_id = ident + '_' + std::to_string(symbolname_cnt[scope_num][ident]);
        }
        if (mode == 0) { //无赋值
            ValueData vd_alloc = ValueData(-1, "alloc", 0, 0, 0, ident_id);
            Value vd_alloc_value = InsertValueDataToAll(vd_alloc);
            InsertValueDataToBlock(vd_alloc, vd_alloc_value);
            symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                         SymbolInfo(0, 0, false, false, false)));
        }
        else {
            Value lhs = initval->DumpKoopa();
            
            ValueData vd_alloc = ValueData(-1, "alloc", 0, 0, 0, ident_id, initval->exp_val);
            Value vd_alloc_value = InsertValueDataToAll(vd_alloc);
            InsertValueDataToBlock(vd_alloc, vd_alloc_value);
            ValueData vd_store = ValueData(-1, "store", lhs, 0, 0,ident_id);
            Value vd_store_value = InsertValueDataToAll(vd_store);
            InsertValueDataToBlock(vd_store, vd_store_value);
            symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                         SymbolInfo(lhs, initval->exp_val, false, false, false)));
        }
    }
    //全局变量。
    else {
        ident_id = ident + "_" + "global";
        int init = 0;
        if (mode == 0) { //无赋值
            symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                         SymbolInfo(0, 0, false, false, false)));
        }
        else {
            Value lhs = initval->DumpKoopa();
            symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                         SymbolInfo(lhs, initval->exp_val, false, false, false)));
            init = initval->exp_val;
        }
        print_ins = true;
        ValueData vd_alloc = ValueData(-1, "globalalloc", 0, 0, 0, ident_id, init);
        Value vd_alloc_value = InsertValueDataToAll(vd_alloc);
        InsertValueDataToBlock(vd_alloc, vd_alloc_value);
        print_ins = false;
    }

    return 0;
}

Value ConstInitValAST::DumpKoopa() {
    auto val = constexp->DumpKoopa();
    exp_val = constexp->exp_val;
    return val;
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

    exp_val = (*vi).second.exp_val;

    if (!(*vi).second.is_const_variable) {
        //变量
        ValueData vd = AllocateValueData("load", lhs_value, rhs_value, 0, (*vi).first);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
    }

    else {
        //常量不需要分配新的值。
        //宛如number。需要新的数据结构但并不需要新的临时标号。
        ValueData vd = ValueData(-1, "lval", lhs_value, rhs_value, 0, (*vi).first);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val; 
    }
    return 0;
}

Value ConstExpAST::DumpKoopa() {
    auto val = exp->DumpKoopa();
    exp_val = exp->exp_val;
    return val;
}

Value InitValAST::DumpKoopa() {
    auto val = exp->DumpKoopa();
    exp_val = exp->exp_val;
    return val; 
}

std::unordered_map<std::string, SymbolInfo>::iterator find_var_in_symbol_table(std::string& ident) {
    std::unordered_map<std::string, SymbolInfo>::iterator it;
    int num = symbolname_cnt[scope_num][ident];
    //倒着查。
    //最近的定义标号较大。
    for (int i = num; i >= 1; --i) {
        for (int j = basic_block_num[scope_num]; j >= 0; --j) {
            std::string s = ident + "_" + std::to_string(i);
            auto vi = symbol_table[scope_num][j].find(s);
            if (vi != symbol_table[scope_num][j].end())
                return vi;
        }
    }

    std::string s = ident + "_" + "global";
    auto vi = symbol_table[0][0].find(s);
    return vi;
    //never reached
}


void change_varvalue_in_symbol_table(std::string &var_name, Value cur_value) {
    for (int j = basic_block_num[scope_num]; j >= 0; --j) {
        auto vi = symbol_table[scope_num][j].find(var_name);
        if (vi != symbol_table[scope_num][j].end()) {
            symbol_table[scope_num][j][var_name].value = cur_value;
        }
    }
}

void enter_block() {
    PrintInstruction();
    basic_block_num[scope_num]++;
    symbol_table[scope_num].push_back(std::unordered_map<std::string, SymbolInfo>());
    loop_broken_or_continued[scope_num].push_back(false);
}
void leave_block() {
    PrintInstruction();
    symbol_table[scope_num].erase(symbol_table[scope_num].begin() + basic_block_num[scope_num]);
    loop_broken_or_continued[scope_num].erase(loop_broken_or_continued[scope_num].begin() + basic_block_num[scope_num]);
    basic_block_num[scope_num]--;
}