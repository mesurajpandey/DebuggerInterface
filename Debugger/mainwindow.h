#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "codeeditor.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void initialize();
    void createActions();
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    CodeEditor* editor;

private slots:
    void open(QAction*);
    void debug(QAction*);
    void debugESI(QAction*);
    void showDebugValue();
    void setBreakPoint();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_9_clicked();

    void on_pushButton_10_clicked();

    void on_pushButton_11_clicked();

protected:
    void resizeEvent(QResizeEvent *event);
};

#endif // MAINWINDOW_H
