%code requires {
  #include <memory>
  #include <string>
  #include "AST.h"
}

%{

#include <iostream>
#include <memory>
#include <string>


// 声明 lexer 函数和错误处理函数
class BaseAST;
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
// (其他相关声明修改？)
%parse-param { std::unique_ptr<BaseAST> &ast }


// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}


// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt Number Exp Unary PrimaryExp UnaryOp

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

//expr : expr PLUS term {语义动作}
//     | term {语义动作}
//     ; 
// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值、
FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1); // dollar符号匹配了该代码块第二行的参数
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

// 同上, 不再解释
FuncType
  : INT {
    auto ast = new FuncTypeAST();
    $$ = ast;
  }
  ;

Block
  : '{' Stmt '}' {
    auto ast = new BlockAST();
    ast -> stmt = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    // auto number = $2; 
    // 该number一直输出261, 这里的Number应该是? ->应该是INT_CONST的值。
    // -> 示例程序中将Number写成了int_val
    // 我们将其变为BaseAST
    ast -> exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->num = ($1);
    $$ = ast;
  }
  ;

Exp
  : UnaryExp {
    auto ast = new ExpAST();
    ast -> unaryaxp = ($1);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryAST();
    ast -> primaryaxp = ($1);
    ast -> mode = 0;
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryAST();
    ast -> unaryop = ($1);
    ast -> unaryexp = ($2);
    ast -> mode = 1;
    $$ = ast
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast -> primaryaxp = ($2);
    ast -> mode = 0;
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast -> number = ($1);
    ast -> mode = 1;
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    auto ast = new UnaryOpAST();
    ast -> mode = 0;
    $$ = ast;
  }
  : '-' {
    auto ast = new UnaryOpAST();
    ast -> mode = 1;
    $$ = ast;
  }
  : '!' {
    auto ast = new UnaryOpAST();
    ast -> mode = 2;
    $$ = ast;
  }
  ;
%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
