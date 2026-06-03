/****************************************************/
/* 文件: compiler_api.h                               */
/* TINY 编译器的字符串编译接口                        */
/* 为 GUI 程序提供无需文件 I/O 的编译入口            */
/****************************************************/

#ifndef _COMPILER_API_H_
#define _COMPILER_API_H_

#include <string>

/* CompileResult : 编译结果结构体
 * success       : 编译是否成功（无错误）
 * quadOutput    : 四元组中间代码文本
 * tmOutput      : TM 汇编代码文本
 * listingOutput : 编译过程消息（含错误信息）
 */
struct CompileResult {
    bool success;
    std::string quadOutput;
    std::string tmOutput;
    std::string listingOutput;
};

/* compileFromString : 从字符串编译 TINY 源程序
 * 参数 sourceCode : TINY 源代码文本
 * 返回值         : CompileResult 包含所有编译输出
 *
 * 此函数会：
 *   1. 重置所有编译器状态
 *   2. 设置 stringstream 作为输入/输出
 *   3. 执行完整的编译流水线（解析→语义分析→代码生成）
 *   4. 捕获所有输出到 CompileResult
 *   5. 恢复默认流指针
 */
CompileResult compileFromString(const std::string& sourceCode);

#endif
