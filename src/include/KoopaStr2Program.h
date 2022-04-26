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

class InstResult {
public:
    //mode = 0 ->on stack;
    //mode = 1 ->on t-register
    //mode = 2 ->on a-register
    //mode = 3 ->a global var
    int mode = 0;
    bool on_stack = false; //mode = 0
    bool global_var = false; //mode = 3
    bool is_pointer = false; //例子：在4(sp)处存放了一个值为0+sp, 需要loadword。
    //在栈或在寄存器上的位置（4（sp））or a1
    int pos = 0;
    std::string format;
    //类型大小
    int type_size = 0;
    InstResult(std::string mode_, int pos_, int type_size_ = 4);
    InstResult() = default;
};

// 函数声明
void Visit      (const koopa_raw_program_t&);
void Visit        (const koopa_raw_slice_t&);
void Visit     (const koopa_raw_function_t&);
void Visit  (const koopa_raw_basic_block_t&);
void Visit       (const koopa_raw_return_t&);
void Visit        (const koopa_raw_store_t&);
void Visit       (const koopa_raw_branch_t&);
void Visit         (const koopa_raw_jump_t&);
InstResult  Visit        (const koopa_raw_value_t&);
InstResult  Visit      (const koopa_raw_integer_t&);
InstResult  Visit       (const koopa_raw_binary_t&);
InstResult  Visit         (const koopa_raw_load_t&);  
InstResult  Visit         (const koopa_raw_call_t&);
InstResult  Visit (const koopa_raw_func_arg_ref_t&);
InstResult  Visit   (const koopa_raw_global_alloc_t&, int, std::string);
InstResult  Visit   (const koopa_raw_get_elem_ptr_t&);

void CountRSA       (const koopa_raw_slice_t&);
void CountRSA (const koopa_raw_basic_block_t&);
void CountRSA       (const koopa_raw_value_t&);


void KoopaStrToProgram(const char *);
bool IsExecuted(void *);
//StoreInsToMap的值一定存储在栈上
InstResult StoreInsToMap(void *inst_pt, int type_size = 4, bool is_pointer_ = false);
//将block对应的label存到ins_result中,返回对应的标签"L"+label_num;
//int  StoreBlockToMap(void *);

int AddReg(); 

int CalTypeSize(const koopa_raw_type_t&);

std::string MoreThan2048(int);