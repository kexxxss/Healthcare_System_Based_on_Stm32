#include "login_widget.h"
#include "ui_login_widget.h"

//数据库
QSqlDatabase mydb;
//登录成功后,管理员信息将在整个程序使用,所以设为全局
qint16 manid;
QString manname;
QString manposition;

login_Widget::login_Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::login_Widget)
{
    ui->setupUi(this);
    // 开启背景设置
    this->setAutoFillBackground(true);
    // 创建调色板对象
    QPalette p = this->palette();
    // 加载图片
    QPixmap pix(":login.jpeg");
    // 设置图片
    p.setBrush(QPalette::Window, QBrush(pix));
    this->setPalette(p);

    //设置标题
    this->setWindowTitle("远程医疗健康");
    this->setWindowIcon(QIcon(":/title_icon.png"));

    //添加数据库
    mydb = QSqlDatabase::addDatabase("QSQLITE");
    //设置库名
    mydb.setDatabaseName("project.db");
    //打开数据库
    if(!mydb.open()){
        //用对话框显示
        QMessageBox::warning(this,"打开数据库",mydb.lastError().text());
        return;
    }
    QSqlQuery query;//用于执行sql语句的对象
    //sql语句，创建管理员表，用于登录该系统进行验证
    //name约束设为唯一性，不可重复
    QString sql =QString("create table if not exists manager(manid int unique,pswd text,manname varchar(32),manpose varchar(32))");
    if (!query.exec(sql))
    {
        QMessageBox::warning(this,"创建表",query.lastError().text());
        return;
    }

    //如果是首次启动则创建表，顺便加一条预设记录
    sql = QString("insert into manager values(%1, '%2','%3','%4')").arg(1025).arg("123").arg("徐铭").arg("系统维护");
    query.exec(sql);

}

login_Widget::~login_Widget()
{
    delete ui;
}

//登录按钮
void login_Widget::on_pushButton_clicked()
{
    QString id = ui->lineEdit_id->text();
    QString pswd = ui->password->text();
    if (id.isEmpty() || pswd.isEmpty())
    {
        QMessageBox::warning(this, "管理员登录", "账号和密码不能为空!");
        return;
    }
    QString sql = QString("select * from manager where manid=%1").arg(id);
    QSqlQuery query;
    query.exec(sql);
    //QSqlQuery返回的数据集，record是停在第一条记录之前的。所以，你获得数据集后，必须执行next()或first()到第一条记录，这时候record才是有效的
    if (!query.first())
    {
        qDebug() << query.lastError().text();
        QMessageBox::warning(this,"登录","该工号不存在");
        ui->lineEdit_id->clear();
        ui->password->clear();
        return;
    }
    else{
        //比较密码
        if(!QString::compare(pswd,query.value(1).toString())){
            QMessageBox::information(this,"登录","管理员登录成功!");
            manid = query.value(0).toInt();
            manname = query.value(2).toString();
            manposition = query.value(3).toString();

            mainwindow = new Mainwindow;//创建堆对象
            mainwindow->show();
            this->hide();
        }
        else {
            QMessageBox::warning(this,"登录","密码错误,登录失败");
            ui->password->clear();
            return;
        }
    }

}
