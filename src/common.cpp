#include "common.h"

std::string trim(std::string s) // eliminate blank and '\t' at the begin and the end of the string  
{
    auto l = s.find_first_not_of(" \t");
    auto r = s.find_last_not_of(" \t");
    if (l == std::string::npos) 
        return "";
    return s.substr(l, r - l + 1);
}

TACLine parseTAC(const IRInstr& line) 
{
    TACLine res;
    size_t eq = line.find('=');
    if (eq == std::string::npos) // invalid IR Code
        return res;

    res.dst = trim(line.substr(0, eq) ); // line[eq] == "=="
    std::string right = trim(line.substr(eq + 1) );

    size_t opPos = right.find_first_of("+*");
    if (opPos == std::string::npos) // not binaryOp => be bound to be assignment 
    {
        res.isAssign = true;
        res.isBinaryOp = false;
        res.op = "=";
        res.s1 = trim(right);
        return res;
    }

    res.isBinaryOp = true;
    res.isAssign = false;
    res.s1 = trim(right.substr(0, opPos));
    res.op = std::string(1, right[opPos]);
    res.s2 = trim(right.substr(opPos + 1) );
    return res;
}