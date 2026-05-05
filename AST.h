#ifndef AST_H
#define AST_H

#include <string>
#include <memory>
#include <vector>

// ===== AST: used for Parser and IRGen
enum class ASTNodeCategory 
{
    PROG,      
    BINOP,          // 二元运算符 * + 
    ASSIGNMENT,
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

struct BinaryOpNode: ASTNode 
{
    ASTNodePtr left, right;
    std::string op; // "*" / "+" / "=" // Note: "=" 也视为 binaryOp
    BinaryOpNode(ASTNodePtr left_, ASTNodePtr right_, std::string op_)
        : ASTNode(ASTNodeCategory::BINOP), left(std::move(left_) ), right(std::move(right_) ), op(std::move(op_) ) {}
};

struct AssignOpNode: ASTNode // optimize: AssignOpNode 本身就表示了 '=' 节点，但 加一个 表示 op(std::string) 的 field 更便于处理，且便于统一
{
    ASTNodePtr left;  // Note: actually point to IdentNode
    ASTNodePtr right; // expr
    AssignOpNode(ASTNodePtr left_, ASTNodePtr right_) : 
        ASTNode(ASTNodeCategory::ASSIGNMENT), left(std::move(left_) ), right(std::move(right_) ) {}
};

struct ProgramNode: ASTNode 
{
    std::vector<ASTNodePtr> stmts;
    ProgramNode() : ASTNode(ASTNodeCategory::PROG) {}
};

#endif 