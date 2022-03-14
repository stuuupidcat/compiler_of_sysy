#include "AST.h"


ExpSign::ExpSign(std::string res_, std::string lhs_, std::string rhs_) {
    result_sign = res_;
    lhs_sign = lhs_;
    rhs_sign = rhs_;
};

int BaseAST::temp_sign_num = 0;
std::stack<std::string> BaseAST::sign_stack = {};

ExpSign BaseAST::AllocSign() {
    //分配临时标号
    //res = sub lhs, rhs .....
    assert(sign_stack.size() >= 1);
    std::string res, lhs, rhs;

    if (sign_stack.size() == 1) {
        lhs = "0";
        rhs = sign_stack.top();
        sign_stack.pop();
        res = "%"+std::to_string(temp_sign_num);
        temp_sign_num++;
        sign_stack.push(res);
    }
    else {
        //1 - -2的优先级问题？
        rhs = sign_stack.top();
        sign_stack.pop();
        
        lhs = sign_stack.top();
        sign_stack.pop();
        
        
        
        res = "%"+std::to_string(temp_sign_num);
        temp_sign_num++;
        sign_stack.push(res);
    }
    return ExpSign(res, lhs, rhs);
}

void BaseAST::PrintInstruction(ExpSign& exp_sign, std::string op) {
    std::cout << "    " << exp_sign.result_sign << " = "<< op << " ";
    std::cout << exp_sign.lhs_sign << ", " << exp_sign.rhs_sign << std::endl;
    return;
} 

void CompUnitAST::Dump() const  {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
}

void FuncDefAST::Dump() const  {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
}

void FuncTypeAST::Dump() const  {
    std::cout << "FuncTypeAST { ";
    std::cout <<  s_int ;
    std::cout << " }";;
}

void BlockAST::Dump() const  {
    std::cout << "BlockAST { ";
    stmt->Dump();
    std::cout << " }";
}

void StmtAST::Dump() const  {
    std::cout << "StmtAST { ";
    exp -> Dump();
    std::cout << " }";
}

void NumberAST::Dump() const  {
    std::cout << num;
}

void ExpAST::Dump() const  {
    addexp -> Dump();
}

void CompUnitAST::DumpKoopa() const {
    std::cout << "fun ";
    func_def -> DumpKoopa();
}

void UnaryExpAST::Dump() const {
    if (mode == 0) {
        primaryexp->Dump();
    }
    else if (mode == 1) {
        unaryexp->Dump();
        unaryop->Dump();
    }
}

void PrimaryExpAST::Dump() const  {
    if (mode == 0) {
        std::cout << "(";
        exp->Dump();
        std::cout << ")" << std::endl;
    }
    else if (mode == 1) {
        number->Dump();
    }
}

void UnaryOpAST::Dump() const {
    if (mode == 0) {
        std::cout << "+";
    }
    else if (mode == 1) {
        std::cout << "-";
    }
    else if (mode == 2) {
        std::cout << "!";
    }
}

void FuncDefAST::DumpKoopa() const  {
    std::cout << "@" << ident << "(): ";
    func_type -> DumpKoopa();
    block -> DumpKoopa();
}

void FuncTypeAST::DumpKoopa() const  {
    std::cout << "i32 ";
}

void BlockAST::DumpKoopa() const  {
    std::cout << "{" << std::endl;
    std::cout << "%entry:" << std::endl;
    stmt->DumpKoopa();
    std::cout << '}' << std::endl;
}

void StmtAST::DumpKoopa() const  {
    exp->DumpKoopa();
    ExpSign exp_sign = AllocSign();
    std::cout << "    ret "; 
    if (exp_sign.result_sign[0] == '%') 
        std::cout << exp_sign.rhs_sign << std::endl;   
    else 
        std::cout << exp_sign.result_sign << std::endl;   
}


void NumberAST::DumpKoopa() const  {
    sign_stack.push(std::to_string(num));
}


void ExpAST::DumpKoopa() const  {
    addexp -> DumpKoopa();
}



void UnaryExpAST::DumpKoopa() const{
    if (mode == 0) {
        primaryexp->DumpKoopa();
    }
    else if (mode == 1) {
        //优先级问题？
        unaryexp->DumpKoopa();
        unaryop->DumpKoopa();        
    }
}



void PrimaryExpAST::DumpKoopa() const {
    if (mode == 0) {
        exp->DumpKoopa();
    }
    else if (mode == 1) {
        number->DumpKoopa();
    }
}

void UnaryOpAST::DumpKoopa() const {
    //pass
    ExpSign exp_sign = AllocSign();
    if (mode == 0) {
        PrintInstruction(exp_sign, "add");
    }
    else if (mode == 1) {
        //std::cout << "-";
        PrintInstruction(exp_sign, "sub");
    }
    else if (mode == 2) {
        //std::cout << "!";
        PrintInstruction(exp_sign, "eq");
    }
    //sign_stack.push(exp_sign.result_sign);
}

void MulExpAST::DumpKoopa() const {
    if (mode == 0) {
        unaryexp -> DumpKoopa();
    }
    else {
        std::string ops[4] = {"", "mul", "div", "mod"};
        //这个运算的前半句是为了处理单目运算符 (!1, -1) ?
        if (unaryexp->mode == 1) {
            unaryexp->DumpKoopa();
            mulexp->DumpKoopa();
        }
        else {
            mulexp->DumpKoopa();
            unaryexp->DumpKoopa();
        }
        ExpSign exp_sign = AllocSign();
        PrintInstruction(exp_sign, ops[mode]);
    }
    return;
}


void AddExpAST::DumpKoopa() const {
    if (mode == 0) {
        mulexp -> DumpKoopa();
    }
    else {
        std::string ops[4] = {"", "add", "sub"};
        //这个运算的前半句是为了处理单目运算符 (!1, -1)
        if ((mulexp->mode == 0 && mulexp->child_mode[0] == 1) || mulexp->mode != 0) {
            mulexp -> DumpKoopa();
            addexp -> DumpKoopa();
        }
        else {
            addexp -> DumpKoopa();
            mulexp -> DumpKoopa();
        }
        ExpSign exp_sign = AllocSign();
        PrintInstruction(exp_sign, ops[mode]);
    }
    return;
}



