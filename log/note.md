## level 1

- 教程中将Number设成了int_val,不知如何处理，改为BaseAST好处理一些。
- 如何安装gdb，进而在vscode中逐行调试？
    * 安装g++_9，g++
    * 安装gmp
- 不能在h文件中定义函数

## level 3

### level 3.1

- 考虑各个表达式的优先级，谨慎调整dumpkoopa位置；```void UnaryExpAST::DumpKoopa()```
- 2022.3.12 要修改的地方：
    * AST中对number和temp_sign的处理；**用栈啊淦**
    * KoopaStr2Program中对二元运算符lhs和rhs的讨论。
### level 3.2
    * 利用if-else分支查看表达式的mode并确定优先级。x
    * dumpkoopa时将这个表达式的值压入栈中？x
    有关Sysy向Koopa IR转变的过程（算术表达式的部分），我目前的思路是将每一个非终结符号都赋予一个“%n”，即每一个非终结符号都有一个值。然而这种思路产生了诸多问题：1.由于在分配标号的过程是自上而下的，写出的koopair程序往往“%n”都是倒过来的，大数反而在前面。2.过程中会产生许多诸如“%3 = %2”这样的无聊式子。 所以想向您请教下较为正常的思路或麻烦您帮我指出我思路中的问题！
    * 利用如下思路。
```c++

using Value = int;

BasicBlock {
    std::vector<Value> insts;
    std::unordered_map<Value, ValueData> values;
}

using ValueData = std::variant<
    OrValueType,
    SubValueType
>

struct OrValueType {
    int no;
    std::string format() {
        return std::format("%{} = or {} {}", no, lhs..., rhs...)
    }
    Value lhs, rhs;
}

// a || b
Value dumpKoopa() {
    // this is AST 
    switch (mode) {
        // a
        case and_expr: {
            auto and_expr;
            //这个如果是一元的就可以直接返回子表达式的值。
            return and_expr.dumpKoopa();
        }
        case or_expr: {
            auto lhs;
            auto rhs;
            Value lhs_value = lhs->dumpKoopa();
            Value rhs_value = rhs->dumpKoopa();
            OrValueType vd = { "%1", lhs_value, rhs_value };
            Value this_value;
            bb.values.insert(this_value, vd);
            bb.insts.push_back(this_value);
            return this_value;
        }
    }
}

for (auto inst : bb.insts()) {
    auto vd = bb.values.find(...);
    cout << vd.format()
}
```

### lv3.3 
* 在and or语句前插入两条ne来用按位异或实现逻辑异或。

## lv4

### lv4.1
* 递归的卡巴斯基范式。
* 符号表如何处理？
    - 处理constdef的时候，将<变量名，对应表达式的id>插入到符号表中。
    - 有对应的lval的时候，在符号表中找到对应的变量名及其ID。同Number处理。
    - 将单独的分号作为一个stmt
    

### lv4.2:
* 将定义分为不同的alloc load及store的组合；
* vardef = alloc + exp + load
* 
* 每一个alloc load 是否需要分配百分号开头的符号？
- alloc不需要
- store不需要
- load需要，可是load什么时候有用呢 lval?

- 改一下value 从奇怪的东西变成了随机long
- 在koopa-》riscv的过程中，将所有的结果都存到了栈上再取回来。利用gdb查看指针是啥。(alloc?)（新增指令。）


## lv5
* 利用vector中的增减来表明变量作用域
* 同名变量的处理再输出的时候直接输出对应的block的下标即可，不可以，还是要有一个计数器。
  因为printinstruction是以block为单位的。
  
  符号表中插入原名。x不可以，利用find_var_in_symbol_table查找。
  但是valuedata中存储的应该是带下标的名字。
  常量
**.second是一种拷贝，好像有bug**
* 好像没数数（拥有返回值的语句）

## lv6
* 如何实现代码块的划分？ if语句里还好，if语句之外要一个bool变量，判断是否需要新的代码块label。？
* 对于常量和数字的问题采用相同的办法处理，虽然很麻烦但是不用改动太多代码。  
* 给block加上标号。(不太行，基本块和{}还是不太一样，。)
* 生成koopaIR时将if语句划分，分成不同的dumpkoopa中间穿插着label.
* branch语句输出bnez和j即可，在处理blocks时，直接输出block->name选项.
* ret语句之后直接跳到end:把栈指针加回来走人.
* if block中的语句可能没有被执行.故num这种每次都要载入一下(?) **还有别的吗**
```riscv
 bnez  t2, L0
  j     L1
L0:
  li    t4, 3
  sw    t4, 52(sp)
  lw    t3, 52(sp)
  sw    t3, 28(sp)
  j     L2
L1:
  lw    t6, 52(sp)
  lw    t5, 52(sp)
```

* 短路求值 分解. vardef 和lval傻傻分不清楚.

## lv7
上面的 Koopa IR 程序是文档作者根据经验, 模仿一个编译器生成出来的 (事实上文档里所有的 Koopa IR 示例都是这么写的) (人形编译器 MaxXing 实锤), 仅代表一种可能的 IR 生成方式.
你会看到, 程序中出现了一个不可达的基本块 %while_body1. 这件事情在人类看来比较费解: 为什么会这样呢? 怎么会事呢? 但对于编译器的 IR 生成部分而言, 这么做是最省事的. 你也许可以思考一下背后的原因.

expbranch:

正常stmt挨个来
.
.
.
遇见break了：（确保一个*循环*（对吗？）里只能遇到一次break/continue）
        jump break的label
    break_label:
        jump end_label

        没有遇到break就要jump jump_exp的label
    最后的jump exp还要有一个label：
        jump jump_exp_label;       <-标记着循环的结束，然后printinstruction的时候遇到break或者continue；x

    end_label:
        ...

然后printinstruction的时候遇到break或者continue就跳，跳到"jump_exp_label为止"x 跳到下一个标签就行了。

每一个块后面都要是跳转或者return？

//不太行，if break; 测试不过。像return一样到下一个标签就行


if或者while的stmt不是block的时候，也要+-block块。
然后每一个块中只能有一个break或者continue。

"fix a bug: leave_block's pop_back"

fun @main(): i32 {
%entry:
  @i_1 = alloc i32
  store 0, @i_1
  jump %L0
%L0:
  %0 = load @i_1
  %1 = lt %0, 10
  br %1, %L1, %L2
%L1:
  %2 = load @i_1
  %3 = lt %2, 5
  br %3, %L4, %L5
%L4:
  %4 = load @i_1
  %5 = add %4, 1
  store %5, @i_1
  jump %L7
%L7:
  jump %L0
%L5:
  jump %L6
%L6:
  jump %L3
%L3:
  jump %L0
%L2:
  ret 0
}

int main() {
  int i = 0;
  while (i < 10) {
    if (i < 5) {
      i = i+1;
      continue;
    }
    else break;
  }
  return 0;
}


## lv8

在yacc中插入vardecl，对参数进行处理。

函数的名字是 function_x;
全局变量的名字是 global_x;
二者的作用域都是0号

参数的本质是数字

//The Bison parser detects a syntax error (or parse error) whenever it reads a token which cannot satisfy any syntax rule. An action in the grammar can also explicitly proclaim an error, using the macro YYERROR (see Special Features for Use in Actions).

分号？

编译期求值实现。

riscv- load store的修改，新的参数引用的处理。-1表明全局变量，负数表明寄存器的位置。x

symbolinfo的引用

随机数bug
//COOMA EXP移入规约冲突
State 67

   90 ArrayInitValItems: ConstExp .
   95                  | ConstExp . ',' ArrayInitVal
   97 ConstExps: ConstExp . ',' ConstExp

    ','  shift, and go to state 105 （reduce using 90）

    $default  reduce using rule 90 (ArrayInitValItems)

State 70

   92 ArrayInitValItems: ConstExps .
   93                  | ConstExps . ',' ArrayInitVal
   98 ConstExps: ConstExps . ',' ConstExp

    ','  shift, and go to state 108 (reduce using 92)

    $default  reduce using rule 92 (ArrayInitValItems)


.get方法