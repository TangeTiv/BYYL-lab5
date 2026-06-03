/****************************************************/
/* 文件: scanner.h                                   */
/* TINY 编译器的词法分析器接口                       */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#ifndef _SCAN_H_
#define _SCAN_H_

/* MAXTOKENLEN : 词法单元字符串的最大长度（包含空字符 '\0'） */
#define MAXTOKENLEN 40

/* tokenString : 存储每个词法单元的词素（lexeme）文本 */
extern char tokenString[MAXTOKENLEN + 1];

/* getToken : 从源代码文件中获取下一个词法单元
 * 返回值类型为 TokenType，表示识别出的词法单元类型
 * 词素文本存储在全局变量 tokenString 中
 */
TokenType getToken(void);

/* resetScanner : 重置词法分析器状态（用于多次编译） */
void resetScanner(void);

#endif
