#include <vector>
#include "CodeGen.h"

int main() 
{
    std::vector<IRInstr> irCodeList;
    std::vector<TACLine> tacLines;
    irCodeList.reserve(100);
    tacLines.reserve(100);

    // === test1 ok
    irCodeList.push_back("t1 = 10");
    irCodeList.push_back("t2 = 120");
    irCodeList.push_back("t3 = ab + 120");
    irCodeList.push_back("t4 = 10 * t3");
    irCodeList.push_back("x = t4");

    for (const IRInstr& irInstr: irCodeList)
    {
        TACLine tacLine = parseTAC(irInstr);
        tacLines.push_back(tacLine);
        //std::cout << "dst: " << tacLine.dst << ", s1: " << tacLine.s1 << ", op: " << tacLine.op << ", s2: " << tacLine.s2 << std::endl;
    }

    CodeGenerator codeGen;
    codeGen.generate(tacLines);   
    codeGen.printAsm(); 
}

/*
    当前目录下 make
    cd build 
    ./test.out

    //g++ common.cpp CodeGen.cpp CodeGenTest.cpp
*/

/*
=== test1 ok
=== abstract asm instructions: ===
MOV r1, #10
MOV r2, #120
MOV r3, ab
ADD r3, #120
MOV r4, #10
MUL r4, r3
MOV r5, r4

*/