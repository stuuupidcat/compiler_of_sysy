#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <string.h>
#include <vector>
#include <unordered_map>

#include "koopa.h"

// 函数声明
void        Visit       (const koopa_raw_program_t&);
void        Visit         (const koopa_raw_slice_t&);
void        Visit      (const koopa_raw_function_t&);
void        Visit   (const koopa_raw_basic_block_t&);
std::string Visit         (const koopa_raw_value_t&);
std::string Visit       (const koopa_raw_integer_t&);
std::string Visit        (const koopa_raw_binary_t&);
void        Visit        (const koopa_raw_return_t&);
std::string Visit          (const koopa_raw_load_t&);
std::string Visit         (const koopa_raw_store_t&);
std::string Visit  (const koopa_raw_global_alloc_t&);

int CountInsts       (const koopa_raw_slice_t&);
int CountInsts (const koopa_raw_basic_block_t&);
int CountInsts       (const koopa_raw_value_t&);

void KoopaStrToProgram(const char *, const char *);
bool IsExecuted(void *);
int  StoreInsToMap(void *);

std::string AddReg(); 

