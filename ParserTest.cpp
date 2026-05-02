#include <iostream>
#include "common.h"
#include "Parser.h"

int main()
{
    /*
    // test1 ok
    Token2CatStream token2CatStream =
    {
        {"10", INTEGER},
        {"*", MUL},
        {"15", INTEGER},
    }; // 10 * 15
    */
    
    // test2 ok
    Token2CatStream token2CatStream =
    {
        {"x", IDENTIFIER},
        {"=", ASSIGN},
        {"10", INTEGER},
        {"*", MUL},
        {"(", LPARENTHESES},
        {"ab", IDENTIFIER},
        {"+", ADD},
        {"120", INTEGER},
        {")", RPARENTHESES},
        {";", SEMICOLON},
    }; // x = 10 * (ab + 120);
    

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
        eof // Note: eof 也
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
    std::cout << "root addr: " << root.get() << std::endl;
    if (root) 
    {
        std::cout << "root not empty: " << std::endl;
#ifdef NEEDPARSERTREE
        printParseTree(root);
#else 
        std::cout << "==========AST structure:" << std::endl;
        printAST(root);
#endif

    }
}

/*
    g++ Parser.cpp ParserTest.cpp
*/
/*
test2 ok

==========AST structure:
root:=
  lhs:x
  rhs:*
    lhs:10
    rhs:+
      lhs:ab
      rhs:120
*/

/*
g++ Parser.cpp ParserTest.cpp -D NEEDPARSERTREE
*/

/*
旧提交 test ok: ，输入中 每种 token 都是单个 字符时
    productions.emplace_back('E', "E+T"); 
    productions.emplace_back('E', "T");
    productions.emplace_back('T', "T*F");
    productions.emplace_back('T', "F");
    productions.emplace_back('F', "(E)");
    productions.emplace_back('F', "");
*/

/*
旧提交 test ok: ，输入中 每种 token 都是单个 字符时
    productions.emplace_back('G', "A");  // G -> A
    productions.emplace_back('A', "Aa"); // A -> Aa  makeConcatenateNode(, makeANode)
    productions.emplace_back('A', "a");  // A -> a   makeANode()
*/

/*
test1 ok: 10 * 15
    {getNonTerminalSym(Pro), {getNonTerminalSym(Term)} },                                                  // Pro -> Term
    {getNonTerminalSym(Term), {getNonTerminalSym(Term), getTerminalSym(MUL), getNonTerminalSym(Factor)} }, // Term -> Term * Factor
    {getNonTerminalSym(Term), {getNonTerminalSym(Factor)} },                                               // Term -> Factor 
    {getNonTerminalSym(Factor), {getTerminalSym(INTEGER)} }                                                // Factor → integer 
*/

/*
test1 output:

===== LR(1) all States: =====
states.size:6

State I0:
  Pro->.Term, {eof}
  Term->.Term*Factor, {eof}
  Term->.Factor, {eof}
  Term->.Term*Factor, {*}
  Term->.Factor, {*}
  Factor->.integer, {eof}
  Factor->.integer, {*}

State I1:
  Factor->integer., {eof}
  Factor->integer., {*}

State I2:
  Pro->Term., {eof}
  Term->Term.*Factor, {eof}
  Term->Term.*Factor, {*}

State I3:
  Term->Factor., {eof}
  Term->Factor., {*}

State I4:
  Term->Term*.Factor, {eof}
  Term->Term*.Factor, {*}
  Factor->.integer, {eof}
  Factor->.integer, {*}

State I5:
  Term->Term*Factor., {eof}
  Term->Term*Factor., {*}

===== State Transition Graph: =====
I0 --integer--> I1
I0 --Term--> I2
I0 --Factor--> I3
I2 --*--> I4
I4 --integer--> I1
I4 --Factor--> I5

===== ActionTable =====
State: if      ident   integer =       *       +       (       )       ;       eof     
I0                    s1                                                              
I1                                    r3                                      r3      
I2                                    s4                                      acc     
I3                                    r2                                      r2      
I4                    s1                                                              
I5                                    r1                                      r1      

===== GotoTable:  =====
State: Pro     StatementASSIG   Expr    Term    Factor  
I0                                    2       3       
I1                                                    
I2                                                    
I3                                                    
I4                                            5       
I5                                                    
inputPtr: -1, curStateIndex: 0
nextToken2Cat: 10, 2
curStateIndex: 0, nextSymInInput: 2, 2, action: s1
shift by s1
inputPtr: 0, curStateIndex: 1
nextToken2Cat: *, 4
curStateIndex: 1, nextSymInInput: 4, 4, action: r3
Reduce by r3
inputPtr: 0, curStateIndex: 3
nextToken2Cat: *, 4
curStateIndex: 3, nextSymInInput: 4, 4, action: r2
Reduce by r2
inputPtr: 0, curStateIndex: 2
nextToken2Cat: *, 4
curStateIndex: 2, nextSymInInput: 4, 4, action: s4
shift by s4
inputPtr: 1, curStateIndex: 4
nextToken2Cat: 15, 2
curStateIndex: 4, nextSymInInput: 2, 2, action: s1
shift by s1
inputPtr: 2, curStateIndex: 1
nextToken2Cat: $, 9
curStateIndex: 1, nextSymInInput: 9, 9, action: r3
Reduce by r3
inputPtr: 2, curStateIndex: 5
nextToken2Cat: $, 9
curStateIndex: 5, nextSymInInput: 9, 9, action: r1
Reduce by r1
inputPtr: 2, curStateIndex: 2
nextToken2Cat: $, 9
curStateIndex: 2, nextSymInInput: 9, 9, action: acc

 Parsing success, AST generated success !!!

[NonTerminal] Term
  [NonTerminal] Term
    [NonTerminal] Factor
      [Terminal   ] 10
  [Terminal   ] *
  [NonTerminal] Factor
    [Terminal   ] 15
*/

/*
test2 output:

==== LR(1) all States: =====
states.size:29

State I0:
  Pro->.Statement, {eof}
  Statement->.ASSIG, {eof}
  ASSIG->.identifier=Expr;, {eof}

State I1:
  ASSIG->identifier.=Expr;, {eof}

State I2:
  Pro->Statement., {eof}

State I3:
  Statement->ASSIG., {eof}

State I4:
  ASSIG->identifier=.Expr;, {eof}
  Expr->.Expr+Term, {;}
  Expr->.Term, {;}
  Expr->.Expr+Term, {+}
  Expr->.Term, {+}
  Term->.Term*Factor, {;}
  Term->.Factor, {;}
  Term->.Term*Factor, {+}
  Term->.Factor, {+}
  Term->.Term*Factor, {*}
  Term->.Factor, {*}
  Factor->.integer, {;}
  Factor->.identifier, {;}
  Factor->.(Expr), {;}
  Factor->.integer, {+}
  Factor->.identifier, {+}
  Factor->.(Expr), {+}
  Factor->.integer, {*}
  Factor->.identifier, {*}
  Factor->.(Expr), {*}

State I5:
  Factor->identifier., {;}
  Factor->identifier., {+}
  Factor->identifier., {*}

State I6:
  Factor->integer., {;}
  Factor->integer., {+}
  Factor->integer., {*}

State I7:
  Factor->(.Expr), {;}
  Factor->(.Expr), {+}
  Factor->(.Expr), {*}
  Expr->.Expr+Term, {)}
  Expr->.Term, {)}
  Expr->.Expr+Term, {+}
  Expr->.Term, {+}
  Term->.Term*Factor, {)}
  Term->.Factor, {)}
  Term->.Term*Factor, {+}
  Term->.Factor, {+}
  Term->.Term*Factor, {*}
  Term->.Factor, {*}
  Factor->.integer, {)}
  Factor->.identifier, {)}
  Factor->.(Expr), {)}
  Factor->.integer, {+}
  Factor->.identifier, {+}
  Factor->.(Expr), {+}
  Factor->.integer, {*}
  Factor->.identifier, {*}
  Factor->.(Expr), {*}

State I8:
  ASSIG->identifier=Expr.;, {eof}
  Expr->Expr.+Term, {;}
  Expr->Expr.+Term, {+}

State I9:
  Expr->Term., {;}
  Expr->Term., {+}
  Term->Term.*Factor, {;}
  Term->Term.*Factor, {+}
  Term->Term.*Factor, {*}

State I10:
  Term->Factor., {;}
  Term->Factor., {+}
  Term->Factor., {*}

State I11:
  Factor->identifier., {)}
  Factor->identifier., {+}
  Factor->identifier., {*}

State I12:
  Factor->integer., {)}
  Factor->integer., {+}
  Factor->integer., {*}

State I13:
  Factor->(.Expr), {)}
  Factor->(.Expr), {+}
  Factor->(.Expr), {*}
  Expr->.Expr+Term, {)}
  Expr->.Term, {)}
  Expr->.Expr+Term, {+}
  Expr->.Term, {+}
  Term->.Term*Factor, {)}
  Term->.Factor, {)}
  Term->.Term*Factor, {+}
  Term->.Factor, {+}
  Term->.Term*Factor, {*}
  Term->.Factor, {*}
  Factor->.integer, {)}
  Factor->.identifier, {)}
  Factor->.(Expr), {)}
  Factor->.integer, {+}
  Factor->.identifier, {+}
  Factor->.(Expr), {+}
  Factor->.integer, {*}
  Factor->.identifier, {*}
  Factor->.(Expr), {*}

State I14:
  Factor->(Expr.), {;}
  Factor->(Expr.), {+}
  Factor->(Expr.), {*}
  Expr->Expr.+Term, {)}
  Expr->Expr.+Term, {+}

State I15:
  Expr->Term., {)}
  Expr->Term., {+}
  Term->Term.*Factor, {)}
  Term->Term.*Factor, {+}
  Term->Term.*Factor, {*}

State I16:
  Term->Factor., {)}
  Term->Factor., {+}
  Term->Factor., {*}

State I17:
  Expr->Expr+.Term, {;}
  Expr->Expr+.Term, {+}
  Term->.Term*Factor, {;}
  Term->.Factor, {;}
  Term->.Term*Factor, {+}
  Term->.Factor, {+}
  Term->.Term*Factor, {*}
  Term->.Factor, {*}
  Factor->.integer, {;}
  Factor->.identifier, {;}
  Factor->.(Expr), {;}
  Factor->.integer, {+}
  Factor->.identifier, {+}
  Factor->.(Expr), {+}
  Factor->.integer, {*}
  Factor->.identifier, {*}
  Factor->.(Expr), {*}

State I18:
  ASSIG->identifier=Expr;., {eof}

State I19:
  Term->Term*.Factor, {;}
  Term->Term*.Factor, {+}
  Term->Term*.Factor, {*}
  Factor->.integer, {;}
  Factor->.identifier, {;}
  Factor->.(Expr), {;}
  Factor->.integer, {+}
  Factor->.identifier, {+}
  Factor->.(Expr), {+}
  Factor->.integer, {*}
  Factor->.identifier, {*}
  Factor->.(Expr), {*}

State I20:
  Factor->(Expr.), {)}
  Factor->(Expr.), {+}
  Factor->(Expr.), {*}
  Expr->Expr.+Term, {)}
  Expr->Expr.+Term, {+}

State I21:
  Expr->Expr+.Term, {)}
  Expr->Expr+.Term, {+}
  Term->.Term*Factor, {)}
  Term->.Factor, {)}
  Term->.Term*Factor, {+}
  Term->.Factor, {+}
  Term->.Term*Factor, {*}
  Term->.Factor, {*}
  Factor->.integer, {)}
  Factor->.identifier, {)}
  Factor->.(Expr), {)}
  Factor->.integer, {+}
  Factor->.identifier, {+}
  Factor->.(Expr), {+}
  Factor->.integer, {*}
  Factor->.identifier, {*}
  Factor->.(Expr), {*}

State I22:
  Factor->(Expr)., {;}
  Factor->(Expr)., {+}
  Factor->(Expr)., {*}

State I23:
  Term->Term*.Factor, {)}
  Term->Term*.Factor, {+}
  Term->Term*.Factor, {*}
  Factor->.integer, {)}
  Factor->.identifier, {)}
  Factor->.(Expr), {)}
  Factor->.integer, {+}
  Factor->.identifier, {+}
  Factor->.(Expr), {+}
  Factor->.integer, {*}
  Factor->.identifier, {*}
  Factor->.(Expr), {*}

State I24:
  Expr->Expr+Term., {;}
  Expr->Expr+Term., {+}
  Term->Term.*Factor, {;}
  Term->Term.*Factor, {+}
  Term->Term.*Factor, {*}

State I25:
  Term->Term*Factor., {;}
  Term->Term*Factor., {+}
  Term->Term*Factor., {*}

State I26:
  Factor->(Expr)., {)}
  Factor->(Expr)., {+}
  Factor->(Expr)., {*}

State I27:
  Expr->Expr+Term., {)}
  Expr->Expr+Term., {+}
  Term->Term.*Factor, {)}
  Term->Term.*Factor, {+}
  Term->Term.*Factor, {*}

State I28:
  Term->Term*Factor., {)}
  Term->Term*Factor., {+}
  Term->Term*Factor., {*}

===== State Transition Graph: =====
I0 --identifier--> I1
I0 --Statement--> I2
I0 --ASSIG--> I3
I1 --=--> I4
I4 --identifier--> I5
I4 --integer--> I6
I4 --(--> I7
I4 --Expr--> I8
I4 --Term--> I9
I4 --Factor--> I10
I7 --identifier--> I11
I7 --integer--> I12
I7 --(--> I13
I7 --Expr--> I14
I7 --Term--> I15
I7 --Factor--> I16
I8 --+--> I17
I8 --;--> I18
I9 --*--> I19
I13 --identifier--> I11
I13 --integer--> I12
I13 --(--> I13
I13 --Expr--> I20
I13 --Term--> I15
I13 --Factor--> I16
I14 --+--> I21
I14 --)--> I22
I15 --*--> I23
I17 --identifier--> I5
I17 --integer--> I6
I17 --(--> I7
I17 --Term--> I24
I17 --Factor--> I10
I19 --identifier--> I5
I19 --integer--> I6
I19 --(--> I7
I19 --Factor--> I25
I20 --+--> I21
I20 --)--> I26
I21 --identifier--> I11
I21 --integer--> I12
I21 --(--> I13
I21 --Term--> I27
I21 --Factor--> I16
I23 --identifier--> I11
I23 --integer--> I12
I23 --(--> I13
I23 --Factor--> I28
I24 --*--> I19
I27 --*--> I23

===== ActionTable =====
State: if      identifierinteger =       *       +       (       )       ;       eof     
I0            s1                                                                      
I1                            s4                                                      
I2                                                                            acc     
I3                                                                            r1      
I4            s5      s6                              s7                              
I5                                    r8      r8                      r8              
I6                                    r7      r7                      r7              
I7            s11     s12                             s13                             
I8                                            s17                     s18             
I9                                    s19     r4                      r4              
I10                                   r6      r6                      r6              
I11                                   r8      r8              r8                      
I12                                   r7      r7              r7                      
I13           s11     s12                             s13                             
I14                                           s21             s22                     
I15                                   s23     r4              r4                      
I16                                   r6      r6              r6                      
I17           s5      s6                              s7                              
I18                                                                           r2      
I19           s5      s6                              s7                              
I20                                           s21             s26                     
I21           s11     s12                             s13                             
I22                                   r9      r9                      r9              
I23           s11     s12                             s13                             
I24                                   s19     r3                      r3              
I25                                   r5      r5                      r5              
I26                                   r9      r9              r9                      
I27                                   s23     r3              r3                      
I28                                   r5      r5              r5                      

===== GotoTable:  =====
State: Pro     StatementASSIG   Expr    Term    Factor  
I0            2       3                               
I1                                                    
I2                                                    
I3                                                    
I4                            8       9       10      
I5                                                    
I6                                                    
I7                            14      15      16      
I8                                                    
I9                                                    
I10                                                   
I11                                                   
I12                                                   
I13                           20      15      16      
I14                                                   
I15                                                   
I16                                                   
I17                                   24      10      
I18                                                   
I19                                           25      
I20                                                   
I21                                   27      16      
I22                                                   
I23                                           28      
I24                                                   
I25                                                   
I26                                                   
I27                                                   
I28                                                   
inputPtr: -1, curStateIndex: 0
nextToken2Cat: x, 1
curStateIndex: 0, nextSymInInput: 1, 1, action: s1
shift by s1
inputPtr: 0, curStateIndex: 1
nextToken2Cat: =, 3
curStateIndex: 1, nextSymInInput: 3, 3, action: s4
shift by s4
inputPtr: 1, curStateIndex: 4
nextToken2Cat: 10, 2
curStateIndex: 4, nextSymInInput: 2, 2, action: s6
shift by s6
inputPtr: 2, curStateIndex: 6
nextToken2Cat: *, 4
curStateIndex: 6, nextSymInInput: 4, 4, action: r7
Reduce by r7
inputPtr: 2, curStateIndex: 10
nextToken2Cat: *, 4
curStateIndex: 10, nextSymInInput: 4, 4, action: r6
Reduce by r6
inputPtr: 2, curStateIndex: 9
nextToken2Cat: *, 4
curStateIndex: 9, nextSymInInput: 4, 4, action: s19
shift by s19
inputPtr: 3, curStateIndex: 19
nextToken2Cat: (, 6
curStateIndex: 19, nextSymInInput: 6, 6, action: s7
shift by s7
inputPtr: 4, curStateIndex: 7
nextToken2Cat: ab, 1
curStateIndex: 7, nextSymInInput: 1, 1, action: s11
shift by s11
inputPtr: 5, curStateIndex: 11
nextToken2Cat: +, 5
curStateIndex: 11, nextSymInInput: 5, 5, action: r8
Reduce by r8
inputPtr: 5, curStateIndex: 16
nextToken2Cat: +, 5
curStateIndex: 16, nextSymInInput: 5, 5, action: r6
Reduce by r6
inputPtr: 5, curStateIndex: 15
nextToken2Cat: +, 5
curStateIndex: 15, nextSymInInput: 5, 5, action: r4
Reduce by r4
inputPtr: 5, curStateIndex: 14
nextToken2Cat: +, 5
curStateIndex: 14, nextSymInInput: 5, 5, action: s21
shift by s21
inputPtr: 6, curStateIndex: 21
nextToken2Cat: 120, 2
curStateIndex: 21, nextSymInInput: 2, 2, action: s12
shift by s12
inputPtr: 7, curStateIndex: 12
nextToken2Cat: ), 7
curStateIndex: 12, nextSymInInput: 7, 7, action: r7
Reduce by r7
inputPtr: 7, curStateIndex: 16
nextToken2Cat: ), 7
curStateIndex: 16, nextSymInInput: 7, 7, action: r6
Reduce by r6
inputPtr: 7, curStateIndex: 27
nextToken2Cat: ), 7
curStateIndex: 27, nextSymInInput: 7, 7, action: r3
Reduce by r3
inputPtr: 7, curStateIndex: 14
nextToken2Cat: ), 7
curStateIndex: 14, nextSymInInput: 7, 7, action: s22
shift by s22
inputPtr: 8, curStateIndex: 22
nextToken2Cat: ;, 8
curStateIndex: 22, nextSymInInput: 8, 8, action: r9
Reduce by r9
inputPtr: 8, curStateIndex: 25
nextToken2Cat: ;, 8
curStateIndex: 25, nextSymInInput: 8, 8, action: r5
Reduce by r5
inputPtr: 8, curStateIndex: 9
nextToken2Cat: ;, 8
curStateIndex: 9, nextSymInInput: 8, 8, action: r4
Reduce by r4
inputPtr: 8, curStateIndex: 8
nextToken2Cat: ;, 8
curStateIndex: 8, nextSymInInput: 8, 8, action: s18
shift by s18
inputPtr: 9, curStateIndex: 18
nextToken2Cat: $, 9
curStateIndex: 18, nextSymInInput: 9, 9, action: r2
Reduce by r2
inputPtr: 9, curStateIndex: 3
nextToken2Cat: $, 9
curStateIndex: 3, nextSymInInput: 9, 9, action: r1
Reduce by r1
inputPtr: 9, curStateIndex: 2
nextToken2Cat: $, 9
curStateIndex: 2, nextSymInInput: 9, 9, action: acc

 Parsing success, AST generated success !!!

[NonTerminal] Statement
  [NonTerminal] ASSIG
    [Terminal   ] x
    [Terminal   ] =
    [NonTerminal] Expr
      [NonTerminal] Term
        [NonTerminal] Term
          [NonTerminal] Factor
            [Terminal   ] 10
        [Terminal   ] *
        [NonTerminal] Factor
          [Terminal   ] (
          [NonTerminal] Expr
            [NonTerminal] Expr
              [NonTerminal] Term
                [NonTerminal] Factor
                  [Terminal   ] ab
            [Terminal   ] +
            [NonTerminal] Term
              [NonTerminal] Factor
                [Terminal   ] 120
          [Terminal   ] )
    [Terminal   ] ;


*/
