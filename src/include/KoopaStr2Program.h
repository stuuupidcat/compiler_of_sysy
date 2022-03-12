#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <string.h>
#include <vector>

#include "koopa.h"

//利用指针查找二元运算指令的结果储存在哪个寄存器中
class BinaryInsResult {
public:
    //这条二元运算指令的指针。
    const koopa_raw_binary_t* ins_pointer;
    //这条二元运算指令存储到了哪个寄存器里。
    int reg_num;
    std::string reg_name;

    BinaryInsResult(const koopa_raw_binary_t*, int); 
};


// 函数声明
void Visit     (const koopa_raw_program_t&, int);
void Visit       (const koopa_raw_slice_t&, int);
void Visit    (const koopa_raw_function_t&, int);
void Visit (const koopa_raw_basic_block_t&, int);
void Visit       (const koopa_raw_value_t&, int);
void Visit      (const koopa_raw_return_t&, int);
void Visit     (const koopa_raw_integer_t&, int);
void Visit      (const koopa_raw_binary_t&, int);

void KoopaStrToProgram(const char *, const char *);

BinaryInsResult* IsExecuted(const koopa_raw_binary_t*);
BinaryInsResult* StoreInsToVec(const koopa_raw_binary_t*);
void delete_ins_result_vec(); 

