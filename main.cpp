/****************************************************/
/* 文件: main.cpp                                    */
/* TINY 编译器的主程序入口                           */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"

/* ================================================================
 * 编译阶段控制宏
 *
 * NO_PARSE   = TRUE  -> 仅执行词法分析（扫描器），跳过后续阶段
 * NO_ANALYZE = TRUE  -> 仅执行语法分析（构建语法树），跳过语义分析
 * NO_CODE    = TRUE  -> 执行语法分析和语义分析，但不生成目标代码
 *
 * 所有宏默认为 FALSE，即执行完整的编译流程：
 *   词法分析 -> 语法分析 -> 语义分析 -> 代码生成
 * ================================================================ */
#define NO_PARSE   FALSE
#define NO_ANALYZE FALSE
#define NO_CODE    FALSE

#include "util.h"
#if NO_PARSE
#include "scanner.h"
#else
#include "parser.h"
#if !NO_ANALYZE
#include "analyzer.h"
#if !NO_CODE
#include "cgen.h"
#endif
#endif
#endif

/* ================================================================
 * 全局变量定义
 * 这些变量在 globals.h 中声明为 extern，在 globals.cpp 中定义
 * ================================================================ */

/* ================================================================
 * main — 编译器主函数
 *
 * 命令行用法: tiny <源文件名>
 *
 * 处理流程：
 *   1. 打开源代码文件（自动添加 .tny 扩展名）
 *   2. 执行语法分析，构建语法树
 *   3. （可选）打印语法树
 *   4. （可选）构建符号表并进行类型检查
 *   5. （可选）生成 TM 虚拟机代码
 *   6. 清理资源并退出
 * ================================================================ */
int main(int argc, char* argv[])
{
    TreeNode* syntaxTree;   // 语法树根节点
    char pgm[120];          // 源代码文件名

    /* ---- 检查命令行参数 ---- */
    if (argc != 2)
    {
        std::cerr << "用法: " << argv[0] << " <文件名>" << std::endl;
        exit(1);
    }

    /* ---- 复制文件名，自动补全 .tny 扩展名 ---- */
    strcpy(pgm, argv[1]);
    if (strchr(pgm, '.') == NULL)
        strcat(pgm, ".tny");

    /* ---- 打开源代码文件 ---- */
    std::ifstream* srcFile = new std::ifstream(pgm);
    if (!srcFile->is_open())
    {
        std::cerr << "文件 " << pgm << " 未找到!" << std::endl;
        delete srcFile;
        exit(1);
    }
    source = srcFile;

    /* ---- 设置列表输出到屏幕 ---- */
    listing = &std::cout;
    *listing << "\nTINY 编译: " << pgm << std::endl;

#if NO_PARSE
    /* ---- 仅词法分析模式 ---- */
    while (getToken() != ENDFILE);
#else
    /* ---- 语法分析阶段 ---- */
    syntaxTree = parse();

    /* ---- 如果需要，打印语法树 ---- */
    if (TraceParse)
    {
        *listing << "\n语法树:" << std::endl;
        printTree(syntaxTree);
    }

#if !NO_ANALYZE
    /* ---- 语义分析阶段（没有错误时才执行）---- */
    if (!Error)
    {
        if (TraceAnalyze)
            *listing << "\n正在构建符号表..." << std::endl;
        buildSymtab(syntaxTree);

        if (TraceAnalyze)
            *listing << "\n正在进行类型检查..." << std::endl;
        typeCheck(syntaxTree);

        if (TraceAnalyze)
            *listing << "\n类型检查完成" << std::endl;
    }

#if !NO_CODE
    /* ---- 代码生成阶段（没有错误时才执行）---- */
    if (!Error)
    {
        char* codefile;
        int fnlen = strcspn(pgm, ".");
        codefile = (char*)calloc(fnlen + 4, sizeof(char));
        strncpy(codefile, pgm, fnlen);
        strcat(codefile, ".tm");

        std::ofstream* tmFile = new std::ofstream(codefile);
        if (!tmFile->is_open())
        {
            std::cout << "无法打开文件 " << codefile << std::endl;
            delete tmFile;
            free(codefile);
            exit(1);
        }
        code = tmFile;

        codeGen(syntaxTree, codefile);

        tmFile->close();
        delete tmFile;
        code = nullptr;
        free(codefile);
    }
#endif
#endif
#endif

    /* ---- 清理资源 ---- */
    srcFile->close();
    delete srcFile;

    return 0;
}
