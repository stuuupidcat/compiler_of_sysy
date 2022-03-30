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
void Visit      (const koopa_raw_program_t&);
void Visit        (const koopa_raw_slice_t&);
void Visit     (const koopa_raw_function_t&);
void Visit  (const koopa_raw_basic_block_t&);
int  Visit        (const koopa_raw_value_t&);
int  Visit      (const koopa_raw_integer_t&);
int  Visit       (const koopa_raw_binary_t&);
void Visit       (const koopa_raw_return_t&);
int  Visit         (const koopa_raw_load_t&);
int  Visit        (const koopa_raw_store_t&);
int  Visit (const koopa_raw_global_alloc_t&);
int  Visit       (const koopa_raw_branch_t&);
int  Visit         (const koopa_raw_jump_t&);

int CountInsts       (const koopa_raw_slice_t&);
int CountInsts (const koopa_raw_basic_block_t&);
int CountInsts       (const koopa_raw_value_t&);

void KoopaStrToProgram(const char *);
bool IsExecuted(void *);
//将指令存到ins_result中,返回指令的结果在栈上的值.
int  StoreInsToMap(void *);
//将block对应的label存到ins_result中,返回对应的标签"L"+label_num;
//int  StoreBlockToMap(void *);

int AddReg(); 