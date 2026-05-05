#ifndef IRGEN_H
#define IRGEN_H
/*
    // IR instruction form: TAC
    IRInstr:
        form 1: temp = op1 OP op2      // operator
        form 2: x = y                  // assignment 
        // form 3: JMP Lable              // jump under no condition
        // form 4: IF cond JMP Lable      // ump under given condition
*/

#include <string>
#include <vector>
#include <iostream>
#include "AST.h"

class IRGen
{
public:
    using IRInstr = std::string;

    IRGen(int tmpCount_ = 0): tmpConut(tmpCount_) {}

    std::string genIR(ASTNodePtr node);
    
    const std::vector<IRInstr>& getIRCodeList() const;
private:

    int tmpConut;
    std::vector<IRInstr> irCodeList;
    
    std::string newTempVar();
    void emit(std::string op, std::string src1, std::string src2, std::string dst);  // directly use AST as input to generate IR Code
};

#endif 