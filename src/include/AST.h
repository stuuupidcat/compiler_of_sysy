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

    //暂时使用的以百分号为开头的标号。
    static std::string temp_sign; 
    //暂时使用的以百分号为开头的标号[1:]。
    static int temp_sign_num;
    //表达式中的数字
    static std::string integer_sign;
    static bool   integer_sign_used;

    //分配temp_sign
    static std::string  AllocTempSign();
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

//Exp         ::= AddExp;
class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> addexp;

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

//MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
class MulExpAST : public BaseAST {
public:
    //mode == 0 -> UnaryExp
    //mode == 1 -> MulExp * UnaryExp
    //mode == 2 -> MulExp / UnaryExp
    //mode == 3 -> MulExp % UnaryExp
    std::unique_ptr<BaseAST> unaryexp;
    std::unique_ptr<BaseAST> mulexp;
    int mode;
    
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};

//AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
class AddExpAST : public BaseAST {
public:
    //mode == 0 -> MulExp
    //mode == 1 -> AddExp + MulExp
    //mode == 2 -> AddExp - MulExp
    std::unique_ptr<BaseAST> addexp;
    std::unique_ptr<BaseAST> mulexp;
    int mode;
    
    virtual void Dump() const override;
    virtual void DumpKoopa() const override;
};