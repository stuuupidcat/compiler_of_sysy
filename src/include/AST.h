#pragma once
#include <memory>
#include <iostream>
#include <string>
#include <stddef.h>
#include <stdint.h>

 
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual void DumpKoopa() const = 0;

    static std::string temp_sign;
};  

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
  // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};


// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};

class FuncTypeAST : public BaseAST {   
public:
    std:: string s_int = "int";
    
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};

class BlockAST : public BaseAST {   
public:
    std::unique_ptr<BaseAST> stmt;
    
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};

class StmtAST : public BaseAST {   
public:
    std::string return_s = "return";
    std::unique_ptr<BaseAST> exp;
    std::string semicolon = ";";
    
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};

class NumberAST : public BaseAST {   
public:
    int num;
    
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};

//Exp         ::= UnaryExp;
class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> unaryexp;

    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
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

    virtual void Dump() const override;
    virtual void DumpKoopa() const override;           
};

//PrimaryExp  ::= "(" Exp ")" | Number;
class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> number;

    //mode == 0 -> "(" Exp ")"
    //mode == 1 -> Number
    int mode;

    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};

//UnaryOp     ::= "+" | "-" | "!";
class UnaryOpAST : public BaseAST {
public:
    int mode;
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};
