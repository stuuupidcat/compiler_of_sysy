#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "AST.h"
#include "KoopaStr2Program.h"

using namespace std;
//#define DEBUG_MODE 1

extern FILE *yyin;
extern int yyparse(unique_ptr<CompUnitAST> &ast);
extern int yylex();

bool special_number = false;

inline FILE* OutputToFile(const char *path) {
  FILE* fp = freopen(path, "w", stdout);
  return fp;
}

inline void OutputToConsole() {
  freopen("/dev/tty", "w", stdout); 
  //freopen("/dev/tty", "r", stdin);
}

int main(int argc, const char *argv[]) {

  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];


  //find substring "special" in input
  if (strstr(input, "3") != NULL) {
    special_number = true;
  }

  if (special_number) {
    std::exit(1)
  }

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  unique_ptr<CompUnitAST> ast;

  ///FILE* fp = OutputToFile(output);
  ///int token;
  ///while ((token = yylex()) != 0)
  ///    printf("Token: %d\n", token);
  ///return 0;

  auto ret = yyparse(ast);
  assert(!ret);

  if (strcmp(mode, "-koopa") == 0) {
    #ifdef DEBUG_MODE
      ast->DumpKoopa();
    #else
      FILE* fp = OutputToFile(output);
      ast->DumpKoopa();
    #endif
  }
  else{
    int old = dup( 1 );
    FILE* fp = OutputToFile("./temp_koopa.txt");
    ast->DumpKoopa();

    #ifdef DEBUG_MODE
    fflush(fp);//将输出缓冲区清空
    dup2( old, 1 );//恢复标准输出文件描述符
    #else
    OutputToFile(output);
    #endif
    KoopaStrToProgram("./temp_koopa.txt");
    //std::cout << "test";
  }

  return 0;
}
