/****************************************************/
/* 文件: globals.cpp                                  */
/* TINY 编译器的全局变量定义                          */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"
#include "scanner.h"
#include "parser.h"
#include "analyzer.h"
#include "symtab.h"
#include "cgen.h"
#include "code.h"

/* ================================================================
 * 全局变量定义
 * 这些变量在 globals.h 中声明为 extern，在此处实际分配内存
 * ================================================================ */
int lineno = 0;                  // 当前行号，从 0 开始
std::istream*  source = nullptr; // 源代码输入流（CLI为ifstream，GUI为istringstream）
std::ostream*  listing = nullptr;// 列表输出流（通常指向 std::cout）
std::ostream*  code = nullptr;   // TM 代码输出流（CLI为ofstream，GUI为ostringstream）
std::ostream*  quad = &std::cout;// 四元组输出流（默认指向 std::cout）

/* 跟踪调试标志（默认全部关闭） */
int EchoSource   = FALSE;   // 是否回显源代码
int TraceScan    = FALSE;   // 是否跟踪词法分析过程
int TraceParse   = FALSE;   // 是否打印语法树
int TraceAnalyze = FALSE;   // 是否跟踪语义分析过程
int TraceCode    = FALSE;   // 是否在代码中写入注释

int Error = FALSE;          // 错误标志，发生错误时阻止后续阶段

/* ================================================================
 * resetCompilerState : 重置所有编译器状态（用于 GUI 多次编译）
 * 依次调用各模块的 reset 函数，确保每次编译都是全新的状态
 * ================================================================ */
void resetCompilerState(void)
{
    lineno = 0;
    Error = FALSE;
    resetScanner();
    resetParser();
    resetAnalyzer();
    st_clear();
    resetCodeGen();
    resetCodeState();
}
