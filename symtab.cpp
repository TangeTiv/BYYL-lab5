/****************************************************/
/* 文件: symtab.cpp                                  */
/* TINY 编译器的符号表实现                           */
/* （只允许一个符号表存在）                          */
/* 符号表采用链地址法（分离链接法）哈希表实现        */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"
#include "symtab.h"

/* SIZE : 哈希表的大小（使用质数以减小冲突概率） */
#define SIZE 211

/* SHIFT : 哈希函数中使用的乘数（2 的幂） */
#define SHIFT 4

/* ================================================================
 * hash : 哈希函数
 *
 * 将字符串 key 映射到 [0, SIZE-1] 范围内的整数。
 * 算法：对每个字符，将当前值左移 SHIFT 位后加上字符 ASCII 码，
 *       再对 SIZE 取模。
 *
 * 参数 key: 要计算哈希值的字符串（变量名）
 * 返回值  : 哈希表桶索引
 * ================================================================ */
static int hash(const char* key)
{
    int temp = 0;
    for (int i = 0; key[i] != '\0'; i++)
    {
        temp = ((temp << SHIFT) + key[i]) % SIZE;
    }
    return temp;
}

/* ================================================================
 * LineListRec — 行号链表节点
 *
 * 记录变量在源代码中出现的所有行号。
 * 用于错误报告和调试信息输出。
 *
 * 字段：
 *   lineno : 行号
 *   next   : 下一个行号节点
 * ================================================================ */
typedef struct LineListRec
{
    int lineno;
    struct LineListRec* next;
} * LineList;

/* ================================================================
 * BucketListRec — 哈希表桶节点
 *
 * 存储每个变量的完整信息：
 *   name   : 变量名
 *   lines  : 该变量出现的所有行号链表
 *   memloc : 分配给该变量的内存位置
 *   next   : 指向同一个桶中的下一个变量
 * ================================================================ */
typedef struct BucketListRec
{
    char* name;
    LineList lines;
    int memloc;                     // 变量的内存位置
    struct BucketListRec* next;
} * BucketList;

/* hashTable : 哈希表（桶数组），使用静态全局变量 */
static BucketList hashTable[SIZE];

/* ================================================================
 * st_insert : 将变量插入符号表
 *
 * 如果变量尚未在表中：
 *   1. 计算哈希值找到对应的桶
 *   2. 创建新的 BucketListRec 节点
 *   3. 记录变量名、首次出现的行号和分配的内存位置
 *   4. 使用头插法将节点插入桶链表
 *
 * 如果变量已在表中：
 *   1. 在已有的行号链表末尾追加新的行号
 *   2. 忽略传入的 memloc 参数（保留首次分配的位置）
 *
 * 参数 name  : 变量名
 * 参数 lineno: 变量出现的行号
 * 参数 loc   : 内存位置（仅首次插入时有效）
 * ================================================================ */
void st_insert(const char* name, int lineno, int loc)
{
    int h = hash(name);
    BucketList l = hashTable[h];

    /* 遍历桶链表，查找是否已存在该变量 */
    while (l != NULL && strcmp(name, l->name) != 0)
        l = l->next;

    if (l == NULL)  /* 变量不在表中，创建新条目 */
    {
        l = new BucketListRec;
        l->name = new char[strlen(name) + 1];
        strcpy(l->name, name);

        l->lines = new LineListRec;
        l->lines->lineno = lineno;
        l->memloc = loc;
        l->lines->next = NULL;

        /* 头插法插入桶链表 */
        l->next = hashTable[h];
        hashTable[h] = l;
    }
    else  /* 变量已在表中，只添加行号 */
    {
        LineList t = l->lines;
        while (t->next != NULL)
            t = t->next;
        t->next = new LineListRec;
        t->next->lineno = lineno;
        t->next->next = NULL;
    }
}

/* ================================================================
 * st_lookup : 查找变量的内存位置
 *
 * 参数 name: 要查找的变量名
 * 返回值   : 变量的内存位置；如果未找到则返回 -1
 * ================================================================ */
int st_lookup(const char* name)
{
    int h = hash(name);
    BucketList l = hashTable[h];

    while (l != NULL && strcmp(name, l->name) != 0)
        l = l->next;

    if (l == NULL)
        return -1;      // 未找到
    else
        return l->memloc;
}

/* ================================================================
 * printSymTab : 将符号表内容格式化输出到指定流
 *
 * 输出格式：
 *   Variable Name  Location   Line Numbers
 *   -------------  --------   ------------
 *   x              0            1    2    5
 *   y              1            1    3
 *   ...
 *
 * 参数 listing: 输出流引用
 * ================================================================ */
void printSymTab(std::ostream& listing)
{
    listing << "变量名        内存位置   出现行号"     << std::endl;
    listing << "-------------  --------   ------------" << std::endl;

    for (int i = 0; i < SIZE; i++)
    {
        if (hashTable[i] != NULL)
        {
            BucketList l = hashTable[i];
            while (l != NULL)
            {
                listing << l->name;
                // 对齐输出
                int pad = 14 - strlen(l->name);
                for (int j = 0; j < pad; j++) listing << " ";
                listing << l->memloc;
                for (int j = 0; j < 6; j++) listing << " ";

                // 输出所有行号
                LineList t = l->lines;
                while (t != NULL)
                {
                    listing << t->lineno << " ";
                    t = t->next;
                }
                listing << std::endl;
                l = l->next;
            }
        }
    }
}
