
#include "common.h"
#include "LexicalAnalysize.h"

int main()
{
    // ==========1. LexicalAnalysis
    // std::string input = "if ifa";                            // test1 ok
    // std::string input = "a = 1; if (a) a = a * 20 + 120;";   // test2 ok
    // std::string input = "res = 10 * (ab + 120);";            // test3 ok
    std::string input = "x = 10 * (ab + 120);";                 // test4 ok
    std::vector<std::pair<std::string, TokenCategory> > token2CatStream;
    token2CatStream = LexicalAnalysis(input);
}

/*
    g++ common.cpp LexicalAnalysize.cpp LexicalAnalysizeTest.cpp
*/

/*
NFA: test ok

    DFA state(d)	NFA states   		epsilon-closure(delt(d, x))
                                        x = a  		x = i   	x = f
                                
    d0              {n0}           		d1       	d2      	d1
    d1              {n3, n4}            d1          d1          d1
    d2              {n3, n1, n4}        d1          d1          d1 | n2 = d3 = {n3, n4, n2} 
    d3              {n3, n4, n2}        d1          d1          d1 

*/

/*
DFA: test ok:
    dFA size: 4
    DFA state id: 1 <-> NFA state id: 0
    -- char: a--> DFA state id: 2
    -- char: f--> DFA state id: 2
    -- char: i--> DFA state id: 3
    DFA state id: 2 <-> NFA state id: 3, 4
    -- char: a--> DFA state id: 2
    -- char: f--> DFA state id: 2
    -- char: i--> DFA state id: 2
    DFA state id: 3 <-> NFA state id: 1, 3, 4
    -- char: a--> DFA state id: 2
    -- char: f--> DFA state id: 4
    -- char: i--> DFA state id: 2
    DFA state id: 4 <-> NFA state id: 2, 3, 4
    -- char: a--> DFA state id: 2
    -- char: f--> DFA state id: 2
    -- char: i--> DFA state id: 2
*/

/*
test4 partial output:
    ...
===== recognize new lexeme: 
=====move to state id:1
ch:x, lexeme: x
=====move to state id:2
ch: , lexeme: x 
lexeme.size: 2
Rollback0: meet character that should be ignore, only remove last character of lexeme.
token: x, category: 1
===== recognize new lexeme: 
=====move to state id:1
ch:=, lexeme: =
=====move to state id:5
ch: , lexeme: = 
lexeme.size: 2
Rollback0: meet character that should be ignore, only remove last character of lexeme.
token: =, category: 3
===== recognize new lexeme: 
=====move to state id:1
ch:1, lexeme: 1
=====move to state id:4
ch:0, lexeme: 10
=====move to state id:4
ch: , lexeme: 10 
lexeme.size: 3
Rollback0: meet character that should be ignore, only remove last character of lexeme.
token: 10, category: 2
===== recognize new lexeme: 
=====move to state id:1
ch:*, lexeme: *
=====move to state id:6
ch: , lexeme: * 
lexeme.size: 2
Rollback0: meet character that should be ignore, only remove last character of lexeme.
token: *, category: 4
===== recognize new lexeme: 
=====move to state id:1
ch:(, lexeme: (
=====move to state id:8
ch:a, lexeme: (a
the (current) state row in 2 dimension DFATable is empty. The state is the last accepted state and is not filled into DFATable!
lexeme.size: 2
Rollback2: current DFA state (transitions) row is not filled into DFATable
token: (, category: 6
===== recognize new lexeme: 
=====move to state id:1
ch:a, lexeme: a
=====move to state id:2
ch:b, lexeme: ab
=====move to state id:2
ch: , lexeme: ab 
lexeme.size: 3
Rollback0: meet character that should be ignore, only remove last character of lexeme.
token: ab, category: 1
===== recognize new lexeme: 
=====move to state id:1
ch:+, lexeme: +
=====move to state id:7
ch: , lexeme: + 
lexeme.size: 2
Rollback0: meet character that should be ignore, only remove last character of lexeme.
token: +, category: 5
===== recognize new lexeme: 
=====move to state id:1
ch:1, lexeme: 1
=====move to state id:4
ch:2, lexeme: 12
=====move to state id:4
ch:0, lexeme: 120
=====move to state id:4
ch:), lexeme: 120)
lexeme.size: 4
Rollback1: reach ERROR DFAState
token: 120, category: 2
===== recognize new lexeme: 
=====move to state id:1
ch:), lexeme: )
=====move to state id:9
ch:;, lexeme: );
the (current) state row in 2 dimension DFATable is empty. The state is the last accepted state and is not filled into DFATable!
lexeme.size: 2
Rollback2: current DFA state (transitions) row is not filled into DFATable
token: ), category: 7
===== recognize new lexeme: 
=====move to state id:1
ch:;, lexeme: ;
=====move to state id:10
ch:, lexeme: ;
lexeme.size: 2
Rollback4: scanning end, only remove last character of lexeme.
token: ;, category: 8
========== tokens: 
token: x, Category: 1
token: =, Category: 3
token: 10, Category: 2
token: *, Category: 4
token: (, Category: 6
token: ab, Category: 1
token: +, Category: 5
token: 120, Category: 2
token: ), Category: 7
token: ;, Category: 8
*/