#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>

class CodeGenerator 
{
public:
    void generate(const std::vector<TACLine>& tacLines);

    const std::vector<std::string>& getAsmInstructions() const 
    {
        return asmInstructions;
    }

    void printAsm() const;

private:
    std::vector<std::string> asmInstructions;
    std::map<std::string, std::string> var2Reg; 
    int nextReg = 1;

    std::string GetNextReg() 
    {
        return "r" + std::to_string(nextReg++);
    }

    bool isConst(const std::string& str); // 判断是否为常量

    void genOp(std::string asmInsHead, const TACLine& tacLine);
};

#endif 