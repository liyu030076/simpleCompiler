#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "CodeGen.h"

void CodeGenerator::generate(const std::vector<TACLine>& tacLines) 
{
    asmInstructions.clear();
    var2Reg.clear();
    nextReg = 1;

    for (const auto& tacLine: tacLines) 
    {
        if ("+" == tacLine.op)
        {
            genOp("ADD ", tacLine);
        }
        else if ("*" == tacLine.op)
        {
            genOp("MUL ", tacLine);
        }
        else if ("=" == tacLine.op)
        {
            genOp("", tacLine);
        }
    }
}

void CodeGenerator::printAsm() const 
{
    std::cout << "=== 5. CodeGen: abstract asm instructions: ===" << std::endl;
    for (const auto& tacLine : asmInstructions) 
    {
        std::cout << tacLine << std::endl;
    }
}

bool CodeGenerator::isConst(const std::string& str) // 判断是否为常量
{
    for (char ch: str) 
        if (!isdigit(ch) ) 
            return false;
    return !str.empty();
}

void CodeGenerator::genOp(std::string asmInsHead, const TACLine& tacLine)
{
    std::string reg = GetNextReg();
    var2Reg[tacLine.dst] = reg;

    if (isConst(tacLine.s1) ) 
    {
        asmInstructions.push_back("MOV " + reg + ", #" + tacLine.s1);
    } 
    else 
    {
        if (!var2Reg.count(tacLine.s1) )
        {
            asmInstructions.push_back("MOV " + reg + ", " + tacLine.s1); 
        }
        else
        {
            asmInstructions.push_back("MOV " + reg + ", " + var2Reg[tacLine.s1]);
        }
        
    }

    if (!tacLine.s2.empty() )
    {
        if (isConst(tacLine.s2) ) 
        {
            asmInstructions.push_back(asmInsHead + reg + ", #" + tacLine.s2);
        } 
        else 
        {
            if (!var2Reg.count(tacLine.s2) )
            {
                asmInstructions.push_back("MOV " + reg + ", " + tacLine.s1); 
            }
            else
            {
                asmInstructions.push_back(asmInsHead + reg + ", " + var2Reg[tacLine.s2]);
            }
        }
    }
}
