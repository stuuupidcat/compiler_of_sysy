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
//class BaseAST;
class CompUnitAST;
int yylex();
void yyerror(std::unique_ptr<CompUnitAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
// (其他相关声明修改？)
%parse-param { std::unique_ptr<CompUnitAST> &ast }


// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<BaseAST*> *ast_vec_val;
  FuncRParamsAST *param_val;

  ArrayInitValAST *array_init_val;
  
}


// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN
%token ASSIGN LT GT LE GE EQ NE ADD SUB
%token AND OR 
%token CONST IF ELSE
%token WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> CompUnit FuncDef Block Stmt Number 
%type <ast_val> Exp UnaryExp PrimaryExp UnaryOp ConstExp
%type <ast_val> RelExp EqExp LAndExp LOrExp MulExp AddExp
%type <ast_val> ConstDef ConstInitVal BlockItems BlockItem LVal             
%type <ast_val> VarDef InitVal IfStmt WhileStmt 
%type <ast_val> FuncFParams FuncFParam Decl ConstDecl VarDecl 

//{"[" ConstExp "]"}
%type <ast_vec_val> ConstExpsWithBrackets
//{"[" Exp "]"}
%type <ast_vec_val> ExpsWithBrackets

%type <array_init_val> ArrayInitVal
%type <array_init_val> ArrayInitValItems
//"{" ConstInitVal {"," ConstInitVal} "}";
%type <ast_vec_val> ConstExps

%type <param_val> FuncRParams 

//https://stackoverflow.com/questions/12731922/reforming-the-grammar-to-remove-shift-reduce-conflict-in-if-then-else
// Precedences go increasing, so "then" < "else".
// 越靠下的见到 优先shift
%precedence EXP
%precedence COMMA

%precedence THEN
%precedence ELSE

%precedence LE GE LT GT
%precedence ADD SUB

%precedence lval
%precedence '(' ')'

%left ','

%start CompUnit

%%

//CompUnit ::= [CompUnit] (Decl | FuncDef);
CompUnit
  : FuncDef {
    auto compunit = make_unique<CompUnitAST>();
    compunit->funcdefs.push_back(unique_ptr<BaseAST>($1));
    ast = move(compunit);
  }
  | Decl {
    auto compunit = make_unique<CompUnitAST>();
    compunit->decls.push_back(unique_ptr<BaseAST>($1));
    ast = move(compunit);
  }
  | CompUnit FuncDef{
    ast->funcdefs.push_back(unique_ptr<BaseAST>($2));
  }
  
  | CompUnit Decl{
    ast->decls.push_back(unique_ptr<BaseAST>($2));
  }
  | CompUnit ';'
  {}
  
  ;


FuncDef
  : VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->mode = 0;
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->mode = 1;
    ast->ident = *unique_ptr<string>($2);
    ast->funcfparams = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);

    
    $$ = ast;
  }
  | INT IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->mode = 2;
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | INT IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->mode = 3;
    ast->ident = *unique_ptr<string>($2);
    ast->funcfparams = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;
//FuncFParams ::= FuncFParam {"," FuncFParam};
FuncFParams
  : FuncFParam {
    auto ast = new FuncFParamsAST();
    ast->funcfparams.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | FuncFParams ',' FuncFParam {
    ((FuncFParamsAST*)$1)->funcfparams.push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

FuncFParam
  : INT IDENT {
    auto ast = new FuncFParamAST();
    ast->mode = 0;
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | INT IDENT '[' ']' {
    auto ast = new FuncFParamAST();
    ast->mode = 1;
    ast->arr_pt_dim = 1;
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | INT IDENT '[' ']' ConstExpsWithBrackets {
    auto ast = new FuncFParamAST();
    ast->mode = 2;
    ast->arr_pt_dim = 1;
    ast->ident = *unique_ptr<string>($2);
    for (auto &pt: *$5) {
      ast->constexps.push_back(unique_ptr<BaseAST>(pt));
      ast->arr_pt_dim++;
    }
    $$ = ast;
  }
  ;

//FuncRParams ::= Exp {"," Exp};
FuncRParams
  : Exp {
    auto ast = new FuncRParamsAST();
    ast->exps.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | FuncRParams ',' Exp {
    (((FuncRParamsAST*)$1)->exps).push_back(unique_ptr<BaseAST>($3));
    $$ = $1;
  }
  ;

//Block ::= "{" {BlockItem} "}";
Block
  : '{' BlockItems '}'{
    $$ = $2;
  }
  | '{' '}' {
    auto ast = new BlockAST();
    $$ = ast;
  }
  ;

BlockItems
  : BlockItem {
    auto ast = new BlockAST();
    ast->blockitems.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | BlockItems BlockItem {
    $$ = $1;
    ((BlockAST*)$$)->blockitems.push_back(unique_ptr<BaseAST>($2));
  }
  ;

Stmt
  : ';' {
    auto ast = new StmtAST();
    ast->mode = 0;
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    // auto number = $2; 
    // 该number一直输出261, 这里的Number应该是? ->应该是INT_CONST的值。
    // -> 示例程序中将Number写成了int_val
    // 我们将其变为BaseAST
    ast-> mode = 1;
    ast -> exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    // auto number = $2; 
    // 该number一直输出261, 这里的Number应该是? ->应该是INT_CONST的值。
    // -> 示例程序中将Number写成了int_val
    // 我们将其变为BaseAST
    ast-> mode = 2;
    $$ = ast;
  }
  | LVal ASSIGN Exp ';' {
    auto ast = new StmtAST();
    ast->mode = 3;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->lval->is_left = true; 
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->mode = 4;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->mode = 5;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IfStmt {
    auto ast = new StmtAST();
    ast->mode = 6;
    ast->ifstmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | WhileStmt {
    auto ast = new StmtAST();
    ast->mode = 7;
    ast->whilestmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->mode = 8;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->mode = 9;
    $$ = ast;
  }
  ;

IfStmt
  : IF '(' Exp ')' Stmt              %prec THEN {            
    auto ast = new IfStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->mode = 0;
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt ELSE Stmt    %prec ELSE{
    auto ast = new IfStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    ast->elsestmt = unique_ptr<BaseAST>($7);
    ast->mode = 1;
    $$ = ast;
  }

WhileStmt:
  WHILE '(' Exp ')' Stmt {
    auto ast = new WhileStmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    auto ast = new NumberAST();
    ast->num = ($1);

    //将integer_sign初始化为数字。
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast -> lorexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast -> primaryexp = unique_ptr<BaseAST>($1);
    ast -> mode = 0;
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast -> unaryop = unique_ptr<BaseAST>($1);
    ast -> unaryexp = unique_ptr<BaseAST>($2);
    ast -> mode = 1;
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast -> ident = *unique_ptr<string>($1);
    ast -> mode = 2;
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    auto ast = new UnaryExpAST();
    ast->mode = 3;
    ast->ident = *unique_ptr<string>($1);
    ast->funcrparams = unique_ptr<FuncRParamsAST>($3);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast -> exp = unique_ptr<BaseAST>($2);
    ast -> mode = 0;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast -> lval = unique_ptr<BaseAST>($1);
    ast -> mode = 1;
    $$ = ast;
  } 
  | Number {
    auto ast = new PrimaryExpAST();
    ast -> number = unique_ptr<BaseAST>($1);
    ast -> mode = 2;
    $$ = ast;
  }
  ;

UnaryOp
  : ADD {
    auto ast = new UnaryOpAST();
    ast -> mode = 0;
    $$ = ast;
  }
  | SUB {
    auto ast = new UnaryOpAST();
    ast -> mode = 1;
    $$ = ast;
  }
  | '!' {
    auto ast = new UnaryOpAST();
    ast -> mode = 2;
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast -> unaryexp = unique_ptr<BaseAST>($1);
    ast -> mode = 0;
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast -> mulexp = unique_ptr<BaseAST>($1);
    ast -> unaryexp = unique_ptr<BaseAST>($3);
    ast -> mode = 1;
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast -> mulexp = unique_ptr<BaseAST>($1);
    ast -> unaryexp = unique_ptr<BaseAST>($3);
    ast -> mode = 2;
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast -> mulexp = unique_ptr<BaseAST>($1);
    ast -> unaryexp = unique_ptr<BaseAST>($3);
    ast -> mode = 3;
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast -> mulexp = unique_ptr<BaseAST>($1);
    ast -> mode = 0;
    $$ = ast;
  }
  | AddExp ADD MulExp  {
    auto ast = new AddExpAST();
    ast -> addexp = unique_ptr<BaseAST>($1);
    ast -> mulexp = unique_ptr<BaseAST>($3);
    ast -> mode = 1;
    $$ = ast;
  }
  | AddExp SUB MulExp {
    auto ast = new AddExpAST();
    ast -> addexp = unique_ptr<BaseAST>($1);
    ast -> mulexp = unique_ptr<BaseAST>($3);
    ast -> mode = 2;
    $$ = ast;
  }
  ;

RelExp
  : AddExp  %prec LT {
    auto ast = new RelExpAST();
    ast -> addexp = unique_ptr<BaseAST>($1);
    ast->mode = 0;
    $$ = ast;
  }
  | RelExp LT AddExp  {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<BaseAST>($1);
    ast->addexp = unique_ptr<BaseAST>($3);
    ast->mode = 1;
    $$ = ast;
  }
  | RelExp GT AddExp  {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<BaseAST>($1);
    ast->addexp = unique_ptr<BaseAST>($3);
    ast->mode = 2;
    $$ = ast;
  }
  | RelExp LE AddExp  {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<BaseAST>($1);
    ast->addexp = unique_ptr<BaseAST>($3);
    ast->mode = 3;
    $$ = ast;
  }
  | RelExp GE AddExp {
    auto ast = new RelExpAST();
    ast->relexp = unique_ptr<BaseAST>($1);
    ast->addexp = unique_ptr<BaseAST>($3);
    ast->mode = 4;
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->relexp = unique_ptr<BaseAST>($1);
    ast->mode = 0;
    $$ = ast;
  }
  | EqExp EQ RelExp {
    auto ast = new EqExpAST();
    ast->eqexp = unique_ptr<BaseAST>($1);
    ast->relexp = unique_ptr<BaseAST>($3);
    ast->mode = 1;
    $$ = ast;
  }
  | EqExp NE RelExp {
    auto ast = new EqExpAST();
    ast->eqexp = unique_ptr<BaseAST>($1);
    ast->relexp = unique_ptr<BaseAST>($3);
    ast->mode = 2;
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eqexp = unique_ptr<BaseAST>($1);
    ast->mode = 0;
    $$ = ast;
  }
  | LAndExp AND EqExp {
    auto ast = new LAndExpAST();
    ast->landexp = unique_ptr<BaseAST>($1);
    ast->eqexp = unique_ptr<BaseAST>($3);
    ast->mode = 1;
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->landexp = unique_ptr<BaseAST>($1);
    ast->mode = 0;
    $$ = ast;
  }
  | LOrExp OR LAndExp {
    auto ast = new LOrExpAST();
    ast->lorexp = unique_ptr<BaseAST>($1);
    ast->landexp = unique_ptr<BaseAST>($3);
    ast->mode = 1;
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->constdecl = unique_ptr<BaseAST>($1);
    ast->mode = 0;
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->vardecl = unique_ptr<BaseAST>($1);
    ast->mode = 1;
    $$ = ast;
  }
  ;

//如何处理{}+?         
ConstDecl 
  : CONST INT ConstDef  {
    auto ast = new ConstDeclAST();
    ast->constdefs.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  | ConstDecl ',' ConstDef {
    ((ConstDeclAST*)$$)->constdefs.push_back(unique_ptr<BaseAST>($3));
    //$$ = ast;
  }
  ;

//ConstDef ::= IDENT {"[" ConstExp "]"} "=" ConstInitVal;
ConstDef
  : IDENT ASSIGN ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->constinitval = unique_ptr<BaseAST>($3);
    ast->mode = 0;
    $$ = ast;
  }
  | IDENT ConstExpsWithBrackets ASSIGN ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    //???
    for (auto &pt: *$2) {
      ast->constexps.push_back(unique_ptr<BaseAST>(pt));
    }
    ast->constinitval = unique_ptr<BaseAST>($4);
    ast->mode = 1;
    $$ = ast;
  } 
  ;

//ConstInitVal  ::= ConstExp | "{" [ConstExp {"," ConstExp}] "}";
ConstInitVal  
  :ConstExp {
    auto ast = new ConstInitValAST();
    ast->mode = 0;
    ast->constexp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ArrayInitVal {
    auto ast = new ConstInitValAST();
    ast->mode = 1;
    for (auto &pt: $1->constexps) {
      ast->constexps.push_back(unique_ptr<BaseAST>(pt));
    }
    ast->braces = $1->braces;
    $$ = ast;
  }

BlockItem
  :Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    ast->mode = 0;
    $$ = ast;
  } 
  | Stmt{
    auto ast = new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    ast->mode = 1;
    $$ = ast;
  }
  ;

LVal
  :IDENT %prec lval{
    auto ast = new LValAST();
    ast->mode = 0;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT ExpsWithBrackets {
    auto ast = new LValAST();
    ast->mode = 1;
    ast->ident = *unique_ptr<string>($1);
    for (auto &pt: *$2) {
      ast->exps.push_back(unique_ptr<BaseAST>(pt));
    }
    $$ = ast;
  }
  ;

ConstExp
  :Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  };

VarDecl 
  : INT VarDef  {
    auto ast = new VarDeclAST();
    ast->vardefs.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | VarDecl ',' VarDef {
    ((VarDeclAST*)$$)->vardefs.push_back(unique_ptr<BaseAST>($3));
    //$$ = ast;
  }
  ;
VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->mode = 0;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT ConstExpsWithBrackets {
    auto ast = new VarDefAST();
    ast->mode = 2;
    ast->ident = *unique_ptr<string>($1);
    for (auto &pt: *$2) {
      ast->constexps.push_back(unique_ptr<BaseAST>(pt));
    }
    //相当于用大括号初始化。
    auto initval = new InitValAST();
    initval->mode = 1;
    ast->initval = unique_ptr<BaseAST>(initval);
    $$ = ast;
  }
  | IDENT ASSIGN InitVal {
    auto ast = new VarDefAST();
    ast->mode = 1;
    ast->ident = *unique_ptr<string>($1);
    ast->initval = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ConstExpsWithBrackets ASSIGN InitVal {
    auto ast = new VarDefAST();
    ast->mode = 3;
    ast->ident = *unique_ptr<string>($1);
    for (auto &pt: *$2) {
      ast->constexps.push_back(unique_ptr<BaseAST>(pt));
    }
    ast->initval = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal 
  : Exp  {
    auto ast = new InitValAST();
    ast->mode = 0;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ArrayInitVal {
    auto ast = new InitValAST();
    ast->mode = 1;
    for (auto &pt: $1->constexps) {
      ast->exps.push_back(unique_ptr<BaseAST>(pt));
    }
    ast->braces = $1->braces;
    $$ = ast;
  }
  ;

ConstExpsWithBrackets:
  '[' ConstExp ']' {
    auto vec = new vector<BaseAST*>();
    vec->push_back($2);
    $$ = vec;
  }
  | ConstExpsWithBrackets '[' ConstExp ']'{
    ($1)->push_back($3);
  }
  ;

ExpsWithBrackets:
  '[' Exp ']' {
    auto vec = new vector<BaseAST*>();
    vec->push_back($2);
    $$ = vec;
  }
  | ExpsWithBrackets '[' Exp ']' {
    ($1)->push_back($3);
  }
  ;

//ConstInitVal  ::= ConstExp | "{" [ConstInitVal {"," ConstInitVal}] "}";
//"{" [ConstInitVal {"," ConstInitVal}] "}";
//中间的出现零次或一次的部分单独处理，变为
//"{" ConstInitVal {"," ConstInitVal} "}";
//我们将其中的每一项都拆分为一个constexp外面可以拥有一个或者多个大括号。

//没有处理括号层数的属性。

ArrayInitVal:
  '{' '}' {
    auto ast = new ArrayInitValAST();
    ast->braces = "{}";
    $$ = ast;
  }
  | '{' ArrayInitValItems '}' {
    ($2)->braces = '{' +  ($2)->braces + '}';
    $$ = $2;
  }
  ;

ArrayInitValItems:
  ConstExp %prec EXP{
    auto ast = new ArrayInitValAST();
    ast->constexps.push_back($1);
    ast->braces = 'e';
    $$ = ast;
  }
  | ConstExp ',' ArrayInitVal  %prec COMMA{
    ($3)->constexps.insert(($3)->constexps.begin(), $1);
    ($3)->braces = 'e' + ($3)->braces;
    $$ = $3;
  }
  | ConstExps %prec EXP{
    auto ast = new ArrayInitValAST();
    for (auto &pt: *$1) {
      ast->constexps.push_back(pt);
      ast->braces += 'e';
    }
    
    $$ = ast;
  }
  | ConstExps ',' ArrayInitVal  %prec COMMA{ 
    for (int i = $1->size() - 1; i >= 0; i--) {
      ($3)->constexps.insert(($3)->constexps.begin(), $1->at(i));
      ($3)->braces = 'e' + ($3)->braces;
    }
    $$ = $3;
  }
  | ArrayInitVal {
    $$ = $1;
  }
  | ArrayInitValItems ',' ConstExp {
    ($1)->constexps.push_back($3);
    ($1)->braces += 'e';
  }
  | ArrayInitValItems ',' ArrayInitVal  {
    for (auto &pt: $3->constexps) {
      ($1)->constexps.push_back(pt);
      
    }
    ($1)->braces += ($3)->braces;
    $$ = $1;
  }

ConstExps:
  ConstExp ',' ConstExp  {
    auto vec = new vector<BaseAST*>();
    vec->push_back($1);
    vec->push_back($3);
    $$ = vec;
  }
  | ConstExps ',' ConstExp {
    ($1)->push_back($3);
    $$ = $1;
  }
  ;



%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<CompUnitAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
