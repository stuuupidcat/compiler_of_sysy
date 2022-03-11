#include <memory>
#include <iostream>
#include <stddef.h>
#include <stdint.h>

/*
    lv3:

    CompUnit  ::= FuncDef;

    FuncDef   ::= FuncType IDENT "(" ")" Block;
    FuncType  ::= "int";

    Block     ::= "{" Stmt "}";
    Stmt      ::= "return" Exp ";";  


    Exp         ::= LOrExp;
    PrimaryExp  ::= "(" Exp ")" | Number;
    Number      ::= INT_CONST;
    UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
    UnaryOp     ::= "+" | "-" | "!";
    MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
    AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
    RelExp      ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
    EqExp       ::= RelExp | EqExp ("==" | "!=") RelExp;
    LAndExp     ::= EqExp | LAndExp "&&" EqExp;
    LOrExp      ::= LAndExp | LOrExp "||" LAndExp;

*/

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual void DumpKoopa() const = 0;
};  

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
  // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    
    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }" << std::endl;
    }

    void DumpKoopa() const override {
        std::cout << "fun ";
        func_def -> DumpKoopa();
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    
    void Dump() const override {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << " }";
    }

    void DumpKoopa() const override {
        std::cout << "@" << ident << "(): ";
        func_type -> DumpKoopa();
        block -> DumpKoopa();
    }
};

class FuncTypeAST : public BaseAST {   
public:
    std:: string s_int = "int";
    
    void Dump() const override {
        std::cout << "FuncTypeAST { ";
        std::cout <<  s_int ;
        std::cout << " }";
    }

    void DumpKoopa() const override {
        std::cout << "i32 ";
    }
};

class BlockAST : public BaseAST {   
public:
    std::unique_ptr<BaseAST> stmt;
    
    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }

    void DumpKoopa() const override {
        std::cout << "{" << std::endl;
        std::cout << "%entry:" << std::endl;
        stmt->DumpKoopa();
        std::cout << '}' << std::endl;
    }
};

class StmtAST : public BaseAST {   
public:
    std::string return_s = "return";
    std::unique_ptr<BaseAST> exp;
    std::string semicolon = ";";
    
    void Dump() const override {
        std::cout << "StmtAST { ";
        exp -> Dump();
        std::cout << " }";
    }

    void DumpKoopa() const override {
        std::cout << "    ret ";
        exp->DumpKoopa();
    }
};

class NumberAST : public BaseAST {   
public:
    int num;
    
    void Dump() const override {
        std::cout << num;
    }

    void DumpKoopa() const override {
        std::cout << num << std::endl;
    }
};

//Exp         ::= UnaryExp;
class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> unaryexp;

    void Dump() const override {
        unaryexp -> Dump();
    }

    void DumpKoopa() const override {
        unaryexp -> DumpKoopa();
    }
};

//UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> primaryexp;
    std::unique_ptr<BaseAST> unaryop;
    std::unique_ptr<BaseAST> unaryexp;

    //mode == 0 -> PrimaryExp
    //mode == 1 -> UnaryOp UnaryExp
    int mode; 

    void Dump() const override {
        if (mode == 0) {
            primaryexp->Dump();
        }
        else if (mode == 1) {
            unaryop->Dump();
            unaryexp->Dump();
        }
    }

    void DumpKoopa() const override {
        if (mode == 0) {
            primaryexp->DumpKoopa();
        }
        else if (mode == 1) {
            unaryop->DumpKoopa();
            unaryexp->DumpKoopa();
        }
    }            
};

//PrimaryExp  ::= "(" Exp ")" | Number;
class PrimaryExp : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> number;

    //mode == 0 -> "(" Exp ")"
    //mode == 1 -> Number
    int mode;

    void Dump() const override {
        if (mode == 0) {
            std::cout << "( ";
            exp->Dump();
            std::cout << " )" << std::endl;
        }
        else if (mode == 1) {
            number->Dump();
        }
    }

    void DumpKoopa() const override {
        if (mode == 0) {
            exp->DumpKoopa();
        }
        else if (mode == 1) {
            number->DumpKoopa();
        }
    }
};

//UnaryOp     ::= "+" | "-" | "!";
class UnaryOpAST : public BaseAST {
    int mode;
    void Dump() const override {
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
    void DumpKoopa() const override {
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
};
