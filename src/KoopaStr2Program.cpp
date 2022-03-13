#include "KoopaStr2Program.h"

int a_reg_num = 0;
int t_reg_num = 0; 
std::vector<InsResult*> ins_result_vec;

InsResult::InsResult(void* ins_pointer_, int reg_num_) {
  ins_pointer = ins_pointer_;
  reg_num = reg_num_;
  reg_name = 't' + std::to_string(reg_num_);                                                
} 

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
  // 访问所有基本块
  Visit(func->bbs);
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
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
  return visit_instruction_result;
}

void Visit(const koopa_raw_return_t &ret) {
  switch (ret.value->kind.tag)
  {
  case KOOPA_RVT_INTEGER:
    std::cout << "  li    a0, ";
    break;
  case KOOPA_RVT_BINARY:
    std::cout << "  mv    a0, ";
    break;
  default:
    std::cout << "  mv    a0, ";
    break;
  }
  
  //在return.value是%n的情况下，是一个指向指令的指针。
  //有了“已经执行的指令的数组”，我们会找到那个指令的结果所使用的寄存器。
  //恰好就可以将返回值打印出来。  
  std::string reg_name = Visit(ret.value);

  std::cout << reg_name << std::endl;
  std::cout << "  ret"  << std::endl;
}

std::string Visit(const koopa_raw_integer_t &integer) {
  //mode == 1, 想要将值0返回为x_0;
  std::string res;
  int32_t value = integer.value;
  void *pt = (void *) value;
  if (value == 0) {
    res = "x0";
  }
  else {
    InsResult* executed = IsExecuted(pt);
    if (!executed) {
      executed = StoreInsToVec(pt);
      std::cout << "  li    " << executed->reg_name << ", " << value << std::endl;
    }
    res = executed->reg_name;
  }
  return res;
}

//访问二元运算指令的简陋的实现。
std::string Visit (const koopa_raw_binary_t& bi) {
  InsResult* executed = IsExecuted((void*)&bi);
  //执行过。
  if (executed) {
    
    return executed->reg_name;
  }

  InsResult* ins_res_pt = StoreInsToVec((void*)&bi);
  std::string left_reg_name = Visit(bi.lhs);
  std::string right_reg_name = Visit(bi.rhs);
  switch (bi.op)
  {
  case KOOPA_RBO_EQ: //%0 = eq 6, 0
    //li    t0, 6
    //xor   t0, t0, x0
    //seqz  t0, t0'

    std::cout << "  xor   " << ins_res_pt->reg_name << ", ";
    std::cout << left_reg_name << ", " << right_reg_name << std::endl;
    std::cout << "  seqz  " << ins_res_pt->reg_name << ", " << ins_res_pt->reg_name << std::endl;
    break;
  case KOOPA_RBO_SUB: //%1 = sub 0, %0
    std::cout << "  sub   " << ins_res_pt->reg_name << ", ";    
    std::cout << left_reg_name << ", " << right_reg_name;
    std::cout << std::endl;
    break;
  case KOOPA_RBO_ADD:
    std::cout << "  add   " << ins_res_pt->reg_name << ", ";    
    std::cout << left_reg_name << ", " << right_reg_name;
    std::cout << std::endl;
    break;
  case KOOPA_RBO_MUL:
    std::cout << "  mul   " << ins_res_pt->reg_name << ", ";    
    std::cout << left_reg_name << ", " << right_reg_name;
    std::cout << std::endl;
    break;
  case KOOPA_RBO_DIV:
    std::cout << "  div   " << ins_res_pt->reg_name << ", ";    
    std::cout << left_reg_name << ", " << right_reg_name;
    std::cout << std::endl;
    break;
  case KOOPA_RBO_MOD:
    std::cout << "  rem   " << ins_res_pt->reg_name << ", ";    
    std::cout << left_reg_name << ", " << right_reg_name;
    std::cout << std::endl;
    break;

  default:
    break;
  }
  return ins_res_pt->reg_name;
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
    delete_ins_result_vec();
}

InsResult* IsExecuted(void* ins_pt) {
  for (auto pt: ins_result_vec) {
    if (pt->ins_pointer == ins_pt)
      return pt;
  }
  return nullptr;
}

InsResult* StoreInsToVec(void* ins_pt) {
  InsResult* ins_res_pt = new InsResult(ins_pt, t_reg_num);
  t_reg_num++;
  ins_result_vec.push_back(ins_res_pt);
  return ins_res_pt;
}

void delete_ins_result_vec() {
  for (auto vec_pt: ins_result_vec) 
    delete vec_pt;
}
