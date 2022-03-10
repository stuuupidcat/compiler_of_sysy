#include <string>
#include <iostream>
#include <memory>


/*
    CompUnit  ::= FuncDef;

    FuncDef   ::= FuncType IDENT "(" ")" Block;
    FuncType  ::= "int";

    Block     ::= "{" Stmt "}";
    Stmt      ::= "return" Number ";";
    Number    ::= INT_CONST;
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
    std::unique_ptr<BaseAST> number;
    std::string semicolon = ";";
    
    void Dump() const override {
        std::cout << "StmtAST { ";
        number -> Dump();
        std::cout << " }";
    }

    void DumpKoopa() const override {
        std::cout << "    ret ";
        number->DumpKoopa();
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


