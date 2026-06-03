/****************************************************/
/* 文件: code.cpp                                    */
/* TM 代码生成工具的实现                             */
/* TINY 编译器（生成 TM 虚拟机代码）                 */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"
#include "code.h"

/* emitLoc : 当前代码发射位置（即当前指令的地址） */
static int emitLoc = 0;

/* highEmitLoc : 已发射的最高代码位置
 * 与 emitSkip、emitBackup、emitRestore 配合使用，
 * 用于实现地址回填（backpatching）机制。
 */
static int highEmitLoc = 0;

/* ================================================================
 * resetCodeState : 重置 TM 代码发射器状态（用于 GUI 多次编译）
 * ================================================================ */
void resetCodeState(void)
{
    emitLoc = 0;
    highEmitLoc = 0;
}

/* ================================================================
 * emitComment : 在代码文件中写入注释行
 *
 * TM 汇编中注释以 '*' 开头。
 * 仅在 TraceCode 为 TRUE 时输出。
 *
 * 参数 c: 注释内容字符串
 * ================================================================ */
void emitComment(const char* c)
{
    if (TraceCode)
        *code << "* " << c << std::endl;
}

/* ================================================================
 * emitRO : 发射一条寄存器到寄存器（Register-Only）的 TM 指令
 *
 * 格式: 地址:  操作码  目标寄存器,源寄存器1,源寄存器2
 * 示例:  12:    ADD      0,1,0
 *
 * 参数 op: 操作码（如 "ADD"、"SUB" 等）
 * 参数 r : 目标寄存器
 * 参数 s : 第一个源寄存器
 * 参数 t : 第二个源寄存器
 * 参数 c : 注释字符串
 * ================================================================ */
void emitRO(const char* op, int r, int s, int t, const char* c)
{
    *code << emitLoc << ":  " << op << "  "
          << r << "," << s << "," << t;
    if (TraceCode) *code << "\t" << c;
    *code << std::endl;
    emitLoc++;
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}

/* ================================================================
 * emitRM : 发射一条寄存器到内存（Register-to-Memory）的 TM 指令
 *
 * 格式: 地址:  操作码  寄存器,偏移量(基址寄存器)
 * 示例:  10:    LD      0,3(5)    -- 加载变量值到累加器
 *
 * 参数 op: 操作码（如 "LD"、"ST" 等）
 * 参数 r : 目标寄存器
 * 参数 d : 偏移量（displacement）
 * 参数 s : 基址寄存器
 * 参数 c : 注释字符串
 * ================================================================ */
void emitRM(const char* op, int r, int d, int s, const char* c)
{
    *code << emitLoc << ":  " << op << "  "
          << r << "," << d << "(" << s << ")";
    if (TraceCode) *code << "\t" << c;
    *code << std::endl;
    emitLoc++;
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}

/* ================================================================
 * emitSkip : 跳过指定数量的代码位置
 *
 * 用于在不确定目标地址时预留空间，
 * 后续通过 emitBackup 和 emitRestore 来填补。
 * 这是实现条件跳转和循环的关键机制。
 *
 * 参数 howMany: 要跳过的位置数量
 * 返回值     : 跳过的起始位置（即当前 emitLoc 的值）
 * ================================================================ */
int emitSkip(int howMany)
{
    int i = emitLoc;
    emitLoc += howMany;
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
    return i;
}

/* ================================================================
 * emitBackup : 将当前代码位置回退到之前跳过的位置
 *
 * 用于回填（backpatch）之前通过 emitSkip 预留的地址。
 * 例如：在生成 IF 语句时，先跳过条件跳转指令的位置，
 * 在确定跳转目标地址后，回退到跳转指令处填入正确的地址。
 *
 * 参数 loc: 要回退到的目标位置
 * ================================================================ */
void emitBackup(int loc)
{
    if (loc > highEmitLoc)
        emitComment("BUG 在 emitBackup 中");
    emitLoc = loc;
}

/* ================================================================
 * emitRestore : 将当前代码位置恢复到最高未发射位置
 *
 * 与 emitBackup 配对使用，完成地址回填后，
 * 将 emitLoc 恢复到之前发射到的最高位置，
 * 以便继续向后生成代码。
 * ================================================================ */
void emitRestore(void)
{
    emitLoc = highEmitLoc;
}

/* ================================================================
 * emitRM_Abs : 发射一条 PC 相对寻址的寄存器到内存 TM 指令
 *
 * 将绝对地址转换为相对于程序计数器（PC）的偏移量。
 * TM 机使用 PC 相对寻址，因此需要计算：
 *   偏移量 = 绝对地址 - (当前地址 + 1)
 * 其中 (当前地址 + 1) 是执行该指令时 PC 的值。
 *
 * 参数 op: 操作码
 * 参数 r : 目标寄存器
 * 参数 a : 绝对内存地址（跳转目标）
 * 参数 c : 注释字符串
 * ================================================================ */
void emitRM_Abs(const char* op, int r, int a, const char* c)
{
    *code << emitLoc << ":  " << op << "  "
          << r << "," << a - (emitLoc + 1) << "(" << pc << ")";
    emitLoc++;
    if (TraceCode) *code << "\t" << c;
    *code << std::endl;
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}
