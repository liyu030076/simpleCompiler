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

#endif 