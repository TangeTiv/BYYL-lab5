/****************************************************/
/* 文件: symtab.h                                    */
/* TINY 编译器的符号表接口                           */
/* （只允许一个符号表存在）                          */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

/* st_insert : 将变量名及其行号和内存位置插入符号表
 * 参数 name  : 变量名
 * 参数 lineno: 变量在源代码中出现的行号
 * 参数 loc   : 变量分配的内存位置（仅第一次插入时有效，后续忽略）
 * 如果变量已在表中，则只添加行号信息
 */
void st_insert(const char* name, int lineno, int loc);

/* st_lookup : 查找变量在符号表中的内存位置
 * 参数 name: 要查找的变量名
 * 返回值   : 变量的内存位置；如果未找到则返回 -1
 */
int st_lookup(const char* name);

/* printSymTab : 将符号表内容以格式化形式打印到指定文件
 * 参数 listing: 输出文件指针
 */
void printSymTab(std::ostream& listing);

#endif
