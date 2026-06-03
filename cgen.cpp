/****************************************************/
/* 文件: cgen.cpp                                    */
/* TINY 编译器的代码生成器实现                       */
/* （生成 TM 虚拟机的汇编代码 + 四元组中间代码）     */
/* 编译器构造：原理与实践                            */
/* Kenneth C. Louden                                 */
/* C++ 重写版 — 添加了详细的中文注释                 */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"
#include "util.h"

/* tmpOffset : 临时变量的内存偏移量
 * 每次存储临时变量时递减，加载时递增。
 * 用于表达式求值过程中的临时结果存储。
 */
static int tmpOffset = 0;

/* 四元组生成器状态（文件作用域，以便 resetCodeGen 重置）*/
static int varOrder = 1;      // 临时变量编号 T1, T2, T3 ...
static int labelCount = 0;    // 标号编号 L1, L2, L3 ...

/* ================================================================
 * 四元组中间代码输出辅助函数
 * ================================================================ */

/* 空操作数常量（表示缺省的操作数）*/
static const OperandsNode EMPTY_OP = {Empty, {0}};

/* resetCodeGen : 重置代码生成器状态（用于 GUI 多次编译）*/
void resetCodeGen(void) {
    tmpOffset = 0;
    varOrder = 1;
    labelCount = 0;
}

/* newTemp : 生成新的临时变量名 T1, T2, T3 ...
 * 返回值 : 用 copyString 分配的字符串，调用者无需释放
 */
static const char* newTemp() {
    std::string s = "T" + std::to_string(varOrder++);
    return copyString(s.c_str());
}

/* newLabel : 生成新的标号 ID（1, 2, 3 ...）
 * 返回值 : 递增的标号编号
 */
static int newLabel() {
    return ++labelCount;
}

/* emitLabel : 输出标号 Ln:
 * 参数 id : 标号 ID
 */
static void emitLabel(int id) {
    *quad << "L" << id << ":" << std::endl;
}

/* emitQuad : 输出自定义控制流四元组 (op, x, y, z)
 * 专用于控制流（跳转/标号），op 是字符串形式的操作名
 */
static void emitQuad(const char* op, const OperandsNode& x,
                     const OperandsNode& y, const OperandsNode& z) {
    *quad << "(" << op;
    if (x.kind == IntConst)       *quad << ", " << x.contents.val;
    else if (x.kind == String && x.contents.name) *quad << ", " << x.contents.name;
    else                          *quad << ", _";
    if (y.kind == IntConst)       *quad << ", " << y.contents.val;
    else if (y.kind == String && y.contents.name) *quad << ", " << y.contents.name;
    else                          *quad << ", _";
    if (z.kind == IntConst)       *quad << ", " << z.contents.val;
    else if (z.kind == String && z.contents.name) *quad << ", " << z.contents.name;
    else                          *quad << ", _";
    *quad << ")" << std::endl;
}

/* ================================================================
 * 四元组输出函数
 * Gen  : (op, x, y, z)   OutputOP: 操作符名  OutputOperand: 操作数
 * 由 codeGen 主函数以外的代码调用，生成数据流四元组。
 * ================================================================ */

void OutputOperand(const OperandsNode &x)
{
    *quad << " , ";
    if (x.kind == IntConst)
        *quad << x.contents.val;
    else if (x.kind == String && x.contents.name != NULL)
        *quad << x.contents.name;
    else
        *quad << "_";
}

void OutputOP(OpKind token)
{
    switch (token)
    {
        case mul:    *quad << "*";  break;
        case add:    *quad << "+";  break;
        case J:      *quad << "J";  break;
        case Jne:    *quad << "J!="; break;
        case Jl:     *quad << "J>";  break;
        case Jr:     *quad << "J<";  break;
        case Jle:    *quad << "J>="; break;
        case Jre:    *quad << "J<="; break;
        case devi:   *quad << "/";  break;
        case sub:    *quad << "-";  break;
        case modx:   *quad << "%";  break;
        case pow:    *quad << "^";  break;
        case rd:     *quad << "rd"; break;
        case wri:    *quad << "WR"; break;
        case eq:     *quad << "="; break;
        case asn:    *quad << ":="; break;
        case lt_op:  *quad << "<";  break;
        case le_op:  *quad << "<="; break;
        case gt_op:  *quad << ">";  break;
        case ge_op:  *quad << ">="; break;
        case ne_op:  *quad << "!="; break;
        case inc_op: *quad << "++"; break;
        case dec_op: *quad << "--"; break;
        default:     break;
    }
}

void Gen(OpKind token, OperandsNode x, OperandsNode y, OperandsNode z)
{
    *quad << "(";
    OutputOP(token);
    OutputOperand(x);
    OutputOperand(y);
    OutputOperand(z);
    *quad << " )" << std::endl;
}

/* ================================================================
 * tokenToOpKind : 将语法树中的 TokenType 运算符映射到 OpKind
 *
 * 参数 tree: 表达式节点（OpK），用于获取 attr.op 和子节点
 * 返回值  : 对应的 OpKind 四元组操作码
 * ================================================================ */
static OpKind tokenToOpKind(TreeNode* tree) {
    switch (tree->attr.op) {
        case PLUS:  return add;
        case MINUS: return sub;
        case TIMES: return mul;
        case OVER:  return devi;
        case MOD:   return modx;
        case POWER: return pow;
        case LT:    return lt_op;
        case LE:    return le_op;
        case GT:    return gt_op;
        case GE:    return ge_op;
        case EQ:    return eq;
        case NEQ:   return ne_op;
        case INC:   return inc_op;
        case DEC:   return dec_op;
        default:    return add; // 不会到达这里
    }
}

/* 内部递归代码生成函数的前向声明 */
static OperandsNode cGen(TreeNode* tree);

/* ================================================================
 * genStmt : 在语句节点上生成代码 + 四元组
 *
 * 根据语句的不同类型（IfK / RepeatK / AssignK / ReadK / WriteK）
 * 生成对应的 TM 汇编指令序列，同时输出等价的三地址码四元组。
 *
 * 返回值 : 语句没有值，总是返回 EMPTY_OP
 * 参数 tree: 语句节点
 * ================================================================ */
static OperandsNode genStmt(TreeNode* tree)
{
    TreeNode* p1, * p2, * p3;
    int savedLoc1, savedLoc2, currentLoc;
    int loc;

    switch (tree->kind.stmt)
    {
        /* ---- IF 语句代码生成 + 四元组 ---- */
        case IfK:
        {
            if (TraceCode) emitComment("-> if");

            p1 = tree->child[0];
            p2 = tree->child[1];
            p3 = tree->child[2];

            OperandsNode cond = cGen(p1);
            int elseLabel = newLabel();
            int endLabel = newLabel();

            /* 四元组：条件为 0 时跳转到 else */
            {
                OperandsNode zeroOp, labelOp;
                zeroOp.kind = IntConst; zeroOp.contents.val = 0;
                labelOp.kind = String;
                labelOp.contents.name = copyString(("L" + std::to_string(elseLabel)).c_str());
                emitQuad("J=", cond, zeroOp, labelOp);
            }

            savedLoc1 = emitSkip(1);
            emitComment("if: 条件跳转（条件为假时跳转到 else）");

            cGen(p2);

            /* 四元组：无条件跳转到 end */
            {
                OperandsNode labelOp;
                labelOp.kind = String;
                labelOp.contents.name = copyString(("L" + std::to_string(endLabel)).c_str());
                emitQuad("J", EMPTY_OP, EMPTY_OP, labelOp);
            }

            savedLoc2 = emitSkip(1);
            emitComment("if: 跳转到 end");

            currentLoc = emitSkip(0);
            emitBackup(savedLoc1);
            emitRM_Abs("JEQ", ac, currentLoc, "if: 条件为假，跳转到 else");
            emitRestore();

            emitLabel(elseLabel);
            if (p3 != NULL) cGen(p3);

            currentLoc = emitSkip(0);
            emitBackup(savedLoc2);
            emitRM_Abs("LDA", pc, currentLoc, "跳转到 if 语句结束");
            emitRestore();

            emitLabel(endLabel);
            if (TraceCode) emitComment("<- if");
            break;
        }

        /* ---- REPEAT 语句代码生成 + 四元组 ---- */
        case RepeatK:
        {
            if (TraceCode) emitComment("-> repeat");

            p1 = tree->child[0];
            p2 = tree->child[1];

            int loopLabel = newLabel();
            int exitLabel = newLabel();
            emitLabel(loopLabel);

            savedLoc1 = emitSkip(0);
            emitComment("repeat: 循环体开始");

            cGen(p1);
            OperandsNode repeatCond = cGen(p2);

            /* 四元组：条件成立 (repeatCond == 1) → 退出循环 */
            {
                OperandsNode oneOp, labelOp;
                oneOp.kind = IntConst; oneOp.contents.val = 1;
                labelOp.kind = String;
                labelOp.contents.name = copyString(("L" + std::to_string(exitLabel)).c_str());
                emitQuad("J=", repeatCond, oneOp, labelOp);
            }
            /* 四元组：条件不成立 → 跳回循环体 */
            {
                OperandsNode labelOp;
                labelOp.kind = String;
                labelOp.contents.name = copyString(("L" + std::to_string(loopLabel)).c_str());
                emitQuad("J", EMPTY_OP, EMPTY_OP, labelOp);
            }
            emitLabel(exitLabel);

            emitRM_Abs("JEQ", ac, savedLoc1, "repeat: 条件为真，跳回循环体");
            if (TraceCode) emitComment("<- repeat");
            break;
        }

        /* ---- 赋值语句代码生成 + 四元组 ---- */
        case AssignK:
        {
            if (TraceCode) emitComment("-> assign");

            OperandsNode exprResult = cGen(tree->child[0]);

            loc = st_lookup(tree->attr.name);
            emitRM("ST", ac, loc, gp, "assign: 存储值");

            {
                OperandsNode targetOp;
                targetOp.kind = String;
                targetOp.contents.name = tree->attr.name;
                Gen(asn, exprResult, EMPTY_OP, targetOp);
            }

            if (TraceCode) emitComment("<- assign");
            break;
        }

        /* ---- READ 语句代码生成 + 四元组 ---- */
        case ReadK:
        {
            emitRO("IN", ac, 0, 0, "读入一个整数值");

            loc = st_lookup(tree->attr.name);
            emitRM("ST", ac, loc, gp, "read: 存储值");

            {
                OperandsNode targetOp;
                targetOp.kind = String;
                targetOp.contents.name = tree->attr.name;
                Gen(rd, EMPTY_OP, EMPTY_OP, targetOp);
            }
            break;
        }

        /* ---- WRITE 语句代码生成 + 四元组 ---- */
        case WriteK:
        {
            OperandsNode writeVal = cGen(tree->child[0]);

            emitRO("OUT", ac, 0, 0, "输出 ac");

            Gen(wri, writeVal, EMPTY_OP, EMPTY_OP);
            break;
        }

        /* ---- WHILE 语句代码生成 + 四元组 ---- */
        case WhileK:
        {
            if (TraceCode) emitComment("-> while");

            p1 = tree->child[0];  // 循环条件
            p2 = tree->child[1];  // 循环体

            int loopLabel = newLabel();
            int exitLabel = newLabel();

            /* 循环起始标号 */
            emitLabel(loopLabel);

            /* 保存循环起始地址（用于回跳） */
            savedLoc1 = emitSkip(0);
            emitComment("while: 循环条件判断开始");

            /* 计算条件表达式 */
            OperandsNode whileCond = cGen(p1);

            /* 四元组：条件为假 (==0) → 退出循环 */
            {
                OperandsNode zeroOp, exitLabelOp;
                zeroOp.kind = IntConst; zeroOp.contents.val = 0;
                exitLabelOp.kind = String;
                exitLabelOp.contents.name = copyString(("L" + std::to_string(exitLabel)).c_str());
                emitQuad("J=", whileCond, zeroOp, exitLabelOp);
            }

            /* TM：预留条件跳转指令位置 */
            savedLoc2 = emitSkip(1);
            emitComment("while: 条件为假时跳转到出口");

            /* 循环体 */
            cGen(p2);

            /* 四元组：无条件跳回循环起始 */
            {
                OperandsNode loopLabelOp;
                loopLabelOp.kind = String;
                loopLabelOp.contents.name = copyString(("L" + std::to_string(loopLabel)).c_str());
                emitQuad("J", EMPTY_OP, EMPTY_OP, loopLabelOp);
            }

            /* TM：跳回循环起始 */
            emitRM_Abs("LDA", pc, savedLoc1, "while: 跳回循环条件判断");

            /* 回填：条件假时跳转到出口 */
            currentLoc = emitSkip(0);
            emitBackup(savedLoc2);
            emitRM_Abs("JEQ", ac, currentLoc, "while: 条件为假，退出循环");
            emitRestore();

            /* 出口标号 */
            emitLabel(exitLabel);

            if (TraceCode) emitComment("<- while");
            break;
        }

        /* ---- FOR 语句代码生成 + 四元组 ---- */
        case ForK:
        {
            if (TraceCode) emitComment("-> for");

            p1 = tree->child[0];  // 初始化赋值语句
            p2 = tree->child[1];  // 循环条件
            p3 = tree->child[2];  // 步进语句（INC/DEC 或赋值）

            int loopLabel = newLabel();
            int exitLabel = newLabel();

            /* 初始化语句 */
            cGen(p1);

            /* 循环起始标号 */
            emitLabel(loopLabel);

            /* 保存循环起始地址（用于回跳） */
            savedLoc1 = emitSkip(0);
            emitComment("for: 循环条件判断开始");

            /* 计算条件表达式 */
            OperandsNode forCond = cGen(p2);

            /* 四元组：条件为假 (==0) → 退出循环 */
            {
                OperandsNode zeroOp, exitLabelOp;
                zeroOp.kind = IntConst; zeroOp.contents.val = 0;
                exitLabelOp.kind = String;
                exitLabelOp.contents.name = copyString(("L" + std::to_string(exitLabel)).c_str());
                emitQuad("J=", forCond, zeroOp, exitLabelOp);
            }

            /* TM：预留条件跳转指令位置 */
            savedLoc2 = emitSkip(1);
            emitComment("for: 条件为假时跳转到出口");

            /* 循环体 */
            cGen(tree->child[3]);

            /* 步进语句 */
            cGen(p3);

            /* 四元组：无条件跳回循环起始 */
            {
                OperandsNode loopLabelOp;
                loopLabelOp.kind = String;
                loopLabelOp.contents.name = copyString(("L" + std::to_string(loopLabel)).c_str());
                emitQuad("J", EMPTY_OP, EMPTY_OP, loopLabelOp);
            }

            /* TM：跳回循环起始 */
            emitRM_Abs("LDA", pc, savedLoc1, "for: 跳回循环条件判断");

            /* 回填：条件假时跳转到出口 */
            currentLoc = emitSkip(0);
            emitBackup(savedLoc2);
            emitRM_Abs("JEQ", ac, currentLoc, "for: 条件为假，退出循环");
            emitRestore();

            /* 出口标号 */
            emitLabel(exitLabel);

            if (TraceCode) emitComment("<- for");
            break;
        }

        default:
            break;
    }
    return EMPTY_OP;
}

/* ================================================================
 * genExp : 在表达式节点上生成代码 + 四元组
 *
 * 根据表达式的不同类型（ConstK / IdK / OpK）
 * 生成对应的 TM 汇编指令序列。
 * 所有表达式的结果都放在累加器 ac 中。
 *
 * 返回值 : 表达式的操作数描述（常量值、变量名或临时变量名）
 * 参数 tree: 表达式节点
 * ================================================================ */
static OperandsNode genExp(TreeNode* tree)
{
    int loc;
    TreeNode* p1, * p2;

    switch (tree->kind.exp)
    {
        /* ---- 常量表达式 ---- */
        case ConstK:
        {
            if (TraceCode) emitComment("-> Const");
            emitRM("LDC", ac, tree->attr.val, 0, "加载常量");
            if (TraceCode) emitComment("<- Const");

            OperandsNode op;
            op.kind = IntConst;
            op.contents.val = tree->attr.val;
            return op;
        }

        /* ---- 标识符表达式 ---- */
        case IdK:
        {
            if (TraceCode) emitComment("-> Id");
            loc = st_lookup(tree->attr.name);
            emitRM("LD", ac, loc, gp, "加载标识符的值");
            if (TraceCode) emitComment("<- Id");

            OperandsNode op;
            op.kind = String;
            op.contents.name = tree->attr.name;
            return op;
        }

        /* ---- 运算符表达式 ---- */
        case OpK:
        {
            if (TraceCode) emitComment("-> Op");

            p1 = tree->child[0];
            p2 = tree->child[1];

            /* ---- 自增/自减是单目运算 ---- */
            if (tree->attr.op == INC || tree->attr.op == DEC) {
                OperandsNode target = cGen(p1);

                emitRM("LDC", ac, 1, 0, "加载 1");
                emitRM("LD", ac1, st_lookup(p1->attr.name), gp, "加载变量值");
                emitRO(tree->attr.op == INC ? "ADD" : "SUB", ac, ac1, ac,
                       tree->attr.op == INC ? "自增" : "自减");
                emitRM("ST", ac, st_lookup(p1->attr.name), gp, "存回变量");

                OperandsNode oneOp;
                oneOp.kind = IntConst;
                oneOp.contents.val = 1;
                Gen(tree->attr.op == INC ? inc_op : dec_op, target, oneOp, target);
                return target;
            }

            /* ---- 双目运算 ---- */

            OperandsNode leftOp = cGen(p1);
            emitRM("ST", ac, tmpOffset--, mp, "op: 左操作数压栈");

            OperandsNode rightOp = cGen(p2);
            emitRM("LD", ac1, ++tmpOffset, mp, "op: 左操作数出栈到 ac1");

            const char* tempName = newTemp();
            OperandsNode resultOp;
            resultOp.kind = String;
            resultOp.contents.name = tempName;

            switch (tree->attr.op)
            {
                case PLUS:
                    emitRO("ADD", ac, ac1, ac, "op +");
                    Gen(add, leftOp, rightOp, resultOp);
                    break;
                case MINUS:
                    emitRO("SUB", ac, ac1, ac, "op -");
                    Gen(sub, leftOp, rightOp, resultOp);
                    break;
                case TIMES:
                    emitRO("MUL", ac, ac1, ac, "op *");
                    Gen(mul, leftOp, rightOp, resultOp);
                    break;
                case OVER:
                    emitRO("DIV", ac, ac1, ac, "op /");
                    Gen(devi, leftOp, rightOp, resultOp);
                    break;
                case MOD:
                    emitComment("TM 不支持取模，仅输出四元组");
                    Gen(modx, leftOp, rightOp, resultOp);
                    break;
                case POWER:
                    emitComment("TM 不支持乘方，仅输出四元组");
                    Gen(pow, leftOp, rightOp, resultOp);
                    break;
                case LT:
                    emitRO("SUB", ac, ac1, ac, "op <");
                    emitRM("JLT", ac, 2, pc, "如果为真则跳转");
                    emitRM("LDC", ac, 0, ac, "结果为假（0）");
                    emitRM("LDA", pc, 1, pc, "无条件跳转");
                    emitRM("LDC", ac, 1, ac, "结果为真（1）");
                    Gen(lt_op, leftOp, rightOp, resultOp);
                    break;
                case LE:
                    emitRO("SUB", ac, ac1, ac, "op <=");
                    emitRM("JLE", ac, 2, pc, "如果为真则跳转");
                    emitRM("LDC", ac, 0, ac, "结果为假（0）");
                    emitRM("LDA", pc, 1, pc, "无条件跳转");
                    emitRM("LDC", ac, 1, ac, "结果为真（1）");
                    Gen(le_op, leftOp, rightOp, resultOp);
                    break;
                case GT:
                    emitRO("SUB", ac, ac1, ac, "op >");
                    emitRM("JGT", ac, 2, pc, "如果为真则跳转");
                    emitRM("LDC", ac, 0, ac, "结果为假（0）");
                    emitRM("LDA", pc, 1, pc, "无条件跳转");
                    emitRM("LDC", ac, 1, ac, "结果为真（1）");
                    Gen(gt_op, leftOp, rightOp, resultOp);
                    break;
                case GE:
                    emitRO("SUB", ac, ac1, ac, "op >=");
                    emitRM("JGE", ac, 2, pc, "如果为真则跳转");
                    emitRM("LDC", ac, 0, ac, "结果为假（0）");
                    emitRM("LDA", pc, 1, pc, "无条件跳转");
                    emitRM("LDC", ac, 1, ac, "结果为真（1）");
                    Gen(ge_op, leftOp, rightOp, resultOp);
                    break;
                case EQ:
                    emitRO("SUB", ac, ac1, ac, "op ==");
                    emitRM("JEQ", ac, 2, pc, "如果为真则跳转");
                    emitRM("LDC", ac, 0, ac, "结果为假（0）");
                    emitRM("LDA", pc, 1, pc, "无条件跳转");
                    emitRM("LDC", ac, 1, ac, "结果为真（1）");
                    Gen(eq, leftOp, rightOp, resultOp);
                    break;
                case NEQ:
                    emitRO("SUB", ac, ac1, ac, "op !=");
                    emitRM("JNE", ac, 2, pc, "如果为真则跳转");
                    emitRM("LDC", ac, 0, ac, "结果为假（0）");
                    emitRM("LDA", pc, 1, pc, "无条件跳转");
                    emitRM("LDC", ac, 1, ac, "结果为真（1）");
                    Gen(ne_op, leftOp, rightOp, resultOp);
                    break;
                default:
                    emitComment("BUG: 未知运算符");
                    break;
            }

            if (TraceCode) emitComment("<- Op");
            return resultOp;
        }

        default:
            break;
    }
    return EMPTY_OP;
}

/* ================================================================
 * cGen : 递归代码生成函数
 *
 * 遍历语法树，对每个节点根据其种类（StmtK 或 ExpK）
 * 调用对应的代码生成函数，然后递归处理兄弟节点。
 *
 * 返回值 : 对于表达式节点返回操作数，语句节点返回 EMPTY_OP
 * 参数 tree: 当前要生成代码的语法树节点
 * ================================================================ */
static OperandsNode cGen(TreeNode* tree)
{
    if (tree != NULL)
    {
        OperandsNode result = EMPTY_OP;
        switch (tree->nodekind)
        {
            case StmtK:
                genStmt(tree);
                break;
            case ExpK:
                result = genExp(tree);
                break;
            default:
                break;
        }
        cGen(tree->sibling);  // 递归处理兄弟节点（顺序语句）
        return result;
    }
    return EMPTY_OP;
}

/**********************************************/
/*     代码生成器的主函数                      */
/**********************************************/

/* ================================================================
 * codeGen : 生成完整的 TM 代码 + 四元组中间代码
 *
 * 代码生成流程：
 *   1. 写入文件信息注释
 *   2. 生成标准前导代码（初始化内存指针）
 *   3. 遍历语法树生成程序主体代码（同时输出四元组）
 *   4. 生成 HALT 指令结束程序
 *
 * 参数 syntaxTree: 语法树的根节点
 * 参数 codefile  : 代码文件名（用于注释）
 * ================================================================ */
void codeGen(TreeNode* syntaxTree, const char* codefile)
{
    // 构造文件注释字符串
    std::string s = "文件: ";
    s += codefile;

    *quad << "\n========== 四元组中间代码 ==========" << std::endl;

    emitComment("TINY 编译到 TM 代码");
    emitComment(s.c_str());

    /* 生成标准前导代码 */
    emitComment("标准前导代码：");
    emitRM("LD", mp, 0, ac, "从位置 0 加载最大地址到 mp");
    emitRM("ST", ac, 0, ac, "清除位置 0");
    emitComment("标准前导代码结束。");

    /* 生成 TINY 程序主体的代码（同时输出四元组）*/
    cGen(syntaxTree);

    /* 程序结束 */
    *quad << "===================================\n" << std::endl;

    emitComment("程序执行结束。");
    emitRO("HALT", 0, 0, 0, "");
}
