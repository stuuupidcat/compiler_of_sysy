#pragma once
#include <memory>
#include <vector>
#include <random>
#include <iostream>
#include <string>
#include <stddef.h>
#include <stdint.h>
#include <stack>
#include <cassert>
#include <unordered_map>

class BaseAST;
class ValueData;

//Record the instructions as (key, value) in an unordered-map.
//We use the unique_ptr's pointer as the key, and a struct as value.
typedef std::unique_ptr<BaseAST>* Value;

//将键值对插入unordered_map,将键插入vector: insts
void InsertKVToMap(Value, ValueData);

//For unordered_map hash function.
class ValueHash
{
public:
    size_t operator() (const Value& value) const noexcept;
}; 

//For unordered_map equal function.
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
    //"number"            -> 不用输出。数字存储在lhs中。
    //"single add/sub/eq" -> 左操作数为0的二元操作。
    //"add/sub/mil/mod"   -> 二元操作。
    //"return"            -> 右操作数为返回值的二元操作。
    std::string inst_type;

    //利用lhs,rhs去unordered_map中查找对应的ValueData.
    //只有一个操作数的用lhs.
    Value lhs, rhs;

    ValueData(int, std::string, Value, Value);
    ValueData() = default;
    
    std::string format();
};

//分配ValueData。增加temp_sign_num。
ValueData AllocateValueData(int, std::string, Value, Value);

//各种指令的输出。
void PrintInstruction();

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual Value DumpKoopa(Value self) = 0;
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

//Block ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST {   
public:
    std::vector<std::unique_ptr<BaseAST>> blockitems;
    
    virtual Value DumpKoopa(Value self)  override;
};

//Stmt:  ';' | "return" Exp ";"
class StmtAST : public BaseAST {   
public:
    std::unique_ptr<BaseAST> exp;
    
    virtual Value DumpKoopa(Value self)  override;
};

class NumberAST : public BaseAST {   
public:
    int num;
    
    virtual Value DumpKoopa(Value self)  override;
};

//Exp ::= LOrExp;
class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> lorexp;

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

//PrimaryExp ::= "(" Exp ")" | LVal | Number
//mode == 0, 1, 2;
class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
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

//RelExp ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
//mode = 0,1,2,3,4
class RelExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> relexp;
    std::unique_ptr<BaseAST> addexp;

    virtual Value DumpKoopa(Value self)  override;
};

//EqExp ::= RelExp | EqExp ("==" | "!=") RelExp;
//mode = 0,1,2
class EqExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> relexp;
    std::unique_ptr<BaseAST> eqexp;

    virtual Value DumpKoopa(Value self)  override;
};

//LAndExp::= EqExp | LAndExp "&&" EqExp;
//mode = 0,1
class LAndExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> eqexp;

    virtual Value DumpKoopa(Value self)  override;
};

//LOrExp::= LAndExp | LOrExp "||" LAndExp;
//mode = 0,1
class LOrExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> lorexp;

    virtual Value DumpKoopa(Value self)  override;
};

//Decl ::= ConstDecl;
class DeclAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> constdecl;

    virtual Value DumpKoopa(Value self)  override;
};

//ConstDecl ::= "const" "int" ConstDef {"," ConstDef} ";";
class ConstDeclAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST>> constdefs;

    virtual Value DumpKoopa(Value self)  override;
};

//ConstDef ::= IDENT "=" ConstInitVal;
class ConstDefAST : public BaseAST {
public:

    std::string ident;
    std::unique_ptr<BaseAST> constinitval;

    virtual Value DumpKoopa(Value self)  override;
};

//ConstInitVal ::= ConstExp;
class ConstInitValAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> constexp;

    virtual Value DumpKoopa(Value self)  override;
};

//BlockItem ::= Decl | Stmt;
class BlockItemAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    virtual Value DumpKoopa(Value self)  override;
};

//LVal ::= IDENT;
class LValAST : public BaseAST {
public:
    //ast->ident = *unique_ptr<string>($n);
    std::string ident;

    virtual Value DumpKoopa(Value self)  override;
};

//ConstExp ::= Exp;
class ConstExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    virtual Value DumpKoopa(Value self)  override;
};
