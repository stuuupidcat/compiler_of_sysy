#include "AST.h"

void CompUnitAST::Dump() const  {
    std::cout << 1;
}

void CompUnitAST::DumpKoopa() const {
    std::cout << "fun ";
    func_def -> DumpKoopa();
}

void FuncDefAST::Dump() const  {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
}

void FuncDefAST::DumpKoopa() const  {
    std::cout << "@" << ident << "(): ";
    func_type -> DumpKoopa();
    block -> DumpKoopa();
}

void FuncTypeAST::Dump() const  {
    std::cout << "FuncTypeAST { ";
    std::cout <<  s_int ;
    std::cout << " }";;
}

void FuncTypeAST::DumpKoopa() const  {
    std::cout << "i32 ";
}

void BlockAST::Dump() const  {
    std::cout << "BlockAST { ";
    stmt->Dump();
    std::cout << " }";
}

void BlockAST::DumpKoopa() const  {
    std::cout << "{" << std::endl;
    std::cout << "%entry:" << std::endl;
    stmt->DumpKoopa();
    std::cout << '}' << std::endl;
}

void StmtAST::Dump() const  {
    std::cout << "StmtAST { ";
    exp -> Dump();
    std::cout << " }";
}

void StmtAST::DumpKoopa() const  {
    std::cout << "    ret ";
    exp->DumpKoopa();
}

void NumberAST::Dump() const  {
    std::cout << num;
}

void NumberAST::DumpKoopa() const  {
    std::cout << num << std::endl;
}

void ExpAST::Dump() const  {
    unaryexp -> Dump();
}

void ExpAST::DumpKoopa() const  {
    unaryexp -> DumpKoopa();
}

void UnaryExpAST::Dump() const {
        if (mode == 0) {
            primaryexp->Dump();
        }
        else if (mode == 1) {
            unaryop->Dump();
            unaryexp->Dump();
        }
    }

void UnaryExpAST::DumpKoopa() const{
    if (mode == 0) {
        primaryexp->DumpKoopa();
    }
    else if (mode == 1) {
        unaryop->DumpKoopa();
        unaryexp->DumpKoopa();
    }
}

void PrimaryExpAST::Dump() const  {
        if (mode == 0) {
            std::cout << "( ";
            exp->Dump();
            std::cout << " )" << std::endl;
        }
        else if (mode == 1) {
            number->Dump();
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

void UnaryOpAST::DumpKoopa() const {
    //pass
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



