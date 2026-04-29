#include "common.h"
#include "LexicalAnalysize.h"
#include "Parser.h"

int main()
{
    /*
    //std::string input = "if ifa"; // test1 ok
    //std::string input = "a = 1; if (a) a = a * 20 + 120;"; // test2 ok
    std::string input = "res = 10 * (ab + 120);"; // test3 ok

    // ==========1. LexicalAnalysis
    std::vector<std::pair<std::string, TokenCategory> > token2CatStream;
    token2CatStream = LexicalAnalysis(input);
    */

    // ==========2. LR1Parsing 
    // std::string input = "aa";          // test ok
    std::string input = "i * (i + i)"; // test ok
    
    initGrammar();

    buildStatesAndStateTransGraph();

    buildActTblAndGotoTbl();
    std::set<GrammarSymType> terms {'i', '+', '*', '(', ')', eof};
    std::set<GrammarSymType> nonTerms {'E', 'T', 'F'};
    //std::set<GrammarSymType> terms = {'a', eof};
    //std::set<GrammarSymType> nonTerms = {'G', 'A'};
    printAll(terms, nonTerms);

#ifdef NEEDPARSERTREE
    ParserTreeNodePtr root = parser(input);
    if (root) 
    {
        printAst(root);
    }
#else
    parser(input);
#endif

    //LR1Parser parser;
    //parser.parse(token2CatStream);
}
