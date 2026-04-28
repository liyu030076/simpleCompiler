#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <set>
#include <memory>
#include <string>

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
const char eof = '$'; 

// =============Note: 编译时指定宏: g++ xxx.c -D 宏名
#ifdef NEEDPARSERTREE
enum ParserTreeNodeType 
{
    NON_TERMINAL, 
    TERMINAL     
};

struct ParserTreeNode 
{
    ParserTreeNodeType nodeType;
    char grammarSym;                    
    std::vector<std::shared_ptr<ParserTreeNode> > children;  

    ParserTreeNode(ParserTreeNodeType tp, char ch) : nodeType(tp), grammarSym(ch) {}

    void addChild(const std::shared_ptr<ParserTreeNode>& child) 
    {
        children.push_back(child);
    }
};

using ParserTreeNodePtr = std::shared_ptr<ParserTreeNode>;

#endif

void initGrammar();

void buildStatesAndStateTransGraph();
void buildActTblAndGotoTbl();

#ifdef NEEDPARSERTREE 
ParserTreeNodePtr 
#else
void*
#endif
parser(std::string input);

void printAll(std::set<char> terms, std::set<char>& nonTerms);

#ifdef NEEDPARSERTREE
void printAst(const ParserTreeNodePtr& root, int indent = 0);
#endif

#endif 