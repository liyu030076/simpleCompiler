#ifndef COMMON_H
#define COMMON_H

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

#endif 