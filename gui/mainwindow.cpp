/****************************************************/
/* 文件: mainwindow.cpp                               */
/* TINY 编译器 GUI 主窗口实现                        */
/****************************************************/

#include "mainwindow.h"
#include "../compiler_api.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QApplication>
#include <QStatusBar>
#include <QStyle>

/* ================================================================
 * 构造函数
 * ================================================================ */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_modified(false)
{
    setupUI();
    setupConnections();
    updateTitle();
}

/* ================================================================
 * setupUI — 构建界面布局
 *
 * 布局结构：
 *   QSplitter (水平)
 *   ├── 左侧面板 (QVBoxLayout)
 *   │   ├── "TINY 源程序编辑" (QGroupBox → QPlainTextEdit)
 *   │   ├── 按钮行 (QHBoxLayout: 新建/打开/保存/编译)
 *   │   └── "编译消息" (QGroupBox → QPlainTextEdit)
 *   └── 右侧面板 (QTabWidget)
 *       ├── Tab 0: "四元组中间代码" (QPlainTextEdit)
 *       └── Tab 1: "TM 目标代码" (QPlainTextEdit)
 * ================================================================ */
void MainWindow::setupUI()
{
    // ---- 窗口基础设置 ----
    setWindowTitle("TINY 编译器 GUI");
    setMinimumSize(900, 600);

    // ---- 等宽字体 ----
    QFont monoFont("Consolas", 11);
    if (!QFontInfo(monoFont).fixedPitch()) {
        monoFont = QFont("Courier New", 11);
    }

    // ==============================================
    // 左侧面板：编辑器 + 按钮 + 消息
    // ==============================================
    QWidget* leftPanel = new QWidget;
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(4, 4, 4, 4);
    leftLayout->setSpacing(6);

    // ---- 源程序编辑器 ----
    QGroupBox* editorGroup = new QGroupBox("TINY 源程序编辑");
    QVBoxLayout* editorLayout = new QVBoxLayout(editorGroup);
    m_sourceEdit = new QPlainTextEdit;
    m_sourceEdit->setFont(monoFont);
    m_sourceEdit->setPlaceholderText("在此输入 TINY 源程序...\n\n示例:\n  read x;\n  read y;\n  z := x + y * 2;\n  write z;\n  if z < 10 then write 0 else write 1 end;");
    m_sourceEdit->setTabStopDistance(32);  // 4 空格缩进
    m_sourceEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    editorLayout->addWidget(m_sourceEdit);
    leftLayout->addWidget(editorGroup, 2);  // stretch factor 2

    // ---- 按钮行 ----
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);

    m_newBtn = new QPushButton("新建");
    m_newBtn->setToolTip("新建空白源文件");

    m_openBtn = new QPushButton("打开文件");
    m_openBtn->setToolTip("打开 .tny 源文件");
    m_openBtn->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));

    m_saveBtn = new QPushButton("保存文件");
    m_saveBtn->setToolTip("保存当前源文件");
    m_saveBtn->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    m_compileBtn = new QPushButton("▶  编  译");
    m_compileBtn->setToolTip("编译当前源程序，生成四元组和 TM 代码");
    m_compileBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #0078D4;"
        "  color: white;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "  padding: 6px 16px;"
        "  border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #106EBE;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #005A9E;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #CCCCCC;"
        "}"
    );

    btnLayout->addWidget(m_newBtn);
    btnLayout->addWidget(m_openBtn);
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_compileBtn);
    leftLayout->addLayout(btnLayout);

    // ---- 编译消息 ----
    QGroupBox* msgGroup = new QGroupBox("编译消息 / 错误输出");
    QVBoxLayout* msgLayout = new QVBoxLayout(msgGroup);
    m_messageEdit = new QPlainTextEdit;
    m_messageEdit->setFont(monoFont);
    m_messageEdit->setReadOnly(true);
    m_messageEdit->setMaximumBlockCount(2000);
    m_messageEdit->setPlaceholderText("编译消息将在这里显示...");
    msgLayout->addWidget(m_messageEdit);
    leftLayout->addWidget(msgGroup, 1);  // stretch factor 1

    // ==============================================
    // 右侧面板：Tab 页（四元组 + TM 代码）
    // ==============================================
    m_tabWidget = new QTabWidget;

    // Tab 0: 四元组中间代码
    m_quadView = new QPlainTextEdit;
    m_quadView->setFont(monoFont);
    m_quadView->setReadOnly(true);
    m_quadView->setPlaceholderText("编译后将在此显示四元组中间代码...");
    m_quadView->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_tabWidget->addTab(m_quadView, "四元组中间代码");

    // Tab 1: TM 目标代码
    m_tmView = new QPlainTextEdit;
    m_tmView->setFont(monoFont);
    m_tmView->setReadOnly(true);
    m_tmView->setPlaceholderText("编译后将在此显示 TM 目标代码...");
    m_tmView->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_tabWidget->addTab(m_tmView, "TM 目标代码");

    // ==============================================
    // 使用 QSplitter 组合左右面板
    // ==============================================
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(m_tabWidget);
    splitter->setStretchFactor(0, 1);   // 左侧占 1 份
    splitter->setStretchFactor(1, 1);   // 右侧占 1 份
    setCentralWidget(splitter);

    // ==============================================
    // 状态栏
    // ==============================================
    m_statusLabel = new QLabel("就绪");
    statusBar()->addWidget(m_statusLabel, 1);
    setStatus("就绪");
}

/* ================================================================
 * setupConnections — 连接信号与槽
 * ================================================================ */
void MainWindow::setupConnections()
{
    // 编辑器内容变化 → 标记已修改
    connect(m_sourceEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (!m_modified) {
            m_modified = true;
            updateTitle();
        }
    });

    // 按钮点击
    connect(m_newBtn,     &QPushButton::clicked, this, &MainWindow::onNewFile);
    connect(m_openBtn,    &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(m_saveBtn,    &QPushButton::clicked, this, &MainWindow::onSaveFile);
    connect(m_compileBtn, &QPushButton::clicked, this, &MainWindow::onCompile);
}

/* ================================================================
 * onNewFile — 新建空白源文件
 * ================================================================ */
void MainWindow::onNewFile()
{
    if (m_modified) {
        int ret = QMessageBox::question(this, "未保存的修改",
            "当前文件有未保存的修改，是否继续？",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }

    m_sourceEdit->clear();
    m_quadView->clear();
    m_tmView->clear();
    m_messageEdit->clear();
    m_currentFilePath.clear();
    m_modified = false;
    updateTitle();
    setStatus("新建文件");
}

/* ================================================================
 * onOpenFile — 打开 .tny 源文件
 * ================================================================ */
void MainWindow::onOpenFile()
{
    if (m_modified) {
        int ret = QMessageBox::question(this, "未保存的修改",
            "当前文件有未保存的修改，是否继续？",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }

    QString filePath = QFileDialog::getOpenFileName(this,
        "打开 TINY 源文件", QString(),
        "TINY 源文件 (*.tny);;文本文件 (*.txt);;所有文件 (*.*)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件: " + filePath);
        return;
    }

    QTextStream in(&file);
    m_sourceEdit->setPlainText(in.readAll());
    file.close();

    m_currentFilePath = filePath;
    m_modified = false;
    m_quadView->clear();
    m_tmView->clear();
    m_messageEdit->clear();
    updateTitle();
    setStatus("已打开: " + filePath);
}

/* ================================================================
 * onSaveFile — 保存当前源文件
 * ================================================================ */
void MainWindow::onSaveFile()
{
    QString filePath = m_currentFilePath;

    if (filePath.isEmpty()) {
        // 首次保存，弹出对话框
        filePath = QFileDialog::getSaveFileName(this,
            "保存 TINY 源文件", "untitled.tny",
            "TINY 源文件 (*.tny);;文本文件 (*.txt);;所有文件 (*.*)");

        if (filePath.isEmpty()) return;

        // 自动补 .tny 扩展名
        if (!filePath.contains('.')) {
            filePath += ".tny";
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法保存文件: " + filePath);
        return;
    }

    QTextStream out(&file);
    out << m_sourceEdit->toPlainText();
    file.close();

    m_currentFilePath = filePath;
    m_modified = false;
    updateTitle();
    setStatus("已保存: " + filePath);
}

/* ================================================================
 * onCompile — 编译当前源程序
 *
 * 流程：
 *   1. 从编辑器获取源代码文本
 *   2. 调用 compileFromString() 执行编译
 *   3. 将四元组显示到 m_quadView
 *   4. 将 TM 代码显示到 m_tmView
 *   5. 将编译消息显示到 m_messageEdit
 * ================================================================ */
void MainWindow::onCompile()
{
    QString source = m_sourceEdit->toPlainText();

    if (source.trimmed().isEmpty()) {
        QMessageBox::information(this, "提示", "请先输入 TINY 源代码。");
        return;
    }

    // 禁用编译按钮，防止重复点击
    m_compileBtn->setEnabled(false);
    m_compileBtn->setText("编译中...");

    // 清空之前的结果
    m_quadView->clear();
    m_tmView->clear();
    m_messageEdit->clear();
    setStatus("正在编译...");
    QApplication::processEvents();  // 刷新 UI

    // 执行编译
    CompileResult result = compileFromString(source.toStdString());

    // 显示四元组
    if (!result.quadOutput.empty()) {
        m_quadView->setPlainText(QString::fromStdString(result.quadOutput));
    } else {
        m_quadView->setPlainText("（无输出）");
    }

    // 显示 TM 代码
    if (!result.tmOutput.empty()) {
        m_tmView->setPlainText(QString::fromStdString(result.tmOutput));
    } else {
        m_tmView->setPlainText("（无输出）");
    }

    // 显示编译消息
    if (!result.listingOutput.empty()) {
        m_messageEdit->setPlainText(QString::fromStdString(result.listingOutput));
    }

    // 状态反馈
    if (result.success) {
        setStatus("编译成功 ✓");
        m_messageEdit->appendPlainText("\n========== 编译成功 ==========");
        // 切换到四元组 Tab
        m_tabWidget->setCurrentIndex(0);
    } else {
        setStatus("编译失败 ✗ — 请查看编译消息", true);
        m_messageEdit->appendPlainText("\n========== 编译失败 ==========");
        // 切换到消息区域（用户可以看到错误）
    }

    // 恢复编译按钮
    m_compileBtn->setEnabled(true);
    m_compileBtn->setText("▶  编  译");
}

/* ================================================================
 * setStatus — 更新状态栏
 * ================================================================ */
void MainWindow::setStatus(const QString& msg, bool isError)
{
    if (isError) {
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
    } else {
        m_statusLabel->setStyleSheet("color: black; font-weight: normal;");
    }
    m_statusLabel->setText(msg);
}

/* ================================================================
 * updateTitle — 更新窗口标题（反映文件修改状态）
 * ================================================================ */
void MainWindow::updateTitle()
{
    QString title;
    if (m_currentFilePath.isEmpty()) {
        title = "未命名";
    } else {
        // 只显示文件名，不显示完整路径
        int idx = m_currentFilePath.lastIndexOf('/');
        if (idx < 0) idx = m_currentFilePath.lastIndexOf('\\');
        title = m_currentFilePath.mid(idx + 1);
    }

    if (m_modified) {
        title += " ●";  // 修改标记
    }

    title += " — TINY 编译器 GUI";
    setWindowTitle(title);
}
