/****************************************************/
/* 文件: util.h                                      */
/* TINY 编译器的工具函数接口                         */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#ifndef _UTIL_H_
#define _UTIL_H_

/* printToken : 将词法单元及其词素打印到列表文件
 * 参数 token      : 要打印的词法单元类型
 * 参数 tokenString: 该词法单元对应的词素字符串
 */
void printToken(TokenType token, const char* tokenString);

/* newStmtNode : 创建一个新的语句节点，用于构建语法树
 * 参数 kind: 语句子类型（IfK / RepeatK / AssignK / ReadK / WriteK）
 * 返回值  : 指向新创建的 TreeNode 的指针
 */
TreeNode* newStmtNode(StmtKind kind);

/* newExpNode : 创建一个新的表达式节点，用于构建语法树
 * 参数 kind: 表达式子类型（OpK / ConstK / IdK）
 * 返回值  : 指向新创建的 TreeNode 的指针
 */
TreeNode* newExpNode(ExpKind kind);

/* copyString : 分配内存并复制一个字符串
 * 参数 s: 要复制的源字符串
 * 返回值: 新分配内存中的字符串副本
 */
char* copyString(const char* s);

/* printTree : 将语法树以缩进形式打印到列表文件
 * 参数 tree: 要打印的语法树根节点
 * 使用缩进层级表示子树的嵌套关系
 */
void printTree(TreeNode* tree);

#endif
