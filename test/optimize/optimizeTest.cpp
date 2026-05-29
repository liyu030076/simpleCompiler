#include "optimize.h"

int main() 
{
    std::vector<IRInstr> tac;
    
    // case1:
    tac.push_back("a = 1");
    tac.push_back("b = a");
    tac.push_back("c = para");
    tac.push_back("d = b + 2");
    tac.push_back("e = a + c");
    tac.push_back("f = 1 + c");

    printTAC(tac, "original TAC");

    optimizeAll(tac);
}

/*
    当前目录下 make
    cd build 
    ./test.out

    //g++ common.cpp optimize.cpp optimizeTest.cpp
*/

/*
// === case1: 手动分析
a = 1     // 1 是 constant => a 记录到 constVal
b = a     // constVal 中 有 a => b 记录到 constVal, 且 用 常量传播 更新该语句为 b = 1;
c = para  // simulate func para: para 既不是 常量，也没被标记为常量 => c 被记录到 copyVal
d = b + 2 // 常量传播: d = 1 + 2 -> 常量折叠: d = 3
e = a + c // copy 传播: e = a + para -> 常量传播: e = 1 + para
f = 1 + c // copy 传播: f = 1 + para

// after cpyPro_constPro_constFold 
a = 1 
b = 1   
c = para
d = 3
e = 1 + para
f = 1 + para;

// after CSE 消除
a = 1
b = 1 
c = para
d = 3
e = 1 + para
f = e
*/

/*
// === case1: output === 与上述手动分析相同
==== original TAC ====
a = 1
b = a
c = para
d = b + 2
e = a + c
f = 1 + c

==== TAC after cpyPro_constPro_constFold optimize: ====
a = 1
b = 1
c = para
d = 3
e = 1 + para
f = 1 + para

==== TAC after eliminateCSE: ====
a = 1
b = 1
c = para
d = 3
e = 1 + para
f = e
*/