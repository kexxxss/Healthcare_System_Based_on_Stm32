#ifndef LOGIN_WIDGET_H
#define LOGIN_WIDGET_H

#include <QWidget>
#include "mainwindow.h"
#include <QSqlDatabase> //数据库
#include <QSqlQuery> //执行sql语句类
#include <QSqlError> //sql错误类
#include <QMessageBox> //消息对话框
#include <QPalette>
#include <QPixmap>
QT_BEGIN_NAMESPACE
namespace Ui { class login_Widget; }
QT_END_NAMESPACE

class login_Widget : public QWidget
{
    Q_OBJECT

public:
    login_Widget(QWidget *parent = nullptr);
    ~login_Widget();

private slots:
    void on_pushButton_clicked();

private:
    Ui::login_Widget *ui;
    Mainwindow *mainwindow;

};
#endif // LOGIN_WIDGET_H
