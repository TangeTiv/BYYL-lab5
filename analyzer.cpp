/****************************************************/
/* 文件: analyzer.cpp                                */
/* TINY 编译器的语义分析器实现                       */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyzer.h"

/* ================================================================
 * 内存位置计数器
 * 用于为每个新遇到的变量分配一个递增的内存槽位。
 * 每个变量在 TM 虚拟机的数据区中占据一个字（word）。
 * ================================================================ */
static int location = 0;

/* ================================================================
 * traverse : 通用的递归语法树遍历函数
 *
 * 对以 t 为根的语法树进行遍历，在每个节点上依次执行：
 *   1. 前序处理函数 preProc（在访问子节点之前调用）
 *   2. 递归遍历所有子节点（child[0..MAXCHILDREN-1]）
 *   3. 后序处理函数 postProc（在访问子节点之后调用）
 *   4. 递归遍历兄弟节点（sibling）
 *
 * 这种设计允许通过传入不同的函数指针来实现不同的遍历需求：
 *   - buildSymtab : preProc = insertNode, postProc = nullProc
 *   - typeCheck   : preProc = nullProc, postProc = checkNode
 *
 * 参数 t        : 当前遍历的节点
 * 参数 preProc  : 前序处理函数指针
 * 参数 postProc : 后序处理函数指针
 * ================================================================ */
static void traverse(TreeNode* t,
                     void (*preProc)(TreeNode*),
                     void (*postProc)(TreeNode*))
{
    if (t != NULL)
    {
        preProc(t);                      // 前序处理

        for (int i = 0; i < MAXCHILDREN; i++)
            traverse(t->child[i], preProc, postProc);  // 递归子节点

        postProc(t);                     // 后序处理
        traverse(t->sibling, preProc, postProc);       // 递归兄弟节点
    }
}

/* ================================================================
 * nullProc : 空操作函数
 *
 * 作为 traverse 的参数使用。
 * 当只需要前序遍历（如构建符号表）时，
 * 将后序处理设为 nullProc；
 * 当只需要后序遍历（如类型检查）时，
 * 将前序处理设为 nullProc。
 * ================================================================ */
static void nullProc(TreeNode* t)
{
    if (t == NULL) return;
}

/* ================================================================
 * insertNode : 将节点中的标识符插入符号表
 *
 * 对语句节点中的 AssignK 和 ReadK，
 * 以及表达式节点中的 IdK 进行处理：
 *   - 如果变量不在表中：分配新的内存位置并插入
 *   - 如果变量已在表中：只添加行号信息，不改变内存位置
 *
 * 参数 t: 当前正在处理的语法树节点
 * ================================================================ */
static void insertNode(TreeNode* t)
{
    switch (t->nodekind)
    {
        case StmtK:
            switch (t->kind.stmt)
            {
                case AssignK:
                case ReadK:
                    // 赋值和读入语句中涉及的变量
                    if (st_lookup(t->attr.name) == -1)
                    {
                        // 不在表中 ⇒ 视为新定义，分配位置
                        st_insert(t->attr.name, t->lineno, location++);
                    }
                    else
                    {
                        // 已在表中 ⇒ 只添加行号，忽略位置参数
                        st_insert(t->attr.name, t->lineno, 0);
                    }
                    break;
                default:
                    break;
            }
            break;

        case ExpK:
            switch (t->kind.exp)
            {
                case IdK:
                    // 表达式中的标识符引用
                    if (st_lookup(t->attr.name) == -1)
                    {
                        st_insert(t->attr.name, t->lineno, location++);
                    }
                    else
                    {
                        st_insert(t->attr.name, t->lineno, 0);
                    }
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

/* ================================================================
 * buildSymtab : 构建符号表
 *
 * 通过前序遍历语法树（preorder traversal），
 * 将所有遇到的标识符插入符号表并分配内存位置。
 * 如果 TraceAnalyze 为 TRUE，输出符号表内容。
 *
 * 参数 syntaxTree: 语法树的根节点
 * ================================================================ */
void buildSymtab(TreeNode* syntaxTree)
{
    // 前序遍历：进入节点时插入符号表，离开时不做处理
    traverse(syntaxTree, insertNode, nullProc);

    if (TraceAnalyze)
    {
        *listing << "\n符号表：" << std::endl;
        printSymTab(*listing);
    }
}

/* ================================================================
 * typeError : 报告类型错误
 *
 * 参数 t      : 发生类型错误的节点
 * 参数 message: 错误描述信息
 * ================================================================ */
static void typeError(TreeNode* t, const char* message)
{
    *listing << "类型错误，行 " << t->lineno << "：" << message << std::endl;
    Error = TRUE;
}

/* ================================================================
 * checkNode : 在单个节点上执行类型检查
 *
 * 检查规则：
 *   运算符节点（OpK）:
 *     - 两个操作数必须为 Integer
 *     - 比较运算符（EQ, LT）的结果为 Boolean
 *     - 算术运算符（PLUS, MINUS, TIMES, OVER）的结果为 Integer
 *
 *   常量节点（ConstK）: 类型为 Integer
 *   标识符节点（IdK） : 类型为 Integer（TINY 只有整数类型）
 *
 *   IF 语句: 条件表达式不能为 Integer（应为 Boolean）
 *   Assign 语句: 右侧表达式必须为 Integer
 *   Write 语句: 表达式必须为 Integer
 *   Repeat 语句: 循环条件不能为 Integer（应为 Boolean）
 *
 * 参数 t: 要检查的语法树节点
 * ================================================================ */
static void checkNode(TreeNode* t)
{
    switch (t->nodekind)
    {
        case ExpK:
            switch (t->kind.exp)
            {
                case OpK:
                    /* 运算符节点：检查操作数类型 */
                    if (t->child[0]->type != Integer ||
                        t->child[1]->type != Integer)
                    {
                        typeError(t, "运算符应用于非整数操作数");
                    }
                    /* 确定运算结果类型 */
                    if (t->attr.op == EQ || t->attr.op == LT ||
                        t->attr.op == LE || t->attr.op == GT ||
                        t->attr.op == GE || t->attr.op == NEQ)
                        t->type = Boolean;   // 比较运算结果为布尔型
                    else
                        t->type = Integer;   // 算术运算结果为整数型
                    break;

                case ConstK:
                    t->type = Integer;       // 常量类型为整数
                    break;

                case IdK:
                    t->type = Integer;       // 标识符类型为整数
                    break;

                default:
                    break;
            }
            break;

        case StmtK:
            switch (t->kind.stmt)
            {
                case IfK:
                    /* IF 语句：条件表达式不能是整数（必须是布尔值）*/
                    if (t->child[0]->type == Integer)
                        typeError(t->child[0], "IF 条件不是布尔类型");
                    break;

                case AssignK:
                    /* 赋值语句：右侧表达式必须是整数 */
                    if (t->child[0]->type != Integer)
                        typeError(t->child[0], "赋值了非整数值");
                    break;

                case WriteK:
                    /* 输出语句：表达式必须是整数 */
                    if (t->child[0]->type != Integer)
                        typeError(t->child[0], "输出了非整数值");
                    break;

                case RepeatK:
                    /* REPEAT 循环：条件表达式不能是整数（必须是布尔值）*/
                    if (t->child[1]->type == Integer)
                        typeError(t->child[1], "REPEAT 条件不是布尔类型");
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}

/* ================================================================
 * typeCheck : 对整个语法树执行类型检查
 *
 * 通过后序遍历语法树（postorder traversal），
 * 先确定子节点的类型，再检查当前节点。
 * 这种顺序保证了在检查父节点时，子节点的类型已经确定。
 *
 * 参数 syntaxTree: 语法树的根节点
 * ================================================================ */
void typeCheck(TreeNode* syntaxTree)
{
    // 后序遍历：进入节点时不处理，离开时进行类型检查
    traverse(syntaxTree, nullProc, checkNode);
}
