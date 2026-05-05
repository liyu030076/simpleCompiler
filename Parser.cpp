#include <iostream>
#include <map>
#include <stack>
#include <queue>
#include <algorithm>
#include <iomanip>

#include "Parser.h"

std::vector<Production> productions;

std::vector<State> states; // state vector/array: store states in sequence, used when lookup GotoTable and ActionTable 

std::map<std::pair<int, GrammarSym>, int> stateTransGraph; // < {srcStateIndex, GrammarSym}, dstStateInex>
std::map<int, std::map<GrammarSym, std::string> > actionTable; // <srcStateIndex, {getTerminalSym, choice_reduceOrShiftOrAccOrErr}>
std::map<int, std::map<GrammarSym, int> > gotoTable;

std::vector<std::pair<NonTerminalId, const char*> > nonTerminalId2Str
{
    {Pro, "Pro"},
    {Statement, "Statement"},
    {ASSIG, "ASSIG"},
    {Expr, "Expr"},
    {Term, "Term"},
    {Factor, "Factor"}
};

std::vector<std::pair<TokenCategory, const char*> > terminalId2Str
{
    {IF, "if"},
    {IDENTIFIER, "identifier"},
    {INTEGER, "integer"},
    {ASSIGN, "="},
    {MUL, "*"},
    {ADD, "+"},
    {LPARENTHESES, "("},
    {RPARENTHESES, ")"},
    {SEMICOLON, ";"},
    {SENTINEL, "eof"}
};

// =============
/*
    better design: 令 productions 的 startSym 唯一 => start production index = 0

global var:
    productions
*/
void initGrammar() 
{
    productions = 
    {
        {getNonTerminalSym(Pro), {getNonTerminalSym(Statement)} },                                                                              // Program → Statement
        // {getNonTerminalSym(Statement), {getTerminalSym(IF), getTerminalSym(LPARENTHESES), getNonTerminalSym(Expr), getTerminalSym(RPARENTHESES), getNonTerminalSym(Statement)} },  // S → if(E)S  // hasn't support if statement
        {getNonTerminalSym(Statement), {getNonTerminalSym(ASSIG)} },                                                                            // Statement → ASSIG
        {getNonTerminalSym(ASSIG), {getTerminalSym(IDENTIFIER), getTerminalSym(ASSIGN), getNonTerminalSym(Expr), getTerminalSym(SEMICOLON)} },  // ASSIG → identifier = Expr;   
        {getNonTerminalSym(Expr), {getNonTerminalSym(Expr), getTerminalSym(ADD), getNonTerminalSym(Term)} },                                    // Expr → Expr + Term           rule3
        {getNonTerminalSym(Expr), {getNonTerminalSym(Term)} },                                                                                  // Expr → Term                  rule4
        {getNonTerminalSym(Term), {getNonTerminalSym(Term), getTerminalSym(MUL), getNonTerminalSym(Factor)} },                                  // Term → Term * Factor         rule5
        {getNonTerminalSym(Term), {getNonTerminalSym(Factor)} },                                                                                // Term → Factor                rule6
        {getNonTerminalSym(Factor), {getTerminalSym(INTEGER)} },                                                                                // Factor → integer             rule7
        {getNonTerminalSym(Factor), {getTerminalSym(IDENTIFIER)} },                                                                             // Factor → identifier          rule8
        {getNonTerminalSym(Factor), {getTerminalSym(LPARENTHESES), getNonTerminalSym(Expr), getTerminalSym(RPARENTHESES)} }                     // Factor → (Expr)              rule9
    };
}

bool isItemExistInState(const LR1Item& item, const State& state)
{
    bool exist = false;
    for (const auto& item_ : state)
    {
        if (item == item_) 
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
    1] 一条 LR1Item 的 lookahead 只有当 dotPos 之后第 2 个 Sym 为 Terminal 或 dotPos 到达末尾(dot 之后为 空) 时才有意义，
        当 dotPos 之后第 2 个 Sym 为 NonTerminal 时没用。
            curItem 中 . 已经到达末尾 =>  闭包产生的 newItem 的 前瞻符号(lookahead) 用 当前 curItem.lookahead
            curItem 中 . 未到达末尾   =>  闭包产生的 newItem 的 前瞻符号(lookahead) 用 dotNextGramSym_FollowGramSym

    2] dotNextGramSym_FollowGramSym_FirstSet -> 新发现的 item 的 item.lookahead -> 最终在 fillActionTable 中作为 actionTable 的 colIndex 
*/
void closure(State& state) // STL container (std::vecror): 不需要 独立副本（容器已存在）时，用 引用传递 -> 且 想修改 容器 => 用 non-const 引用
{
    // [1] in order to do BFS traverse/process, use a itemQue.
    std::queue<LR1Item> itemQue;
    for (const auto& item : state) 
        itemQue.push(item);
    
    // [2] while (!itemQue.empty() )
    while (!itemQue.empty() ) 
    {
        // [2.1] extract the front of the itemQue, then pop.
        LR1Item curItem = itemQue.front();
        itemQue.pop();

        // [2.2] get curItemDotNextGramSym
        GrammarSym curItemDotNextGramSym = productions[curItem.prodIndex].rhs[curItem.dotPos];

        // [2.3] closure doesn't generate new item for the item whose dot(placeholder) has reached the end, 
        //                                                   or whose curItemDotNextGramSym is TerminalSym. 
        int curItemRhsSymNum = productions[curItem.prodIndex].rhs.size();

        if (curItem.dotPos >= curItemRhsSymNum || 
            TERMINAL == curItemDotNextGramSym.grammarSymCat) 
            continue;

        // [2.4] calculate lookahead/dotNextGramSym_FollowGramSym_FirstSet for items generated by closure(curItem) 
        GrammarSym dotNextGramSym_FollowGramSym_FirstSet;
        
        if (curItem.dotPos < curItemRhsSymNum - 1) // optimize: this branch may has high hit ratio // curItem.dotPos hasn't reached the previous position of the end of the right-hand side
        {
            GrammarSym cutItemDotNextGramSym_FollowGramSym = productions[curItem.prodIndex].rhs[curItem.dotPos + 1]; // A -> .Aa, eof => cutItemDotNextGramSym_FollowGramSym = a
            if (TERMINAL == cutItemDotNextGramSym_FollowGramSym.grammarSymCat)
                dotNextGramSym_FollowGramSym_FirstSet = cutItemDotNextGramSym_FollowGramSym;
            // else if (NON_TERMINAL == cutItemDotNextGramSym_FollowGramSym.grammarSymCat)
                // dotNextGramSym_FollowGramSym_FirstSet is useless when cutItemDotNextGramSym_FollowGramSym is an NonTerminal
        }
        else // if (curItem.dotPos == curItemRhsSymNum - 1) // curItem.dotPos reach the previous position of the end of the right-hand side
        {
            dotNextGramSym_FollowGramSym_FirstSet = curItem.lookahead;
        }
            
        /* 
            [2.5] for every production that define the curItemDotNextGramSym, 
                1] create a item with dotPos is 0, and lookahead is dotNextGramSym_FollowGramSym_FirstSet
                
                2] check if it's not exist in the current state, 
                    add the new item to the current state to complement(补全) the state and 
                    add the new item to the itemQueue to continue BFS traverse/process. 
        */
        for (int prodIndex = 0; prodIndex < productions.size(); prodIndex++)   
        {
            if (productions[prodIndex].lhs == curItemDotNextGramSym)  
            {
                LR1Item item;
                item.prodIndex = prodIndex;
                item.dotPos = 0;
                item.lookahead = dotNextGramSym_FollowGramSym_FirstSet;    // A -> .Aa, eof => item.lookahead = cutItemDotNextGramSym_FollowGramSym = a

                bool existInState = isItemExistInState(item, state);
                if (!existInState) 
                {
                    state.push_back(item); // modify state to become larger // 对 所有定义 curItemDotNextGramSym(A) 的 production:  A -> Aa / A -> a, 生成新 item: A -> .Aa, a / A -> .a, a
                    itemQue.push(item);
                }
            }
        }
    }
}

/*
    for a given curState and a given grammarSym, build/return the transDstState.
    algorithm: 
        [1] first calculate initial transDstState, 
        [2] then calculate transDstState = closure(initial transDstState)

    栈变量（容器）必须以 独立副本 返回 => 1] 返回值用 值传递 -> 且想 接管（效率高）容器副本中的元素 => 2] return std::move(容器独立副本);
    容器 curState: 只读 => 用 const State& 传递
*/
State buildTransDstState(const State& curState, GrammarSym grammarSym) // STL container (std::vecror): 不需要 独立副本时，用 引用传递 -> 且 想修改 容器 => 用 non-const 引用
{
    // [1] For the current state, only its items whose next symbol (of the placeholder) equal to the given grammarSym truly contribute to transDstState, 
    //     so the transDstState is initialized to the set of those items.
    State transDstState; 
    for (const auto& item : curState) 
    {
        if (item.dotPos >= productions[item.prodIndex].rhs.size() ) 
            continue;
        if (grammarSym == productions[item.prodIndex].rhs[item.dotPos]) 
        {
            // Note: move dot to next position to generate a new item
            LR1Item newItem = item; // Note: C++ struct 赋值 默认为 逐成员赋值
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
    LR1Item uniqueStartItem;
    uniqueStartItem.prodIndex = 0; // point to the start production whose production index = 0 (productions already designed as has unique startSym)
    uniqueStartItem.dotPos = 0; 
    uniqueStartItem.lookahead = eof;

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
        std::set<GrammarSym> curStateDotNextGrammarSyms;
        for (const auto& item : states[curStateIndex]) 
        {
            if (item.dotPos < productions[item.prodIndex].rhs.size() )
                curStateDotNextGrammarSyms.insert(productions[item.prodIndex].rhs[item.dotPos]);
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
        for (GrammarSym dotNextGramarSym: curStateDotNextGrammarSyms)
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

/*
    For a given state(index), fill GotoTable by searching stateTransGraph for the matched links 
        whose srcStateIndex is the same as the given state(index) and the related grammarSym is NonTerminal.

global var:
    stateTransGraph: 
    gotoTable
*/
void fillGotoTable(int stateIndex)
{
    for (const auto& link : stateTransGraph) 
    {
        int srcStateInex = link.first.first;
        GrammarSym grammarSym = link.first.second;
        int dstStateIndex = link.second;
        if (srcStateInex == stateIndex && NON_TERMINAL == grammarSym.grammarSymCat) 
        {
            gotoTable[stateIndex][grammarSym] = dstStateIndex;
        }
    }
}

/*
global var:
    actionTable
    stateTransGraph: for shift reduce

    Action 表的列 = 下一个输入符号（只能是终结符 / EOF）
    Action 表的行 = 当前状态

    [1] ActionTable 的 shift 部分 (+ GotoTable) 由 stateTransGraph + state 中 placeholder 未到达末尾的 items + dotNextGramarSym 可得到 要转移到的 状态 (index) 确定

    [2] ActionTable 的 reduce 和 acc 部分 由 state + stateIndex + state 中 placeholder 到达末尾的 items 的 lookahead +要归约的 production index 确定 

    为什么 shift 时，actionTable[stateIndex][dotNextGramarSym] 却要用 dotNextGramarSym = productions[item.prodIndex].rhs[item.dotPos];
        而 reduce 或 Accept 时 actionTable[stateIndex][lookahead] 中的 lookahead 是取 item.lookahead。

    答: Shift 动作的触发条件 = 点・后面紧跟一个终结符，点・后面还可以 再读 终结符。
        Reduce / Accept 的触发条件 = 点・已经在产生式最右边（归约项），点・后面没有终结符可读了，只能看 产生式本身的 前瞻符号。 
*/
void fillActionTable(int stateIndex, const State& state)
{
    for (const auto& item : state) 
    {
        if (item.dotPos < productions[item.prodIndex].rhs.size() ) // shift
        {
            GrammarSym dotNextGramarSym = productions[item.prodIndex].rhs[item.dotPos];
            if (TERMINAL == dotNextGramarSym.grammarSymCat && stateTransGraph.count({stateIndex, dotNextGramarSym}) ) 
            {
                int transDstStateIndex = stateTransGraph[{stateIndex, dotNextGramarSym}];
                actionTable[stateIndex][dotNextGramarSym] = "s" + std::to_string(transDstStateIndex);
            }
            // else if (NON_TERMINAL == dotNextGramarSym.grammarSymCat) 
                // doNothing // actionTable[stateIndex][dotNextGramarSym] is empty, because NonTerminal is invaild col in ActionTable
        }
        else if (item.dotPos == productions[item.prodIndex].rhs.size() ) 
        {
            if (productions[item.prodIndex].lhs == productions[0].lhs) // accepted ==== Check
            {
                if (TERMINAL == item.lookahead.grammarSymCat) // ======optimize: only getTerminalSym and eof (viewed as getTerminalSym) is vaild col in ActionTable
                    actionTable[stateIndex][item.lookahead] = "acc";
            } 
            else // reduce
            {
                if (TERMINAL == item.lookahead.grammarSymCat)
                    actionTable[stateIndex][item.lookahead] = "r" + std::to_string(item.prodIndex);
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
    /*
    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) 
    {
        actionTable[stateIndex] = std::map<GrammarSym, std::string>();
        gotoTable[stateIndex] = std::map<GrammarSym, int>();
    }
    */

    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) 
    {
        fillActionTable(stateIndex, states[stateIndex]);

        fillGotoTable(stateIndex);
    }
}

void shiftOpForAST(std::stack<ASTNodePtr>& astRootsPtrStack, const Token2Cat& nextToken2Cat)
{
    ASTNodePtr subRoot; // Note: std::unique_ptr default value is empty, == nullptr
    if (IDENTIFIER == nextToken2Cat.second) 
    {
        subRoot = std::make_unique<IdentNode>(nextToken2Cat.first);
        std::cout << "===========IDENTIFIER: root addr: " << subRoot.get() << std::endl;
    }
    else if (INTEGER == nextToken2Cat.second) 
    {
        subRoot = std::make_unique<IntegerNode>(std::stoi(nextToken2Cat.first) );
        std::cout << "==========INTEGER: root addr: " << subRoot.get() << std::endl;
    }
    /*
    else if (MUL == nextToken2Cat.second ||
            ADD == nextToken2Cat.second ||
            LPARENTHESES == nextToken2Cat.second ||
            RPARENTHESES == nextToken2Cat.second ||
            SEMICOLON == nextToken2Cat.second
        ) // operator '(' ')' ';' 不需要创建AST节点，push nullptr to astRootsPtrStack()
    {
        // doNothing
    }
    */
    std::cout << "shiftOpForAST: root addr: " << subRoot.get() << std::endl;
    if (subRoot)
    {
        astRootsPtrStack.push(std::move(subRoot) );
    }
}

void getValidChildren(std::stack<ASTNodePtr>& astRootsPtrStack, int prodIndex, std::vector<ASTNodePtr>& children)
{
    switch (static_cast<PRODUCTIONINDEX>(prodIndex) )
    {
        // case UnActualActionForCodeGen:
        case PRODUCTIONINDEX::TERM_REDUCED2_EXPR:
        case PRODUCTIONINDEX::FACTOR_REDUCED2_TERM:
        case PRODUCTIONINDEX::PARENTHESIS_REDUCED2_FACTOR:
        case PRODUCTIONINDEX::ASSIGN_REDUCED2_STATEMENT: 
        // Note: for the previous, actually don't need to create anyNode
        // case the node has already created when shift
        // Note: the following, node already created when shift. diff from SDT(语法制导翻译)
        case PRODUCTIONINDEX::INTERGE_REDUCED2_FACTOR:                   
        case PRODUCTIONINDEX::IDENT_REDUCED2_FACTOR:
        {
            std::cout << "======prodIndex: " << prodIndex << " astRootsPtrStack Top NodeCat: " << 
                static_cast<int>(astRootsPtrStack.top()->category) << std::endl;
            children.insert(children.begin(), std::move(astRootsPtrStack.top() ) );
            astRootsPtrStack.pop();
            break;
        }
        case PRODUCTIONINDEX::MUL:
        case PRODUCTIONINDEX::ADD:
        case PRODUCTIONINDEX::ASSIGN:
        {
            for (int childrenIndex = 0; childrenIndex < 2; childrenIndex++) // Note: 2 个有效子节点
            {
                std::cout << "========prodIndex: " << prodIndex << " astRootsPtrStack Top NodeCat: " << 
                    static_cast<int>(astRootsPtrStack.top()->category) << std::endl;
                children.insert(children.begin(), std::move(astRootsPtrStack.top() ) );
                astRootsPtrStack.pop();
            }
            break;
        }
        case PRODUCTIONINDEX::STATEMENT_REDUCED2_PROG: 
        {
            std::cout << "========prodIndex: " << prodIndex << " astRootsPtrStack Top NodeCat: " << 
                static_cast<int>(astRootsPtrStack.top()->category) << std::endl;
            children.insert(children.begin(), std::move(astRootsPtrStack.top() ) );
            astRootsPtrStack.pop();
            break;
        }
    }
}

ASTNodePtr CreateBinaryOpNode(std::vector<ASTNodePtr>& children, const std::string& op)
{
    ASTNodePtr parent = std::make_unique<BinaryOpNode>(std::move(children[0]), std::move(children[1]), op);
    return parent; // optimize: tell compiler to do NRVO, compiler may do NRVO.
    // return std::make_unique<BinaryOpNode>(std::move(children[0]), std::move(children[1]), op);
}

ASTNodePtr CreateAssignmentNode(std::vector<ASTNodePtr>& children)
{
    ASTNodePtr parent = std::make_unique<AssignOpNode>(std::move(children[0]), std::move(children[1]) );
    return parent;
}

/*
    设计方案：
        在 有效 shift((not corresponds to operator/'('/')'/';' token)) 时，创建 ASTLeafNode;
        在 reduce is from Integer/Ident to Factor 时，
            1] 先 extract 出 stack.Top(), added to children node, then Pop.
            2] use these children and the production index to get/create parent node
            3] Push the parent node to the stack, which serves as the new root


    SDT(语法制导翻译) 不同：
        SDT 在 shift 时，不创建 ASTNode;
            在 reduce from Integer/Ident to Factor 时，创建 ASTLeafNode。

    Note:
        编译器可选优化（GCC/Clang/MSVC 通常会做）
            RVO/NRVO: 消除返回对象时(2次)不必要的 拷贝/移动，直接在 caller 的内存位置 构造 对象，提升性能。
                RVO (Return Value Optimization)
                    返回临时对象
                        eg. return T();
                NRVO (Named ...)
                    返回具名局部变量
                        eg. 
                            T x; 
                            return x; 

                eg.
                    1] 若 compiler 无优化（两次拷贝 / 移动）
                        T f() 
                        {
                            T x;        // [1] 构造 x（函数内）
                            return x;   // [2] 拷贝/移动 到 临时返回值
                        }

                        T y = f();      // [3] 拷贝/移动临时到 y
                    
                    2] 若 compiler 进行了 NRVO 优化（零拷贝/移动，而是用一次 直接构造）
                        T f() 
                        {
                            T x;      // [1] 构造 x（函数内）
                            return x; // [2] 无拷贝/移动
                        }

                        T y = f();   // [3] 无 copy/move，而是 直接在 y 的位置（用 f() 中的 local x）构造 y。

            NRVO 可能生效条件（缺一不可）
                1] return the same type of (no-volatile) local var 
                2] return the same named local variable on all return paths
*/
ASTNodePtr getParentNode(int prodIndex, std::vector<ASTNodePtr>& children)
{
    ASTNodePtr parent;

    switch (static_cast<PRODUCTIONINDEX>(prodIndex) )
    {
        // case UnActualActionForCodeGen:
        case PRODUCTIONINDEX::TERM_REDUCED2_EXPR:
        case PRODUCTIONINDEX::FACTOR_REDUCED2_TERM:
        case PRODUCTIONINDEX::PARENTHESIS_REDUCED2_FACTOR:
        case PRODUCTIONINDEX::ASSIGN_REDUCED2_STATEMENT: 
        // Note: for the previous, actually don't need to create anyNode
        // case the node has already created when shift
        // Note: the following, node already created when shift. diff from SDT(语法制导翻译)
        case PRODUCTIONINDEX::INTERGE_REDUCED2_FACTOR:                   
        case PRODUCTIONINDEX::IDENT_REDUCED2_FACTOR:
            parent = std::move(children[0]);
            break;    
            //return std::move(children[0]); // the unique subroot is automatically upgraded to(自动升级为) the root of growing AST.  
        case PRODUCTIONINDEX::MUL:
            parent = CreateBinaryOpNode(children, "*"); 
            break;
            //return CreateBinaryOpNode(children, "*");    
        case PRODUCTIONINDEX::ADD:
            parent = CreateBinaryOpNode(children, "+");
            break;
            //return CreateBinaryOpNode(children, "+");
        case PRODUCTIONINDEX::ASSIGN:
            parent = CreateAssignmentNode(children); //return CreateBinaryOpNode(children, "=");  
            break;

        case PRODUCTIONINDEX::STATEMENT_REDUCED2_PROG: 
        {
            auto prog = std::make_unique<ProgramNode>();
            prog->stmts.push_back(std::move(children[0]) );
            parent = std::move(prog); // Note: std::unique_ptr<Derived> will be implicitly converted to std::unique_ptr<Base>. Requirment: Base must has virtial Dtor.
            break;
            //auto prog = std::make_unique<ProgramNode>();
            //prog->stmts.push_back(std::move(children[0]) );
            //return std::move(prog); // Note: Local obj which supports move semantics, such as unique_ptr, can be returned directly, compiler will automatically move. However, Explicitly use std::move may suppress NRVO optimization 
        }
        default:
            //return std::move(children[0]);
            parent = std::move(children[0]);
            break;
    }
    return parent; // optimize: tell compiler to do NRVO, compiler may do NRVO.
}

void reduceOpForAST(std::stack<ASTNodePtr>& astRootsPtrStack, int prodIndex)
{
    std::vector<ASTNodePtr> children;
    getValidChildren(astRootsPtrStack, prodIndex, children);
    std::cout << "reduceOpForAST: children: \n";
    for (const auto& child: children)
    {
        std::cout << "==========prodIndex: " << prodIndex << " astRootsPtrStack Top NodeCat: " << 
            static_cast<int>(child->category) << std::endl;
    }
    
    ASTNodePtr parent = getParentNode(prodIndex, children);
    astRootsPtrStack.push(std::move(parent) );
}

/*
Pseudo Code
    stack.push(s0)
    state = s0
    inputPtr = -1;
    strLen = string.size();

    While ( inputPtr <= strLen)
        chiose = ActionTable[state][lookahead = input[inputPtr+1])] // lookahead: nextSymInInput
        
        if chiose.shift == true:
            inputPtr++ // shift: consum the nextSymInInput
            state = choice.transDstState // move to the dst transition state 
            stack.push(lookahead) // nextSymInInput(getTerminalSym) pushed to stack
            stack.push(state) 
        if chiose.reduction== true:
            // 将该规则右边所有 文法符号（终结符、非终结符）全部 pop
            for (symInex = 0; symInex < reductionProductionRightSymNum; symInex ++)
                pop stackTop: the latest transition dst state
                pop stackTop: the latest transition Sym: getTerminalSym or non-getTerminalSym
            
            // Goto 基于 pop 后的 栈顶 & reduction non-getTerminalSym
            reductionNonTerminal = chiose.reduction.left  //  production left 
            state = Goto[stack.top(): the latest transition src state][reductionNonTerminal] // move to the dst transition state 
            stack.push(reductionNonTerminal) // reductionNonTerminal pushed to stack
            stack.push(state)
        if chiose.Accept== true:
            // 将该规则右边所有 文法符号（终结符、非终结符）全部 pop
            for (symInex = 0; symInex < reductionProductionRightSymNum; symInex ++)
                pop stackTop: the latest transition dst state
                pop stackTop: the latest transition Sym: getTerminalSym or non-getTerminalSym
            
            if lookahead = input[inputPtr+1])  == eof && stack.top() == s0
                inputPtr++;
                return success
        else 
            return Error

*/

/*
implement:

    (1) create two stacks:
        stateIndexStack: stores state (indices) in reverse order along the parsing path in the state transition graph
        grammarSymStack: stores grammar symbols in reverse order along the parsing path in the state transition graph

    (2) for every valid nextSymInInput in input: 

        base on [curStateIndex][nextSymInInput] to lookup ActionTable to get action,
        
        if action == shift:
            1] inputPtr++ to consum the nextSymInInput
            2] update curStateIndex = std::stoi(action.substr(1) ) 为 要转移到的 状态（索引），
                即 move to the transDst state (index) 。
            3] nextSymInInput(getTerminalSym) / curStateIndex Pushed to grammarSymStack / stateIndexStack
        else if action == reduce:
            1] Pop all states (indices) and grammar symbols corresponding to the right-hand side of the production, 
            2] find the transDst state (index) by looking up the GotoTable based on the current stack-top state and non-getTerminalSym being reduced to,
               move to the transDst state (index) 
            3] reductionNonTerminal / curStateIndex Pushed to grammarSymStack / stateIndexStack

    (3) astRootsPtrStack: 
        [1] After a valid(not corresponds to operator/'('/')'/';' token) shift, push the current subRoot of the growing AST to the stack.
            Concretely, after a valid shift, make the corresponding ASTLeafNode, push it to the stack.
            The ASTLeafNode is relative to the parent ASTNode that will be created by later reduction that contributes to actual action(实际行为) for CodeGen.

            => After a shift, the stack stores multiple subRoots of the current AST in reverse order(left/right child is in the bottom/top).

        [2] After a reduce, the stack stores only the current root of the growing AST.

            if reduced by the rule that contribute to actual action(operator token) for CodeGen:
                1] Iterate as many times as the number of operands in the rule.
                   In each iteration,
                        extract the Top of the stack, 
                        add this subNode/subRoot to the children in reverse order, 
                        then Pop. 
                
                2] Create corresponding parent node that corresponds to actual action (operator token of the rule).
                   which serves as the new root.

                3] Push the parent(new root) to the statk.

            else if ... doesn't contribute to actual action for CodeGen = only Grammar substitution:
                don't Create node 
                => the unique(唯一的) subRoot in the stack is automatically upgraded to(自动升级为) the root of growing AST.

        [3] When Accepted, extract the Top of the stack, that is the final root of the final AST.

        === detail:
        [1] when shift
            
            if (token.Cataegory == IDENTIFIER || INTEGER)
                1] correspondingASTLeafNode = CreateCorrespondingASTLeafNode(token.value)
                                           CreateIdentASTLeafNode
                                           CreateIntegerASTLeafNode
                2] correspondingASTNode pushed to astRootsPtrStack 
            else if (tokenCat == operator || '(' || ')' || ';' )
                do nothing = ignore operator/'('/')'/';'

            => result:
                if AST is a binaryTree, 
                a] first valid(not corresponds to operator/'('/')'/';' token) shift: only one subRoot is in the stack.
                b] later valid shift = shift after a reduce: 
                   precious reduce contribute a leftSubRoot to be the bottom of the stack,
                   current shift contribute a rightSubRoot to be the top of the stack.

        [2] when reduced by rule n(ruleNum):

            ASTNodePtr getParentNode(ruleNum, children)
            {
                switch (ruleNum)
                {
                    case MULRule:
                        return CreateMulNode(children)     
                    case ADDRule:
                        return CreateMulNode(children) 
                    case ASSIGN:
                        return CreateMulNode(children)

                    //case SingleTerminalReduceNonterminal: // node already created when shift. diff from SDT(语法制导翻译) 
                    //    doNothing 

                    case UnActualActionForCodeGen:
                        return children[0] // the unique subroot is automatically upgraded to(自动升级为) the root of growing AST.
                    
                    case StatementsReducedToProRule: 
                    {
                        auto prog = std::make_unique<ProgramAST>();
                        prog->stmts.push_back(std::move(children[0]));
                        return prog;
                    }

                }
            }

    (4) ParserTreeNodePtrStack: 
        action is shift: 
            Create a Terminal Node using nextSymInInput,
            Push it to ParserTreeNodePtrStack
        action is reduce: build subTree by add  every ParserTreeNodePtr to related Grammar Symbol in right-side to left Grammar Symbol's ParserTreeNodePtr children.
            Create a NonTerminal Node using the left Grammar Symbol of reduction/production, 
            Pop reduction.right.size() ASTNodePtrs as NonTerminal Node's children, 
            Reverse NonTerminal Node's children to ensure children is the same sequence as right of the reduction.      
        
        
        parserTreeNodeStack: 
            shift: [a (top)] -> Reduce: [empty -> A (children: a) (top)] -> shift: [A (children: a), a (top)] 
                -> Reduce: [empty -> A (children: A (children: a), a) ]  -> Accepted: [empty -> G( children: A (children: A (children: a), a) ) ]
        <=> Parse Tree:

            -> shift:
                a
            
            -> Reduce:
                A         
                |
                a  
        
            -> shift:
                 A   a
                |
                a

            -> Reduce:
                 A
                / \
                A   a
                |
                a

            -> Accepted:
                 A
                / \
                A   a
                |
                a
*/
/*
global var:
    actionTable
    productions
    gotoTable
*/

#ifdef NEEDPARSERTREE 
ParserTreeNodePtr 
#else
ASTNodePtr
#endif
parser(const Token2CatStream& token2CatStream)
{
    std::stack<int> stateIndexStack;
    stateIndexStack.push(0);
    
    std::stack<GrammarSym> grammarSymStack;

    std::stack<ASTNodePtr> astRootsPtrStack; 

#ifdef NEEDPARSERTREE 
    std::stack<ParserTreeNodePtr> parserTreeNodeStack;
#endif

    Token2CatStream input = token2CatStream;
    input.emplace_back("$", static_cast<TokenCategory>(eof.termTokenCat_nonTermId) ); // Note: add a sentinel token2Cat to token2CatStream

    int inputPtr = -1;
    int curStateIndex = -1;
    int transDstStateIndex = -1;
    while (true)
    {
        int curStateIndex = stateIndexStack.top();

        std::cout << "inputPtr: " << inputPtr << ", curStateIndex: " << curStateIndex << std::endl;
        const auto& nextToken2Cat = input[inputPtr + 1];
        GrammarSym nextTermSym = getTerminalSym(nextToken2Cat.second); // Note: the last is just the eof
        std::cout << "nextToken2Cat: " << nextToken2Cat.first << ", " << nextToken2Cat.second << std::endl;

        if (actionTable[curStateIndex].count(nextTermSym) )  // action not empty
        {
            std::string action = actionTable[curStateIndex][nextTermSym];
            std::cout << "curStateIndex: " << curStateIndex << ", nextSymInInput: " << nextToken2Cat.second << ", " << nextToken2Cat.second << 
                ", action: " << action << std::endl;

            if ('s' == action[0]) // Note 
            {
                std::cout << "shift by "<< action << std::endl;

                inputPtr++; // shift: truly consum the nextSymInInput that just be read // Note: 第1步必定为 shift

                grammarSymStack.push(nextTermSym); // nextSymInInput(getTerminalSym) pushed to stack
                transDstStateIndex = std::stoi(action.substr(1) ); // move to the dst transition state 
                stateIndexStack.push(transDstStateIndex); 
                
                shiftOpForAST(astRootsPtrStack, nextToken2Cat);

#ifdef NEEDPARSERTREE 
                ParserTreeNodePtr termNode = std::make_shared<ParserTreeNode>(nextTermSym, nextToken2Cat.first);
                parserTreeNodeStack.push(termNode); 
#endif 

            }
            else if ('r' == action[0])    
            {
                std::cout << "Reduce by "<< action << std::endl; 

                // 1]
                int prodIndex = std::stoi(action.substr(1) );

                reduceOpForAST(astRootsPtrStack, prodIndex);
                      
#ifdef NEEDPARSERTREE 
                ParserTreeNodePtr nonTermNode = std::make_shared<ParserTreeNode>(productions[prodIndex].lhs, "");
#endif
                // 2] 
                for (int symInex = 0; symInex < productions[prodIndex].rhs.size(); symInex ++)
                {
                    stateIndexStack.pop();
                    grammarSymStack.pop();

#ifdef NEEDPARSERTREE 
                    nonTermNode->addChild(parserTreeNodeStack.top() ); 
                    parserTreeNodeStack.pop();
#endif
                }

#ifdef NEEDPARSERTREE 
                // reverse to ensure childs is the same sequence as right of the reduction/production
                std::reverse(nonTermNode->children.begin(), nonTermNode->children.end() );
                parserTreeNodeStack.push(nonTermNode);
#endif 
                
                // 3]
                GrammarSym reductionNonTerminal = productions[prodIndex].lhs;
                if (gotoTable.count(stateIndexStack.top() ) &&
                    gotoTable[stateIndexStack.top()].count(reductionNonTerminal) )
                {
                    grammarSymStack.push(reductionNonTerminal);
                    int transDstStateIndex = gotoTable[stateIndexStack.top()][reductionNonTerminal];                        
                    stateIndexStack.push(transDstStateIndex); // move to the dst transition state 
                }
                
            }
            else if("acc" == action)
            {
                inputPtr++; // truly consum the nextSymInInput which is the eof that just be read
                if (inputPtr + 1 == input.size() )
                {
                    std::cout << "\n Parsing success, AST generated success !!!\n" << std::endl;

#ifdef NEEDPARSERTREE
                    return parserTreeNodeStack.top();
#else 
                    return std::move(astRootsPtrStack.top() ); // Note: stack.pop 返回的 栈顶元素的引用（即使是临时变量）=> 是左值
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

void printItem(const LR1Item& item) 
{
    std::cout << nonTerminalId2Str[productions[item.prodIndex].lhs.termTokenCat_nonTermId].second << "->";

    for (int symIndex = 0; symIndex < productions[item.prodIndex].rhs.size(); ++symIndex) 
    {
        if (symIndex == item.dotPos) 
            std::cout << ".";

        if (NON_TERMINAL == productions[item.prodIndex].rhs[symIndex].grammarSymCat)
            std::cout << nonTerminalId2Str[productions[item.prodIndex].rhs[symIndex].termTokenCat_nonTermId].second;
        else 
            std::cout << terminalId2Str[productions[item.prodIndex].rhs[symIndex].termTokenCat_nonTermId].second; 
    }
    if (item.dotPos == productions[item.prodIndex].rhs.size() ) 
        std::cout << ".";
    
    std::cout << ", {";
    if (TERMINAL == item.lookahead.grammarSymCat)
    {
        std::cout << terminalId2Str[item.lookahead.termTokenCat_nonTermId].second;
    }
    else  
        std::cout << "noNeedCalcLookahead when dotNextFollowSym is NonTerminal!" << " ";

    std::cout << "}";
}
/*
global var:
    states
    stateTransGraph
    actionTable
*/
void printAll(const std::set<GrammarSym>& terms, const std::set<GrammarSym>& nonTerms) 
{
    std::cout << "\n===== LR(1) all States: =====" << std::endl;
    std::cout << "states.size:" << states.size() << "\n";
    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) 
    {
        std::cout << "\nState I" << stateIndex << ":" << std::endl;
        for (const auto& item : states[stateIndex]) 
        {
            std::cout << "  ";
            printItem(item);
            std::cout << std::endl;
        }
    }

    std::cout << "\n===== State Transition Graph: =====" << std::endl;
    for (const auto& link : stateTransGraph) 
    {
        int srcStateIndex = link.first.first;
        GrammarSym grammarSym = link.first.second;
        int dstStateIndex = link.second;
        std::cout << "I" << srcStateIndex << " --" << 
            (NON_TERMINAL == grammarSym.grammarSymCat ? 
             nonTerminalId2Str[grammarSym.termTokenCat_nonTermId].second :
             terminalId2Str[grammarSym.termTokenCat_nonTermId].second )  << 
            "--> I" << dstStateIndex << std::endl;
    }

    std::cout << "\n===== ActionTable =====" << std::endl;
    std::cout << std::left << std::setw(6) << "State: ";
    for (GrammarSym term : terms) 
        std::cout << std::setw(8) << terminalId2Str[term.termTokenCat_nonTermId].second;
    std::cout << std::endl;

    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) 
    {
        std::cout << std::setw(6) << "I" + std::to_string(stateIndex);
        
        for (GrammarSym term : terms) 
        {
            if (actionTable[stateIndex].count(term) )
                std::cout << std::setw(8) << actionTable[stateIndex][term];
            else
                std::cout << std::setw(8) << "";
        }
        std::cout << std::endl;
    }

    std::cout << "\n===== GotoTable:  =====" << std::endl;
    std::cout << std::left << std::setw(6) << "State: ";
    for (GrammarSym nonTerm : nonTerms) 
        std::cout << std::setw(8) << nonTerminalId2Str[nonTerm.termTokenCat_nonTermId].second;
    std::cout << std::endl;

    for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) 
    {
        std::cout << std::setw(6) << "I" + std::to_string(stateIndex);
        for (GrammarSym nonTerm : nonTerms) 
        {
            if (gotoTable[stateIndex].count(nonTerm) )
                std::cout << std::setw(8) << gotoTable[stateIndex][nonTerm];
            else
                std::cout << std::setw(8) << "";
        }
        std::cout << std::endl;
    }
}

// 先处理根 + 递归处理每个孩子 => BFS 遍历打印 AST: 
void printAST(const ASTNodePtr& root, int indent, std::string rootLeftRight) // const ASTNodePtr&: 目的是 只读 root
{
    if (!root) 
        return;

    for (int i = 0; i < indent; ++i) 
        std::cout << "  ";

    if (ASTNodeCategory::PROG == root->category)
    {
        for (const auto& stmt : static_cast<ProgramNode*>(root.get() )->stmts) 
        {
            printAST(stmt, indent + 1);
        }
    }
    else if (ASTNodeCategory::BINOP == root->category)
    {
        std::cout << rootLeftRight << static_cast<BinaryOpNode*>(root.get() )->op << std::endl;
        printAST(static_cast<BinaryOpNode*>(root.get() )->left, indent + 1, "lhs:");
        printAST(static_cast<BinaryOpNode*>(root.get() )->right, indent + 1, "rhs:");
    }
    else if (ASTNodeCategory::ASSIGNMENT == root->category)
    {
        std::cout << rootLeftRight << "=" << std::endl;
        printAST(static_cast<AssignOpNode*>(root.get() )->left, indent + 1, "lhs:");
        printAST(static_cast<AssignOpNode*>(root.get() )->right, indent + 1, "rhs:");
    }
    else if (ASTNodeCategory::INTEGER == root->category)
    {
        std::cout << rootLeftRight << static_cast<IntegerNode*>(root.get() )->value << std::endl;
    }
    else if (ASTNodeCategory::IDENT == root->category)
    {
        std::cout << rootLeftRight << static_cast<IdentNode*>(root.get() )->value << std::endl;
    }
}

#ifdef NEEDPARSERTREE
// 先处理根 + 递归处理每个孩子 => BFS 遍历打印 ParseTree: 
void printParseTree(const ParserTreeNodePtr& root, int indent) 
{
    if (!root) 
        return;

    for (int i = 0; i < indent; ++i) 
        std::cout << "  ";
    
    std::cout << (NON_TERMINAL == root->grammarSym.grammarSymCat ? "[NonTerminal] " : "[Terminal   ] ") <<
            // root->grammarSym.termTokenCat_nonTermId << ", " << 
                (NON_TERMINAL == root->grammarSym.grammarSymCat ? 
                    nonTerminalId2Str[root->grammarSym.termTokenCat_nonTermId].second : 
                    root-> tokenStrValue) 
              << std::endl;
    
    for (const auto& child : root->children) 
    {
        printParseTree(child, indent + 1);
    }
}

#endif