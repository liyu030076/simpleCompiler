#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <set>
#include <memory>
#include <string>

#include "common.h"

// === 1. interface to Lexical Analyzer
using Token2Cat = std::pair<std::string, TokenCategory>;
using Token2CatStream = std::vector<Token2Cat>;
// === interface to Lexical Analyzer end 

// ===== 2. Grammar
enum GrammarSymCat 
{
    TERMINAL,
    NON_TERMINAL
};

enum NonTerminalId 
{
    Pro = 0,  // Program
    Statement = 1,  // Statement
    ASSIG = 2,      
    Expr = 3,       
    Term = 4,       
    Factor = 5
};

extern std::vector<std::pair<NonTerminalId, const char*> > nonTerminalId2Str;

// === may need furture optimize 
struct GrammarSym 
{
    GrammarSymCat grammarSymCat;
    int termTokenCat_nonTermId; // for Terminal / NON_TERMINAL, correspond to enum TokenCategory / enum NonTerminalId, they can be converted to int 

    bool operator==(const GrammarSym& other) const 
    {
        return grammarSymCat == other.grammarSymCat &&
               termTokenCat_nonTermId == other.termTokenCat_nonTermId;
    }

    // Note: 因为 GrammarSym 要放到 std::set 中 => 必须定义 operator <, 实际上业务逻辑不用，只是 操作 set 时会用
    bool operator<(const GrammarSym& other) const 
    {
        if (grammarSymCat != other.grammarSymCat) 
            return grammarSymCat < other.grammarSymCat;
        return termTokenCat_nonTermId < other.termTokenCat_nonTermId;
    }

};

inline GrammarSym getTerminalSym(TokenCategory tokenCat) 
{
    return {TERMINAL, static_cast<int>(tokenCat) };
}

inline GrammarSym getNonTerminalSym(NonTerminalId nonTerminalId) 
{
    return {NON_TERMINAL, static_cast<int>(nonTerminalId) };
}

struct Production 
{
    GrammarSym lhs;
    std::vector<GrammarSym> rhs;
};

/*
const 全局变量
    C++
        默认 文件作用域(内部链接)
        头文件定义 const -> 多文件包含 不重定义

    C 语言
        默认 全局作用域(外部链接)
        
        const 全局变量 跨文件共享 如何实现？
            错误做法：头文件定义 const -> 多文件包含 必重定义
            正确做法：头文件 extern 声明, 一个源文件定义，其他源文件包含头文件后直接引用
*/
const GrammarSym eof{TERMINAL, SENTINEL}; 

// ===== Grammar end 

// ===== 3. LR1Item
struct LR1Item 
{
    // Note: this is the better design: LR1Item only need to have the prodIndex of the productions, rather than the concrete production.
    int prodIndex; // point to the start production 

    int dotPos;    // Note: init. value is 0

    /*
        Note: this is the more clear design, 把 同一 production 但 前瞻符号不同 的 items 作为多个 item，
                而不是作为1个 item 带 不同的 lookahead 构成的集合。
                only meaningful if dotPosNextSym_FellowSym is terminal.
    */
    GrammarSym lookahead; 

    bool operator==(const LR1Item& other) const 
    {
        if (prodIndex != other.prodIndex) 
            return false;
        if (dotPos != other.dotPos) 
            return false;
        return lookahead == other.lookahead;
    }
};
// ===== LR1Item end
using State = std::vector<LR1Item>;

// ===== 4. AST

enum class ASTNodeCategory 
{
    PROG,      
    BINOP,          // 二元运算符 * + 
    // ASSIGNMENTOP,
    INTEGER,       
    IDENT              
};

struct ASTNode 
{
    ASTNodeCategory category;
    ASTNode(ASTNodeCategory cat) : category(cat) {}
    virtual ~ASTNode() {} // Note: Base must has virtial Dtor, to support std::unique_ptr<Derived> will be implicitly converted to std::unique_ptr<Base>.
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

struct IdentNode: ASTNode 
{
    std::string value;
    IdentNode(std::string name_) : 
        ASTNode(ASTNodeCategory::IDENT), value(std::move(name_) ) {}
};

struct IntegerNode: ASTNode 
{
    int value; // Problem：应该用 int 吗？此时，编译器指定 int 是啥吗？
    IntegerNode(int val) : 
        ASTNode(ASTNodeCategory::INTEGER), value(val) {}
};

/*
struct AssignOpNode: ASTNode // optimize: AssignOpNode 本身就表示了 '=' 节点，但 加一个 表示 op(std::string) 的 field 更便于处理，且便于统一
{
    ASTNodePtr left;
    ASTNodePtr expr;
    AssignOpNode(ASTNodePtr left_, ASTNodePtr expr_) : 
        ASTNode(ASTNodeCategory::AssignStmt), left(std::move(left_) ), expr(std::move(expr_) ) {}
};
*/

struct BinaryOpNode: ASTNode 
{
    ASTNodePtr left, right;
    std::string op; // "*" / "+" / "=" // Note: "=" 也视为 binaryOp
    BinaryOpNode(ASTNodePtr left_, ASTNodePtr right_, std::string op_)
        : ASTNode(ASTNodeCategory::BINOP), left(std::move(left_) ), right(std::move(right_) ), op(std::move(op_) ) {}
};

struct ProgramNode: ASTNode 
{
    std::vector<ASTNodePtr> stmts;
    ProgramNode() : ASTNode(ASTNodeCategory::PROG) {}
};

enum class PRODUCTIONINDEX
{
    STATEMENT_REDUCED2_PROG,
    ASSIGN_REDUCED2_STATEMENT,
    ASSIGN,
    ADD,
    TERM_REDUCED2_EXPR,
    MUL,
    FACTOR_REDUCED2_TERM,
    INTERGE_REDUCED2_FACTOR,
    IDENT_REDUCED2_FACTOR,
    PARENTHESIS_REDUCED2_FACTOR 
};

// ===== 5. ParserTree
// =============Note: 编译时指定宏: g++ xxx.c -D 宏名
#ifdef NEEDPARSERTREE
struct ParserTreeNode 
{
    GrammarSym grammarSym;
    std::string tokenStrValue; 

    std::vector<std::shared_ptr<ParserTreeNode> > children;  

    ParserTreeNode(GrammarSym grammarSym_, std::string tokenStrValue_): grammarSym(grammarSym_), tokenStrValue(tokenStrValue_) {}

    void addChild(const std::shared_ptr<ParserTreeNode>& child) 
    {
        children.push_back(child);
    }
};

using ParserTreeNodePtr = std::shared_ptr<ParserTreeNode>;

#endif

// ===== ParserTree end

void initGrammar();

void buildStatesAndStateTransGraph();
void buildActTblAndGotoTbl();

#ifdef NEEDPARSERTREE 
ParserTreeNodePtr 
#else
ASTNodePtr
#endif
parser(const Token2CatStream& token2CatStream);

void printAll(const std::set<GrammarSym>& terms, const std::set<GrammarSym>& nonTerms);

void printAST(const ASTNodePtr& root, int indent = 0, std::string rootLeftRight = "root:");

#ifdef NEEDPARSERTREE
void printParseTree(const ParserTreeNodePtr& root, int indent = 0);
#endif

#endif 