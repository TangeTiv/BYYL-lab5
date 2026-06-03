/****************************************************/
/* 文件: util.cpp                                    */
/* TINY 编译器的工具函数实现                         */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"
#include "util.h"

/* ================================================================
 * printToken : 将词法单元及其词素打印到列表文件
 *
 * 参数 token      : 要打印的词法单元类型
 * 参数 tokenString: 该词法单元对应的词素字符串
 *
 * 对不同类别的词法单元采用不同的输出格式：
 *   - 保留字  → "reserved word: xxx"
 *   - 特殊符号 → 直接输出符号本身
 *   - NUM     → "NUM, val= xxx"
 *   - ID      → "ID, name= xxx"
 *   - ERROR   → "ERROR: xxx"
 * ================================================================ */
void printToken(TokenType token, const char* tokenString)
{
    switch (token)
    {
        /* ---- 保留字（关键字）---- */
        case IF:
        case THEN:
        case ELSE:
        case END:
        case REPEAT:
        case UNTIL:
        case READ:
        case WRITE:
            *listing << "保留字: " << tokenString << std::endl;
            break;

        /* ---- 特殊符号 ---- */
        case ASSIGN: *listing << ":=" << std::endl; break;
        case LT:     *listing << "<"  << std::endl; break;
        case EQ:     *listing << "="  << std::endl; break;
        case LPAREN: *listing << "("  << std::endl; break;
        case RPAREN: *listing << ")"  << std::endl; break;
        case SEMI:   *listing << ";"  << std::endl; break;
        case PLUS:   *listing << "+"  << std::endl; break;
        case MINUS:  *listing << "-"  << std::endl; break;
        case TIMES:  *listing << "*"  << std::endl; break;
        case OVER:   *listing << "/"  << std::endl; break;
        case ENDFILE:*listing << "EOF" << std::endl; break;

        /* ---- 数字常量 ---- */
        case NUM:
            *listing << "数字, 值= " << tokenString << std::endl;
            break;

        /* ---- 标识符 ---- */
        case ID:
            *listing << "标识符, 名称= " << tokenString << std::endl;
            break;

        /* ---- 错误标记 ---- */
        case ERROR:
            *listing << "错误: " << tokenString << std::endl;
            break;

        /* ---- 未知类型（不应发生）---- */
        default:
            *listing << "未知词法单元: " << (int)token << std::endl;
            break;
    }
}

/* ================================================================
 * newStmtNode : 创建一个新的语句节点
 *
 * 分配一个 TreeNode 结构体，初始化各字段：
 *   - 所有子节点指针置为 NULL
 *   - 兄弟节点指针置为 NULL
 *   - nodekind 设为 StmtK（语句节点）
 *   - 设置语句子类型（IfK / RepeatK / AssignK / ReadK / WriteK）
 *   - 记录当前行号
 *
 * 参数 kind: 语句子类型
 * 返回值  : 指向新节点指针，内存不足时返回 NULL
 * ================================================================ */
TreeNode* newStmtNode(StmtKind kind)
{
    TreeNode* t = new TreeNode();
    if (t == NULL)
        *listing << "内存不足！错误发生在行 " << lineno << std::endl;
    else
    {
        for (int i = 0; i < MAXCHILDREN; i++)
            t->child[i] = NULL;
        t->sibling     = NULL;
        t->nodekind    = StmtK;
        t->kind.stmt   = kind;
        t->lineno      = lineno;
    }
    return t;
}

/* ================================================================
 * newExpNode : 创建一个新的表达式节点
 *
 * 分配一个 TreeNode 结构体，初始化各字段：
 *   - 所有子节点指针置为 NULL
 *   - 兄弟节点指针置为 NULL
 *   - nodekind 设为 ExpK（表达式节点）
 *   - 设置表达式子类型（OpK / ConstK / IdK）
 *   - 记录当前行号
 *   - 类型初始化为 Void
 *
 * 参数 kind: 表达式子类型
 * 返回值  : 指向新节点指针，内存不足时返回 NULL
 * ================================================================ */
TreeNode* newExpNode(ExpKind kind)
{
    TreeNode* t = new TreeNode();
    if (t == NULL)
        *listing << "内存不足！错误发生在行 " << lineno << std::endl;
    else
    {
        for (int i = 0; i < MAXCHILDREN; i++)
            t->child[i] = NULL;
        t->sibling   = NULL;
        t->nodekind  = ExpK;
        t->kind.exp  = kind;
        t->lineno    = lineno;
        t->type      = Void;    // 表达式类型初始化为 Void
    }
    return t;
}

/* ================================================================
 * copyString : 复制一个字符串到新分配的内存中
 *
 * 参数 s: 要复制的源字符串
 * 返回值: 新分配内存中的字符串副本
 *         如果 s 为 NULL，返回 NULL
 * ================================================================ */
char* copyString(const char* s)
{
    if (s == NULL) return NULL;
    int n = strlen(s) + 1;
    char* t = new char[n];
    if (t == NULL)
        *listing << "内存不足！错误发生在行 " << lineno << std::endl;
    else
        strcpy(t, s);
    return t;
}

/* ================================================================
 * 语法树打印功能
 *
 * indentno : 当前缩进空格数（用于 printTree 显示树形结构）
 * ================================================================ */
static int indentno = 0;    // 当前缩进级别（原版 C 代码此处漏掉了类型！）

/* INDENT / UNINDENT : 增加/减少缩进层级 */
#define INDENT     indentno += 2
#define UNINDENT   indentno -= 2

/* printSpaces : 按当前缩进级别打印空格 */
static void printSpaces(void)
{
    for (int i = 0; i < indentno; i++)
        *listing << " ";
}

/* ================================================================
 * printTree : 将语法树以缩进形式打印到列表文件
 *
 * 使用缩进来表示子树嵌套关系（类似于目录树结构）。
 * 遍历方式：先打印当前节点，然后递归打印所有子节点，
 * 最后继续处理兄弟节点。
 *
 * 参数 tree: 要打印的语法树根节点
 *
 * 输出示例：
 *   If
 *     Op: <
 *       Const: 0
 *     ...
 * ================================================================ */
void printTree(TreeNode* tree)
{
    INDENT;  // 进入下一层，增加缩进

    while (tree != NULL)
    {
        printSpaces();

        /* ---- 处理语句节点 ---- */
        if (tree->nodekind == StmtK)
        {
            switch (tree->kind.stmt)
            {
                case IfK:     *listing << "If"     << std::endl; break;
                case RepeatK: *listing << "Repeat" << std::endl; break;
                case AssignK: *listing << "赋值给: " << tree->attr.name << std::endl; break;
                case ReadK:   *listing << "读入: "   << tree->attr.name << std::endl; break;
                case WriteK:  *listing << "写出"    << std::endl; break;
                default:
                    *listing << "未知语句节点类型" << std::endl;
                    break;
            }
        }
        /* ---- 处理表达式节点 ---- */
        else if (tree->nodekind == ExpK)
        {
            switch (tree->kind.exp)
            {
                case OpK:
                    *listing << "运算符: ";
                    printToken(tree->attr.op, "\0");
                    break;
                case ConstK:
                    *listing << "常量: " << tree->attr.val << std::endl;
                    break;
                case IdK:
                    *listing << "标识符: " << tree->attr.name << std::endl;
                    break;
                default:
                    *listing << "未知表达式节点类型" << std::endl;
                    break;
            }
        }
        else
        {
            *listing << "未知节点类型" << std::endl;
        }

        /* 递归打印所有子节点（最多 MAXCHILDREN 个）*/
        for (int i = 0; i < MAXCHILDREN; i++)
            printTree(tree->child[i]);

        /* 继续处理下一个兄弟节点 */
        tree = tree->sibling;
    }

    UNINDENT;  // 返回上一层，减少缩进
}
