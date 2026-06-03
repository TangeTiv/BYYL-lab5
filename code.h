/****************************************************/
/* 文件: code.h                                      */
/* TINY 编译器的代码生成工具接口                     */
/* 以及与 TM 虚拟机的接口定义                        */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#ifndef _CODE_H_
#define _CODE_H_

/* ================================================================
 * TM 虚拟机寄存器编号定义
 *
 * pc  (寄存器 7) : 程序计数器（Program Counter）
 *     指向当前执行的指令地址
 *
 * mp  (寄存器 6) : 内存指针（Memory Pointer）
 *     指向内存顶部，用于临时存储
 *
 * gp  (寄存器 5) : 全局指针（Global Pointer）
 *     指向内存底部，用于全局变量存储
 *
 * ac  (寄存器 0) : 累加器（Accumulator）
 *     主要运算寄存器
 *
 * ac1 (寄存器 1) : 第二累加器
 *     辅助运算寄存器
 * ================================================================ */
#define pc  7
#define mp  6
#define gp  5
#define ac  0
#define ac1 1

/* emitComment : 在代码文件中写入注释行
 * 参数 c: 注释内容字符串
 * 仅在 TraceCode 为 TRUE 时输出
 */
void emitComment(const char* c);

/* emitRO : 生成一条寄存器到寄存器的 TM 指令
 * 参数 op: 操作码（如 ADD、SUB 等）
 * 参数 r : 目标寄存器
 * 参数 s : 第一个源寄存器
 * 参数 t : 第二个源寄存器
 * 参数 c : 注释字符串
 */
void emitRO(const char* op, int r, int s, int t, const char* c);

/* emitRM : 生成一条寄存器到内存的 TM 指令
 * 参数 op: 操作码
 * 参数 r : 目标寄存器
 * 参数 d : 偏移量（displacement）
 * 参数 s : 基址寄存器
 * 参数 c : 注释字符串
 */
void emitRM(const char* op, int r, int d, int s, const char* c);

/* emitSkip : 跳过指定数量的代码位置，用于后续回填（backpatching）
 * 参数 howMany: 要跳过的位置数量
 * 返回值     : 跳过的起始位置（当前代码位置）
 */
int emitSkip(int howMany);

/* emitBackup : 将当前代码位置回退到之前跳过的位置（用于回填地址）
 * 参数 loc: 要回退到的目标位置
 */
void emitBackup(int loc);

/* emitRestore : 将当前代码位置恢复到之前未发射的最高位置 */
void emitRestore(void);

/* emitRM_Abs : 发射一条 PC 相对寻址的寄存器到内存 TM 指令
 * （将绝对地址转换为相对于 PC 的偏移量）
 * 参数 op: 操作码
 * 参数 r : 目标寄存器
 * 参数 a : 绝对内存地址
 * 参数 c : 注释字符串
 */
void emitRM_Abs(const char* op, int r, int a, const char* c);

#endif
