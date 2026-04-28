#include <iostream>

#include <map>

#include <stack>
#include <queue>

#include <algorithm>
#include <iomanip>

#include "Parser.h"

char startSymbol = 'S';
char uniqueStartSymAdded = 'Z';

struct Item 
{
    char left;
    std::string right;
    int dotPos; // Note: init. value is 0
    std::set<char> lookaheads;

    bool operator==(const Item& other) const 
    {
        if (left != other.left) 
            return false;
        if (right != other.right) 
            return false;
        if (dotPos != other.dotPos) 
            return false;
        return lookaheads == other.lookaheads; // Note: 2 empty set is equal.
    }
};

typedef std::vector<Item> State;

std::vector<std::pair<char, std::string> > productions;
std::map<std::string, int> prodStr2IndexMap;

std::vector<State> states; // state vector/array: store states in sequence, used when lookup GotoTable and ActionTable 
std::map<std::pair<int, char>, int> stateTransGraph; // < {srcStateIndex, GrammarSym}, dstStateInex>
std::map<int, std::map<char, std::string> > actionTable; // <srcStateIndex, {terminal, choice_reduceOrShiftOrAccOrErr}>
std::map<int, std::map<char, int> > gotoTable;

bool isTerminal(char c) 
{
    return islower(c) || c == eof || c == '+' || c == '*' || c == '(' || c == ')';
}

bool isNonTerminal(char c) 
{
    return isupper(c) && c != uniqueStartSymAdded;
}

void printItem(const Item& item) 
{
    std::cout << item.left << " → ";
    for (int i = 0; i < item.right.size(); ++i) {
        if (i == item.dotPos) std::cout << "•";
        std::cout << item.right[i];
    }
    if (item.dotPos == item.right.size()) std::cout << "•";
    std::cout << ", { ";
    for (char c : item.lookaheads) std::cout << c << " ";
    std::cout << "}";
}

// =============
bool isItemExistInState(const Item& item, const State& state)
{
    bool exist = false;
    for (const auto& item_ : state)
    {
        if (item_.left == item.left && 
            item_.right == item.right &&
            item_.dotPos == item.dotPos && 
            item_.lookaheads == item.lookaheads) 
        {
            exist = true;
            break;
        }
    }
    return exist;
}

/*
===================== state closure
design scheme:

    1] dotNextGramSym_FollowGramSym_StartStr 
        为空 
        或 第1个符号，即 dotNextGramSym_FollowGramSym 为 终结符时，计算 dotNextGramSym_FollowGramSym_FirstSet 才有意义

        if ( dotNextGramSym_FollowGramSym_StartStr is empty)
            dotNextGramSym_FollowGramSym_FirstSet = curItem.lookaheads;                  // 产生式 中 . 已经到底末尾 => 前瞻符号 用 当前 curItem.lookaheads
        else if( isTerminal(dotNextGramSym_FollowGramSym = dotNextGramSym_FollowGramSym_StartStr[0]) )
            dotNextGramSym_FollowGramSym_FirstSet.insert(dotNextGramSym_FollowGramSym);  // 产生式 中 . 为到底末尾 => 前瞻符号 用 dotNextGramSym_FollowGramSym

    2] dotNextGramSym_FollowGramSym_FirstSet -> 新发现的 item 的 item.lookaheads -> 最终在 fillActionTable 中作为 actionTable 的 colIndex 

*/
void closure(State& state) // STL container (std::vecror): 不需要 独立副本（容器已存在）时，用 引用传递 -> 且 想修改 容器 => 用 non-const 引用
{
    // [1] in order to do BFS traverse/process, use a itemQue.
    std::queue<Item> itemQue;
    for (const auto& item : state) 
        itemQue.push(item);
    
    // [2] while (!itemQue.empty() )
    while (!itemQue.empty() ) 
    {
        // [2.1] extract the front of the itemQue, then pop.
        Item curItem = itemQue.front();
        itemQue.pop();

        // [2.2] get dotNextGramarSym
        char dotNextGramarSym = curItem.right[curItem.dotPos];

        // [2.3] closure doesn't generate new item for the item whose dot(placeholder) has reached the end, or whose dotNextGramarSym is terminal. 
        if (curItem.dotPos >= curItem.right.size() || 
            isTerminal(dotNextGramarSym) ) 
            continue;

        // [2.4] calculate lookahead dotNextGramSym_FollowGramSym_FirstSet for items whose 
        std::string dotNextGramSym_FollowGramSym_StartStr = curItem.right.substr(curItem.dotPos + 1);
        std::set<char> dotNextGramSym_FollowGramSym_FirstSet;
        if (dotNextGramSym_FollowGramSym_StartStr.empty() )
            dotNextGramSym_FollowGramSym_FirstSet = curItem.lookaheads;
        else
        {
            char dotNextGramSym_FollowGramSym = dotNextGramSym_FollowGramSym_StartStr[0]; // A -> .Aa, eof => dotNextGramSym_FollowGramSym = a

            if ( isTerminal(dotNextGramSym_FollowGramSym) )
                dotNextGramSym_FollowGramSym_FirstSet.insert(dotNextGramSym_FollowGramSym);
            // else if (isNonTerminal(dotNextGramSym_FollowGramSym) )
                // dotNextGramSym_FollowGramSym_FirstSet is useless when dotNextGramSym_FollowGramSym is a NonTerminal
        }
            
        /* 
            [2.5] for every production that define the dotNextGramarSym, 
                1] create a item with dotPos is 0, and lookaheads is dotNextGramSym_FollowGramSym_FirstSet
                
                2] check if it's not exist in the current state, 
                    add the new item to the current state to complement(补全) the state and 
                    add the new item to the itemQueue to continue BFS traverse/process. 
        */
        for (const auto& prod : productions)   
        {
            if (prod.first == dotNextGramarSym)  
            {
                Item item;
                item.left = prod.first;
                item.right = prod.second;
                item.dotPos = 0;
                item.lookaheads = dotNextGramSym_FollowGramSym_FirstSet;    // A -> .Aa, eof => item.lookaheads = dotNextGramSym_FollowGramSym = a

                bool existInState = isItemExistInState(item, state);
                if (!existInState) 
                {
                    state.push_back(item); // modify state to become larger // 对 所有定义 dotNextGramarSym(A) 的 production:  A -> Aa / A -> a, 生成新 item: A -> .Aa, a / A -> .a, a
                    itemQue.push(item);
                }
            }
        }
    }
}

/*
    for a given curState and a given grammarSym, build/return the transDstState.

    栈变量（容器）必须以 独立副本 返回 => 1] 返回值用 值传递 -> 且想 接管（效率高）容器副本中的元素 => 2] return std::move(容器独立副本);
    容器 curState: 只读 => 用 const State& 传递
*/
State buildTransDstState(const State& curState, char grammarSym) // STL container (std::vecror): 不需要 独立副本时，用 引用传递 -> 且 想修改 容器 => 用 non-const 引用
{
    // [1] For the current state, only its items whose next symbol (of the placeholder) is equal to the given grammarSym truly contribute to transDstState, 
    //     so the transDstState is initialized to the set of those items.
    State transDstState; 
    for (const auto& item : curState) 
    {
        if (item.dotPos >= item.right.size() ) 
            continue;
        if (item.right[item.dotPos] == grammarSym) 
        {
            // move dot to next position to generate a new item
            Item newItem = item;
            newItem.dotPos++; 

            transDstState.push_back(newItem);
        }
    }

    // [2] Compute the closure of the transDstState, taking the initial transDstState as its argument.
    closure(transDstState);

    return std::move(transDstState); // 触发 移动语义
}

int searchInStates(const State& transDstState)
{
    int matchedStateIndex = -1;
    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) 
    {
        if (states[stateIndex].size() != transDstState.size() ) // Two states are bound to be different if their number of items differs.
            continue;
            
        bool isStateItemsAllSame = true;
        // check whether the transDstState is exactly the same as the possible matched state in states
        for (int itemIndex = 0; itemIndex < states[stateIndex].size(); ++itemIndex)  
        {
            
            if (!(states[stateIndex][itemIndex] == transDstState[itemIndex]) ) 
            {
                isStateItemsAllSame = false;
                break;
            }
        }
        if (isStateItemsAllSame) // found a match state => the transDstState has already built.
        {
            matchedStateIndex = stateIndex; 
            break;
        }
    }
    return matchedStateIndex;
}

void buildStatesAndStateTransGraph() 
{
    Item uniqueStartItem;
    uniqueStartItem.left = uniqueStartSymAdded;
    uniqueStartItem.right = std::string(1, startSymbol);
    uniqueStartItem.dotPos = 0; 
    uniqueStartItem.lookaheads.insert(eof);

    // [1] calc startState by closure(startState init. value with single uniqueStartItem)
    State startState;
    startState.push_back(uniqueStartItem);
    closure(startState);

    // [2] startState added to state set
    states.push_back(startState);

    // [3] build states and stateTransGraph by BFS traverse => using a stateIndexQue
    std::queue<int> stateIndexQue;
    stateIndexQue.push(0); 
    while (!stateIndexQue.empty() ) 
    {
        // [3.1] extract then Pop the current state(Index) to be processed
        int curStateIndex = stateIndexQue.front();
        stateIndexQue.pop();

        // [3.2] traverse all items in current state where dotPos hasn't reached the end of the right-hand side, to get their dotNextSyms.
        std::set<char> curStateDotNextGrammarSyms;
        for (const auto& item : states[curStateIndex]) 
        {
            if (item.dotPos < item.right.size() )
                curStateDotNextGrammarSyms.insert(item.right[item.dotPos]);
        }

        /* 
            [3.3] for current state and every vaild dotNextGramarSym in the current state, 
                1] build the dst transition state(by closure),
                    When the same state is constructed multiple times, their items are identical in order/sequence.
                    => Comparing two states only requires comparing each item in sequence.
                    then 
                2] search transDstState in States to find a matched state (inedx)  
                3] if not found, 
                    Push transDstState to states,
                    Push transDstState index to stateIndexQue.
                4] link current state (index) to transDstState.
        */
        for (char dotNextGramarSym: curStateDotNextGrammarSyms)
        {
            State transDstState = buildTransDstState(states[curStateIndex], dotNextGramarSym);
            if (transDstState.empty() ) 
                continue;
            
            int matchedStateIndex = searchInStates(transDstState);

            // doesn't find a matched state => next state is a new state
            if (-1 == matchedStateIndex) 
            {
                matchedStateIndex = states.size();
                states.push_back(transDstState);   // states become larger
                stateIndexQue.push(matchedStateIndex);
            }

            stateTransGraph[{curStateIndex, dotNextGramarSym}] = matchedStateIndex;
        }
    }
}

// For a given state(index), fill GotoTable by searching stateTransGraph for the matched links whose srcStateIndex is the same as the given state(index).
/*
global var:
    stateTransGraph: 
    gotoTable
*/
void fillGotoTable(int stateIndex)
{
    for (const auto& link : stateTransGraph) 
    {
        int srcStateInex = link.first.first;
        char grammarSym = link.first.second;
        int dstStateIndex = link.second;
        if (srcStateInex == stateIndex && isNonTerminal(grammarSym) ) 
        {
            gotoTable[stateIndex][grammarSym] = dstStateIndex;
        }
    }
}

/*
global var:
    uniqueStartSymAdded: for accepted str
    actionTable
    prodStr2IndexMap: for reduce str
    stateTransGraph: for shift reduce

    Action 表的列 = 下一个输入符号（只能是终结符 / EOF）
    Action 表的行 = 当前状态

    [1] ActionTable 的 shift 部分 (+ GotoTable) 由 stateTransGraph + state 中 placeholder 未到达末尾的 items + dotNextGramarSym 可得到 要转移到的 状态 (index)

    [2] ActionTable 的 reduce 和 acc 部分 由 state + stateIndex + state 中 placeholder 到达末尾的 items 的 lookahead + production2proIndex 可得 要归约的 production index

    为什么 shift 时，actionTable[stateIndex][dotNextGramarSym] 却要用 dotNextGramarSym = item.right[item.dotPos];
        而 reduce 或 Accept 时 actionTable[stateIndex][lookahead] 中的 lookahead 是取 item.lookaheads 的元素。

    答: Shift 动作的触发条件 = 点・后面紧跟一个终结符，点・后面还可以 再读 终结符。
        Reduce / Accept 的触发条件 = 点・已经在产生式最右边（归约项），点・后面没有终结符可读了，只能看 产生式本身的 前瞻符号。 
*/
void fillActionTable(int stateIndex, const State& state)
{
    for (const auto& item : state) 
    {
        if (item.dotPos < item.right.size() ) // shift
        {
            char dotNextGramarSym = item.right[item.dotPos];
            if (isTerminal(dotNextGramarSym) && stateTransGraph.count({stateIndex, dotNextGramarSym}) ) 
            {
                int transDstStateIndex = stateTransGraph[{stateIndex, dotNextGramarSym}];
                actionTable[stateIndex][dotNextGramarSym] = "s" + std::to_string(transDstStateIndex);
            }
            // else if (isNonTerminal(X) ) 
                // doNothing // actionTable[stateIndex][X] is empty, because NonTerminal is invaild col in ActionTable
        }
        else if (item.dotPos == item.right.size() ) 
        {
            if (item.left == uniqueStartSymAdded) // accepted
            {
                for (char lookahead: item.lookaheads) 
                    if (isTerminal(lookahead) ) // ======optimize: only terminal and eof (viewed as terminal) is vaild col in ActionTable
                        actionTable[stateIndex][lookahead] = "acc";
            } 
            else // reduce
            {
                std::string prodStr = std::string(1, item.left) + "→" + item.right;
                int prodIndex = prodStr2IndexMap[prodStr];
                for (char lookahead: item.lookaheads)
                    if (isTerminal(lookahead) )
                        actionTable[stateIndex][lookahead] = "r" + std::to_string(prodIndex);
            }
        } 
        
    }
}
        
/*
global var:
    actionTable
    gotoTable
*/
void buildActTblAndGotoTbl() 
{
    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) 
    {
        actionTable[stateIndex] = std::map<char, std::string>();
        gotoTable[stateIndex] = std::map<char, int>();
    }

    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) 
    {
        fillActionTable(stateIndex, states[stateIndex]);

        fillGotoTable(stateIndex);
    }
}

void printAll(std::set<char> terms, std::set<char>& nonTerms) 
{
    std::cout << "\n===== LR(1) all States: =====" << std::endl;
    for (int i = 0; i < states.size(); ++i) {
        std::cout << "\n状态 I" << i << ":" << std::endl;
        for (const auto& item : states[i]) {
            std::cout << "  ";
            printItem(item);
            std::cout << std::endl;
        }
    }

    std::cout << "\n===== State Transition Graph: =====" << std::endl;
    for (const auto& t : stateTransGraph) {
        int f = t.first.first;
        char c = t.first.second;
        int to = t.second;
        std::cout << "I" << f << " --" << c << "--> I" << to << std::endl;
    }

    std::cout << "\n===== ActionTable =====" << std::endl;
    //std::set<char> terms = {'i', '+', '*', '(', ')', eof};
    std::cout << std::left << std::setw(6) << "State: ";
    for (char c : terms) 
        std::cout << std::setw(8) << c;
    std::cout << std::endl;

    for (int i = 0; i < states.size(); ++i) {
        std::cout << std::setw(6) << "I" + std::to_string(i);
        for (char c : terms) 
        {
            if (actionTable[i].count(c))
                std::cout << std::setw(8) << actionTable[i][c];
            else
                std::cout << std::setw(8) << "";
        }
        std::cout << std::endl;
    }

    std::cout << "\n===== GotoTable:  =====" << std::endl;
    //std::set<char> nonTerms = {'E', 'T', 'F'};
    std::cout << std::left << std::setw(6) << "State: ";
    for (char c : nonTerms) 
        std::cout << std::setw(8) << c;
    std::cout << std::endl;

    for (int i = 0; i < states.size(); ++i) 
    {
        std::cout << std::setw(6) << "I" + std::to_string(i);
        for (char c : nonTerms) {
            if (gotoTable[i].count(c))
                std::cout << std::setw(8) << gotoTable[i][c];
            else
                std::cout << std::setw(8) << "";
        }
        std::cout << std::endl;
    }
}

/*
global var:
    productions
    startSymbol
    prodStr2IndexMap
*/
void initGrammar() 
{
    productions.clear();
    
    productions.emplace_back('E', "E+T"); 
    productions.emplace_back('E', "T");
    productions.emplace_back('T', "T*F");
    productions.emplace_back('T', "F");
    productions.emplace_back('F', "(E)");
    productions.emplace_back('F', "i");
    startSymbol = 'E';
    
    /*
    productions.emplace_back('G', "A");  // G -> A
    productions.emplace_back('A', "Aa"); // A -> Aa  makeConcatenateNode(, makeANode)
    productions.emplace_back('A', "a");  // A -> a   makeANode()
    startSymbol = 'G';
    */

    for (int prodIndex = 0; prodIndex < productions.size(); ++prodIndex) 
    {
        std::string key = std::string(1, productions[prodIndex].first) + "→" + productions[prodIndex].second;
        prodStr2IndexMap[key] = prodIndex;
    }
}

/*
Pseudo Code
    stack.push(s0)
    state = s0
    nextInputSymPtr = -1;
    strLen = string.size();

    While ( nextInputSymPtr <= strLen)
        chiose = ActionTable[state][lookahead = input[nextInputSymPtr+1])] // lookahead: nextSymInInput
        
        if chiose.shift == true:
            nextInputSymPtr++ // shift: consum the nextSymInInput
            state = choice.transDstState // move to the dst transition state 
            stack.push(lookahead) // nextSymInInput(terminal) pushed to stack
            stack.push(state) 
        if chiose.reduction== true:
            // 将该规则右边所有 文法符号（终结符、非终结符）全部 pop
            for (symInex = 0; symInex < reductionProductionRightSymNum; symInex ++)
                pop stackTop: the latest transition dst state
                pop stackTop: the latest transition Sym: terminal or non-terminal
            
            // Goto 基于 pop 后的 栈顶 & reduction non-terminal
            reductionNonTerminal = chiose.reduction.left  //  production left 
            state = Goto[stack.top(): the latest transition src state][reductionNonTerminal] // move to the dst transition state 
            stack.push(reductionNonTerminal) // reductionNonTerminal pushed to stack
            stack.push(state)
        if chiose.Accept== true:
            // 将该规则右边所有 文法符号（终结符、非终结符）全部 pop
            for (symInex = 0; symInex < reductionProductionRightSymNum; symInex ++)
                pop stackTop: the latest transition dst state
                pop stackTop: the latest transition Sym: terminal or non-terminal
            
            if lookahead = input[nextInputSymPtr+1])  == eof && stack.top() == s0
                nextInputSymPtr++;
                return success
        else 
            return Error

*/

/*
implement:

    [1] create two stacks:
        stateIndexStack: stores state (indices) in reverse order along the parsing path in the state transition graph
        grammarSymStack: stores grammar symbols in reverse order along the parsing path in the state transition graph

    [2] for every valid nextSymInInput in input: 

        base on [curStateIndex][nextSymInInput] to lookup ActionTable to get action,
        
        if action == shift:
            1] nextInputSymPtr++ to consum the nextSymInInput
            2] update curStateIndex = std::stoi(action.substr(1) ) 为 要转移到的 状态（索引），
                即 move to the transDst state (index) 。
            3] nextSymInInput(terminal) / curStateIndex Pushed to grammarSymStack / stateIndexStack
        else if action == reduce:
            1] Pop all states (indices) and grammar symbols corresponding to the right-hand side of the production, 
            2] find the transDst state (index) by looking up the GotoTable based on the current stack-top state and non-terminal being reduced to,
               move to the transDst state (index) 
            3] reductionNonTerminal / curStateIndex Pushed to grammarSymStack / stateIndexStack

    [3] ASTNodePtrStack: 
        action is shift: 
            Create a Terminal Node using nextSymInInput,
            Push it to ASTNodePtrStack
        action is reduce: build subTree by add  every ParserTreeNodePtr to related Grammar Symbol in right-side to left Grammar Symbol's ParserTreeNodePtr children.
            Create a NonTerminal Node using the left Grammar Symbol of reduction/production, 
            Pop reduction.right.size() ASTNodePtrs as NonTerminal Node's children, 
            Reverse NonTerminal Node's children to ensure children is the same sequence as right of the reduction.                 
*/
/*
global var:

*/

#ifdef NEEDPARSERTREE 
ParserTreeNodePtr 
#else
void*
#endif
parser(std::string input)
{
    std::stack<int> stateIndexStack;
    stateIndexStack.push(0);
    int curStateIndex = 0;

    std::stack<char> grammarSymStack;

#ifdef NEEDPARSERTREE 
    std::stack<ParserTreeNodePtr> parserTreeNodeStack;
#endif

    int nextInputSymPtr = -1;
    int inputLen = input.size();
    input = input + eof;  // add eof for simple process
    char nextSymInInput;  // nextSymInInput
    while (nextInputSymPtr < inputLen)
    {
        std::cout << "nextInputSymPtr: " << nextInputSymPtr << std::endl;

        nextSymInInput = input[nextInputSymPtr+1];      
        
        if ( ' ' == nextSymInInput ||
            '\n' == nextSymInInput ||
            '\t' == nextSymInInput) // ignore whitespace in input
        {
            nextInputSymPtr++;
            continue;
        }

        if (actionTable[curStateIndex].count(nextSymInInput) )  // action not empty
        {
            std::string action = actionTable[curStateIndex][nextSymInInput];
            std::cout << "curStateIndex: " << curStateIndex << ", nextSymInInput: " << nextSymInInput << ", action: " << action << std::endl;

            if ('s' == action[0])
            {
                // 1]
                nextInputSymPtr++; // shift: consum the nextSymInInput

                // 2]
                curStateIndex = std::stoi(action.substr(1) ); // move to the dst transition state 
                std::cout << "shift by "<< action << std::endl; 
                
                // 3]
                grammarSymStack.push(nextSymInInput); // nextSymInInput(terminal) pushed to stack
                stateIndexStack.push(curStateIndex); 

#ifdef NEEDPARSERTREE 
                ParserTreeNodePtr termNode = std::make_shared<ParserTreeNode>(TERMINAL, nextSymInInput);
                parserTreeNodeStack.push(termNode); // parserTreeNodeStack: [a (top) -> empty -> A (top)]-> [A a (top)]
#endif 

            }
            else if ('r' == action[0])    
            {
                std::cout << "Reduce by "<< action << std::endl; 

                // 1]
                int prodIndex = std::stoi(action.substr(1) );
                char reductionNonTerminal = productions[prodIndex].first;   // reduction/production left 
                std::string reductionRight = productions[prodIndex].second; // reduction/production right

#ifdef NEEDPARSERTREE 
                ParserTreeNodePtr nonTermNode = std::make_shared<ParserTreeNode>(NON_TERMINAL, reductionNonTerminal);
#endif
                // 2] 
                for (int symInex = 0; symInex < reductionRight.size(); symInex ++)
                {
                    grammarSymStack.pop();
                    stateIndexStack.pop();

                    /*

                    AST:
                            A         
                            |
                            a  
                    
                        ->

                             A
                            / \
                           A   a
                           |
                           a
                       ->
                       
                            G
                            |
                            A
                           / \
                          A   a
                          |
                          a

                        parserTreeNodeStack: [a (top) -> empty -> A (top)] -> [A a (top) -> empty]
                    */
#ifdef NEEDPARSERTREE 
                    nonTermNode->addChild(parserTreeNodeStack.top() ); 
                    parserTreeNodeStack.pop();
#endif
                }

#ifdef NEEDPARSERTREE 
                // reverse to ensure childs is the same sequence as right of the reduction/production
                std::reverse(nonTermNode->children.begin(), nonTermNode->children.end() );

                /*
                    parserTreeNodeStack: [A] -> [A] -> [G]
                */
                parserTreeNodeStack.push(nonTermNode);
#endif 
                
                // 3]
                if (gotoTable.count(stateIndexStack.top() ) &&
                    gotoTable[stateIndexStack.top()].count(reductionNonTerminal) )
                {
                    
                    curStateIndex = gotoTable[stateIndexStack.top()][reductionNonTerminal];  

                    grammarSymStack.push(reductionNonTerminal);                          
                    stateIndexStack.push(curStateIndex);
                }
                
            }
            else if("acc" == action)
            {
                nextInputSymPtr++;
                if (nextInputSymPtr + 1 == input.size() )
                {
                    std::cout << "\n Parsing success, AST generated success !!!\n" << std::endl;

#ifdef NEEDPARSERTREE 
                    return parserTreeNodeStack.top();
#endif 
                }
            } 
        }  
        else 
        {
            std::cout << "parsing Error!!!\n";
            return nullptr;
        }   
    }
    return nullptr;
}

#ifdef NEEDPARSERTREE
// 先处理根 + 递归处理每个孩子 => BFS 遍历打印 AST: 
void printAst(const ParserTreeNodePtr& root, int indent) 
{
    if (!root) 
        return;

    for (int i = 0; i < indent; ++i) 
        std::cout << "  ";
    
    std::cout << (NON_TERMINAL == root->nodeType ? "[NonTerminal] " : "[Terminal   ] ")
              << root->grammarSym << std::endl;
    
    for (const auto& child : root->children) 
    {
        printAst(child, indent + 1);
    }
}
#endif