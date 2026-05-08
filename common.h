#ifndef COMMON_H
#define COMMON_H
#include <string>

enum TokenCategory
{
    IF = 0,
    IDENTIFIER,
    INTEGER,
    ASSIGN,
    MUL,
    ADD,
    LPARENTHESES,
    RPARENTHESES,
    SEMICOLON,
    SENTINEL, // Note: no use for lexical analyser, only user for parser
    IVALID_TOKEN_CATEGORY
};

// === IRGen and Optimize
using IRInstr = std::string;

// === Optimize and CodeGen
struct TACLine  // a TAC instruction: operator/operands/dst are all represented as string, judge and convert when using.
{
    std::string dst;
    std::string s1, op, s2;
    bool isBinaryOp;  // => form: dst = s1 op s2
    bool isAssign;    // => form: dst = s1

    TACLine(): dst(""), s1(""), op(""), s2(""), isBinaryOp(false), isAssign(false) {}
};

std::string trim(std::string s);
TACLine parseTAC(const IRInstr& line);

#endif 