#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>

#include "koopa.h"

class InstResPos {
public:
    //mode = 0 ->on stack;
    //mode = 1 ->on t-register
    //mode = 2 ->on a-register
    //mode = 3 ->a global var
    int mode = 0;
    bool on_stack = false; //mode = 0
    bool global_var = false; //mode = 3
    int pos = 0;
    std::string format;
    InstResPos(int, int);
    InstResPos() = default;
};

// 函数声明
void Visit      (const koopa_raw_program_t&);
void Visit        (const koopa_raw_slice_t&);
void Visit     (const koopa_raw_function_t&);
void Visit  (const koopa_raw_basic_block_t&);
InstResPos  Visit        (const koopa_raw_value_t&);
InstResPos  Visit      (const koopa_raw_integer_t&);
InstResPos  Visit       (const koopa_raw_binary_t&);
void Visit       (const koopa_raw_return_t&);
InstResPos  Visit         (const koopa_raw_load_t&);
void  Visit        (const koopa_raw_store_t&);
InstResPos  Visit (const koopa_raw_global_alloc_t&, std::string);
void  Visit       (const koopa_raw_branch_t&);
void  Visit         (const koopa_raw_jump_t&);
InstResPos  Visit         (const koopa_raw_call_t&);
InstResPos  Visit (const koopa_raw_func_arg_ref_t&);

void CountRSA       (const koopa_raw_slice_t&);
void CountRSA (const koopa_raw_basic_block_t&);
void CountRSA       (const koopa_raw_value_t&);


void KoopaStrToProgram(const char *);
bool IsExecuted(void *);
//StoreInsToMap的值一定存储在栈上
InstResPos StoreInsToMap(void *);
//将block对应的label存到ins_result中,返回对应的标签"L"+label_num;
//int  StoreBlockToMap(void *);

int AddReg(); 