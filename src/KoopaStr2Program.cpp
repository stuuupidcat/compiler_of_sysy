#include "KoopaStr2Program.h"

int cur_avaliable_pos = 0;
int t_reg_num = 0;
//利用指令的指针查找二元运算指令的结果储存在栈中的哪个位置中。
//或者利用块的指针查找块的label的number是多少。(不用了，label有名字)

std::unordered_map<void*, InstResPos> inst_result;
std::string cur_function_name;

//Visit Value的时候是否需要把值存到caller栈帧中或寄存器中
//used in Visit(const koopa_raw_call_t &call)
bool save_args = false;
int arg_index = 0;

int stack_usage;
int R = 0;
int S = 0;
int A = 0;

InstResPos::InstResPos(int mode_, int pos_, bool global_var_ = false) {
        mode = mode_;
        global_var = global_var_;
        if (mode == 0) {
            format = std::to_string(pos_) + "(sp)";
            on_stack = true;
        }
        else if (mode == 1) {
            format = "t" + std::to_string(pos_);
            on_stack = false;
        }
        else if (mode == 2) {
            format = "a" + std::to_string(pos_);
            on_stack = false;
        }
        else if (mode == 3) {

        }
        else {
            assert(false);
        }
    }

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  
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
  if (func->bbs.len == 0) {
    // 函数声明, 不需要生成代码
    return;
  }
  cur_function_name = func->name+1;
  std::cout << "\n  .text" << std::endl;
  std::cout << "  .globl " << func->name+1 << "\n";
  std::cout << func->name+1 << ":" << std::endl;

  R = 0;
  S = 0;
  A = 0;
  CountRSA(func->bbs);
  stack_usage = R + S + A;
  stack_usage = ((stack_usage*4 + 15) / 16)*16;
  
  //这个要考虑参数个数，等会看看在哪能找到。 
  cur_avaliable_pos = A*4;
  
  std::cout << "  li    " << "t0, " << -stack_usage << "\n";
  std::cout << "  add   sp, sp, t0\n";
  //保存ra
  if (R != 0) {
    std::cout << "  sw    ra, " << stack_usage-4 << "(sp)\n";
  } 
  // 访问所有基本块
  Visit(func->bbs);
  //避免重名
  std::cout << func->name+1 << "_end:" << std::endl; 
  //恢复ra
  if (R != 0) {
    std::cout << "  lw    ra, " << stack_usage-4 << "(sp)\n";
  }
  std::cout << "  li    t0, " << stack_usage << "\n";
  std::cout << "  add   sp, sp, t0\n"; 
  std::cout << "  ret"  << std::endl;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  if (strcmp(bb->name+1, "entry") != 0) {
    std::cout << bb->name+1 << ":" << std::endl;
  }
  Visit(bb->insts);
}

// 访问指令
InstResPos Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  // 我们想给这些指令加一个返回值, 表明他们指定的结果存到了哪里。
  // 主要是interger指令和二元运算符指令。
  const auto &kind = value->kind;
  InstResPos visit_instruction_result;
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
      visit_instruction_result = Visit(kind.data.global_alloc, "");
      break;
    case KOOPA_RVT_LOAD:
      visit_instruction_result = Visit(kind.data.load);
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_CALL:
      visit_instruction_result = Visit(kind.data.call);
      break;
    case KOOPA_RVT_FUNC_ARG_REF:
      visit_instruction_result = Visit(kind.data.func_arg_ref);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      visit_instruction_result = Visit(kind.data.global_alloc, value->name+1);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
  if (save_args) {
    if (visit_instruction_result.on_stack) { //存在栈上
      if (arg_index <= 7) {
        std::cout << "  lw    " << "a" << arg_index << ", " << visit_instruction_result.format << std::endl;
      }
      else {
        std::string tempreg = "t" + std::to_string(AddReg());
        std::cout << "  lw    " << tempreg << ", " << visit_instruction_result.format << std::endl;
        std::cout << "  sw    " << tempreg << ", " << 4*(arg_index-8)<< "(sp)\n";
      }
    }
    else { //在寄存器上。
      if (arg_index <= 7) {
        std::cout << "  mv    " << "a" << arg_index << "," << visit_instruction_result.format << std::endl;
      }
      else {
        std::cout << "  sw    " << visit_instruction_result.format << ", " << 4*(arg_index-8)<< "(sp)\n";
      }
    }
    arg_index++;
  }
  return visit_instruction_result;
}

//函数参数引用。
InstResPos Visit(const koopa_raw_func_arg_ref_t &func_arg_ref) {
  InstResPos res;
  if (func_arg_ref.index <= 7) {
    //on a-regsiter
    res = InstResPos(2, func_arg_ref.index);
  }
  else {
    //on stack
    res = InstResPos(0, 4*(func_arg_ref.index-8) + stack_usage);
  }
  return res;
}

InstResPos Visit(const koopa_raw_call_t &call) {
  bool executed = IsExecuted((void*)&call);
  //执行过。
  if (executed) {
    //std::string res = "t"+std::to_string(AddReg());
    InstResPos pos = inst_result[(void*)&call];
    //std::cout << "  lw    " << res << ", " << pos << "(sp)\n";     
    //res = std::to_string(inst_result[(void*)&bi]) + "(sp)";
    return pos;
  }
  // 访问 call 指令
  // 访问所有参数
  // 将参数存到他们该去的地方，寄存器，栈的。
  save_args = true;
  arg_index = 0;
  Visit(call.args);
  save_args = false;

  std::cout << "  call " << call.callee->name+1 << std::endl;
  //将函数的返回值保存在栈帧中，
  InstResPos res_pos = StoreInsToMap((void *) &call);
  std::cout << "  sw    a0, " << res_pos.format << std::endl;
  return res_pos;
}

void Visit(const koopa_raw_return_t &ret) {
  //是否为void函数
  if (ret.value == 0) {
    std::cout << "  j     "<< cur_function_name << "_end\n";
    return;
  }
  InstResPos pos = Visit(ret.value);
  if (pos.on_stack) {
    std::cout << "  lw    a0, " << pos.format << std::endl;
  }
  else {
    std::cout << "  mv    a0, " << pos.format << std::endl;
  }
  
  //在return.value是%n的情况下，是一个指向指令的指针。
  //有了“已经执行的指令的map”，我们会找到那个指令的结果所使用的寄存器。
  //恰好就可以将返回值打印出来。 
  std::cout << "  j     "<< cur_function_name << "_end\n";
}

InstResPos Visit(const koopa_raw_integer_t &integer) {
  //不管执行过否
  //重新载入
  //if_bug
  //mode == 1, 想要将值0返回为x_0;
  std::string res, reg_name;
  int32_t value = integer.value;
  InstResPos pos;
  int reg_no = AddReg();
  reg_name = "t" + std::to_string(reg_no);
  std::cout << "  li    " << reg_name << ", " << value << std::endl;
  pos = InstResPos(1, reg_no);
  inst_result.insert(std::make_pair((void *) &integer, pos));
  return pos;
}

//访问二元运算指令.
InstResPos Visit (const koopa_raw_binary_t& bi) {
  bool executed = IsExecuted((void*)&bi);
  //执行过。
  if (executed) {
    InstResPos pos = inst_result[(void*)&bi];
    return pos;
  }
  std::string left_reg_name = "t"+std::to_string(AddReg());
  std::string right_reg_name = "t"+std::to_string(AddReg());
  InstResPos left_inst_pos = Visit(bi.lhs);
  InstResPos right_inst_pos = Visit(bi.rhs);
  if (left_inst_pos.on_stack) {
    std::cout << "  lw    " << left_reg_name << ", " << left_inst_pos.format << std::endl;
  }
  else {
    left_reg_name = left_inst_pos.format;
  }
  if (right_inst_pos.on_stack) {
    std::cout << "  lw    " << right_reg_name << ", " << right_inst_pos.format << std::endl;
  }
  else {
    right_reg_name = right_inst_pos.format;
  }
  
  std::string reg_name = "t"+std::to_string(AddReg());
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
      std::cout << "  or    " << reg_name << ", ";    
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
  InstResPos pos = StoreInsToMap((void*)&bi);
  std::cout << "  sw    " << reg_name << ", " << pos.format << std::endl;
  return pos;
}

//load指令
InstResPos Visit (const koopa_raw_load_t& lw) {
  bool executed = IsExecuted((void*)&lw);
  //执行过。
  if (executed) {
    return inst_result[(void*)&lw];
  } 
  else {
    if (lw.src->kind.tag == KOOPA_RVT_ALLOC) { //局部变量。
      InstResPos pos = StoreInsToMap((void *)&lw); //该条指令的存储位置
      InstResPos src_pos = Visit(lw.src); //src结果的存储位置。
      if (src_pos.on_stack) {
        std::string reg_name = "t"+std::to_string(AddReg());
        std::cout << "  lw    " << reg_name << ", " << src_pos.format << std::endl;
        std::cout << "  sw    " << reg_name << ", " << pos.format << std::endl;
      }
      else {
        std::cout << "  sw    " << src_pos.format << ", " << pos.format << std::endl;
      }
      return pos;
    }
    else { //全局变量。
      Visit(lw.src);//第一次要打印全局变量“.data....”
      InstResPos pos = StoreInsToMap((void *)&lw);
      std::string reg_name = "t"+std::to_string(AddReg());
      std::cout << "  la    " << reg_name << ", " << lw.src->name+1 << std::endl;
      std::cout << "  lw    " << reg_name << ", 0(" << reg_name << ')' << std::endl;
      std::cout << "  sw    " << reg_name << ", " << pos.format << std::endl;
      //std::string res = std::to_string(pos) + "(sp)";
      return pos;
    }
  }
}


//store指令
void Visit (const koopa_raw_store_t& sw) {
  InstResPos var_pos = Visit(sw.dest); //var_pos = -1为全局变量
  std::string store_pos;
  std::string addr_reg = "t" + std::to_string(AddReg());
  if (var_pos.global_var) { //全局变量
    std::cout << "  la    " << addr_reg << ", " << sw.dest->name+1 << std::endl;
    store_pos = "0(" + addr_reg + ")";
  }
  else { //局部变量
    store_pos = var_pos.format;
  }


  if (sw.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {//如果是函数参数的引用
    InstResPos value_pos = Visit(sw.value);
    if (value_pos.on_stack) {//存在栈中。
      std::string reg_name = "t"+std::to_string(AddReg());
      std::cout << "  lw    " << reg_name << ", " << value_pos.format << std::endl;
      std::cout << "  sw    " << reg_name << ", " << store_pos << std::endl;
    } else { //存放在寄存器中。
      std::cout << "  sw    " << value_pos.format << ", " << store_pos << std::endl;
    } 
  }
  else {
    InstResPos value_pos = Visit(sw.value);
    std::string reg_name = "t"+std::to_string(AddReg());

    if (value_pos.on_stack) {
      std::cout << "  lw    " << reg_name << ", " << value_pos.format << std::endl;
      std::cout << "  sw    " << reg_name << ", " << store_pos << std::endl;
    }
    else {
      std::cout << "  sw    " << value_pos.format << ", " << store_pos << std::endl;
    }
  }
}

//branch instruction
void Visit(const koopa_raw_branch_t& br) {
  InstResPos cond_pos = Visit(br.cond);
  if (cond_pos.on_stack) {
    std::string reg = "t"+std::to_string(AddReg());
    std::cout << "  lw    " << reg << ", " << cond_pos.format << std::endl;
    std::cout << "  bnez  " << reg << ", " << br.true_bb->name+1 <<  std::endl;
    std::cout << "  j     " << br.false_bb->name+1 << std::endl;
  }
  else {
    std::cout << "  bnez  " << cond_pos.format << ", " << br.true_bb->name+1 <<  std::endl;
    std::cout << "  j     " << br.false_bb->name+1 << std::endl;
  }
}

//jump instruction
void Visit(const koopa_raw_jump_t& jmp) {
  std::cout << "  j     " << jmp.target->name+1 << std::endl;
}

//访问变量，如果是全局变量会给名字参数。
InstResPos Visit  (const koopa_raw_global_alloc_t& alloc, std::string name) {
  if (name == "") {
    bool executed = IsExecuted((void*)&alloc);
    //执行过。
    if (executed) {
      return inst_result[(void*)&alloc];
    } else {
      InstResPos pos = StoreInsToMap((void *)&alloc);
      //std::string res = std::to_string(pos) + "(sp)";
      return pos;
    }
  }
  else { //全局变量。
    bool executed = IsExecuted((void*)&alloc);
    //执行过。
    if (executed) {
      return inst_result[(void*)&alloc];
    } else {
      InstResPos pos = InstResPos(3, 0, true);
      inst_result.insert(std::make_pair((void *)&alloc, pos));
      std::cout << "  .data\n";
      std::cout << "  .globl " << name << "\n";
      std::cout << name << ":\n";
      std::cout << "  .word " << alloc.init->kind.data.integer.value << std::endl << std::endl;;
      return pos; //-1表示全局变量
    }
  }
}

//从文件中读取字符串形式的koopa IR转换为raw program
void KoopaStrToProgram(const char *input) {
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

InstResPos StoreInsToMap(void* inst_pt) {
  InstResPos temp = InstResPos(0, cur_avaliable_pos);
  inst_result.insert(std::make_pair(inst_pt, temp)); 
  cur_avaliable_pos += 4;
  return temp;
}

//int StoreBlockToMap(void* block_pt) {
//  //没啥用 block中有name.
//  int res = riscv_label_num;
//  inst_result.insert(std::make_pair(block_pt, res)); 
//  riscv_label_num++;
//  return res;
//}

void CountRSA(const koopa_raw_slice_t& slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        CountRSA(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        // 好像有什么东西没有处理，指令是否有结果要存储。
        CountRSA(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        break;
    }
  }
  return;
}

void CountRSA (const koopa_raw_basic_block_t& bb) {
  CountRSA(bb->insts);
}

void CountRSA (const koopa_raw_value_t& inst) {
  S += 1; //數字都存在了棧上。
  if (inst->kind.tag == KOOPA_RVT_CALL) {
    S += inst->kind.data.call.args.len; //多给点。
    R = 1;
    int arg_num = inst->kind.data.call.args.len;
    A = (arg_num-8 > A) ? arg_num-8 : A;
  }
}


int AddReg() {
  int res = t_reg_num;
  t_reg_num = (t_reg_num+1) % 7;
  return res;
}

