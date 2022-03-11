#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <string.h>

#include "koopa.h"


// 函数声明
void Visit     (const koopa_raw_program_t&);
void Visit       (const koopa_raw_slice_t&);
void Visit    (const koopa_raw_function_t&);
void Visit (const koopa_raw_basic_block_t&);
void Visit       (const koopa_raw_value_t&);
void Visit      (const koopa_raw_return_t&);
void Visit     (const koopa_raw_integer_t&);

void KoopaStrToProgram(const char *, const char *);
