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

//当前所处函数的名字。
std::string cur_func_name;

//符号表。下标为当前的(scope_num, block_num)
std::vector<std::vector<std::unordered_map<std::string, SymbolInfo>>> symbol_table; 
//处理同名变量。下标为scope_num
std::vector<std::unordered_map<std::string, int>> symbolname_cnt;

//循环的信息。
std::vector<LoopData> loop_data;
//std::vector<std::vector<bool>> loop_broken_or_continued;

//编译期求值
bool print_ins = true;
//需要输出一个标签
bool need_label = false;

bool calculating_params = false;

ValueData AllocateValueData(std::string inst_type_, Value lhs_, Value rhs_, Value jump_cond_ = 0, std::string name_="", int initializer_=0) {
    ValueData vd = {temp_sign_num[scope_num], inst_type_, lhs_, rhs_, jump_cond_, name_, initializer_};
    if (print_ins)
        temp_sign_num[scope_num]++;
    return vd;
}

ValueData AllocateValueData(std::string inst_type_, std::string symbol_name_,int offset, Value offset_value_) {
    ValueData vd = {temp_sign_num[scope_num], inst_type_, symbol_name_, offset, offset_value_};
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

ValueData::ValueData(std::string inst_type_, std::string symbol_name_, int arr_dim_, std::vector<int> arr_sizes_, std::vector<int> arr_item_exp_algoresults_) {
    inst_type = inst_type_;
    symbol_name = symbol_name_;
    arr_dim = arr_dim_;
    arr_sizes = arr_sizes_;
    arr_item_exp_algoresults = arr_item_exp_algoresults_;
    //arr_item_values = arr_item_values_;
}

ValueData::ValueData(int no_, std::string inst_type_, std::string symbol_name_,int offset_, Value offset_value_) {
    no = no_;
    inst_type = inst_type_;
    symbol_name = symbol_name_;
    offset = offset_;
    offset_value = offset_value_;
}

void InsertValueDataToBlock(ValueData valuedata, Value allocated_val) {
    if (print_ins) {
        block_insts.insert(std::make_pair(allocated_val, valuedata));
        block_values.push_back(allocated_val);
        //all_insts.insert(std::make_pair(allocated_val, valuedata));
        //all_values.push_back(allocated_val);
    }
}

//记得清零。
int make_aggregate_pt = 0;
int braces_pt = 1;
ArrayInitVal* MakeAggregate(std::vector<int>& exp_algoresults, std::vector<Value>& subast_values, std::string& braces) {
    auto res = new Aggregate();
    res->mode = 1; //aggregate
    //找到连续的integer值。
    while (make_aggregate_pt < subast_values.size() && braces_pt < braces.size() - 1) {
        if (braces[braces_pt] == '{') {
            braces_pt++;
            auto agg = MakeAggregate(exp_algoresults, subast_values, braces);
            res->values.push_back(std::unique_ptr<ArrayInitVal>(agg));
        }
        else if (braces[braces_pt] == '}') {
            braces_pt++;
            break;
        }
        else {
            auto int_val = new Integer();
            int_val->mode = 0;
            int_val->value = subast_values[make_aggregate_pt];
            int_val->algoresult = exp_algoresults[make_aggregate_pt];

            res->values.push_back(std::unique_ptr<ArrayInitVal>(int_val));
            make_aggregate_pt++;
            braces_pt++;
        }
    }
    return res;
}


void Fill(std::vector<Value>& array_initval_values, std::vector<int>& array_initval_algoresult, Aggregate* aggregate, std::vector<int>& nums, int already_filled, int should_fill_num) {
    int aggregate_pt = 0; //对aggregate的指针；
    int cur_filled_num = 0; //当前已经填充的数量。
    while (aggregate_pt < aggregate->values.size()) {
        if (aggregate->values[aggregate_pt]->mode == 0) { //遇见了一个整数，int[2][3][4], 把整数都填完。
            while ( aggregate_pt < aggregate->values.size() && aggregate->values[aggregate_pt]->mode == 0) {
                auto ptr = (Integer *)(aggregate->values[aggregate_pt].get());
                array_initval_values.push_back(ptr->value);
                array_initval_algoresult.push_back(ptr->algoresult);
                cur_filled_num++;
                aggregate_pt++;
            }
        } 
        else {
            int will_fill_num = 0;
            std::vector<int> sub_nums;
            for (int i = nums.size() - 2; i >= 0; i--) { //-2是因为nums[nums.size() - 1]是数组的大小。
                if (cur_filled_num % nums[i] == 0) {
                    will_fill_num = nums[i];
                    for (int j = 0; j <= i; j++) {
                        sub_nums.push_back(nums[j]); //只递归一个子集。
                    }
                    break;
                }
            }
            Fill(array_initval_values, array_initval_algoresult, (Aggregate *)(aggregate->values[aggregate_pt].get()), sub_nums, already_filled+cur_filled_num, will_fill_num);
            cur_filled_num += will_fill_num;
            aggregate_pt++;
        }
    }
    auto zero = new NumberAST();
    zero->num = 0;
    Value zero_val = zero->DumpKoopa();
    //把剩下的填充。
    for (int i = cur_filled_num; i < should_fill_num; i++) {
        array_initval_values.push_back(zero_val);
        array_initval_algoresult.push_back(0);
    }

}


int initvals_pt = 0;
std::string GlobalArrayInit(std::vector<int>& initvals, std::vector<int>& vec_dims, int vec_dims_pt) {
    std::string res;
    if (vec_dims_pt == vec_dims.size() - 1) {
        for (int i = 0; i < vec_dims[vec_dims_pt]; i++) {
            if (i == 0) {
                res += std::to_string(initvals[initvals_pt]);
                initvals_pt++;
            }
            else {
                res += ", " + std::to_string(initvals[initvals_pt]);
                initvals_pt++;
            }
        }

    }
    else {
        for (int i = 0; i < vec_dims[vec_dims_pt]; i++) {
            if (i == 0) {
                res += GlobalArrayInit(initvals, vec_dims, vec_dims_pt + 1);
            }
            else {
                res += ", " + GlobalArrayInit(initvals, vec_dims, vec_dims_pt + 1);
            }
        }
    }
    return "{" + res + "}";
}

//复用initvals_pt;
void LocalArrayInit(std::vector<Value>& initvals, std::vector<int>& vec_dims, int vec_dims_pt, std::string ident_id) {
    if (vec_dims_pt == vec_dims.size() - 1) {
        for (int i = 0; i < vec_dims[vec_dims_pt]; i++) {
            ValueData get_ptr_vd = AllocateValueData("getelemptr", ident_id, i, 0);
            Value get_ptr_vd_value = InsertValueDataToAll(get_ptr_vd);
            InsertValueDataToBlock(get_ptr_vd, get_ptr_vd_value);
            ValueData store_vd = ValueData(-1, "storetotemp", initvals[initvals_pt], get_ptr_vd_value);
            initvals_pt++;
            Value store_vd_value = InsertValueDataToAll(store_vd);
            InsertValueDataToBlock(store_vd, store_vd_value); 
        }
    }
    else {
        for (int i = 0; i < vec_dims[vec_dims_pt]; i++) {
            ValueData get_ptr_vd = AllocateValueData("getelemptr", ident_id, i, 0);
            Value get_ptr_vd_value = InsertValueDataToAll(get_ptr_vd);
            InsertValueDataToBlock(get_ptr_vd, get_ptr_vd_value);
            LocalArrayInit(initvals, vec_dims, vec_dims_pt + 1, "%" + std::to_string(get_ptr_vd.no));
        }
    }
}


void BaseAST::CalIdentID() {
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

//To block之前必须toall
Value InsertValueDataToAll(ValueData valuedata) {
    Value val = random();
    //冲突
    while (all_insts.find(val) != all_insts.end()) {
        val = random();
    }
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
find_label:
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
        if (vd.inst_type == "return" || vd.inst_type == "break" || vd.inst_type == "continue" || vd.inst_type == "jump" || vd.inst_type == "br") {     
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
            goto find_label;
        }
        //要调到循环结尾的jump_exp_label 不用
        if (vd.inst_type == "label") {
            std::cout << vd.format();
        }
        else if (vd.inst_type == "globalalloc" || vd.inst_type == "global_array_alloc") {
            std::cout << vd.format();
        }
        else if (vd.inst_type != "number" && vd.inst_type != "lval")
            std::cout << "  " <<vd.format();
    }
    block_insts.erase(block_insts.begin(), block_insts.end());
    block_values.erase(block_values.begin(), block_values.end());
}

//for variable
SymbolInfo::SymbolInfo(Value value_, int exp_algoresult_, bool is_const_) {
    value = value_;
    exp_algoresult = exp_algoresult_;
    is_const_variable = is_const_;
}

//for function
SymbolInfo::SymbolInfo(bool is_void_function_) {
    is_void_function = is_void_function_;
}

//for array
SymbolInfo::SymbolInfo(int arr_dims_, std::vector<int> arr_size_, bool is_array_pt_) {
    arr_dims = arr_dims_;
    arr_size = arr_size_;
    is_array_pt = is_array_pt_;
}




std::string ValueData::format() {
    std::string res;
    ValueData lhs_vd, rhs_vd, offset_vd, jump_cond_vd;
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
    if (offset_value != 0) {
        offset_vd = all_insts[offset_value];
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
            res = std::to_string(vi.exp_algoresult);
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
    //指令类型为alloc // alloc+type
    else if (inst_type.find("alloc") != std::string::npos && (inst_type.size() == 5 || inst_type[5] == '*')) {
        //定义且赋值
        if (inst_type.size() == 5) {
            res = "@" + symbol_name + " = alloc i32\n";
        }
        else {
            res = "@" + symbol_name + " = alloc " + inst_type.substr(5) + "\n";
        }
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
    else if (inst_type == "storetotemp") {
        res = "store ";
        if (lhs_vd.inst_type == "number" || lhs_vd.inst_type == "lval") {
            res += lhs_vd.format() + ", ";
        }
        else {
            res += "%" + std::to_string(lhs_vd.no) + ", ";
        }
        res += "%" + std::to_string(rhs_vd.no) + "\n";

    }
    //载入一个变量
    else if (inst_type == "load") {
        if (symbol_name != "") {
             res = "%" + std::to_string(no) + " = load @" + symbol_name + "\n";
        }
        else {
            res = "%" + std::to_string(no) + " = load %" + std::to_string(lhs_vd.no) + "\n";
        }
       
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
    else if (inst_type == "global_array_alloc") {
        res = "global @" + symbol_name + " = alloc ";
        res += ArrayType(arr_sizes, 0); 
        res += ", ";
        
        initvals_pt = 0;
        res += GlobalArrayInit(arr_item_exp_algoresults, arr_sizes, initvals_pt);
        res += "\n";
    }
    else if (inst_type == "local_array_alloc") {
        res = "@" + symbol_name + " = alloc ";
        res += ArrayType(arr_sizes, 0) + "\n";; 
    }
    else if (inst_type == "getelemptr") {
        if (symbol_name[0] != '%') {
            symbol_name = "@" + symbol_name;
        } 

        if (offset != -1) {
            res = "%" + std::to_string(no) + " = getelemptr " + symbol_name + ", " + std::to_string(offset) + "\n";
        }
        else {
            if (offset_vd.inst_type == "number" || offset_vd.inst_type == "lval") {
                res = "%" + std::to_string(no) + " = getelemptr " + symbol_name + ", " + offset_vd.format() + "\n";
            }
            else {
                res = "%" + std::to_string(no) + " = getelemptr " + symbol_name + ", %" + std::to_string(offset_vd.no) + "\n";
            }
        }
    }
    else if (inst_type == "getptr") {
        if (offset != -1) {
            res = "%" + std::to_string(no) + " = getptr @" + symbol_name + ", " + std::to_string(offset) + "\n";
        }
        else {
            if (offset_vd.inst_type == "number" || offset_vd.inst_type == "lval") {
                res = "%" + std::to_string(no) + " = getptr " + symbol_name + ", " + offset_vd.format() + "\n";
            }
            else {
                res = "%" + std::to_string(no) + " = getptr " + symbol_name + ", %" + std::to_string(offset_vd.no) + "\n";
            }
        }
    }
    return res;
}


Value CompUnitAST::DumpKoopa()  {
    srand(time(NULL));
    std::cout << "decl @getint(): i32\n";
    std::cout << "decl @getch(): i32\n";
    std::cout << "decl @getarray(*i32): i32\n";
    std::cout << "decl @putint(i32)\n";
    std::cout << "decl @putch(i32)\n";
    std::cout << "decl @putarray(i32, *i32)\n";
    std::cout << "decl @starttime()\n";
    std::cout << "decl @stoptime()\n\n";



    symbol_table.resize(1000);
    for (auto &val: symbol_table)
        val.resize(1000);

    //loop_broken_or_continued.resize(100);
    //for (auto &val: loop_broken_or_continued)
    //    val.resize(100);
    symbolname_cnt.resize(1000);

    basic_block_num.resize(1000);
    for (auto &val: basic_block_num)
        val = 0;
    
    temp_sign_num.resize(1000);
    for (auto &val: temp_sign_num)
        val = 0;
    
    for (auto& decl: decls) {
        print_ins = false; //要用的时候再打开.
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
                                                     SymbolInfo(true)));
        }

        else {
            symbol_table[scope_num][bb_num].insert(std::make_pair(func->ident_id,
                                                     SymbolInfo(false)));
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
    cur_func_name = ident_id;

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
    //enter_block();
    if (mode == 1 || mode == 3) {
        auto vardecl = new VarDeclAST();
        for (auto& param : ((FuncFParamsAST*)funcfparams.get())->funcfparams){
            //number的ident不为空时直接输出这个名字。
            //number的num正好表示个数组维度。
            auto number = new NumberAST();
            number->ident = param->ident;
            auto pt = (FuncFParamAST*)(param.get());
            number->num = pt->constexps.size()+1;
            number->exp_algoresults.push_back(1);
            for (auto &constexp : pt->constexps) {
                number->exp_algoresults.push_back(constexp->exp_algoresult);
            }
        
            auto vardef = new VarDefAST();
            vardef->ident = param->ident;
            vardef->type = param->type;
            vardef->initval = std::unique_ptr<BaseAST>(number);
            if (param->mode == 0)
                vardef->mode = 1; //普通变量
            else vardef->mode = 5; //数组参数
            vardecl->vardefs.push_back(std::unique_ptr<BaseAST>(vardef));
        }
        vardecls.push_back(std::unique_ptr<BaseAST>(vardecl));
    }

    for (auto& vardecl: vardecls) {
        vardecl->DumpKoopa();
    }
    PrintInstruction();

    block -> DumpKoopa();
    //void
    if (mode == 0 || mode == 1) {
        ValueData vd = ValueData(-1, "return", 0, 0, 0, "", 0);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
    }
    //int
    else {  
        auto zero = new NumberAST();
        zero->num = 0;
        Value zero_val = zero->DumpKoopa();
        ValueData vd = ValueData(-1, "return", zero_val, 0);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
    } 
    
    PrintInstruction();

    //leave_block();

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

std::string ArrayType(std::vector<int>& vec, int pt) {
    std::string res;
    if (pt == vec.size() - 1) {
        res = "[i32, " + std::to_string(vec[pt]) + "]";
    }
    else {
        res = "[" + ArrayType(vec, pt+1) + ", " + std::to_string(vec[pt]) + "]";
    }
    return res;
}

Value FuncFParamAST::DumpKoopa()   {
    //将符号插入到符号表中。
    //不用 本质上是数字
    if (mode == 0) {
        std::cout << "@" << ident << " :i32";
        type = "i32";
    }
    else if (mode == 1) { //..., int a[], ...
        std::cout << "@" << ident << " :*i32";
        type = "*i32";
    }
    else if (mode == 2) { //..., int a[][2][3], ...
        bool print_ins_temp = print_ins;
        print_ins = false;
        for (auto &constexp: constexps) {
            constexp->DumpKoopa();
            exp_algoresults.push_back(constexp->exp_algoresult);
        }
        print_ins = print_ins_temp;
        type = "*" + ArrayType(exp_algoresults, 0);
        std::cout << "@" << ident << " :" << type;
    }
    else {}
    return 0;
}


Value FuncRParamsAST::DumpKoopa()   {
    calculating_params = true;
    for (auto &exp: exps) {
        exp->DumpKoopa();
    }
    calculating_params = false;
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
        ValueData vd = ValueData(-1, "return", 0, 0, 0, "", 0);
        Value val = InsertValueDataToAll(vd);
        InsertValueDataToBlock(vd, val);
        return val;
    }
    else if (mode == 3) { //lval = exp;
        
        //可是lval就是变量。
        auto vi = find_var_in_symbol_table(lval->ident);
        //if lval是变量。
        if ((*vi).second.arr_dims == 0) {
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
        else {
            Value lval_value = lval->DumpKoopa();
            Value exp_value = exp->DumpKoopa();
            ValueData store_vd = ValueData(-1, "storetotemp", exp_value, lval_value);
            Value store_vd_value = InsertValueDataToAll(store_vd);
            InsertValueDataToBlock(store_vd, store_vd_value); 
            return store_vd_value;
        }
        return 0;
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
        //if (!loop_broken_or_continued[scope_num][basic_block_num[scope_num]]) {
            //loop_broken_or_continued[scope_num][basic_block_num[scope_num]] = true;
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

        //}
        return 0;
    }
    else if (mode == 9) {
        //if (!loop_broken_or_continued[scope_num][basic_block_num[scope_num]]) {
            //loop_broken_or_continued[scope_num][basic_block_num[scope_num]] = true;
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
        //}
        
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
    Value lhs_value = (Value)num;
    Value rhs_value = 0;

    exp_algoresult = num;
    //不需要分配新的值。
    ValueData vd = ValueData(-1, "number", lhs_value, rhs_value, 0, ident);
    Value val = InsertValueDataToAll(vd);
    InsertValueDataToBlock(vd, val);
    return val;
}


Value ExpAST::DumpKoopa()  {
    auto val = lorexp -> DumpKoopa();
    exp_algoresult = lorexp->exp_algoresult;
    return val;
}


Value UnaryExpAST::DumpKoopa() {
    if (mode == 0) {
        auto val = primaryexp->DumpKoopa();
        exp_algoresult = primaryexp->exp_algoresult;
        return val;
    }
    else if (mode == 1) {//?
        std::string ops[3] = {"singleadd", "singlesub", "singleeq"};

        Value lhs_value = 0;
        Value rhs_value = unaryexp->DumpKoopa();

        if (unaryop->mode == 0) {
            exp_algoresult = unaryexp->exp_algoresult;
        }
        else if (unaryop->mode == 1) {
            exp_algoresult = -(unaryexp->exp_algoresult);
        }
        else if (unaryop->mode == 2) {
            exp_algoresult = !(unaryexp->exp_algoresult);
            //std::cout << "here!" << exp_algoresult;
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

            calculating_params = true;
            for (auto& exp: funcrparams->exps) {
                temp_vec.push_back(exp->DumpKoopa());
            }
            calculating_params = false;

            ValueData vd = AllocateValueData("call", 0, 0, 0, ident);
            vd.parameters = temp_vec;
            Value val = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, val);
            return val;
        }

        if (ident == "putint" || ident == "putch" || ident == "putarray") {
            std::vector<Value> temp_vec;

            calculating_params = true;
            for (auto& exp: funcrparams->exps) {
                temp_vec.push_back(exp->DumpKoopa());
            }
            calculating_params = false;

            ValueData vd = ValueData(-1, "call", 0, 0, 0, ident);
            vd.parameters = temp_vec;
            Value val = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, val);
            return val;
        }

        std::string func_name = ident + "_function";
        SymbolInfo symbol_info = symbol_table[0][0][func_name];
        ValueData vd;

        std::vector<Value> temp_vec;

        calculating_params = true;
        for (auto& exp: funcrparams->exps) {
            temp_vec.push_back(exp->DumpKoopa());
        }
        calculating_params = false;

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
        exp_algoresult = exp->exp_algoresult;
    }
    else if (mode == 1) {
        val = lval->DumpKoopa();
        exp_algoresult = lval->exp_algoresult;
    }
    else if (mode == 2) {
        val = number->DumpKoopa();
        exp_algoresult = number->exp_algoresult;
    }

    return val;
}


Value UnaryOpAST::DumpKoopa()  {
    return 0;
}


Value MulExpAST::DumpKoopa()  {
    if (mode == 0) {
        auto val = unaryexp->DumpKoopa();
        exp_algoresult = unaryexp->exp_algoresult;
        return val;
    }
    else {
        std::string ops[4] = {"", "mul", "div", "mod"};

        Value lhs_value = mulexp->DumpKoopa();
        Value rhs_value = unaryexp->DumpKoopa();

        if (mode == 1) {
            exp_algoresult = mulexp->exp_algoresult * unaryexp->exp_algoresult;
        }
        else if (mode == 2) {
            if (unaryexp->exp_algoresult != 0) {
                exp_algoresult = mulexp->exp_algoresult / unaryexp->exp_algoresult;
            }
            else {
                exp_algoresult = 0;
            }
            //exp_algoresult = mulexp->exp_algoresult / unaryexp->exp_algoresult;
        }
        else if (mode == 3) {
            if (unaryexp->exp_algoresult != 0) {
                exp_algoresult = mulexp->exp_algoresult % unaryexp->exp_algoresult;
            }
            else {
                exp_algoresult = 0;
            }
            //exp_algoresult = mulexp->exp_algoresult % unaryexp->exp_algoresult;
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
        exp_algoresult = mulexp->exp_algoresult;
        return val;
    }
    else {
        std::string ops[4] = {"", "add", "sub"};
        Value lhs_value = addexp->DumpKoopa();
        Value rhs_value = mulexp->DumpKoopa();
        if (mode == 1) {
            exp_algoresult = addexp->exp_algoresult + mulexp->exp_algoresult;
        }
        else {
            exp_algoresult = addexp->exp_algoresult - mulexp->exp_algoresult;
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
        exp_algoresult = addexp->exp_algoresult;
        return val;
    }
    else {
        std::string ops[5] = {"", "lt", "gt", "le", "ge"};
        Value lhs_value = relexp->DumpKoopa();
        Value rhs_value = addexp->DumpKoopa();

        if (mode == 1) {
            exp_algoresult = (relexp->exp_algoresult < addexp->exp_algoresult);
        }
        else if (mode == 2) {
            exp_algoresult = (relexp->exp_algoresult > addexp->exp_algoresult);
        }
        else if (mode == 3) {
            exp_algoresult = (relexp->exp_algoresult <= addexp->exp_algoresult);
        }
        else if (mode == 4) {
            exp_algoresult = (relexp->exp_algoresult >= addexp->exp_algoresult);
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
       exp_algoresult = relexp->exp_algoresult;
       return val;
    }
    else {
        std::string ops[5] = {"", "eq", "ne"};
        Value lhs_value = eqexp->DumpKoopa();
        Value rhs_value = relexp->DumpKoopa();
        
        if (mode == 1) {
            exp_algoresult = (eqexp->exp_algoresult == relexp->exp_algoresult);
        }
        else {
            exp_algoresult = (eqexp->exp_algoresult != relexp->exp_algoresult);
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
        exp_algoresult = eqexp->exp_algoresult;
        return val;
    }
    else {
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
        auto zero1 = new NumberAST();
        zero1->num = 0;

        //lhs_value != 0
        auto eqexp1 = new EqExpAST();
        eqexp1->mode = 2;
        eqexp1->eqexp = std::move(landexp);
        eqexp1->relexp = std::unique_ptr<BaseAST>(zero1);

        //rhs_value != 0;
        auto zero2 = new NumberAST();
        zero2->num = 0;
        auto eqexp2 = new EqExpAST();
        eqexp2->mode = 2;
        eqexp2->eqexp = std::move(eqexp);
        eqexp2->relexp = std::unique_ptr<BaseAST>(zero2);

        //result = rhs_value != 0;
        auto lval = new LValAST();
        lval->mode = 0;
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
        //exp_algoresult = 0;
        //if (eqexp1->eqexp->exp_algoresult != 0) {
        //    exp_algoresult = eqexp2->eqexp->exp_algoresult != 0;
        //}
        exp_algoresult = eqexp1->eqexp->exp_algoresult && eqexp2->eqexp->exp_algoresult;

        //返回__short_circuit_result的值.
        auto vi = find_var_in_symbol_table(vardef1->ident);
        ident_id = (*vi).first;
    
        if (!(*vi).second.is_const_variable) {
            //变量
            ValueData vd = AllocateValueData("load", 0, 0, 0, ident_id);
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
        exp_algoresult = landexp->exp_algoresult;
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

        auto zero1 = new NumberAST();
        zero1->num = 0;
        //rhs_value != 0;
        auto eqexp2 = new EqExpAST();
        eqexp2->mode = 2;
        eqexp2->eqexp = std::move(landexp);
        eqexp2->relexp = std::unique_ptr<BaseAST>(zero1);

        //result = rhs_value != 0;
        auto lval = new LValAST();
        lval->mode = 0;
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
        exp_algoresult = eqexp1->eqexp->exp_algoresult || eqexp2->eqexp->exp_algoresult;
        //if (eqexp1->eqexp->exp_algoresult == 0) {
        //    exp_algoresult = eqexp2->eqexp->exp_algoresult != 0;
        //}
        
        //返回__short_circuit_result的值.
        auto vi = find_var_in_symbol_table(vardef1->ident);
        ident_id = (*vi).first;
    
        if (!(*vi).second.is_const_variable) {
            //变量
            ValueData vd = AllocateValueData("load", 0, 0, 0, ident_id);
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
        print_ins = false;
        constdef->DumpKoopa();
    }
    print_ins = true;
    return 0;
}

Value VarDeclAST::DumpKoopa() {
    for (auto& vardef: vardefs) {
        vardef->DumpKoopa();
    }
    return 0;
}

Value ConstDefAST::DumpKoopa() {
    if (mode == 0) {
        Value exp_value = constinitval->DumpKoopa();
        //处理局部同名变量
        if (scope_num != 0) {
            CalIdentID();
        }
        else {
            ident_id = ident + '_' + "global";
        }
    
        //将符号插入到符号表中。
        symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                    SymbolInfo(exp_value, constinitval->exp_algoresult, true)));
    }
    else if (mode == 1) { //数组
        //IDENT [ ConstExp ]...[ ConstExp ] "=" ConstInitVal;
        if (scope_num != 0) { //要算数的，
            print_ins = true;
        }
        for (auto& constexp: constexps) {
            constexp->DumpKoopa();
            exp_algoresults.push_back(constexp->exp_algoresult);
        }
        //数组的大小。
        std::vector<int> array_sizes = exp_algoresults;
        constinitval->DumpKoopa();

        //我们有什么：
        //array_sizes, 一个vector, 每个元素是数组的维度
        //将constinitval转换为一个指向constinitvalast的指针。
        auto constinitval_ptr = (ConstInitValAST*)constinitval.get();

        make_aggregate_pt = 0;
        braces_pt = 1;
        auto aggregate = std::unique_ptr<ArrayInitVal>(MakeAggregate(constinitval_ptr->exp_algoresults, constinitval_ptr->subast_values, constinitval_ptr->braces));  
        auto aggregate_pt = (Aggregate*)aggregate.get();
        
        std::vector<Value> array_initval_values;
        std::vector<int> array_initval_algoresults;

        std::vector<int> nums; //各个维度的数目。
        int array_dim = array_sizes.size();
        int mul = 1;
        for (int i = array_dim - 1; i >= 0; --i) {
            mul *= array_sizes[i];
            nums.push_back(mul);
        }
        //填充数组。
        Fill(array_initval_values, array_initval_algoresults, aggregate_pt, nums, 0, nums[nums.size() - 1]);
        
        //要打印数组的声明
        //处理局部同名变量
        if (scope_num != 0) {
            CalIdentID();
            //最后一个参数没啥用。
            ValueData vd = ValueData("local_array_alloc", ident_id, 1, array_sizes, array_initval_algoresults);
            Value vd_value = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, vd_value);
            
            initvals_pt = 0;
            LocalArrayInit(array_initval_values, array_sizes, 0, ident_id);

        }
        else {
            print_ins = true;
            ident_id = ident + '_' + "global";
            ValueData vd = ValueData("global_array_alloc", ident_id, 1, array_sizes, array_initval_algoresults);
            Value vd_value = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, vd_value);
            print_ins = false;
        }
    
        //将符号插入到符号表中。
        symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                    SymbolInfo(1, array_sizes, false)));
        print_ins = false;
    }
    return 0;
}

Value VarDefAST::DumpKoopa() {
    //分成三个指令
    //alloc
    //计算exp的值
    //store
    //处理同名变量
    if (mode <= 1) { //变量
        if (ident == "__short_circuit_or_result" || ident == "__short_circuit_and_result") { 
            //do nothing 
        }
        else {
            print_ins = true;
        }
        if (scope_num != 0) {
            CalIdentID();
            if (mode == 0) { //无赋值
                ValueData vd_alloc = ValueData(-1, "alloc", 0, 0, 0, ident_id);
                Value vd_alloc_value = InsertValueDataToAll(vd_alloc);
                InsertValueDataToBlock(vd_alloc, vd_alloc_value);
                symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                             SymbolInfo(0, 0, false)));
            }
            else {
                Value lhs = initval->DumpKoopa();

                ValueData vd_alloc = ValueData(-1, "alloc", 0, 0, 0, ident_id, initval->exp_algoresult);
                Value vd_alloc_value = InsertValueDataToAll(vd_alloc);
                InsertValueDataToBlock(vd_alloc, vd_alloc_value);
                ValueData vd_store = ValueData(-1, "store", lhs, 0, 0,ident_id);
                Value vd_store_value = InsertValueDataToAll(vd_store);
                InsertValueDataToBlock(vd_store, vd_store_value);
                symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                             SymbolInfo(lhs, initval->exp_algoresult, false)));
            }
        }
        //全局变量。
        else {
            ident_id = ident + "_" + "global";
            int init = 0;
            if (mode == 0) { //无赋值
                symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                             SymbolInfo(0, 0, false)));
            }
            else {
                bool print_ins_tmp = print_ins;
                print_ins = false;
                Value lhs = initval->DumpKoopa();
                symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                             SymbolInfo(lhs, initval->exp_algoresult, false)));
                init = initval->exp_algoresult;
                print_ins = print_ins_tmp;
            }
            ValueData vd_alloc = ValueData(-1, "globalalloc", 0, 0, 0, ident_id, init);
            Value vd_alloc_value = InsertValueDataToAll(vd_alloc);
            InsertValueDataToBlock(vd_alloc, vd_alloc_value);
        }
    }
    else if (mode == 5) { //函数参数中的数组指针。
        //number中存着变量名。
        
        print_ins = true;
        CalIdentID();
        Value lhs = initval->DumpKoopa();

        ValueData vd_alloc = ValueData(-1, "alloc"+type, 0, 0, 0, ident_id, initval->exp_algoresult);
        Value vd_alloc_value = InsertValueDataToAll(vd_alloc);
        InsertValueDataToBlock(vd_alloc, vd_alloc_value);
        ValueData vd_store = ValueData(-1, "store", lhs, 0, 0,ident_id);
        Value vd_store_value = InsertValueDataToAll(vd_store);
        InsertValueDataToBlock(vd_store, vd_store_value);
        symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                             SymbolInfo(
                                                                 ((NumberAST*)(initval.get()))->num, ((NumberAST*)(initval.get()))->exp_algoresults, true)));
    }

    else { //数组
        if (scope_num != 0) { //要算数的，
            print_ins = true;
        }
        for (auto& constexp: constexps) {
            constexp->DumpKoopa();
            exp_algoresults.push_back(constexp->exp_algoresult);
        }
        //数组的大小。
        std::vector<int> array_sizes = exp_algoresults;
        initval->DumpKoopa();

        //the initialization of the array.
        auto initval_ptr = (InitValAST*)initval.get();

        //将数组转换为聚合类型和整数的集合。
        make_aggregate_pt = 0;
        braces_pt = 1;
        auto aggregate = std::unique_ptr<ArrayInitVal>(MakeAggregate(initval_ptr->exp_algoresults, initval_ptr->subast_values, initval_ptr->braces));  
        auto aggregate_pt = (Aggregate*)aggregate.get();
        
        std::vector<Value> array_initval_values;
        std::vector<int> array_initval_algoresults;

        std::vector<int> nums; //各个维度的数目。
        int array_dim = array_sizes.size();
        int mul = 1;
        for (int i = array_dim - 1; i >= 0; --i) {
            mul *= array_sizes[i];
            nums.push_back(mul);
        }
        //填充数组。
        Fill(array_initval_values, array_initval_algoresults, aggregate_pt, nums, 0, nums[nums.size() - 1]);
        
        //要打印数组的声明
        //处理局部同名变量
        if (scope_num != 0) {
            CalIdentID();
            //最后一个参数没啥用。
            ValueData vd = ValueData("local_array_alloc", ident_id, 1, array_sizes, array_initval_algoresults);
            Value vd_value = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, vd_value);
            
            initvals_pt = 0;
            LocalArrayInit(array_initval_values, array_sizes, 0, ident_id);
        }
        else {
            print_ins = true;
            ident_id = ident + '_' + "global";
            ValueData vd = ValueData("global_array_alloc", ident_id, 1, array_sizes, array_initval_algoresults);
            Value vd_value = InsertValueDataToAll(vd);
            InsertValueDataToBlock(vd, vd_value);
            print_ins = false;
        }
    
        //将符号插入到符号表中。
        symbol_table[scope_num][basic_block_num[scope_num]].insert(std::make_pair(ident_id,
                                                    SymbolInfo(array_dim, array_sizes, false)));
    }
    

    return 0;
}

Value ConstInitValAST::DumpKoopa() {
    if (mode == 0) {
        auto val = constexp->DumpKoopa();
        exp_algoresult = constexp->exp_algoresult;
        return val;
    }
    else if (mode == 1) {
        for (auto& constexp: constexps) {
            subast_values.push_back(constexp->DumpKoopa());
            exp_algoresults.push_back(constexp->exp_algoresult);
        }
    }
    return 0;
    
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
    if (mode == 0) {//普通变量 
        auto vi = find_var_in_symbol_table(ident);
        ident_id = (*vi).first;
        
        if ((*vi).second.arr_dims > 0) {
            is_left = true;
            goto ARRAY;
        }


        Value lhs_value = (*vi).second.value;
        Value rhs_value = 0;

        exp_algoresult = (*vi).second.exp_algoresult;

        if (!(*vi).second.is_const_variable) {
            //变量
            ValueData vd = AllocateValueData("load", 0, 0, 0, (*vi).first);
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
    }
    else { //数组的解引用
ARRAY:
        auto vi = find_var_in_symbol_table(ident);
        ident_id = (*vi).first;
        SymbolInfo si = (*vi).second;
        std::vector<int> nums;
        
        if (si.is_array_pt == false) { //是一个数组
            int mul = 1;
            for (int i = si.arr_size.size() - 1; i >= 0; --i) {
                mul *= si.arr_size[i];
                nums.push_back(mul);
            }

            for (auto& exp: exps) {
                subast_values.push_back(exp->DumpKoopa());
                exp_algoresults.push_back(exp->exp_algoresult);
            }
            

            //部分维度的数组指针。补全一个0获得其首元素的指针。
            if (calculating_params && exps.size() < si.arr_dims) {
                is_left = true;
                auto zero = new NumberAST();
                zero->num = 0;
                Value zero_val = zero->DumpKoopa();
                subast_values.push_back(zero_val);
                exp_algoresults.push_back(0);
            }

            Value res;
            for (int i = 0; i < subast_values.size(); ++i) {
                ValueData vd = AllocateValueData("getelemptr", ident_id, -1, subast_values[i]);
                Value val = InsertValueDataToAll(vd);
                InsertValueDataToBlock(vd, val);
                ident_id = "%" + std::to_string(vd.no);
                res = val;
            }

            if (!is_left) {
                ValueData load_vd = AllocateValueData("load", res, 0, 0, "");
                Value load_vd_value = InsertValueDataToAll(load_vd);
                InsertValueDataToBlock(load_vd, load_vd_value);
                res = load_vd_value;
            }
            return res;
        }
        else { //是一个数组指针
            for (auto& exp: exps) {
                subast_values.push_back(exp->DumpKoopa());
                exp_algoresults.push_back(exp->exp_algoresult);
            }

            if (calculating_params && exps.size() < si.arr_dims) { //如果在计算参数，那么要补全一个0，来得到一个getelemptr的值。
                is_left = true;
                auto zero = new NumberAST();
                zero->num = 0;
                Value zero_val = zero->DumpKoopa();

                subast_values.push_back(zero_val);
                exp_algoresults.push_back(0);
            }

            ValueData loadptr_vd = AllocateValueData("load", 0, 0, 0, ident_id);
            Value loadptr_vd_value = InsertValueDataToAll(loadptr_vd);
            InsertValueDataToBlock(loadptr_vd, loadptr_vd_value);

            ValueData getptr_vd = AllocateValueData("getptr", "%"+std::to_string(loadptr_vd.no), -1, subast_values[0]);
            Value res = InsertValueDataToAll(getptr_vd);
            InsertValueDataToBlock(getptr_vd, res);


            ValueData vd;

            int no = getptr_vd.no;
            for (int i = 1; i < subast_values.size(); ++i) {
                vd = AllocateValueData("getelemptr", "%"+std::to_string(no), -1, subast_values[i]);
                res = InsertValueDataToAll(vd);
                InsertValueDataToBlock(vd, res);
                no = vd.no;
            } 


            if (!is_left) {
                vd = AllocateValueData("load", res, 0, 0, "");
                res = InsertValueDataToAll(vd);
                InsertValueDataToBlock(vd, res);
            }

            return res;
            
            




        }

    }
    return 0;
}

Value ConstExpAST::DumpKoopa() {
    auto val = exp->DumpKoopa();
    exp_algoresult = exp->exp_algoresult;
    return val;
}

Value InitValAST::DumpKoopa() {
    if (mode == 0) {
        auto val = exp->DumpKoopa();
        exp_algoresult = exp->exp_algoresult;
        return val; 
    }
    else if (mode == 1) {
        for (auto& exp: exps) {
            subast_values.push_back(exp->DumpKoopa());
            exp_algoresults.push_back(exp->exp_algoresult);
        }
    }
    return 0;
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
    //loop_broken_or_continued[scope_num].push_back(false);
}
void leave_block() {
    PrintInstruction();
    symbol_table[scope_num].erase(symbol_table[scope_num].begin() + basic_block_num[scope_num]);
    //loop_broken_or_continued[scope_num].erase(loop_broken_or_continued[scope_num].begin() + basic_block_num[scope_num]);
    basic_block_num[scope_num]--;
}


