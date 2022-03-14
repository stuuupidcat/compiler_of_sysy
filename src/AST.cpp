#include "AST.h"

std::vector<ASTResult*> ast_result_vec;
int temp_sign_num = 0;

void delete_ast_res_vec() {
    for (auto val : ast_result_vec) {
        delete val;
    }
}

ASTResult::ASTResult(std::unique_ptr<BaseAST>* ast_pointer_, int temp_sign_num_) {
  ast_pointer = ast_pointer_;
  sign_num = temp_sign_num_;
  sign_name = '%' + std::to_string(temp_sign_num_);                                                
} 

ASTResult* IsASTAllocated(std::unique_ptr<BaseAST>* ast_pointer_) {
  for (auto pt: ast_result_vec) {
    if (pt->ast_pointer == ast_pointer_)
      return pt;
  }
  return nullptr;
}

ASTResult* StoreASTToVec(std::unique_ptr<BaseAST>* ins_pt) {
  ASTResult* ast_res_pt = new ASTResult(ins_pt, temp_sign_num);
  temp_sign_num++;
  ast_result_vec.push_back(ast_res_pt);
  return ast_res_pt;
}

//分配结果器，管他分没分配结果，在dumpkoopa之前执行以下。执行完毕之后要继续dumpkoopa。
ASTResult* Allocate(std::unique_ptr<BaseAST> *pt) {
    ASTResult* allocated = IsASTAllocated(pt);
    if (!allocated) 
      allocated = StoreASTToVec(pt);
    return allocated;
}

void CompUnitAST::Dump()   {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
}

void FuncDefAST::Dump()   {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
}

void FuncTypeAST::Dump()   {
    std::cout << "FuncTypeAST { ";
    std::cout <<  s_int ;
    std::cout << " }";;
}

void BlockAST::Dump()   {
    std::cout << "BlockAST { ";
    stmt->Dump();
    std::cout << " }";
}

void StmtAST::Dump()   {
    std::cout << "StmtAST { ";
    exp -> Dump();
    std::cout << " }";
}

void NumberAST::Dump()   {
    std::cout << num;
}

void ExpAST::Dump()   {
    addexp -> Dump();
}

void UnaryExpAST::Dump()  {
    if (mode == 0) {
        primaryexp->Dump();
    }
    else if (mode == 1) {
        unaryexp->Dump();
        unaryop->Dump();
    }
}

void PrimaryExpAST::Dump()   {
    if (mode == 0) {
        std::cout << "(";
        exp->Dump();
        std::cout << ")" << std::endl;
    }
    else if (mode == 1) {
        number->Dump();
    }
}

void UnaryOpAST::Dump()  {
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

void MulExpAST::Dump()  {
    return;
}

void AddExpAST::Dump()  {
    return;
}

void CompUnitAST::DumpKoopa(ASTResult* self)  {
    std::cout << "fun ";
    //ASTResult* allocated = Allocate(&func_def);
    func_def -> DumpKoopa(nullptr);
    delete_ast_res_vec();
}

void FuncDefAST::DumpKoopa(ASTResult* self)   {
    std::cout << "@" << ident << "(): ";
    //ASTResult* allocated_1 = Allocate(&func_type);
    func_type -> DumpKoopa(nullptr);
    //ASTResult* allocated_2 = Allocate(&block);
    block -> DumpKoopa(nullptr);
}

void FuncTypeAST::DumpKoopa(ASTResult* self)   {
    std::cout << "i32 ";
}

void BlockAST::DumpKoopa(ASTResult* self)   {
    std::cout << "{" << std::endl;
    std::cout << "%entry:" << std::endl;
    //ASTResult* allocated = Allocate(&stmt);
    stmt->DumpKoopa(nullptr);
    std::cout << '}' << std::endl;
}

void StmtAST::DumpKoopa(ASTResult* self) {
    //ASTResult* allocated = Allocate(&exp);
    exp->DumpKoopa(nullptr);
}


void NumberAST::DumpKoopa(ASTResult* self) {
    std::cout << "  " << self->sign_name << " = " << num << std::endl; 
}


void ExpAST::DumpKoopa(ASTResult* self)  {
    ASTResult* allocated = Allocate(&addexp);
    addexp -> DumpKoopa(allocated);
}



void UnaryExpAST::DumpKoopa(ASTResult* self) {
    if (mode == 0) {
        ASTResult* allocated = Allocate(&primaryexp);
        primaryexp->DumpKoopa(allocated);
        std::cout << "  " << self->sign_name << " " << allocated->sign_name << endl;
    }
    else if (mode == 1) {//?
        std::string ops[3] = {"add", "sub", "eq"};
        ASTResult* allocated_rhs = Allocate(&unaryexp);
        unaryexp->DumpKoopa(allocated_rhs);
        std::cout << "  " << self->sign_name << " = " << ops[unaryop->mode] << ", 0, " << allocated_rhs->sign_name <<  std::endl;  
    }
}



void PrimaryExpAST::DumpKoopa(ASTResult* self)  {
    if (mode == 0) {
        ASTResult* allocated = Allocate(&exp);
        exp->DumpKoopa(nullptr);
        std::cout << 
    }
    else if (mode == 1) {
        ASTResult* allocated = Allocate(&number);
        number->DumpKoopa(allocated);
        std::cout << 
    }
}

void UnaryOpAST::DumpKoopa(ASTResult* self)  {
    /*pass
    if (mode == 0) {
        std::cout << "  add";
    }
    else if (mode == 1) {
        std::cout << "  sub";
    }
    else if (mode == 2) {
        std::cout << "  eq";
    }*/
    return;
}

void MulExpAST::DumpKoopa(ASTResult* self)  {
    if (mode == 0) {
        ASTResult* allocated = Allocate(&unaryexp);
        unaryexp -> DumpKoopa(allocated);
    }
    else {
        std::string ops[4] = {"", "  mul ", "  div ", "  mod "};
        //这个运算的前半句是为了处理单目运算符 (!1, -1) ?
        ASTResult* allocated_lhs = Allocate(&mulexp);
        mulexp->DumpKoopa(allocated_lhs);
        ASTResult* allocated_rhs = Allocate(&unaryexp);
        unaryexp->DumpKoopa(allocated_rhs);
        std::cout<< "  " << self->sign_name << ops[mode] << allocated_lhs->sign_name << ", " << allocated_rhs->sign_name;
    }
    return;
}


void AddExpAST::DumpKoopa(ASTResult* self)  {
    if (mode == 0) {
        ASTResult* allocated = Allocate(&mulexp);
        mulexp -> DumpKoopa(allocated);
    }
    else {
        std::string ops[4] = {"", "  add ", "  sub "};
        //这个运算的前半句是为了处理单目运算符 (!1, -1)
        ASTResult* allocated_lhs = Allocate(&addexp);
        addexp->DumpKoopa(allocated_lhs);
        ASTResult* allocated_rhs = Allocate(&mulexp);
        mulexp->DumpKoopa(allocated_rhs);
        std::cout << "  " << self->sign_name << " = " << ops[mode] << allocated_lhs->sign_name << ", " << allocated_rhs->sign_name;
    }
    return;
}



