#include "AST.h"

int temp_sign_num = 0;
int basic_block_num = -1;

std::vector<std::unordered_map<Value, ValueData, ValueHash, ValueEqual>> blocks_values;
std::vector<std::vector<Value>> blocks_insts;

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
        auto vd = blocks_values[basic_block_num].find(inst);
        if ((*vd).second.inst_type != "number")
            std::cout << " " <<(*vd).second.format();
    }
}

ValueData::ValueData(int no_, std::string inst_type_, Value lhs_, Value rhs_) {
    no = no_;
    inst_type = inst_type_;
    lhs = lhs_;
    rhs = rhs_;
}

ValueData::ValueData() = default;

std::string ValueData::format() {
    std::string res;
    ValueData lhs_vd, rhs_vd;

    if (inst_type == "number") {
        res = std::to_string((long)lhs);
        return res;
    }
    
    if (lhs != nullptr) {
        lhs_vd = (*blocks_values[basic_block_num].find(lhs)).second;
    }
    if (rhs != nullptr) {
        rhs_vd = (*blocks_values[basic_block_num].find(rhs)).second;
    }


    if (inst_type.find("single") != std::string::npos) {
        res = "%" + std::to_string(no) + " = " + inst_type.substr(6);
        res += " 0, ";
        if (rhs_vd.inst_type == "number") {
            res += rhs_vd.format();
        }
        else {
            res += "%";
            res += std::to_string(rhs_vd.no);
        }
        res += "\n";
    }

    else if (inst_type == "return") {
        res = "ret ";
        if (rhs_vd.inst_type == "number") {
            res += rhs_vd.format();
        }
        else {
            res += "%";
            res += std::to_string(rhs_vd.no);
        }
        res += "\n";
    }

    else { //add, sub, mul, mod, div
        res = "%" + std::to_string(no) + " = " + inst_type + " ";
        if (lhs_vd.inst_type == "number") {
            res += lhs_vd.format();
            res += ", ";
        }
        else {
            res += "%";
            res += std::to_string(lhs_vd.no);
            res += ", ";
        }
        if (rhs_vd.inst_type == "number") {
            res += rhs_vd.format();
            res += "\n";
        }
        else {
            res += "%";
            res += std::to_string(rhs_vd.no);
            res += "\n";
        }
    }
    return res;
}

ValueData AllocateValueData(std::string inst_type_, Value lhs_, Value rhs_) {
    ValueData vd = {temp_sign_num, inst_type_, lhs_, rhs_};
    temp_sign_num++;
    return vd;
}

Value CompUnitAST::DumpKoopa(Value self)  {
    std::cout << "fun ";
    //Value allocated = Allocate(&func_def);
    blocks_values.resize(100);
    blocks_insts.resize(100);
    func_def -> DumpKoopa(nullptr);
    return self;
}

Value FuncDefAST::DumpKoopa(Value self)   {
    std::cout << "@" << ident << "(): ";
    //Value allocated_1 = Allocate(&func_type);
    func_type -> DumpKoopa(nullptr);
    //Value allocated_2 = Allocate(&block);
    block -> DumpKoopa(nullptr);
    return self;
}

Value FuncTypeAST::DumpKoopa(Value self)   {
    std::cout << "i32 ";
    return self;
}

Value BlockAST::DumpKoopa(Value self)   {
    basic_block_num++;
    std::cout << "{" << std::endl;
    std::cout << "%entry:" << std::endl;
    //Value allocated = Allocate(&stmt);
    stmt->DumpKoopa(nullptr);
    PrintInstruction();
    std::cout << '}' << std::endl;
    return self;
}

Value StmtAST::DumpKoopa(Value self) {
    //Value allocated = Allocate(&exp);
    Value lhs_value = nullptr;
    Value rhs_value = exp->DumpKoopa(&exp);
    ValueData vd = AllocateValueData("return", lhs_value, rhs_value);
    Value this_value = self;
    blocks_values[basic_block_num].insert(std::make_pair(this_value, vd));
    blocks_insts[basic_block_num].push_back(this_value);
    return this_value;
}

//这以上的后期再改。关于他们的传入值，输出格式等等。

Value NumberAST::DumpKoopa(Value self) {
    Value lhs_value = (Value) num;
    Value rhs_value = nullptr;
    ValueData vd = ValueData(-1, "number", lhs_value, rhs_value);
    Value this_value = self;
    blocks_values[basic_block_num].insert({this_value, vd});
    blocks_insts[basic_block_num].push_back(this_value);
    return this_value;
}


Value ExpAST::DumpKoopa(Value self)  {
    return addexp -> DumpKoopa(&addexp);
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
        blocks_values[basic_block_num].insert(std::make_pair(this_value, vd));
        blocks_insts[basic_block_num].push_back(this_value);
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
        blocks_values[basic_block_num].insert(std::make_pair(this_value, vd));
        blocks_insts[basic_block_num].push_back(this_value);
        return this_value;
    }
}

Value AddExpAST::DumpKoopa(Value self)  {
    if (mode == 0) {
        return mulexp -> DumpKoopa(&mulexp);
    }
    else {
        std::string ops[4] = {"", "add", "sub"};
        //这个运算的前半句是为了处理单目运算符 (!1, -1)
        Value lhs_value = addexp->DumpKoopa(&addexp);
        Value rhs_value = mulexp->DumpKoopa(&mulexp);
        ValueData vd = AllocateValueData(ops[mode], lhs_value, rhs_value);
        Value this_value = self;
        blocks_values[basic_block_num].insert(std::make_pair(this_value, vd));
        blocks_insts[basic_block_num].push_back(this_value);
        return this_value;
    }
}



