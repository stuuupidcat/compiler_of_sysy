#include "KoopaStr2Program.h"

int cur_avaliable_pos = 0;
int t_reg_num = 0;
//利用指令的指针查找二元运算指令的结果储存在哪个寄存器中。
std::unordered_map<void*, int> inst_result;

/*InsResult::InsResult(void* ins_pointer_, int reg_num_) {
  ins_pointer = ins_pointer_;
  reg_num = reg_num_;
  reg_name = 't' + std::to_string(reg_num_);                                                
} */

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
   
  std::cout << "  .text" << std::endl;
  
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  std::cout << "  .global " << func->name+1 << "\n";
  std::cout << func->name+1 << ":" << std::endl;

  int stack_usage = CountInsts(func->bbs);
  stack_usage = ((stack_usage*4 + 15) / 16)*16;
  cur_avaliable_pos = 0;
  
  std::cout << "  li    " << "t0, " << stack_usage << "\n";
  std::cout << "  add   sp, sp, t0\n\n"; 
  // 访问所有基本块
  Visit(func->bbs);

  std::cout << "\n  li    " << "t0, " << stack_usage << "\n";
  std::cout << "  sub   sp, sp, t0\n"; 
  std::cout << "  ret"  << std::endl;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
std::string Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  // 我们想给这些指令加一个返回值, 表明他们指定的结果存到了哪里。
  // 主要是interger指令和二元运算符指令。
  const auto &kind = value->kind;
  std::string visit_instruction_result = "";
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret); //koopa_raw_return_t ret;
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      visit_instruction_result = Visit(kind.data.integer); //koopa_raw_integer_t;
      break;
    case KOOPA_RVT_BINARY:
      // 访问二元运算符指令
      visit_instruction_result = Visit(kind.data.binary);
      break;
    case KOOPA_RVT_ALLOC:
      visit_instruction_result = Visit(kind.data.global_alloc);
      break;
    case KOOPA_RVT_LOAD:
      visit_instruction_result = Visit(kind.data.load);
      break;
    case KOOPA_RVT_STORE:
      visit_instruction_result = Visit(kind.data.store);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
  return visit_instruction_result;
}

void Visit(const koopa_raw_return_t &ret) {
  //是否是寄存器。
  std::string reg_name = Visit(ret.value);
  std::cout << "  lw    a0, ";
  
  //在return.value是%n的情况下，是一个指向指令的指针。
  //有了“已经执行的指令的map”，我们会找到那个指令的结果所使用的寄存器。
  //恰好就可以将返回值打印出来。 
  std::cout << reg_name << std::endl;
}

std::string Visit(const koopa_raw_integer_t &integer) {
  //mode == 1, 想要将值0返回为x_0;
  std::string res, reg_name;
  int32_t value = integer.value;
  void *pt = (void *)(long)value;
  int pos;
  reg_name = AddReg();
  bool executed = IsExecuted(pt);
  if (!executed) {
    pos = StoreInsToMap(pt);
    std::cout << "  li    " << reg_name << ", " << value << std::endl;
    std::cout << "  sw    " << reg_name << ", " << pos << "(sp)\n";
  }
  else {
    pos = inst_result[pt];
    std::cout << "  lw    " << reg_name << ", " << pos << "(sp)\n";
  }
  res = std::to_string(pos) + "(sp)";
  
  return res;
}

//访问二元运算指令.
std::string Visit (const koopa_raw_binary_t& bi) {
  bool executed = IsExecuted((void*)&bi);
  //执行过。
  if (executed) {
    std::string res = AddReg();
    std::cout << "  lw    " << res << ", " << inst_result[(void*)&bi] << "(sp)\n";     
    res = std::to_string(inst_result[(void*)&bi]) + "(sp)";
    return res;
  }
  std::string left_reg_name = AddReg();
  std::string right_reg_name = AddReg();
  std::string left_stack_pos = Visit(bi.lhs);
  std::string right_stack_pos = Visit(bi.rhs);
  std::cout << "  lw    " << left_reg_name << ", " << left_stack_pos  << std::endl;   
  std::cout << "  lw    " << right_reg_name << ", " << right_stack_pos << std::endl;      
  
  std::string reg_name = AddReg();
  switch (bi.op)
  {
    case KOOPA_RBO_EQ: //%0 = eq 6, 0
      //li    t0, 6
      //xor   t0, t0, x0
      //seqz  t0, t0
  
      std::cout << "  xor   " << reg_name << ", ";
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      //seqz r1, r2 命令 r2 == 0, 将1写入rd
      //snez r1, r2 命令 r2 != 0, 将1写入rd
      std::cout << "  seqz  " << reg_name << ", " << reg_name << std::endl;
      break;
    case KOOPA_RBO_NOT_EQ: //同上。
      std::cout << "  xor   " << reg_name << ", ";
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      std::cout << "  snez  " << reg_name << ", " << reg_name << std::endl;
      break;
    case KOOPA_RBO_SUB: //%1 = sub 0, %0
      std::cout << "  sub   " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    case KOOPA_RBO_ADD:
      std::cout << "  add   " <<reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    case KOOPA_RBO_MUL:
      std::cout << "  mul   " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    case KOOPA_RBO_DIV:
      std::cout << "  div   " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    case KOOPA_RBO_MOD:
      std::cout << "  rem   " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    case KOOPA_RBO_AND:
      std::cout << "  and   " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    case KOOPA_RBO_OR:
      std::cout << "  or   " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    //slt t0, t1, t2 指令的含义是, 判断寄存器 t1 的值是否小于 t2 的值
    case KOOPA_RBO_LT:
      std::cout << "  slt  " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    case KOOPA_RBO_GT: //注意是反的。
      std::cout << "  sgt  " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      break;
    case KOOPA_RBO_LE:
      std::cout << "  sgt  " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      std::cout << "  seqz  " << reg_name << ", " << reg_name << std::endl;    
      break;
    case KOOPA_RBO_GE:
      std::cout << "  slt  " << reg_name << ", ";    
      std::cout << left_reg_name << ", " << right_reg_name << std::endl;
      std::cout << "  seqz  " << reg_name << ", " << reg_name << std::endl;    
      break;
    default:
      break;
  }
  int pos = StoreInsToMap((void*)&bi);
  std::cout << "  sw    " << reg_name << ", " << pos << "(sp)\n";
  std::string res = std::to_string(pos) + "sp";
  return res;
}

//load指令
std::string Visit (const koopa_raw_load_t& lw) {
  bool executed = IsExecuted((void*)&lw);
  //执行过。
  if (executed) {
    std::string res = std::to_string(inst_result[(void*)&lw]) + "(sp)";
    return res;
  } 
  else {
    int pos = StoreInsToMap((void *)&lw);
    std::string src = Visit(lw.src);
    std::string reg_name = AddReg();
    std::cout << "  lw    " << reg_name << ", " << src << std::endl;
    std::cout << "  sw    " << reg_name << ", " << std::to_string(pos) + "(sp)\n";
    std::string res = std::to_string(pos) + "(sp)";
    return res;
  }
  return "";
}

std::string Visit (const koopa_raw_store_t& sw) {
  std::string reg_name = AddReg();
  std::string load_pos = Visit(sw.value);
  std::string store_pos = Visit(sw.dest);
  std::cout << "  lw    " << reg_name << ", " << load_pos << std::endl;
  std::cout << "  sw    " << reg_name << ", " << store_pos << std::endl;
  return "";
}

std::string Visit  (const koopa_raw_global_alloc_t& alloc) {
  bool executed = IsExecuted((void*)&alloc);
  //执行过。
  if (executed) {
    std::string res = std::to_string(inst_result[(void*)&alloc]) + "(sp)";
    return res;
  } else {
    int pos = StoreInsToMap((void *)&alloc);
    std::string res = std::to_string(pos) + "(sp)";
    return res;
  }
  return "";
}

//从文件中读取字符串形式的koopa IR转换为raw program
void KoopaStrToProgram(const char *input, const char *output) {
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_file(input, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错

    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();

    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);

    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    // 处理 raw program
    Visit(raw);

    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program builder 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}

bool IsExecuted(void* inst_pt) {
  bool flag = false;
  if (inst_result.count(inst_pt) == 0) 
    flag = false;
  else 
    flag = true;
  return flag;
}

int StoreInsToMap(void* inst_pt) {
  int res = cur_avaliable_pos;
  inst_result.insert(std::make_pair(inst_pt, res)); 
  cur_avaliable_pos += 4;
  return res;
}

int CountInsts(const koopa_raw_slice_t& slice) {
  int res = 0;
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        res += CountInsts(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        res += 1;
        break;
      default:
        break;
    }
  }
  return res;
}

int CountInsts (const koopa_raw_basic_block_t& bb) {
  int res = 0;
  res += CountInsts(bb->insts);
  return res;
}

std::string AddReg() {
  std::string res = "t" + std::to_string(t_reg_num);
  t_reg_num = (t_reg_num+1) % 7;
  return res;
}

