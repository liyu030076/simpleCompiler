#include "common.h"
#include "LexicalAnalysize.h"

int main()
{
    // std::string input = "if ifa"; // test1 ok
    std::string input = "a = 1; if (a) a = a * 20 + 120;"; // test2 ok

    // ==========1. 
    std::vector<std::pair<std::string, TokenCategory> > token2CatStream;
    token2CatStream = LexicalAnalysis(input);

     // ==========1. 

}
