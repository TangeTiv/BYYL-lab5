/****************************************************/
/* 文件: compiler_api.cpp                             */
/* TINY 编译器的字符串编译接口实现                    */
/* 为 GUI 程序提供无需文件 I/O 的编译入口            */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "parser.h"
#include "analyzer.h"
#include "cgen.h"
#include "compiler_api.h"

/* 编译阶段控制宏（与 main.cpp 保持一致）*/
#define NO_PARSE   FALSE
#define NO_ANALYZE FALSE
#define NO_CODE    FALSE

/* ================================================================
 * compileFromString : 从字符串编译 TINY 源程序
 *
 * 工作流程：
 *   1. 重置所有编译器状态
 *   2. 将源代码字符串包装为 istringstream
 *   3. 将输出目标改为 ostringstream
 *   4. 执行编译流水线
 *   5. 收集所有输出
 *   6. 恢复默认流指针
 * ================================================================ */
CompileResult compileFromString(const std::string& sourceCode)
{
    CompileResult result;
    result.success = false;

    /* ---- 1. 重置编译器状态 ---- */
    resetCompilerState();

    /* ---- 2. 创建字符串流作为输入/输出 ---- */
    std::istringstream sourceStream(sourceCode);
    std::ostringstream quadStream;
    std::ostringstream tmStream;
    std::ostringstream listingStream;

    /* ---- 3. 设置全局流指针 ---- */
    source  = &sourceStream;
    listing = &listingStream;
    quad    = &quadStream;
    code    = &tmStream;

    /* ---- 4. 执行编译流水线 ---- */
    TreeNode* syntaxTree = NULL;

#if NO_PARSE
    /* 仅词法分析模式 */
    while (getToken() != ENDFILE);
#else
    /* 语法分析阶段 */
    syntaxTree = parse();

#if !NO_ANALYZE
    /* 语义分析阶段（无错误时执行）*/
    if (!Error)
    {
        buildSymtab(syntaxTree);
        typeCheck(syntaxTree);
    }

#if !NO_CODE
    /* 代码生成阶段（无错误时执行）*/
    if (!Error)
    {
        codeGen(syntaxTree, "<GUI Input>");
    }
#endif
#endif
#endif

    /* ---- 5. 收集输出结果 ---- */
    result.success = !Error;
    result.quadOutput    = quadStream.str();
    result.tmOutput      = tmStream.str();
    result.listingOutput = listingStream.str();

    /* ---- 6. 恢复默认流指针（防止悬空指针）---- */
    source  = nullptr;
    listing = nullptr;
    quad    = &std::cout;
    code    = nullptr;

    return result;
}
