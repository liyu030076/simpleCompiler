#include <iostream>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <queue>
#include <stack>

// ===================== 1. NFA 结构定义 =====================

constexpr int invalidPrecedence = 999;

enum TokenCategory
{
    IF = 1,
    IDENTIFIER,
    IVALID_TOKEN_CATEGORY
};

// NFA 状态
struct NFAState 
{
    int id;                       // 状态编号
    bool isAccepted;              // 是否为 accepted stated
    int acceptedPrecedence = invalidPrecedence; //  invalid; valid only when is accepted state. prior if acceptedPrecedence is smaller.
    bool isStart = false;

    TokenCategory tokenCategory = IVALID_TOKEN_CATEGORY;

    std::map<char, std::vector<NFAState*> > transitions;  // 从当前状态出去的 转移表: 字符 → 转移到的状态列表

    NFAState(int id) : id(id), isAccepted(false) {}
};

// NFA 片段（起始状态 + 终态）
struct NFA 
{
    NFAState* startState;
    NFAState* endState;

    NFA(NFAState* s, NFAState* e) : startState(s), endState(e) {}
};

// 全局状态编号计数器
int state_id = 0;

// 创建新状态
NFAState* new_state() 
{
    return new NFAState(state_id++);
}

typedef struct 
{
    std::string pattern;
    std::string category;
} Rule;

std::vector<Rule> parse_re_file(const char* filename) 
{
    std::vector<Rule> rules;  // 存储所有解析出来的规则
    std::ifstream fs(filename);  // 使用传入的文件名！
    if (!fs.is_open()) {
        std::cerr << "error: can't open file: " << filename << '\n';
        return rules;
    }

    std::string line;
    while (std::getline(fs, line) ) 
    {  

        if (line.empty() ) 
            continue;
        if (line[0] == '#' ) 
        {
            std::cout << "ignore this line\n";
            continue;
        }

        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos) 
        {
            std::cerr << "warning: invalid line(no blank) " << line << '\n';
            continue;
        }

        Rule rule;
        rule.pattern = line.substr(0, spacePos);
        rule.category = line.substr(spacePos + 1);

        std::cout << "RE: " << rule.pattern << std::endl;
        std::cout << "tokenCategory: " << rule.category << std::endl;

        rules.push_back(rule);
    }

    return rules;
}


// ===================== 2. RE → 后缀表达式 =====================
// 获取运算符优先级：* > 连接(.) > |
int reOpPrecedence(char op) // 1. reOpPrecedence 的设计并不合理，因为操作符不支持时，只是报错提示，用户可能很难察觉，待改进。
{
    if (op == '*') 
        return 3;
    if (op == '.') 
        return 2;
    if (op == '|') 
        return 1;

    std::cout << "Error: Unsupported RE operator" << op << std::endl;
    return 0;
}

std::string add_explicit_concat(const std::string& regex) 
{
    std::string res;
    int n = regex.size();
    for (int i = 0; i < n; i++) 
    {
        res += regex[i];

        if (i < n-1) 
        {
            char c1 = regex[i];
            char c2 = regex[i+1];
            // 需要加连接符的情况：字符/右括号/* 后面跟 字符/左括号
            if ( (std::isalnum(c1) || c1 == ')' || c1 == '*') && 
                 (std::isalnum(c2) || c2 == '(') ) 
            {
                res += '.';
            }
        }
    }
    return res;
}

/*
RE → 后缀表达式
    数据结构:
        输出队列：存储最终后缀表达式(只含 操作数 和 运算符)
        运算符栈：只含 运算符 和 括号
    
    运算符优先级 从高到低：
        括号 ( )
        闭包 *（单目运算符，右结合）
        连接 ·（正则里省略，必须手动补上）
        或 |

        且 当前 language 的操作符是 左结合

    遍历正则的每个符号（字符 / 运算符）：
        [1] if 操作数
            直接加入 输出队列。
        
        [2] if 左括号 (
            入栈
        [3] if 右括号 )
            循环 出栈 到 输出队列 ，直到左括号 (; 左括号 出栈但不加入输出队列（丢弃括号）.

        [4] if 运算符 (| . * + ?)
            循环：栈不为空 & 栈顶不是左括号 & (栈顶优先级> 当前运算符 或 (优先级相等且是左结合) )
                → 出栈 到 输出队列

            最后，入栈 当前运算符。

        [5] 遍历结束
            栈中剩余所有运算符 （依次）出栈 到 输出队列

    a · ( b | c ) * -> 后缀: a b c | * ·

*/
std::string regex_to_postfix(const std::string& regex) {
    std::stack<char> op_stack;
    std::string postfix;       // output queue
    std::string processed = add_explicit_concat(regex);  // 先加连接符

    for (char c : processed) 
    {
        if (std::isalnum(c) )  // 操作数：直接输出
        {  
            postfix += c;
        } 
        else if (c == '(') 
        {  // 左括号：入栈
            op_stack.push(c);
        } 
        else if (c == ')' ) 
        {  // 右括号：循环出栈 到 输出，直到左括号；左括号 出栈但不加入输出队列（丢弃括号）.
            while (!op_stack.empty() && 
                    op_stack.top() != '(' ) 
            {
                postfix += op_stack.top();
                op_stack.pop();
            }
            if (op_stack.top() == '(' )
            {
                op_stack.pop(); 
            }
        } 
        else // 运算符：* > . > |
        {  
            while (!op_stack.empty() && 
                    op_stack.top() != '(' &&
                    reOpPrecedence(op_stack.top() ) >= reOpPrecedence(c) ) 
            {
                postfix += op_stack.top();
                op_stack.pop();
            }
            op_stack.push(c);
        }
    }

    // 栈中剩余所有运算符 （依次）出栈 到 输出队列
    while (!op_stack.empty() ) 
    {
        postfix += op_stack.top();
        op_stack.pop();
    }
    return postfix;
}

// ===================== 3. 后缀表达式 → NFA: Thompson Construction 算法 =====================
// 基础 NFA：单字符
NFA build_char_nfa(char c) {
    NFAState* startState = new_state();
    NFAState* endState = new_state();
    startState->transitions[c].push_back(endState);
    endState->isAccepted = true;
    return NFA(startState, endState);
}

// 连接运算：N1 . N2
NFA build_concat_nfa(NFA& n1, NFA& n2) 
{
    n1.endState->isAccepted = false;
    n1.endState->transitions['\0'].push_back(n2.startState);  // '\0': represent ε 空转移
    return NFA(n1.startState, n2.endState);
}

// 或运算：N1 | N2
NFA build_or_nfa(NFA& n1, NFA& n2) {
    NFAState* startState = new_state();
    NFAState* endState = new_state();
    startState->transitions['\0'].push_back(n1.startState);
    startState->transitions['\0'].push_back(n2.startState);
    n1.endState->transitions['\0'].push_back(endState);
    n2.endState->transitions['\0'].push_back(endState);
    n1.endState->isAccepted = false;
    n2.endState->isAccepted = false;
    endState->isAccepted = true;
    return NFA(startState, endState);
}

// star 运算：N*
NFA build_star_nfa(NFA& n) {
    NFAState* startState = new_state();
    NFAState* endState = new_state();
    startState->transitions['\0'].push_back(n.startState);
    startState->transitions['\0'].push_back(endState);
    n.endState->transitions['\0'].push_back(n.startState);
    n.endState->transitions['\0'].push_back(endState);
    n.endState->isAccepted = false;
    endState->isAccepted = true;
    return NFA(startState, endState);
}

/*
thompsonConstruction 算法核心：栈实现
    遍历后缀表达式：
        遇到字符：压入对应基础 NFA
        遇到运算符： 弹出 n 个（运算符需要的操作数数量）元素，
                    按运算符构造新 NFA，
                    再压回栈。
    遍历结束，栈中 唯一 NFA 就是最终结果
*/
NFA thompsonConstruction(const std::string& postfix) 
{
    std::stack<NFA> nfa_stack;

    for (char c : postfix) 
    {
        if (std::isalnum(c) ) // 字符：压入 基础 NFA
        {  
            std::cout << c << std::endl;
            nfa_stack.push(build_char_nfa(c) );
        } 
        else // 运算符
        {  
            if (c == '*') // 单目运算符：弹出1个 NFA
            {  
                NFA nfa = nfa_stack.top();
                nfa_stack.pop();
                nfa_stack.push(build_star_nfa(nfa) );
            } 
            else if (c == '.' || c == '|') // 双目运算符：. | 弹出2个 NFA
            {  
                NFA nfa2 = nfa_stack.top(); 
                nfa_stack.pop();
                NFA nfa1 = nfa_stack.top(); 
                nfa_stack.pop();
                
                if (c == '.') 
                {
                    nfa_stack.push(build_concat_nfa(nfa1, nfa2) );
                } 
                else if (c == '|') 
                {
                    nfa_stack.push(build_or_nfa(nfa1, nfa2) );
                }
            }
            else
            {
                std::cout << "Error: Unsupported RE operator" << c << std::endl;
            }
        }
    }

    // 栈中唯一 NFA 就是结果
    return nfa_stack.top();
}

/*
NFA: test ok
               a-Z | 0-9
                 _ _
                |   \|     epsilon
                 - n3 - - - - - - 
                 /\             |
               / a-Z            |/
        --> n0                  n4
               \ i              /\
                \/     f       /  epsilon
                   n1  --> n2


*/  
NFA build_if_id_nfa()
{
    NFAState* state0 = new_state();
    NFAState* state1 = new_state();
    NFAState* state2 = new_state();
    NFAState* state3 = new_state();
    NFAState* state4 = new_state();

    state0->transitions['i'].push_back(state1);
    state1->transitions['f'].push_back(state2);
    for (char ch = 'A'; ch <= 'Z'; ch++) 
    {
        state0->transitions[ch].push_back(state3);
        state3->transitions[ch].push_back(state3);
    }
    for (char ch = 'a'; ch <= 'z'; ch++) 
    {
        state0->transitions[ch].push_back(state3);
        state3->transitions[ch].push_back(state3);
    }
    for (char ch = '0'; ch <= '9'; ch++) 
    {
        state3->transitions[ch].push_back(state3);
    }

    state2->transitions['\0'].push_back(state4);
    state3->transitions['\0'].push_back(state4);

    state0->isStart = true;
    state4->isAccepted = true;

    state2->acceptedPrecedence = 1;
    state3->acceptedPrecedence = 2;

    state2->tokenCategory = IF;
    state3->tokenCategory = IDENTIFIER;

    return NFA(state0, state4);
}

// ===================== 4. 测试工具：打印NFA转移关系 =====================
/*
test: 
    输入正则：b|c
    后缀表达式：bc|

    最终 NFA 结构：
        状态 4: --ε--> 状态 0
        状态 0: --b--> 状态 1
        状态 1: --ε--> 状态 5
        状态 4: --ε--> 状态 2
        状态 2: --c--> 状态 3
        状态 3: --ε--> 状态 5
*/
void print_nfa(NFAState* s, std::set<NFAState*>& visited) 
{
    if (visited.count(s) ) // 当前 NFAState 已访问过，则直接返回
        return;

    visited.insert(s);

    for (auto& pair : s->transitions) 
    {
        char c = pair.first;
        auto& states = pair.second;
        for (NFAState* next : states) 
        {
            std::cout << "状态 " << s->id << (s->isAccepted ? " (终态)" : "") << ": ";
            std::cout << "--" << (c == '\0' ? "ε" : std::string(1, c) ) << "--> 状态 " << next->id << "\n";
            print_nfa(next, visited);
        }
    }
}

// 求一个 NFA 状态集合 的 epsilonClosure
std::vector<NFAState*> epsilonClosure(std::vector<NFAState*> states) 
{
    // 结果集合: 一个 NFA 状态的 epsilonClosure 首先包含该状态本身
    std::vector<NFAState*> closure = states;

    // BFS 队列：遍历所有可通过 ε 到达的状态
    std::queue<NFAState*> que;

    // 初始化队列：把所有初始状态入队
    for (auto state : states) 
    {
        que.push(state);
    }

    while (!que.empty() ) // 对队列中每个状态
    {
        NFAState* curState = que.front();
        que.pop();

        auto it = curState->transitions.find('\0'); // 查找当前状态是否有 ε 转移
        if (it != curState->transitions.end() ) 
        {
            for (NFAState* next : it->second) // 遍历 经过1次 epsilon 转移能到达的所有 next 状态 => BFS 遍历
            {
                auto iter = std::find(closure.begin(), closure.end(), next);
                if ( iter == closure.end() ) // 若该状态还没加入 epsilonClosure
                {
                    closure.push_back(next);
                    que.push(next);       // 将 epsilon next 状态入队，待后续处理
                }
            }
        }
    }

    return closure;
}

typedef struct DFAState 
{
    int id;
    bool isAccepted = false;
    int acceptedPrecedence = invalidPrecedence; // DFA state precedence is the highest precedence NFA state precedence of all its NFA states 
    bool isStart = false;
    TokenCategory tokenCategory = IVALID_TOKEN_CATEGORY;

    std::vector<NFAState*> nFAStates;

    DFAState(int id_) : id(id_), isAccepted(false) {}
} DFAState;

// 全局状态编号计数器
int dfaStateId = 0;

// 创建新状态
DFAState* new_dfa_state() 
{
    return new DFAState(dfaStateId++);
}

static DFAState* d_phi = new_dfa_state();

DFAState* dFAStartState = NULL;

// 对 dfa state 中每个 nfa state 的 一步 ch 转移
std::vector<NFAState*> delta(std::vector<NFAState*> nFAStates, char ch)
{
    std::vector<NFAState*> nextNFAStates;

    for (const auto& nfaState: nFAStates)
    {
        auto iter = nfaState->transitions.find(ch);
        if (iter != nfaState->transitions.end() ) 
        {
            for (const auto& nfaState: iter->second) // Note: NFA 一个状态的 ch 转移可能有多个
            {
                nextNFAStates.push_back(nfaState);
            }
        }
    }
    return nextNFAStates;
}

std::pair<bool, DFAState*> 
isSubSet(std::vector<NFAState* > nFAStates, 
         std::vector<DFAState* > fullList)
{
    std::pair<bool, DFAState*> isInFullList2DFAState = {false, NULL};

    if(!nFAStates.empty() && !fullList.empty() )
    {
        for (const auto& dFAState : fullList) // 遍历 fullList 中的每一个 子集(元素)
        {
            bool isInFullList = true;  // 标记 nFAStates 中每一个状态 是否都在 当前 subset 中 
            
            for (NFAState* nFAState : nFAStates) // 检查 nFAStates 中每一个状态 是否都在 当前 subset 中
            {
                auto it = std::find(dFAState->nFAStates.begin(), dFAState->nFAStates.end(), nFAState);
                if (it == dFAState->nFAStates.end() ) // 有一个不在 => 不都在
                {  
                    isInFullList = false;
                    break;
                }
            }
            
            if (isInFullList) 
            {
                isInFullList2DFAState.first = true;
                isInFullList2DFAState.second = dFAState;
                return isInFullList2DFAState;
            }
        }
    }

    return isInFullList2DFAState;
}

std::pair<int, TokenCategory> getDFAStatePrecedence(DFAState* dFAState)
{
    std::pair<int, TokenCategory> minPrecedence2BoundTokenCategory = {invalidPrecedence, IVALID_TOKEN_CATEGORY};
    if (dFAState->isAccepted)
    {
        //std::cout << "dFAState: id: " << dFAState->id << std::endl;
        for (const auto& nFAState: dFAState->nFAStates)
        {
            if (nFAState->acceptedPrecedence < minPrecedence2BoundTokenCategory.first)
            {
                minPrecedence2BoundTokenCategory.first = nFAState->acceptedPrecedence;
                minPrecedence2BoundTokenCategory.second = nFAState->tokenCategory;
                //std::cout << "nFAState: id: " << nFAState->id << 
                //    " , acceptedPrecedence: " << nFAState->acceptedPrecedence << 
                //    " , tokenCategory: " << nFAState->tokenCategory << std::endl;
            } 
        }   
    }

    return minPrecedence2BoundTokenCategory;
}

/*
NFA -> DFA: Subset Construction algorithm

    d0= epsilon-closure(n0) 
    Add d0 to FullList and WorkList // 跟踪2个 lists
    
    // 每次从 worklist 中取一个 state，看其 transitions。
    while WorkList is not empty
        d = WorkList.ExtractFirst() // Worklist 变小
        for every symbol c in Sigma 
            // 发现一个（DFA）state，即 NFA states  
            t = epsilon-closure( Delta(d, c) ) // Delta: 转移函数
            // 将 t 放到一个 table，DFA 可以表示为2维 table
            T[d][c] = t
            
            //check t 是 新 state 还是 之前已经发现的 state：
            // 若 t is new，将 t 放到 FullList（因为 它是 新 state），同时放到 WorkList（因为 next/接着 就要 处理它）；
            // 若 t 之前已经被发现，do nothing（因为已经处理过了）。
            if t is not in FullList // a new state 
                Add t to FullList & WorkList // 这种情况下，一次 while 迭代后， Worklist 可能变大
            // else do nothing // 这种情况下，一次 while 迭代后， Worklist 变小。最后，WorkList 为空。
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

std::map<DFAState*, std::map<char, DFAState* > > 
nFA2DFA(NFA nfa)
{
    NFAState* n0 = nfa.startState;

    DFAState* d0 = new_dfa_state(); 
    d0->nFAStates.push_back(n0);
    d0->nFAStates = epsilonClosure(d0->nFAStates);
    if (n0->isAccepted)
    {
        d0->isAccepted = true;

        auto minPrecedence2BoundTokenCategory = getDFAStatePrecedence(d0);
        d0->acceptedPrecedence = minPrecedence2BoundTokenCategory.first;
        d0->tokenCategory = minPrecedence2BoundTokenCategory.second;
    }
    d0->isStart = true;
    dFAStartState = d0;

    std::vector<DFAState* > fullList;
    std::vector<DFAState* > workList;
    fullList.push_back(d0);
    workList.push_back(d0);

    /*
    std::vector<char> sigma;
    for (char ch = 'A'; ch <= 'Z'; ch++)
        sigma.push_back(ch);
    for (char ch = 'a'; ch <= 'z'; ch++)
        sigma.push_back(ch);
    for (char ch = '0'; ch <= '9'; ch++)
        sigma.push_back(ch);

    sigma.push_back('+');
    */
    std::vector<char> sigma = {'a', 'i', 'f'};  // simplify test: ok

    std::map<DFAState*, 
             std::map<char, DFAState* > 
            > DFATable;

    while (!workList.empty() )
    {
        auto iter = workList.begin();
        std::cout << "worklist head: DFA state id: " << (*iter)->id << std::endl;
        DFAState* curDFAState = workList.front(); // dFAState: d0 -->
        workList.erase(workList.begin() );     // workList: empty --> 

        std::cout << "====================process a new row:\n";
        for (auto ch: sigma) // a i f
        {
            std::vector<NFAState*> nextNFAStates = delta(curDFAState->nFAStates, ch); // {n3}  -> {n3, n1}      -> {n3}      -> 
            std::vector<NFAState*> newNFAStates = epsilonClosure(nextNFAStates);   // {n3, n4} -> {n3, n4, n1} ->  {n3, n4} ->

            std::cout << "newNFAStates: \n";
            for (const auto& nfastate: newNFAStates)
            {
                std::cout << "nfaState id: " << nfastate->id << std::endl;
            }
            
            std::cout << "fullList: \n";
            for (const auto& dfaState: fullList)
            {
                std::cout << "dfaState id: " << dfaState->id << std::endl;
                for (const auto& nfastate: dfaState->nFAStates)
                {
                    std::cout << "nfaState id: " << nfastate->id << std::endl;
                }
            }

            std::pair<bool, DFAState*> isInFullList2DFAState = isSubSet(newNFAStates, fullList);
            /*
            Note: 分支结构要设计得精简且覆盖所有所有情况，否则，分支间关系可能重叠或很难理清，导致程序也会很难写。
                非空, 空 => 不存在这种情况(因为 !workList.empty() , 且 fullList >= workList)
                空, else => 在其中, newNFAStates 对应 d_phi  => 不创建新 DFA 状态, 只在 DFATalbe 中填 d_phi  
                非空，非空: 
                    [1] 在其中 => 不创建新 DFA 状态, 找到相应于 fullList 中的 DFAState
                    [2] 不在其中(即新发现的 DFA 状态) => 创建新 DFAState，填在 DFATable 
            */
            DFAState* dstDFAState;

            if (newNFAStates.empty() )
            {
                std::cout << "d_phi: " << std::endl;
                dstDFAState = d_phi;
            }
            else if(!newNFAStates.empty() && !fullList.empty() )
            {
                if (!isInFullList2DFAState.first)
                {
                    std::cout << "newDiscovered: "  << std::endl;
                    dstDFAState = new_dfa_state(); 
                    dstDFAState->nFAStates = newNFAStates;
                    for (const auto& nFAState: newNFAStates) // DFA state is accepted if one of it's NFA states is accepted
                    {
                        if (nFAState->isAccepted)
                        {
                            dstDFAState->isAccepted = true;

                            auto minPrecedence2BoundTokenCategory = getDFAStatePrecedence(dstDFAState);
                            dstDFAState->acceptedPrecedence = minPrecedence2BoundTokenCategory.first;
                            dstDFAState->tokenCategory = minPrecedence2BoundTokenCategory.second;

                            break;
                        }
                    }

                    fullList.push_back(dstDFAState);
                    workList.push_back(dstDFAState);
                }
                else 
                {
                    std::cout << "hasDiscovered: " << std::endl;
                    dstDFAState = isInFullList2DFAState.second; // only this branch need matched DFAState
                }
            }

            DFATable[curDFAState][ch] = dstDFAState;
        }
    }

    /*
    DFA state - isAccepted: test ok
        DFAState id: 1, DFAState isAccepted: 0
        DFAState id: 2, DFAState isAccepted: 1
        DFAState id: 3, DFAState isAccepted: 1
        DFAState id: 4, DFAState isAccepted: 1
    */
    for (const auto& dFAState: fullList)
    {
        std::cout << "DFAState id: " << dFAState->id << 
            ", isAccepted: " << dFAState->isAccepted << 
            ", acceptedPrecedence: " << dFAState->acceptedPrecedence << std::endl;
    }

    return DFATable;
}

/*
DFA: test1 ok:
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
void printDFA(std::map<DFAState*, std::map<char, DFAState* > >  dFA) 
{
    std::cout << "dFA size: " << dFA.size() << std::endl;
    for (const auto& pair : dFA) 
    {
        std::cout << "DFA state id: " << pair.first->id << " <-> NFA state id: " ;

        /* 
            只在每2个 元素 之间打印 ","（即除了最后一个 元素 之外的每个 元素 后面打印 ",": 要定位最后一个 元素）
            <=> 在除了第1个 元素 之外的每个 元素 前面打印 ","
        */
        bool firstProcessed = true;
        for(const auto& nFAState: pair.first->nFAStates)
        {
            if (!firstProcessed) 
            {
                std::cout << ", ";  
            }
            std::cout << nFAState->id;
            firstProcessed = false;
        }
        std::cout << std::endl;

        for (const auto& pair2 : pair.second) 
        {
            std::cout << "-- char: " << pair2.first << "--> DFA state id: " << pair2.second->id << std::endl;
        }
    }
}

int scanningPos = 0;
char GetNextChar(const std::string& input)
{
    if (scanningPos != input.size() )
    {
        return input[scanningPos++];
    }
    return '\0';
};

void Rollback(const std::string& input)
{
    if (scanningPos != -1) // 没有回退到 input 第1个字符 左侧(无效位置)
    {
        scanningPos--;
    }
}

std::vector<std::pair<std::string, TokenCategory> > 
scanner(std::map<DFAState*, std::map<char, DFAState* > > dFATable, const std::string& input, std::vector<Rule> rules)
{
    std::vector<std::pair<std::string, TokenCategory> > token2CatStream;

    while (scanningPos != input.size() )
    {
        DFAState* state = dFAStartState; // start state
        std::string lexeme; 

        std::stack<DFAState*> stateStack;
        std::stack<DFAState*>().swap(stateStack); // clear stateStack

        // the longest mathing 
        while (state != NULL) // while (state != Se） // 不是 Error state
        {
            
            char ch = GetNextChar(input);
            lexeme = lexeme + ch;

            if (state->isAccepted) 
            {
                std::stack<DFAState*>().swap(stateStack); // 清空 stack 中该 accept state 之前的 states
            }
                
            stateStack.push(state);
            
            /*
                DFA 表不是严格二维表结构，对某个 DFAState 的 某个 char 没有转移时，
                不是转移到 一个设定的 Error state，而是不填入 DFA map。
            */
             
            auto iter1  = dFATable.find(state); 
            if (iter1 != dFATable.end() )
            {
                auto sym2State = iter1->second;

                auto iter2 = sym2State.find(ch);
                if (iter2 != sym2State.end() )
                {
                    state = iter2->second; // new state
                }
                else 
                {
                    state = NULL;
                }
            }
            else 
            {
                state = NULL;
            }
        } 
        
        if (!stateStack.empty() )
        {
            state = stateStack.top();
            // 回退：最终到达 the last latest accept state 或 NULL state
            while (!state->isAccepted)
            {
                state = stateStack.top();
                stateStack.pop();

                lexeme.resize(lexeme.size()-1); // remove last character 
                Rollback(input); // move the pointer in lexeme to the previous character
            }

            if(state->isAccepted)
            {
                std::pair<std::string, TokenCategory> token2Cat = {lexeme, state->tokenCategory};
                //std::cout << "\ntoken: " << lexeme << ", category: " << state->tokenCategory << std::endl;
                token2CatStream.push_back(token2Cat);
            }
            else
            {
                std::cout << "identify token error!!!\n";
            }
        }
    }

    return token2CatStream;
}

/*
token stream: test ok
    ========== tokens: 
    token: if , Category: 1
    token: ifa, Category: 2
*/
void printToken2CatStream(std::vector<std::pair<std::string, TokenCategory> > token2CatStream)
{
    std::cout << "========== tokens: \n";
    for (const auto& token2Cat: token2CatStream)
    {
        std::cout << "token: " << token2Cat.first << ", Category: " << token2Cat.second << std::endl;
    }
}

// ===================== 主函数：测试 =====================
int main() 
{
    std::cout << "==========1. Lexical Analysis:\n";
    /*
        test1:
            a ( b | c ) * -> 加连接符: a · ( b | c ) * -> 后缀: a b c | * · -> 后缀转 NFA:

            循环入栈，若遇到运算符时，出栈 1 个或 2 个 NFA，计算结果 入栈；直到所有符号入栈，栈中唯一 NFA 即 转换的 NFA。

        test2: (b|c)*
    */
    // std::string regex = "a(b|c)*"; // 后缀表达式 ok
    // std::string regex = "bc";      // NFA ok
    
    //std::vector<Rule> rules;
    //rules = parse_re_file("./rules.re");

    /*
    // === 1.1 RE -> 后缀表达式
    std::cout << "RE: " << rule.pattern << std::endl;
    std::string postfix = regex_to_postfix(rule.pattern);       // ===== test1 ok
    std::cout << "后缀表达式：" << postfix << std::endl;
    // === 1.2 Thompson Construction（栈）算法生成 NFA
    NFA final_nfa = thompsonConstruction(postfix);

    std::cout << "\n最终 NFA 结构：\n";
    std::set<NFAState*> visited;
    print_nfa(final_nfa.startState, visited);
    */

    std::vector<Rule> rules;
    Rule rule1 = {"if", "IF"};
    Rule rule2 = {"([a-Z])([a-Z]|[0-9])*", "IDENTIFIER"};
    rules.push_back(rule1);
    rules.push_back(rule2);

    NFA final_nfa = build_if_id_nfa();
    
    std::set<NFAState*> visited;
    print_nfa(final_nfa.startState, visited);

    std::map<DFAState*, std::map<char, DFAState* > >  dFA = nFA2DFA(final_nfa);
    printDFA(dFA);

    std::string input = "if ifa";
    std::vector<std::pair<std::string, TokenCategory> > token2CatStream;
    token2CatStream = scanner(dFA, input, rules);
    printToken2CatStream(token2CatStream);
}
