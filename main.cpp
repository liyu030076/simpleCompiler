#include <iostream>

#include "common.h"
#include "LexicalAnalysize.h"
#include "Parser.h"
#include "IRGen.h"
#include "optimize.h"

int main()
{
    // LexicalAnalysis + Parser: test ok
    std::string input = "x = 10 * (ab + 120);"; // test1 ok
    //std::string input = "b = 1; c = para; d = b + 2; e = 1 + c; f = 1 + c;"; // test2 ok
    // std::string input = "ab = 2; b = ab; c = ab + 3 + 5; d = ab + 120; x = 10 * (ab + 120); y = c + d + x;"; // test2 ok
    // ==========1. LexicalAnalysis
    std::vector<std::pair<std::string, TokenCategory> > token2CatStream;
    token2CatStream = LexicalAnalysis(input);

    // ==========2. LR1Parsing 
    initGrammar();
    nonTermDeriveEpsilonCalc();
    computeFirstSet();
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
        {NON_TERMINAL, StatementList},  
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
    std::vector<IRInstr> irCodeList = irGen.getIRCodeList();
    std::cout << "==========IR Code list:" << std::endl;
    for (const auto& irCode: irCodeList)
    {
        std::cout << irCode << std::endl;
    }

    // ==========4. Optimize
    printTAC(irCodeList, "original TAC");

    optimizeAll(irCodeList);
}

/*
    g++ LexicalAnalysize.cpp Parser.cpp IRGen.cpp optimize.cpp main.cpp
*/

/*
test1 output:
==========AST structure:
prog: 
  subroot:=
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

==== original TAC ====
t1 = 10
t2 = 120
t3 = ab + t2
t4 = t1 * t3
x = t4

==== TAC after cpyPro_constPro_constFold optimize: ====
t1 = 10
t2 = 120
t3 = ab + 120
t4 = 10 * t3
x = t4

==== TAC after eliminateCSE: ====
t1 = 10
t2 = 120
t3 = ab + 120
t4 = 10 * t3
x = t4
*/

/*
test2 output:
==========AST structure:
prog: 
  subroot:=
    lhs:b
    rhs:1
  subroot:=
    lhs:c
    rhs:para
  subroot:=
    lhs:d
    rhs:+
      lhs:b
      rhs:2
  subroot:=
    lhs:e
    rhs:+
      lhs:1
      rhs:c
  subroot:=
    lhs:f
    rhs:+
      lhs:1
      rhs:c
==========IR Code list:
t1 = 1
b = t1
c = para
t2 = 2
t3 = b + t2
d = t3
t4 = 1
t5 = t4 + c
e = t5
t6 = 1
t7 = t6 + c
f = t7

==== original TAC ====
t1 = 1
b = t1
c = para
t2 = 2
t3 = b + t2
d = t3
t4 = 1
t5 = t4 + c
e = t5
t6 = 1
t7 = t6 + c
f = t7

==== TAC after cpyPro_constPro_constFold optimize: ====
t1 = 1
b = 1
c = para
t2 = 2
t3 = 3
d = 3
t4 = 1
t5 = 1 + para
e = t5
t6 = 1
t7 = 1 + para
f = t7

==== TAC after eliminateCSE: ====
t1 = 1
b = 1
c = para
t2 = 2
t3 = 3
d = 3
t4 = 1
t5 = 1 + para
e = t5
t6 = 1
t7 = t5
f = t7

*/