# simpleCompiler
用编译器设计中的各种技术:
    Lexical Analyze: REs -> NFA -> DFA 
    Parser: LR(1) / AST
    IRGen: TAC
    Optimize: constant propagation / constant folding / CSE eliminate
    CodeGen: Abstract asm (register number Unlimited)

    实现了一个具有 词法分析、语法分析（输出 AST）、中间代码生成（TAC）、优化、代码生成（虚拟代码生成：寄存器数量无限制） 的简单编译器。 
    
    基于全英文（无中文字幕）课程《CSC151: Compiler Construction》 California State University, Dr. Ghassan Shobako. 

    Provide the Pseudo-Code and corresponding C++ implementation Code for the key algorithms.
    The C++ implementation Code may include space or time efficiency optimizations and may be refactored multiple times; 
    you can track refactoring ideas/methods and changes via the Git version history/logs.