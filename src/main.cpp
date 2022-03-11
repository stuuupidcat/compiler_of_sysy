#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>

#include "AST.h"

using namespace std;

//#define DEBUG_MODE 0

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
  
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  // dump AST
  // ast->Dump();
  
  //dump Koopa IR
  //cd ./compiler;  make clean; make; ./build/compiler -koopa ./debug/hello.c -o ./debug/output

#ifdef DEBUG_MODE

#else
  freopen(output, "w", stdout);
#endif

  if (strcmp(mode, "-koopa") == 0)
    ast->DumpKoopa();

  return 0;
}
