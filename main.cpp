#include "common.h"
#include "LexicalAnalysize.h"
#include "Parser.h"

int main()
{
    /*
    // std::string input = "if ifa"; // test1 ok
    std::string input = "a = 1; if (a) a = a * 20 + 120;"; // test2 ok

    // ==========1. LexicalAnalysis
    std::vector<std::pair<std::string, TokenCategory> > token2CatStream;
    token2CatStream = LexicalAnalysis(input);
    */

    // ==========2. LR1Parsing 
    initGrammar();
    
    buildStatesAndStateTransGraph();

    buildActTblAndGotoTbl();
    std::set<char> terms {'i', '+', '*', '(', ')', eof};
    std::set<char> nonTerms {'E', 'T', 'F'};
    //std::set<char> terms = {'a', eof};
    //std::set<char> nonTerms = {'G', 'A'};
    printAll(terms, nonTerms);

    std::string input = "i * (i + i)";
    //std::string input = "aa";

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
