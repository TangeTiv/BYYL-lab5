/****************************************************/
/* 文件: mainwindow.h                                 */
/* TINY 编译器 GUI 主窗口                            */
/****************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QLabel>
#include <QSplitter>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void onNewFile();       // 新建文件
    void onOpenFile();      // 打开文件
    void onSaveFile();      // 保存文件
    void onCompile();       // 编译

private:
    void setupUI();         // 初始化界面
    void setupConnections();// 连接信号槽
    void setStatus(const QString& msg, bool isError = false);
    void updateTitle();

    // === UI 组件 ===
    // 左侧：源代码编辑区
    QPlainTextEdit* m_sourceEdit;

    // 右侧：输出查看区 (Tab)
    QTabWidget*     m_tabWidget;
    QPlainTextEdit* m_quadView;    // 四元组查看器
    QPlainTextEdit* m_tmView;      // TM 代码查看器

    // 底部：编译消息
    QPlainTextEdit* m_messageEdit;

    // 按钮
    QPushButton* m_newBtn;
    QPushButton* m_openBtn;
    QPushButton* m_saveBtn;
    QPushButton* m_compileBtn;

    // 状态栏
    QLabel* m_statusLabel;

    // === 状态 ===
    QString m_currentFilePath;  // 当前文件路径（空 = 未保存）
    bool    m_modified;         // 是否有未保存的修改
};

#endif // MAINWINDOW_H
