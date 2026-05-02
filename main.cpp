#include <iostream>

#include "common.h"
#include "LexicalAnalysize.h"
#include "Parser.h"

int main()
{
    // LexicalAnalysis + Parser: test ok
    std::string input = "x = 10 * (ab + 120);"; // test4 ok

    // ==========1. LexicalAnalysis
    std::vector<std::pair<std::string, TokenCategory> > token2CatStream;
    token2CatStream = LexicalAnalysis(input);

    // ==========2. LR1Parsing 
    initGrammar();

    buildStatesAndStateTransGraph();

    buildActTblAndGotoTbl();
    
    std::set<GrammarSym> terms {
        {TERMINAL, IF},
        {TERMINAL, IDENTIFIER},
        {TERMINAL, INTEGER},
        {TERMINAL, ASSIGN},
        {TERMINAL, MUL},
        {TERMINAL, ADD},
        {TERMINAL, LPARENTHESES},
        {TERMINAL, RPARENTHESES},
        {TERMINAL, SEMICOLON},
        eof // Note: eof 
    };

    std::set<GrammarSym> nonTerms {
        {NON_TERMINAL, Pro},  
        {NON_TERMINAL, Statement},  
        {NON_TERMINAL, ASSIG},      
        {NON_TERMINAL, Expr},       
        {NON_TERMINAL, Term},       
        {NON_TERMINAL, Factor}
    };

    printAll(terms, nonTerms);

    auto root = parser(token2CatStream);
    if (root) 
    {
#ifdef NEEDPARSERTREE
        printParseTree(root);
#else 
        std::cout << "==========AST structure:" << std::endl;
        printAST(root);
#endif

    }
}

/*
    g++ LexicalAnalysize.cpp Parser.cpp main.cpp
*/
