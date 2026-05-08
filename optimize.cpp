#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include "optimize.h"

// ====================== utilities ======================
static bool isNumber(const std::string& s) 
{
    if (s.empty() ) 
        return false;
    int idx = 0;
    if (s[0] == '-') 
        idx = 1;

    for (; idx < s.size(); idx++)
        if (!std::isdigit(s[idx]) ) 
            return false;
    return true;
}

static int calc(int a, int b, const std::string& op)
{
    if (op == "+") 
        return a + b;
    else if (op == "*") 
        return a * b;
    else 
    {
        std::cout << "don't support!\n";
        return 0;
    }
}

static std::string exprUniqueKey(const std::string& s1, const std::string& op, const std::string& s2) 
{
    return s1 + "|" + op + "|" + s2;
}

// 1. 复制传播 + 常量传播 + 常量折叠

/*
    复制传播：把变量换成另一个变量 
        作用：消除冗余复制赋值，给 后续 常量传播、常量折叠、死代码消除创造条件
        eg:
         
            t1 = 10
            a  = t1     // 复制：a 就是 t1, 后面所有 a 直接用 t1
            t2 = a + 5
        
        做复制传播之后
            把 a 全都替换成 t1：
            t1 = 10
            t2 = t1 + 5
*/
void cpyPro_constPro_constFold(std::vector<IRInstr>& src) 
{
    std::vector<IRInstr> res;
    std::unordered_map<std::string, int> constVal;        // 常量映射: dst -> rhsConstant
    std::unordered_map<std::string, std::string> copyVal; // 复制传播映射 x = y: dst -> rhsVariable

    for (auto& line: src) 
    {
        auto tac = parseTAC(line);
        if (tac.dst.empty() ) 
        {
            res.push_back(line);
            continue;
        }

        // function that do copy propagation: if para is not recorded as result of cpyPropag, return para (copy) itself
        auto cpyPropag = [&](std::string s) 
        {
            while (copyVal.count(s) ) 
                s = copyVal[s];
            return s;
        };

        if (tac.isAssign) // Note: don't do constant propagation for assignment
        {
            std::string rhs = cpyPropag(tac.s1);
            std::string newLine = tac.dst + " = " + rhs;
            //res.push_back(newLine); // [1] copy propagation

            // record constant propagation + constant propagation after copy propagation + copy propagation
            if (isNumber(rhs) ) 
            {
                constVal[tac.dst] = std::stoi(rhs); // string to int 
            } 
            else if (constVal.count(rhs) ) // case tac.op == =": tac.dst will be constant, if cpyPropag(tac.s1) is constant.
            {
                constVal[tac.dst] = constVal[rhs];
                
                newLine = tac.dst + " = " + std::to_string(constVal[rhs]);
            } 
            else 
            {
                copyVal[tac.dst] = rhs;
            }
            // continue;

            res.push_back(newLine); // [1] copy propagation // constant propagation after copy propagation 
        }
        else if (tac.isBinaryOp) 
        {
            // [1] ready for copy propagation 
            std::string s1 = cpyPropag(tac.s1);
            std::string s2 = cpyPropag(tac.s2);

            // [2] ready for constant propagation
            if (constVal.count(s1) ) 
                s1 = std::to_string(constVal[s1]);
            if (constVal.count(s2) ) 
                s2 = std::to_string(constVal[s2]);

            // [3.1] constant folding (if leftOperand and rOperand are all numbers) after copy propagation and constant propagation.
            if (isNumber(s1) && isNumber(s2) ) 
            {
                int v1 = std::stoi(s1);
                int v2 = std::stoi(s2);
                int rv = calc(v1, v2, tac.op);

                res.push_back(tac.dst + " = " + std::to_string(rv) );
                constVal[tac.dst] = rv;
            } 
            else // [3.2] only copy propagation + constant propagation
            {
                res.push_back(tac.dst + " = " + s1 + " " + tac.op + " " + s2);
            }
        }
    }
    src = res;
    //return res;
}

/*
    For TAC, CSE eliminate only process CSE in binaryOp actually.
*/
void eliminateCSE(std::vector<IRInstr>& src) 
{
    std::vector<IRInstr> res;
    std::unordered_map<std::string, std::string> exprToTemp;

    for (const auto& line : src) 
    {
        auto tac = parseTAC(line);
        if (tac.isAssign) // Note: now only support two cases: isAssign/isBinaryOp
        {
            res.push_back(line);
            continue;
        }
        else if (tac.isBinaryOp) // Note: this version, CSE only use for binary Operator(+ *)
        {
            std::string key = exprUniqueKey(tac.s1, tac.op, tac.s2);
            if (exprToTemp.count(key) ) // if exist the same expression, directly reuse the corresponding temp variable 
            {
                res.push_back(tac.dst + " = " + exprToTemp[key]);
            } 
            else 
            {
                exprToTemp[key] = tac.dst;
                res.push_back(line);
            }
        }
    }
    src = res;
    //return res;
}

void optimizeAll(std::vector<IRInstr>& tac) 
{
    cpyPro_constPro_constFold(tac); 
    printTAC(tac, "4.1 TAC after cpyPro_constPro_constFold optimize:");    
    eliminateCSE(tac);  
    printTAC(tac, "4.2 TAC after eliminateCSE:");    
}

void printTAC(const std::vector<IRInstr>& tac, const std::string& title) 
{
    std::cout << "\n==== " << title << " ====\n";
    for (auto& line: tac) 
        std::cout << line << "\n";
}

