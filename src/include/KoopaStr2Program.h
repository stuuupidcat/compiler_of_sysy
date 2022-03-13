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
class InsResult {
public:
    //这条二元运算指令的指针。
    //想把数字也看成指针
    void* ins_pointer;
    //这条二元运算指令存储到了哪个寄存器里。
    int reg_num;
    std::string reg_name;

    InsResult(void*, int); 
};


// 函数声明
void        Visit     (const koopa_raw_program_t&);
void        Visit       (const koopa_raw_slice_t&);
void        Visit    (const koopa_raw_function_t&);
void        Visit (const koopa_raw_basic_block_t&);
std::string Visit       (const koopa_raw_value_t&);
std::string Visit     (const koopa_raw_integer_t&);
std::string Visit      (const koopa_raw_binary_t&);
void        Visit      (const koopa_raw_return_t&);


void KoopaStrToProgram(const char *, const char *);

InsResult* IsExecuted(void *);
InsResult* StoreInsToVec(void *);
void delete_ins_result_vec(); 

