/****************************************************/
/* 文件: globals.h                                   */
/* TINY 编译器的全局类型和变量定义                    */
/* 必须最先被其他文件包含                            */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <iostream>   // C++ 标准输入输出流
#include <fstream>    // C++ 文件输入输出流
#include <string>     // C++ 字符串类
#include <cstdlib>    // C 标准库（malloc, free, atoi 等）
#include <cctype>     // 字符处理函数（isdigit, isalpha 等）
#include <cstring>    // C 字符串函数（strcmp, strcpy 等）

#ifndef FALSE
#define FALSE 0       // 布尔值：假
#endif

#ifndef TRUE
#define TRUE 1        // 布尔值：真
#endif

/* MAXRESERVED = 保留字（关键字）的数量 */
#define MAXRESERVED 11

/* ================================================================
 * TokenType — 词法单元（记号）类型枚举
 * 定义了编译器识别的所有词法单元类型：
 *   ENDFILE / ERROR : 文件结束 / 错误（簿记用）
 *   IF / THEN / ELSE / END / REPEAT / UNTIL / READ / WRITE : 保留字
 *   ID / NUM : 多字符词法单元（标识符 / 数字）
 *   ASSIGN / EQ / LT / PLUS / MINUS / TIMES / OVER / LPAREN / RPAREN / SEMI : 特殊符号
 *   LE,GT,GE,NEQ,INC,DEC,MOD,POWER 后面添加特殊符号
 * ================================================================ */
typedef enum
   {ENDFILE,ERROR,
    IF,THEN,ELSE,END,REPEAT,UNTIL,READ,WRITE,WHILE,ENDWHILE,FOR,
    ID,NUM,
    ASSIGN,EQ,LT,PLUS,MINUS,TIMES,OVER,LPAREN,RPAREN,SEMI,LE,GT,GE,NEQ,INC,DEC,MOD,POWER
   } TokenType;
/* ================================================================
 * 全局文件指针
 * source  : 源代码文件（输入）
 * listing : 列表输出文件（屏幕或文件）
 * code    : TM 模拟器代码文件（输出）
 * ================================================================ */
extern std::ifstream* source;   // 源代码文本文件
extern std::ostream*  listing;  // 列表输出文本文件（通常为 stdout）
extern std::ofstream* code;     // TM 模拟器代码文本文件

extern int lineno;              // 源代码当前行号（用于列表输出）

/**************************************************/
/***********   语法树定义（用于语法分析） ************/
/**************************************************/

/* NodeKind : 语法树节点的种类 —— 语句节点 或 表达式节点 */
typedef enum {StmtK, ExpK} NodeKind;

/* StmtKind : 语句节点的子类型 */
typedef enum {IfK, RepeatK, AssignK, ReadK, WriteK, WhileK, ForK} StmtKind;

/* ExpKind : 表达式节点的子类型 */
typedef enum {OpK, ConstK, IdK} ExpKind;

/* ExpType : 表达式类型（用于类型检查） */
typedef enum {Void, Integer, Boolean} ExpType;

/* 每个节点的最大子节点数 */
#define MAXCHILDREN 4

/* ================================================================
 * TreeNode — 语法树节点结构体
 *
 * 结构说明：
 *   child[3]  : 最多三个子节点指针（if 有 3 个，repeat 有 2 个，等等）
 *   sibling   : 兄弟节点指针（同一层的下一个语句）
 *   lineno    : 该节点对应的源代码行号
 *   nodekind  : 节点种类（StmtK 或 ExpK）
 *   kind      : 联合体 —— 根据 nodekind 选择 stmt 或 exp
 *   attr      : 联合体 —— 根据节点类型存储不同属性：
 *               op   : 运算符类型（OpK 时）
 *               val  : 整数值（ConstK 时）
 *               name : 标识符名称（IdK、AssignK、ReadK 时）
 *   type      : 表达式类型（用于类型检查）
 * ================================================================ */
typedef struct treeNode
   { struct treeNode* child[MAXCHILDREN];  // 子节点指针数组
     struct treeNode* sibling;             // 兄弟节点指针
     int lineno;                           // 行号
     NodeKind nodekind;                    // 节点种类：语句 / 表达式
     union { StmtKind stmt; ExpKind exp; } kind;   // 具体节点子类型
     union { TokenType op;                // 运算符
             int val;                      // 常量值
             char* name;                   // 标识符名称
           } attr;
     ExpType type;                         // 表达式类型（类型检查用）
   } TreeNode;

// 1. 定义操作符类型枚举
typedef enum {rd, Jr, J, asn, add, mul, devi, sub, eq, wri, Jl, Jre, Jle, Jne, modx, pow,
              lt_op, le_op, gt_op, ge_op, ne_op, inc_op, dec_op} OpKind;

// 2. 定义地址的种类（空、整型常量、字符串名）
typedef enum {Empty, IntConst, String} AddrKind;

// 3. 定义单个地址的数据结构
struct OperandsNode{
    AddrKind kind;   // 标记当前地址存放的是什么类型的值
    union {
        int val;       // 存放整型常量值
        const char* name;   // 存放变量或标号的名字
    } contents;
};



/**************************************************/
/***********   跟踪调试标志          ****************/
/**************************************************/

/* EchoSource : 若为 TRUE，则在语法分析时将源代码回显到列表文件（带行号） */
extern int EchoSource;

/* TraceScan : 若为 TRUE，则在词法分析器识别每个词法单元时，将信息打印到列表文件 */
extern int TraceScan;

/* TraceParse : 若为 TRUE，则将语法树以缩进形式打印到列表文件 */
extern int TraceParse;

/* TraceAnalyze : 若为 TRUE，则将符号表的插入和查找操作报告到列表文件 */
extern int TraceAnalyze;

/* TraceCode : 若为 TRUE，则在生成代码时向 TM 代码文件写入注释 */
extern int TraceCode;

/* Error : 若为 TRUE，表示发生了错误，阻止后续处理阶段继续执行 */
extern int Error;

#endif
