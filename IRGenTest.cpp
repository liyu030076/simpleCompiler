#include <iostream>

#include "common.h"
#include "LexicalAnalysize.h"
#include "Parser.h"
#include "IRGen.h"

int main()
{
    // LexicalAnalysis + Parser: test ok
    std::string input = "x = 10 * (ab + 120);"; // test1 ok

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

    // ==========3. IR CodeGen
    IRGen irGen;
    irGen.genIR(std::move(root) );
    std::vector<IRGen::IRInstr> irCodeList = irGen.getIRCodeList();
    std::cout << "==========IR Code list:" << std::endl;
    for (const auto& irCode: irCodeList)
    {
        std::cout << irCode << std::endl;
    }
}

/*
    g++ LexicalAnalysize.cpp Parser.cpp IRGen.cpp IRGenTest.cpp
*/

/*
test1 ok
    ==========AST structure:
    root:=
    lhs:x
    rhs:*
        lhs:10
        rhs:+
        lhs:ab
        rhs:120
    ==========IR Code list:
    t1 = 10
    t2 = 120
    t3 = ab + t2
    t4 = t1 * t3
    x = t4
*/