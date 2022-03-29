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
//*****abort**********************************************************
//Record the instructions as (key, value) in an unordered-map.********
//We use the unique_ptr's pointer as the key, and a struct as value.**
//*****abort**********************************************************
typedef long Value;

//将键值对插入unordered_map,将键插入vector: insts
Value InsertValuedata(ValueData, Value);

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
    
    Value jump_cond=0;

    //load指令中变量的名字。
    std::string variable_name;

    ValueData(int, std::string, Value, Value, Value, std::string);
    ValueData() = default;
    
    std::string format();
};

//分配ValueData。增加temp_sign_num。
ValueData AllocateValueData(int, std::string&, Value, Value, std::string);

//各种指令的输出
void PrintInstruction();
//,以及符号表的删除。
void update();

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual Value DumpKoopa() = 0;
    int mode = 0;
    //变量的名字
    std::string ident;
    //变量通过下标处理后的名字。
    std::string ident_id;
};  

class VariableInfo {
public:
    Value value;
    bool is_const_variable = false;
    VariableInfo(Value, bool);
    VariableInfo() = default;
};


class CompUnitAST : public BaseAST {
public:
  // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    
    virtual Value DumpKoopa()  override;
};


// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::unique_ptr<BaseAST> block;
    
    virtual Value DumpKoopa()  override;
};

class FuncTypeAST : public BaseAST {   
public:
    std:: string s_int = "int";
    
    virtual Value DumpKoopa()  override;
};

//Block ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST {   
public:
    std::vector<std::unique_ptr<BaseAST>> blockitems;
    
    virtual Value DumpKoopa()  override;
};

//Stmt: 
// mode = 0 ->  ';' 
// mode = 1 -> "return" Exp ";"
// mode = 2 -> return ';' 
// mode = 3 -> LVal "=" Exp ";" 
// mode = 4 -> exp ';' 
// mode = 5 -> Block 
// mode = 6 -> ifstmt
class StmtAST : public BaseAST {   
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> block;

    //if
    std::unique_ptr<BaseAST> ifstmt;
    
    virtual Value DumpKoopa()  override;
};

//mode = 0; IF '(' Cond ')' Stmt 
//mode = 1; IF '(' Cond ')' Stmt ELSE Stmt
class IfStmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt;
    std::unique_ptr<BaseAST> elsestmt;
    
    virtual Value DumpKoopa()  override;
};

class NumberAST : public BaseAST {   
public:
    int num;
    
    virtual Value DumpKoopa()  override;
};

//Exp ::= LOrExp;
class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> lorexp;

    virtual Value DumpKoopa()  override;
};

//UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
//mode == 0 -> PrimaryExp
//mode == 1 -> UnaryOp UnaryExp
class UnaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> primaryexp;
    std::unique_ptr<BaseAST> unaryop;
    std::unique_ptr<BaseAST> unaryexp;

    virtual Value DumpKoopa()  override;           
};

//PrimaryExp ::= "(" Exp ")" | LVal | Number
//mode == 0, 1, 2;
class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> number;


    virtual Value DumpKoopa()  override;
};

//UnaryOp     ::= "+" | "-" | "!";
//mode == 0 -> +
//mode == 1 -> -
//mode == 2 -> !
class UnaryOpAST : public BaseAST {
public:
    virtual Value DumpKoopa()  override;
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
    
    virtual Value DumpKoopa()  override;
};

//AddExp      ::= MulExp | AddExp ("+" | "-") MulExp;
//mode == 0 -> MulExp
//mode == 1 -> AddExp + MulExp
//mode == 2 -> AddExp - MulExp
class AddExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> addexp;
    std::unique_ptr<BaseAST> mulexp;
    
    virtual Value DumpKoopa()  override;
};

//RelExp ::= AddExp | RelExp ("<" | ">" | "<=" | ">=") AddExp;
//mode = 0,1,2,3,4
class RelExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> relexp;
    std::unique_ptr<BaseAST> addexp;

    virtual Value DumpKoopa()  override;
};

//EqExp ::= RelExp | EqExp ("==" | "!=") RelExp;
//mode = 0,1,2
class EqExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> relexp;
    std::unique_ptr<BaseAST> eqexp;

    virtual Value DumpKoopa()  override;
};

//LAndExp::= EqExp | LAndExp "&&" EqExp;
//mode = 0,1
class LAndExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> eqexp;

    virtual Value DumpKoopa()  override;
};

//LOrExp::= LAndExp | LOrExp "||" LAndExp;
//mode = 0,1
class LOrExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> lorexp;

    virtual Value DumpKoopa()  override;
};

//Decl ::= ConstDecl | VarDecl;
class DeclAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> constdecl;
    std::unique_ptr<BaseAST> vardecl;
    virtual Value DumpKoopa()  override;
};

//ConstDecl ::= "const" "int" ConstDef {"," ConstDef} ";";
class ConstDeclAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST>> constdefs;

    virtual Value DumpKoopa()  override;
};

//ConstDef ::= IDENT "=" ConstInitVal;
class ConstDefAST : public BaseAST {
public:

    std::unique_ptr<BaseAST> constinitval;

    virtual Value DumpKoopa()  override;
};

//ConstInitVal ::= ConstExp;
class ConstInitValAST: public BaseAST {
public:
    std::unique_ptr<BaseAST> constexp;

    virtual Value DumpKoopa()  override;
};

//BlockItem ::= Decl | Stmt;
class BlockItemAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    virtual Value DumpKoopa()  override;
};

//LVal ::= IDENT;
class LValAST : public BaseAST {
public:
    //ast->ident = *unique_ptr<string>($n);

    virtual Value DumpKoopa()  override;
};

//ConstExp ::= Exp;
class ConstExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    virtual Value DumpKoopa()  override;
};

//VarDecl ::= BType VarDef {"," VarDef} ";";
class VarDeclAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST>> vardefs;

    virtual Value DumpKoopa()  override;
};

//VarDef        ::= IDENT | IDENT "=" InitVal;
class VarDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> initval;

    virtual Value DumpKoopa()  override;
};

//InitVal ::= Exp;
class InitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    virtual Value DumpKoopa()  override;
};

//向外层嵌套查找变量
std::unordered_map<std::string, VariableInfo>::iterator find_var_in_symbol_table(std::string&);
void change_varvalue_in_symbol_table(std::string&, Value);

