#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>

#include "AST.h"
#include "KoopaStr2Program.h"

using namespace std;
//#define DEBUG_MODE 0

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

inline void OutputToFile(const char *path) {
  freopen(path, "w", stdout);
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

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

#ifdef DEBUG_MODE
#else
#endif

  if (strcmp(mode, "-koopa") == 0) {
    OutputToFile(output);
    ast->DumpKoopa(nullptr);
  }
  else if (strcmp(mode, "-riscv") == 0) {
    
    OutputToFile("./temp/temp_koopa");
    ast->DumpKoopa(nullptr);

    OutputToFile(output);
    KoopaStrToProgram("./temp/temp_koopa", output);
  }
    
  OutputToConsole();
  return 0;
}
