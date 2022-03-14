#pragma once
#include <memory>
#include <vector>
#include <iostream>
#include <string>
#include <stddef.h>
#include <stdint.h>
#include <stack>
#include <cassert>

class ASTResult;

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump()  = 0;
    virtual void DumpKoopa(ASTResult*)  = 0;


    int mode = 0;
};  

//利用指针查找表达式的结果储存在哪个以百分号开头的临时变量存储器中
class ASTResult {
public:
    std::unique_ptr<BaseAST>* ast_pointer = nullptr;
    int sign_num = 0;
    std::string sign_name; 

    ASTResult(std::unique_ptr<BaseAST>*, int);

};


//这个函数要对类中的每一个智能指针的指针调用。
ASTResult* IsASTAllocated(std::unique_ptr<BaseAST>*);
ASTResult* Allocate(std::unique_ptr<BaseAST> *);
ASTResult* StoreASTToVec(std::unique_ptr<BaseAST>*);
void delete_ast_res_vec();

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
  // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};


// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};

class FuncTypeAST : public BaseAST {   
public:
    std:: string s_int = "int";
    
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};

class BlockAST : public BaseAST {   
public:
    std::unique_ptr<BaseAST> stmt;
    
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};

class StmtAST : public BaseAST {   
public:
    std::string return_s = "return";
    std::unique_ptr<BaseAST> exp;
    std::string semicolon = ";";
    
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};

class NumberAST : public BaseAST {   
public:
    int num;
    
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};

//Exp         ::= AddExp;
class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> addexp;

    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};

//UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
//mode == 0 -> PrimaryExp
//mode == 1 -> UnaryOp UnaryExp
class UnaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> primaryexp;
    std::unique_ptr<BaseAST> unaryop;
    std::unique_ptr<BaseAST> unaryexp;


    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;           
};

//PrimaryExp  ::= "(" Exp ")" | Number;
//mode == 0 -> "(" Exp ")"
//mode == 1 -> Number
class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> number;


    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};

//UnaryOp     ::= "+" | "-" | "!";
//mode == 0 -> +
//mode == 1 -> -
//mode == 2 -> !
class UnaryOpAST : public BaseAST {
public:
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
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
    
    
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};

//AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
//mode == 0 -> MulExp
//mode == 1 -> AddExp + MulExp
//mode == 2 -> AddExp - MulExp
class AddExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> addexp;
    std::unique_ptr<BaseAST> mulexp;
    
    
    virtual void Dump()  override;
    virtual void DumpKoopa(ASTResult*)  override;
};