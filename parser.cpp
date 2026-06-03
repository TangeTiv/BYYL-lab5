/****************************************************/
/* 文件: parser.cpp                                  */
/* TINY 编译器的语法分析器实现                       */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "scanner.h"
#include "parser.h"
#include <iostream>

/* ================================================================
 * TINY 语言的 BNF 文法（EBNF 形式）
 *
 * program      → stmt_sequence
 * stmt_sequence → statement { ";" statement }
 * statement    → if_stmt | repeat_stmt | assign_stmt | read_stmt | write_stmt | while_stmt | for_stmt 
 * if_stmt      → IF exp THEN stmt_sequence
 *                [ ELSE stmt_sequence ] END
 * while_stmt   -> WHILE (exp) stmt-sequence ENDWHILE
 * for_stmt     -> FOR (assign-stmt ; exp ;  INC| DEC) stmt-sequence END
 * repeat_stmt  → REPEAT stmt_sequence UNTIL exp
 * assign_stmt  → ID ":=" exp
 * read_stmt    → READ ID
 * write_stmt   → WRITE exp
 * exp          → simple_exp [ comparison_op simple_exp ]
 * comparison_op → "<" | "=" | "<=" | ">" | ">=" | "<>"
 * simple_exp   → term { add_op term }
 * add_op       → "+" | "-"
 * term         → power { mul_op power }
 * mul_op       → "*" | "/" | "%"
 * power        -> factor ["^"power]
 * factor       → "++" ID | "--" ID | NUM | ID | "(" exp ")"
 *
 * 该文法采用递归下降分析法（Recursive Descent Parsing），
 * 每个非终结符对应一个同名的递归函数。
 * ================================================================ */

static TokenType token;  // 保存当前词法单元（全局供所有分析函数使用）

/* resetParser : 重置语法分析器状态（用于 GUI 多次编译）*/
void resetParser(void)
{
    token = ENDFILE;
}

/* ================================================================
 * 递归下降分析函数的前向声明
 * 按照 TINY 文法的非终结符逐一声明
 * ================================================================ */
static TreeNode* stmt_sequence(void);
static TreeNode* statement(void);
static TreeNode* if_stmt(void);
static TreeNode* repeat_stmt(void);
static TreeNode* assign_stmt(void);
static TreeNode* read_stmt(void);
static TreeNode* write_stmt(void);
static TreeNode* while_stmt(void);
static TreeNode* for_stmt(void);
static TreeNode* exp(void);
static TreeNode* simple_exp(void);
static TreeNode* term(void);
static TreeNode* power(void);
static TreeNode* factor(void);
static TreeNode* regexexp(void);
static TreeNode* regex_term(void);
static TreeNode* regex_factor(void);
static TreeNode* regex_atom(void);

/* ================================================================
 * syntaxError : 报告语法错误
 *
 * 在列表文件中输出错误信息，包括出错的源代码行号。
 * 同时将全局 Error 标志设为 TRUE，阻止后续编译阶段。
 *
 * 参数 message: 错误描述信息
 * ================================================================ */
static void syntaxError(const char* message)
{
    *listing << "\n>>> ";
    *listing << "语法错误，行 " << lineno << "：" << message;
    Error = TRUE;
}

/* ================================================================
 * match : 匹配当前词法单元
 *
 * 如果当前词法单元与期望的类型一致，则读取下一个词法单元；
 * 否则报告语法错误。
 *
 * 这是递归下降分析中最基本的匹配操作，
 * 用于确认当前输入确实包含期望的词法单元。
 *
 * 参数 expected: 期望的词法单元类型
 * ================================================================ */
static void match(TokenType expected)
{
    if (token == expected)
        token = getToken();
    else
    {
        syntaxError("不期望的词法单元 -> ");
        printToken(token, tokenString);
        *listing << "      ";
    }
}




/* ================================================================
 * stmt_sequence : 分析语句序列
 *
 * 文法: stmt_sequence → statement { ";" statement }
 *
 * 分析一个或多个由分号分隔的语句。
 * 所有语句通过 sibling（兄弟）指针链接成链表。
 * 遇到 ENDFILE、END、ELSE 或 UNTIL 时停止。
 *
 * 返回值: 语句序列链表的头节点
 * ================================================================ */
TreeNode* stmt_sequence(void)
{
    

    TreeNode* t = statement();       // 第一条语句
    TreeNode* p = t;

    while (token != ENDFILE && token != END &&
           token != ELSE && token != UNTIL &&
           token != ENDWHILE)
    {
        match(SEMI);                 // 匹配分号
        TreeNode* q = statement();   // 下一条语句
        if (q != NULL)
        {
            if (t == NULL)
                t = p = q;          // 第一条语句
            else
            {
                p->sibling = q;     // 通过 sibling 链接
                p = q;
            }
        }
    }
    return t;
}

/* ================================================================
 * statement : 分析单个语句
 *
 * 文法: statement → if_stmt | repeat_stmt | assign_stmt
 *                 | read_stmt | write_stmt
 *
 * 根据当前词法单元决定使用哪个产生式：
 *   IF     → if_stmt
 *   REPEAT → repeat_stmt
 *   ID     → assign_stmt
 *   READ   → read_stmt
 *   WRITE  → write_stmt
 *   其他   → 语法错误
 *
 * 返回值: 指向语句节点的指针
 * ================================================================ */
TreeNode* statement(void)
{
    TreeNode* t = NULL;
    switch (token)
    {
        case IF:     t = if_stmt();      break;
        case REPEAT: t = repeat_stmt();  break;
        case ID:     t = assign_stmt();  break;
        case READ:   t = read_stmt();    break;
        case WRITE:  t = write_stmt();   break;
        case FOR:    t = for_stmt();   break;
        case WHILE:  t = while_stmt();   break;
        case INC:                        // ++x; / ++x
        case DEC:                        // --x; / --x
            {
                TreeNode* p = newExpNode(OpK);
                if (p != NULL) p->attr.op = token;
                match(token);
                TreeNode* q = newExpNode(IdK);
                if (q != NULL && token == ID)
                    q->attr.name = copyString(tokenString);
                match(ID);
                if (p != NULL) p->child[0] = q;
                t = p;
            }
            break;
        default:
            syntaxError("不期望的词法单元 -> ");
            printToken(token, tokenString);
            token = getToken();
            break;
    }
    return t;
}

TreeNode* for_stmt(void)
{
    TreeNode* t = newStmtNode(ForK);
    match(FOR);
    match(LPAREN);
    if (t != NULL) t->child[0] = assign_stmt();            // 循环变量赋初值
    match(SEMI);
    if (t != NULL) t->child[1] = exp();                    // 循环条件
    match(SEMI);
    /* 步进可为赋值语句（如 x := x+1）或自增自减（如 ++x / --x） */
    if (t != NULL)
    {
        if (token == INC || token == DEC)
            t->child[2] = factor();                        // ++x / --x
        else
            t->child[2] = assign_stmt();                   // x := exp
    }
    match(RPAREN);
    if (t != NULL) t->child[3] = stmt_sequence();
    match(END);
    return t;
}

TreeNode* while_stmt(void)
{
    TreeNode* t = newStmtNode(WhileK);
    match(WHILE);
    match(LPAREN);
    if (t != NULL) t->child[0] = exp();
    match(RPAREN);
    if (t != NULL) t->child[1] = stmt_sequence();
    match(ENDWHILE);
    return t;
}









TreeNode* power()
{
    TreeNode* t = factor();
    if (token == POWER)
    {
        TreeNode* p = newExpNode(OpK);
        if(p != NULL)
        {
            p->child[0] = t;
            p->attr.op = token;
            t = p;
        }
        match(token);
        if(t != NULL)
        {
            t->child[1] = power();
        }
    }
    return t;
}

/* ================================================================
 * if_stmt : 分析 IF 语句
 *
 * 文法: if_stmt → IF exp THEN stmt_sequence
 *                 [ ELSE stmt_sequence ] END
 *
 * 语法树结构（IfK 节点）：
 *   child[0] = 条件表达式（exp）
 *   child[1] = THEN 分支的语句序列
 *   child[2] = ELSE 分支的语句序列（可选）
 *
 * 返回值: 指向 IfK 节点的指针
 * ================================================================ */
TreeNode* if_stmt(void)
{
    OpKind op;
    std::string str;
    OperandsNode x,y,z;
    TreeNode* t = newStmtNode(IfK);
    match(IF);
    if (t != NULL) t->child[0] = exp();            // 条件表达式
    match(THEN);
    if (t != NULL) t->child[1] = stmt_sequence();  // THEN 分支

    if (token == ELSE)                              // 可选的 ELSE 分支
    {
        match(ELSE);
        if (t != NULL) t->child[2] = stmt_sequence();
    }
    match(END);
    return t;
}

/* ================================================================
 * repeat_stmt : 分析 REPEAT 语句
 *
 * 文法: repeat_stmt → REPEAT stmt_sequence UNTIL exp
 *
 * 语法树结构（RepeatK 节点）：
 *   child[0] = 循环体语句序列
 *   child[1] = 循环条件表达式
 *
 * 语义：重复执行循环体，直到条件为真。
 *
 * 返回值: 指向 RepeatK 节点的指针
 * ================================================================ */
TreeNode* repeat_stmt(void)
{
    TreeNode* t = newStmtNode(RepeatK);
    match(REPEAT);
    if (t != NULL) t->child[0] = stmt_sequence();  // 循环体
    match(UNTIL);
    if (t != NULL) t->child[1] = exp();             // 循环条件
    return t;
}

/* ================================================================
 * assign_stmt : 分析赋值语句
 *
 * 文法: assign_stmt → ID ":=" exp
 *
 * 语法树结构（AssignK 节点）：
 *   attr.name = 被赋值的变量名
 *   child[0]  = 右侧表达式
 *
 * 返回值: 指向 AssignK 节点的指针
 * ================================================================ */
TreeNode* assign_stmt(void)
{
    char * temp = NULL;
    TreeNode* t = NULL;
    if (token == ID)
    temp = copyString(tokenString);    // 保存变量名
    match(ID);
    if(token == ASSIGN)
    {
        t = newStmtNode(AssignK);
        if(t != NULL)
        {
            t->attr.name = temp;
        }
        match(ASSIGN);
        if (t!=NULL) t->child[0] = exp();
    }
    else
    {
        syntaxError("不期望的词法单元 -> ");
        printToken(token, tokenString);
        *listing << "      ";
    }
    return t;
}

/* ================================================================
 * read_stmt : 分析 READ 语句
 *
 * 文法: read_stmt → READ ID
 *
 * 语法树结构（ReadK 节点）：
 *   attr.name = 要读入的变量名
 *
 * 语义：从输入读取一个整数值并存入指定变量。
 *
 * 返回值: 指向 ReadK 节点的指针
 * ================================================================ */
TreeNode* read_stmt(void)
{
    TreeNode* t = newStmtNode(ReadK);
    match(READ);
    if (t != NULL && token == ID)
        t->attr.name = copyString(tokenString);    // 保存变量名
    match(ID);
    return t;
}

/* ================================================================
 * write_stmt : 分析 WRITE 语句
 *
 * 文法: write_stmt → WRITE exp
 *
 * 语法树结构（WriteK 节点）：
 *   child[0] = 要输出的表达式
 *
 * 语义：计算表达式的值并输出到屏幕。
 *
 * 返回值: 指向 WriteK 节点的指针
 * ================================================================ */
TreeNode* write_stmt(void)
{
    TreeNode* t = newStmtNode(WriteK);
    match(WRITE);
    if (t != NULL) t->child[0] = exp();    // 要输出的表达式
    return t;
}

/* ================================================================
 * exp : 分析比较表达式
 *
 * 文法: exp → simple_exp [ ("<" | "=") simple_exp ]
 *
 * 如果包含比较运算符，创建一个 OpK 节点：
 *   child[0] = 左侧 simple_exp
 *   attr.op  = 比较运算符（LT 或 EQ）
 *   child[1] = 右侧 simple_exp
 *
 * 返回值: 指向表达式节点的指针
 * ================================================================ */
TreeNode* exp(void)
{
    TreeNode* t = simple_exp();
    if (token == LT || token == EQ || token == LE || token == GE || token == GT || token == NEQ)
    {
        TreeNode* p = newExpNode(OpK);
        if (p != NULL)
        {
            p->child[0] = t;
            p->attr.op  = token;
            t = p;
        }
        match(token);
        if (t != NULL)
            t->child[1] = simple_exp();
    }
    return t;
}

/* ================================================================
 * simple_exp : 分析算术表达式（加减）
 *
 * 文法: simple_exp → term { ("+" | "-") term }
 *
 * 左结合处理：遇到 "+" 或 "-" 时，
 * 将当前已构建的表达式作为左操作数，
 * 新建 OpK 节点，递归处理右操作数。
 *
 * 返回值: 指向表达式节点的指针
 * ================================================================ */
TreeNode* simple_exp(void)
{
    TreeNode* t = term();
    while (token == PLUS || token == MINUS)
    {
        TreeNode* p = newExpNode(OpK);
        if (p != NULL)
        {
            p->child[0] = t;
            p->attr.op  = token;
            t = p;
            match(token);
            t->child[1] = term();
        }
    }
    return t;
}

/* ================================================================
 * term : 分析项（乘除）
 *
 * 文法: term → factor { ("*" | "/") factor }
 *
 * 左结合处理（与 simple_exp 类似）。
 *
 * 返回值: 指向表达式节点的指针
 * ================================================================ */
TreeNode* term(void)
{
    TreeNode* t = power();
    while (token == TIMES || token == OVER  || token == MOD)
    {
        TreeNode* p = newExpNode(OpK);
        if (p != NULL)
        {
            p->child[0] = t;
            p->attr.op  = token;
            t = p;
            match(token);
            p->child[1] = power();
        }
    }
    return t;
}

/* ================================================================
 * factor : 分析因子
 *
 * 文法: factor → NUM | ID | "(" exp ")"
 *
 * 三种情况：
 *   NUM     → 创建 ConstK 节点，保存数值
 *   ID      → 创建 IdK 节点，保存标识符名
 *   "("exp")" → 递归分析 exp，括号本身不创建节点
 *
 * 返回值: 指向表达式节点的指针
 * ================================================================ */
TreeNode* factor(void)
{
    TreeNode* t = NULL;
    TreeNode* p = NULL;
    switch (token)
    {
        case NUM:                          // 数字常量
            t = newExpNode(ConstK);
            if (t != NULL && token == NUM)
                t->attr.val = atoi(tokenString);
            match(NUM);
            break;
        case INC:                          // 数字常量
            t = newExpNode(OpK);
            if (t != NULL && token == INC)
                t->attr.op = token;
            match(INC);
            p = newExpNode(IdK);
            if (p != NULL && token == ID)
                p->attr.name = copyString(tokenString);
            match(ID);
            if(t != NULL) t->child[0] = p;
            break;
        case DEC:                          // 数字常量
            t = newExpNode(OpK);
            if (t != NULL && token == DEC)
                t->attr.op = token;
            match(DEC);
            p = newExpNode(IdK);
            if (p != NULL && token == ID)
                p->attr.name = copyString(tokenString);
            match(ID);
            if(t != NULL) t->child[0] = p;
            break;
        case ID:                           // 标识符
            t = newExpNode(IdK);
            if (t != NULL && token == ID)
                t->attr.name = copyString(tokenString);
            match(ID);
            break;

        case LPAREN:                       // 括号表达式
            match(LPAREN);
            t = exp();
            match(RPAREN);
            break;

        default:                           // 语法错误
            syntaxError("不期望的词法单元 -> ");
            printToken(token, tokenString);
            token = getToken();
            break;
    }
    return t;
}

/****************************************/
/*     语法分析器的主函数                */
/****************************************/

/* ================================================================
 * parse : 对源代码进行完整的语法分析，构建语法树
 *
 * 处理流程：
 *   1. 调用 getToken() 获取第一个词法单元
 *   2. 从 stmt_sequence() 开始递归下降分析
 *   3. 检查是否已到达文件末尾（ENDFILE）
 *   4. 返回构建完成的语法树根节点
 *
 * 返回值: 语法树的根节点指针
 * ================================================================ */
TreeNode* parse(void)
{
    TreeNode* t;
    token = getToken();          // 初始化：读取第一个词法单元
    t = stmt_sequence();         // 从语句序列开始分析
    if (token != ENDFILE)        // 检查是否完整消耗了所有输入
        syntaxError("文件结束前还有代码\n");
    return t;
}
