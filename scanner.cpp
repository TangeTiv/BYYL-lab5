/****************************************************/
/* 文件: scanner.cpp                                 */
/* TINY 编译器的词法分析器（扫描器）实现             */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "scanner.h"

/* ================================================================
 * 词法分析器的 DFA（确定性有限自动机）状态枚举
 *
 * START     : 起始状态，等待读入第一个字符
 * INASSIGN  : 正在识别赋值运算符 ":="
 * INGT      : 正在识别 ">=" / ">"
 * INCOMMENT : 正在处理注释 "{ ... }"
 * INPLUS    : 正在识别 "++" / "+"
 * INMINUS   : 正在识别 "--" / "-"
 * INNUM     : 正在识别数字
 * INID      : 正在识别标识符或保留字
 * INLT      : 正在识别 "<" / "<=" / "<>"
 * DONE      : 当前词法单元识别完成
 * ================================================================ */
typedef enum
   { START, INASSIGN, INCOMMENT, INNUM, INID, DONE,
    INGT, INPLUS, INMINUS, INLT
}
   StateType;

/* tokenString : 存储标识符或保留字的词素 */
char tokenString[MAXTOKENLEN + 1];

/* BUFLEN : 源代码行输入缓冲区的长度 */
#define BUFLEN 256

/* ================================================================
 * 行缓冲区及相关变量
 * lineBuf  : 存储从源文件读取的当前行
 * linepos  : 当前在 lineBuf 中的位置
 * bufsize  : 当前缓冲区字符串的实际大小
 * EOF_flag : 标记是否已遇到文件结束，用于修正 ungetNextChar 的行为
 * ================================================================ */
static char lineBuf[BUFLEN];    // 保存当前行的缓冲区
static int  linepos  = 0;       // 当前在 lineBuf 中的位置
static int  bufsize  = 0;       // 当前缓冲区的字符串长度
static int  EOF_flag = FALSE;   // 是否已遇到 EOF

/* ================================================================
 * myFgets : 自定义的 fgets 等价函数（使用 C++ 文件流）
 *
 * 从文件中读取一行，包括换行符。行为与 C 标准库的 fgets 完全一致：
 *   - 读取字符直到遇到换行符、文件结束或缓冲区满
 *   - 换行符会存入缓冲区（与 getline 不同，getline 丢弃换行符）
 *   - 自动在末尾添加 '\0'
 *
 * 参数 s     : 目标缓冲区
 * 参数 n     : 缓冲区大小
 * 参数 stream: 输入文件流
 * 返回值     : 成功返回 s，失败或到达文件末尾返回 NULL
 * ================================================================ */
static char* myFgets(char* s, int n, std::istream* stream)
{
    int i = 0;
    int c;
    while (i < n - 1)
    {
        c = stream->get();
        if (c == EOF)
        {
            if (i == 0) return NULL;  // 没有读到任何字符就遇到 EOF
            break;                     // 已经读到了字符，退出循环
        }
        s[i++] = (char)c;
        if (c == '\n') break;         // 遇到换行符就停止（换行符也存入缓冲区）
    }
    s[i] = '\0';
    return s;
}

/* ================================================================
 * getNextChar : 从 lineBuf 中获取下一个字符
 *
 * 工作流程：
 *   1. 如果 lineBuf 已耗尽（linepos >= bufsize），则读取新的一行
 *   2. 从 source 文件中读取一行到 lineBuf（使用 myFgets 保留换行符）
 *   3. 如果 EchoSource 为 TRUE，将源代码行回显到列表输出（带行号）
 *   4. 返回当前字符并将 linepos 加 1
 *   5. 如果到达文件末尾，设置 EOF_flag 并返回 EOF
 * ================================================================ */
static int getNextChar(void)
{
    if (!(linepos < bufsize))
    {
        // 当前缓冲区已读完，读取新的一行
        lineno++;
        if (myFgets(lineBuf, BUFLEN, source))
        {
            bufsize = strlen(lineBuf);   // 实际读取的字符数（含换行符）
            if (EchoSource)
                *listing << lineno << ": " << lineBuf;
            linepos = 0;
            return lineBuf[linepos++];
        }
        else
        {
            // 读取失败，说明已到文件末尾
            EOF_flag = TRUE;
            return EOF;
        }
    }
    else
    {
        return lineBuf[linepos++];
    }
}

/* ================================================================
 * ungetNextChar : 在 lineBuf 中回退一个字符
 *
 * 用于在超前读取了一个不属于当前词法单元的字符后，
 * 将该字符"放回"缓冲区，供下一次 getNextChar 读取。
 * 如果已经遇到了 EOF，则不执行回退操作。
 * ================================================================ */
static void ungetNextChar(void)
{
    if (!EOF_flag) linepos--;
}

/* ================================================================
 * resetScanner : 重置扫描器的内部状态
 *
 * 将行缓冲区、行位置、EOF 标志重置为初始状态。
 * 用于 GUI 多轮编译时，确保每次编译开始时扫描器状态一致。
 * ================================================================ */
void resetScanner(void)
{
    linepos  = 0;
    bufsize  = 0;
    EOF_flag = FALSE;
}

/* ================================================================
 * 保留字（关键字）查找表
 * 存储 TINY 语言的所有保留字及其对应的 TokenType
 * ================================================================ */
static struct
{
    const char* str;    // 保留字字符串
    TokenType   tok;    // 对应的词法单元类型
} reservedWords[MAXRESERVED]
{
    {"if",     IF},
    {"then",   THEN},
    {"else",   ELSE},
    {"end",    END},
    {"repeat", REPEAT},
    {"until",  UNTIL},
    {"read",   READ},
    {"write",  WRITE},
    {"while",  WHILE},
    {"endwhile",ENDWHILE},
    {"for",    FOR}
};

/* ================================================================
 * reservedLookup : 查找标识符是否为保留字
 *
 * 使用线性查找法遍历保留字表。
 * 如果找到匹配项，返回对应的 TokenType；
 * 否则返回 ID，表示这是一个用户定义的标识符。
 *
 * 参数 s: 要查找的标识符字符串
 * 返回值: 匹配的 TokenType（找到保留字）或 ID（未找到）
 * ================================================================ */
static TokenType reservedLookup(const char* s)
{
    for (int i = 0; i < MAXRESERVED; i++)
    {
        if (strcmp(s, reservedWords[i].str) == 0)
            return reservedWords[i].tok;
    }
    return ID;
}

/****************************************/
/*     词法分析器的主函数                */
/****************************************/

/* ================================================================
 * getToken : 从源代码中识别并返回下一个词法单元
 *
 * 使用基于 DFA 的状态机进行词法分析。
 * 完整的状态转移过程：
 *
 *   START ——> INNUM     （遇到数字）
 *   START ——> INID      （遇到字母）
 *   START ——> INASSIGN  （遇到 ':'，可能是 ":=" 或 "::="）
 *   START ——> INPLUS    （遇到 '+'，可能是 "++" 或单独 '+'）
 *   START ——> INMINUS   （遇到 '-'，可能是 "--" 或单独 '-'）
 *   START ——> INLT      （遇到 '<'，可能是 <, <=, <>）
 *   START ——> INGT      （遇到 '>'，可能是 >, >=）
 *   START ——> INCOMMENT （遇到 '{'，进入注释）
 *   START ——> DONE      （遇到其他单字符词法单元或 EOF）
 *
 *   INASSIGN ——> DONE        （读入 '=' → ":="）
 *   INASSIGN ——> ERROR  （其他字符 → 单独的 ':' 是错误）
 *
 *   INLT ——> DONE   （读入 '=' → "<="，读入 '>' → "<>"，其他 → "<"）
 *   INGT ——> DONE   （读入 '=' → ">="，其他 → ">"）
 *   INPLUS ——> DONE （读入 '+' → "++"，其他 → "+"）
 *   INMINUS ——> DONE（读入 '-' → "--"，其他 → "-"）
 *
 *   INNUM ——> DONE  （遇到非数字字符）
 *   INID  ——> DONE  （遇到非字母字符）
 *
 *   INCOMMENT ——> START（遇到 '}' 回到开始）
 *   INCOMMENT ——> DONE （遇到 EOF）
 *
 * 识别完成后，如果当前词法单元是 ID，则进一步查找是否为保留字。
 * 如果 TraceScan 为 TRUE，将词法单元信息打印到列表文件。
 *
 * 返回值: 识别出的词法单元类型
 * ================================================================ */
TokenType getToken(void)
{
    int tokenStringIndex = 0;   // tokenString 的当前写入位置
    TokenType currentToken;     // 存储将要返回的词法单元类型
    StateType state = START;    // 状态机从 START 状态开始
    int save;                   // 标志：是否将当前字符保存到 tokenString

    while (state != DONE)
    {
        int c = getNextChar();
        save = TRUE;  // 默认情况下保存字符

        switch (state)
        {
            /* ---- 起始状态 ---- */
            case START:
                if (isdigit(c))
                    state = INNUM;          // 遇到数字 → 进入数字状态
                else if (isalpha(c))
                    state = INID;           // 遇到字母 → 进入标识符状态
                else if (c == '<')
                    state = INLT;   // 遇到 '<' → 可能是 <, <=, <>
                else if (c == '>')
                    state = INGT;   // 遇到 '>' → 可能是 >, >=
                else if (c == '+')
                    state = INPLUS; // 遇到 '+' → 可能是 ++, +
                else if (c == '-')
                    state = INMINUS;// 遇到 '-' → 可能是 --, -
                else if (c == ':')
                    state = INASSIGN;       // 遇到冒号 → 可能是赋值运算符
                else if (c == ' ' || c == '\t' || c == '\n')
                    save = FALSE;           // 空白字符 → 不保存，继续
                else if (c == '{')
                {
                    save = FALSE;           // 注释开始 → 不保存
                    state = INCOMMENT;      // 进入注释状态
                }
                else
                {
                    state = DONE;           // 其他字符 → 识别完成
                    switch (c)
                    {
                        case EOF:
                            save = FALSE;
                            currentToken = ENDFILE;
                            break;
                        case '=': currentToken = EQ;     break;
                        case '*': currentToken = TIMES;  break;
                        case '/': currentToken = OVER;   break;
                        case '(': currentToken = LPAREN; break;
                        case ')': currentToken = RPAREN; break;
                        case ';': currentToken = SEMI;   break;
                        case '%': currentToken = MOD;   break;
                        case '^': currentToken = POWER;   break;
                        default:  currentToken = ERROR;  break;
                    }
                }
                break;

            /* ---- 注释状态 ---- */
            case INCOMMENT:
                save = FALSE;               // 注释内容不保存到词素
                if (c == EOF)
                {
                    state = DONE;
                    currentToken = ENDFILE;
                }
                else if (c == '}')
                    state = START;          // 注释结束，回到起始状态
                break;

            /* ---- 赋值运算符 ":=" 与正则赋值 "::=" 识别起始状态 ---- */
            case INASSIGN:

                if (c == '=')
                {
                    state = DONE;
                    currentToken = ASSIGN;  // 识别为 ":="
                }
                else
                {
                    ungetNextChar();        // 不是 '='，回退字符
                    save = FALSE;
                    currentToken = ERROR;   // 单独的 ':' 是错误
                }
                break;
            

            /* ---- 小于/小于等于/不等号识别状态 ---- */
            case INLT:
                state = DONE;
                if (c == '=')
                    currentToken = LE;    // 识别为 "<="
                else if(c == '>')
                    currentToken = NEQ;   // 识别为 "<>"（不等于）
                else
                {
                    ungetNextChar();      // 不是 '=' 也不是 '>'，回退
                    save = FALSE;
                    currentToken = LT;    // 识别为单独的 "<"
                }
                break;

            /* ---- 大于/大于等于识别状态 ---- */
            case INGT:
                state = DONE;
                if (c == '=')
                    currentToken = GE;    // 识别为 ">="
                else
                {
                    ungetNextChar();      // 不是 '='，回退
                    save = FALSE;
                    currentToken = GT;    // 识别为单独的 ">"
                }
                break;

            /* ---- 自增运算符 "++" 识别状态 ---- */
            case INPLUS:
                state = DONE;
                if (c == '+')
                    currentToken = INC;   // 识别为 "++"
                else
                {
                    ungetNextChar();      // 不是 '+'，回退
                    save = FALSE;
                    currentToken = PLUS;  // 识别为单独的 "+"
                }
                break;

            /* ---- 自减运算符 "--" 识别状态 ---- */
            case INMINUS:
                state = DONE;
                if (c == '-')
                    currentToken = DEC;   // 识别为 "--"
                else
                {
                    ungetNextChar();      // 不是 '-'，回退
                    save = FALSE;
                    currentToken = MINUS;   // 单独的 '-' 
                }
                break;

            /* ---- 数字识别状态 ---- */
            case INNUM:
                if (!isdigit(c))
                {
                    ungetNextChar();        // 不是数字，回退
                    save = FALSE;
                    state = DONE;
                    currentToken = NUM;     // 识别为数字常量
                }
                break;

            /* ---- 标识符识别状态 ---- */
            case INID:
                if (!isalpha(c))
                {
                    ungetNextChar();        // 不是字母，回退
                    save = FALSE;
                    state = DONE;
                    currentToken = ID;      // 识别为标识符
                }
                break;

            /* ---- DONE 状态（不应发生） ---- */
            case DONE:
            default:
                *listing << "词法分析器内部错误：state = " << state << std::endl;
                state = DONE;
                currentToken = ERROR;
                break;
        }

        /* 如果需要保存且 tokenString 未满，将当前字符存入 */
        if (save && tokenStringIndex <= MAXTOKENLEN)
            tokenString[tokenStringIndex++] = (char)c;

        /* 识别完成时，对 tokenString 封口，并检查是否为保留字 */
        if (state == DONE)
        {
            tokenString[tokenStringIndex] = '\0';
            if (currentToken == ID)
                currentToken = reservedLookup(tokenString);
        }
    }

    /* 如果启用了跟踪，打印词法单元信息 */
    if (TraceScan)
    {
        *listing << "\t行 " << lineno << ": ";
        printToken(currentToken, tokenString);
    }

    return currentToken;
}
