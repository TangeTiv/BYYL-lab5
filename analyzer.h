/****************************************************/
/* 文件: analyzer.h                                  */
/* TINY 编译器的语义分析器接口                       */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#ifndef _ANALYZE_H_
#define _ANALYZE_H_

/* buildSymtab : 通过前序遍历语法树构建符号表
 * 参数 syntaxTree: 语法树的根节点
 * 遍历过程中将所有标识符插入符号表，
 * 为每个新变量分配内存位置
 */
void buildSymtab(TreeNode* syntaxTree);

/* typeCheck : 通过后序遍历语法树进行类型检查
 * 参数 syntaxTree: 语法树的根节点
 * 检查所有表达式和语句的类型正确性，
 * 发现类型错误时报告错误信息
 */
void typeCheck(TreeNode* syntaxTree);

#endif
