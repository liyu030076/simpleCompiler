/*
    All CodeGens, whether (actual) CodeGen or IR CodeGen, if directly use AST as input, will have the same implement idea.
*/

/*
==========1. global var
    using IRInstr = std::string;
    irCodeList: std::vector <IRInstr>

    tmpConut
*/

/*
==========2. getNewTempVar()

string newTempVar():
    tmpConut = tmpConut + 1
    return "t" + to_string(tmpConut)

string NEW_LABEL():
    labelCount = labelCount + 1
    return "L" + to_string(labelCount)
*/

/*
==========3. Emit know how to emit right instruction. 

Emit(op, src1, src2, dst)   // directly use AST as input to generate IR Code
    Emit know how to emit right instruction. 
    if Op is load immediate, Emit will konw src1（3）must be a num，src2 is not used，dst is a register.
    else if Op is load variable, Emit will know src1  is the base(rarp), src2 is the offset(@a), dst is the register。

    switch Op:
        case "LoadInteger":
            irCodeList.push_back(dst + " = " + src1)
        case "+":
        case "*"
            irCodeList.push_back(dst + " = " + src1 + " " + op + " " + src2)

        case "=": // Note: diff from Gen AST in Parser, here Assignment is not viewed as BinaryOp
            irCodeList.push_back( src1 + " = " + src2);
*/

/*
==========4. generate IR Code 

    input: current ASTNode traversed 
    output: the IR operand corresponds to the current ASTNode


string genIR(ASTNode node)

    switch node.category:

        case ASTNodeCategory::INTEGER:
            t = newTempVar()
            emit("LoadInteger", to_string(node.value), "", t)
            return t

        case ASTNodeCategory::IDENT:
            return node.value // directly return variable name as operand

        case ASTNodeCategory::BINOP:
            // recursively generate left、right subTree IR
            lOprand  = genIR(node.left)
            rOprand = genIR(node.right)
            
            // generate current operator IR
            t = newTempVar()
            emit(node.op, lOprand, rOprand, t) 
            return t

        case ASTNodeCategory::ASSIGNMENT:
            rOprand = genIR(node.right); // recursively generate right subTree IR
            emit("assign", (static_cast<IdentNode>(node.left)).value, rOprand, ""); // generate current operator IR
            return "" // Note: for "=", don't generate new temp variable for left of assignment, that don't generate left operand.

        default:
            ERROR("unknow ASTNode type Category!")

*/

/*
directly use AST as input to generate virtual/actual Code

Emit(Op, src1, src2, dst)
    Emit know how to emit right instruction. 
    if Op is load immediate，Emit will konw src1（3）must be a num，src2 is not used，dst is a register.
    else if Op is load variable，Emit will know src1  is the base(rarp), src2 is the offset(@a), dst is the register。

    switch Op:
        case LoadNum:

        case LoadIdent:

        case BinaryOp:

        case Assign: // Note: diff from Gen AST in Parser, here Assignment is not viewed as BinaryOp
*/

#include <iostream>
#include "IRGen.h"

std::string IRGen::newTempVar()
{
    tmpConut = ++tmpConut;
    return "t" + std::to_string(tmpConut); // RVO
}

void IRGen::emit(std::string op, std::string src1, std::string src2, std::string dst)   // directly use AST as input to generate IR Code
{   
    if ("LoadInteger" == op)
    {
        irCodeList.push_back(dst + " = " + src1);
    }   
    else if ("+" == op ||
             "*" == op)
    {
        irCodeList.push_back(dst + " = " + src1 + " " + op + " " + src2);
    }
    else if ("=" == op) 
    {
        irCodeList.push_back(src1 + " = " + src2);
    }
    else 
    {
        // ...
    }
}

/*
1. a optimize point later: use virtual function to replace switch-case

    class ASTNode 
    {
        public:
            virtual ~ASTNode() = default;
            virtual std::string generateIR() = 0; // Derived override
    };

*/
std::string IRGen::genIR(ASTNodePtr node)
{
    std::string t; // std::string default value = ""
    switch(node->category) // ===a optimize point later: use virtual function to replace switch-case, 
    {
        case ASTNodeCategory::INTEGER:
        {
            t = newTempVar();
            emit("LoadInteger", std::to_string(static_cast<IntegerNode*>(node.get() )->value), "", t);
            break;
            //return t;
        }
        case ASTNodeCategory::IDENT:
            t = std::move(static_cast<IdentNode*>(node.get() )->value);
            break;
            //return node->value; // directly return variable name as operand

        case ASTNodeCategory::BINOP:
        {
            // recursively generate left、right subTree IR
            std::string lOprand  = genIR(std::move(static_cast<BinaryOpNode*>(node.get() )->left) );
            std::string rOprand = genIR(std::move(static_cast<BinaryOpNode*>(node.get() )->right) );
            
            // generate current operator IR
            t = newTempVar();
            emit(static_cast<BinaryOpNode*>(node.get() )->op, lOprand, rOprand, t); 
            break;
            //return t;
        }
        case ASTNodeCategory::ASSIGNMENT:
        {
            std::string rOprand = genIR(std::move(static_cast<AssignOpNode*>(node.get() )->right) ); // recursively generate right subTree IR
            emit("=", static_cast<IdentNode*>(static_cast<AssignOpNode*>(node.get() )->left.get() )->value, rOprand, ""); // generate current operator IR
            break;
            // Note: for "=", don't generate new temp variable for left of assignment, that don't generate left operand.
            //return ""; 
        }
        case ASTNodeCategory::PROG:
        {
            /*
            std::vector<std::unique_ptr<Base> > 的正确遍历：
                [1] 只读 容器内的 up 元素
                    for (const std::unique_ptr<Base>& up : vec) 
                        // 用 up-> 访问成员的引用。ptr 是引用，没有拷贝，安全
                [2] 想修改（如 重置、move）容器内的 up 元素
                    for (std::unique_ptr<Base>& up : vec)
                        // up.reset();
            */
            for(ASTNodePtr& subRoot: static_cast<ProgramNode*>(node.get() )->stmts)
            {
                // Note: after this move, the up left in root/prog.stmts are all empty(== nullptr)
                genIR(std::move(subRoot) ); // doesn't care return value 
            } 
            break;
        }
        default:
        {
            std::cout << "unknow ASTNode type Category!\n";
            break;
        }    
    }
    return t; // NRVO
}

const std::vector<IRInstr>& IRGen::getIRCodeList() const
{
    return irCodeList;
}