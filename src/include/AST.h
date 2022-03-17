#pragma once
#include <memory>
#include <vector>
#include <iostream>
#include <string>
#include <stddef.h>
#include <stdint.h>
#include <stack>
#include <cassert>
#include <unordered_map>

class BaseAST;
typedef std::unique_ptr<BaseAST>* Value;

//for unordered_map hash
//...
class ValueHash
{
public:
    size_t operator() (const Value& value) const noexcept;
}; 

//for unordered_map
class ValueEqual
{
public:
    size_t operator() (const Value& val_1, const Value& val_2) const noexcept;
}; 

class ValueData{
public:
    //分配的标号。%n
    int no;
    //指令的类型。
    //"number", 直接输出
    //"single add/sub/eq", lhs为0的二元操作
    //"add/sub/mil/mod"
    //"return"
    std::string inst_type;
    Value lhs, rhs;
    ValueData(int, std::string, Value, Value);
    ValueData();
    std::string format();
};

ValueData AllocateValueData(int, std::string, Value, Value);
void PrintInstruction();

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual Value DumpKoopa(Value self)  = 0;
    

    int mode = 0;
};  


class CompUnitAST : public BaseAST {
public:
  // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    
    virtual Value DumpKoopa(Value self)  override;
};


// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    
    virtual Value DumpKoopa(Value self)  override;
};

class FuncTypeAST : public BaseAST {   
public:
    std:: string s_int = "int";
    
    virtual Value DumpKoopa(Value self)  override;
};

class BlockAST : public BaseAST {   
public:
    std::unique_ptr<BaseAST> stmt;
    
    virtual Value DumpKoopa(Value self)  override;
};

class StmtAST : public BaseAST {   
public:
    std::string return_s = "return";
    std::unique_ptr<BaseAST> exp;
    std::string semicolon = ";";
    
    virtual Value DumpKoopa(Value self)  override;
};

class NumberAST : public BaseAST {   
public:
    int num;
    
    virtual Value DumpKoopa(Value self)  override;
};

//Exp         ::= AddExp;
class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> addexp;

    virtual Value DumpKoopa(Value self)  override;
};

//UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
//mode == 0 -> PrimaryExp
//mode == 1 -> UnaryOp UnaryExp
class UnaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> primaryexp;
    std::unique_ptr<BaseAST> unaryop;
    std::unique_ptr<BaseAST> unaryexp;

    virtual Value DumpKoopa(Value self)  override;           
};

//PrimaryExp  ::= "(" Exp ")" | Number;
//mode == 0 -> "(" Exp ")"
//mode == 1 -> Number
class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> number;


    virtual Value DumpKoopa(Value self)  override;
};

//UnaryOp     ::= "+" | "-" | "!";
//mode == 0 -> +
//mode == 1 -> -
//mode == 2 -> !
class UnaryOpAST : public BaseAST {
public:
    virtual Value DumpKoopa(Value self)  override;
};

//MulExp      ::= UnaryExp | MulExp ("*" | "/" | "%") UnaryExp;
//mode == 0 -> UnaryExp
//mode == 1 -> MulExp * UnaryExp
//mode == 2 -> MulExp / UnaryExp
//mode == 3 -> MulExp % UnaryExp
class MulExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> unaryexp;
    std::unique_ptr<BaseAST> mulexp;
    
    virtual Value DumpKoopa(Value self)  override;
};

//AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
//mode == 0 -> MulExp
//mode == 1 -> AddExp + MulExp
//mode == 2 -> AddExp - MulExp
class AddExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> addexp;
    std::unique_ptr<BaseAST> mulexp;
    
    virtual Value DumpKoopa(Value self)  override;
};