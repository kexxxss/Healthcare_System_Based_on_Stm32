#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QMessageBox>
#include <QDesktopWidget> //获取屏幕信息的类
#include <QScreen>
#include <QChart>   //图表类
#include <QChartView>   //图表视图类
#include <QValueAxis>  //数值型坐标轴
#include <QSplineSeries> //曲线
#include <QLineSeries>  //折线
#include <QDateTime>
#include <QDateTimeAxis>
#include <QMap> //容器

QT_CHARTS_USE_NAMESPACE //using namespace QT_CHARTS_NAMESPACE;

//与服务端通信协议宏
#define LOGIN "login"
#define LOGIN_FAMILY_ERR 1001
#define LOGIN_KEY_ERR 1002
#define LOGIN_DEVID_ERR 1003
#define LOGIN_SUCCESS 1004
#define REGIST "regist"
#define REGIST_SUCCESS 1004
#define REGIST_ERR 1005
#define GETFAMILY "getfamily"
#define GETMEMBERINFO "getmemberinfo"
#define Y "y"
#define N "n"
#define ADDMEMBER "addmember"
#define ADDMEMBER_REPEAT 1006
#define ADDMEMBER_ERROR 1007
#define ADDMEMBER_SUCESS 1008

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
signals:
    void jump();//登录成功时发送信号,跳转界面
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void loginctl(QString str2);
    void registctl(QString str2);
    void memberinfoctl(QString str);
    void initchart();//初始化图表
    void paintchart();//绘制图表

private:
    Ui::Widget *ui;
    QTcpSocket *tcpSocket;
    //以下三个容器存储的数据用于图表中显示
    QMap<QDateTime, float> map_temperature;//关联记录时间和体温的容器
    QMap<QDateTime, int> map_heart;//关联时间和心率
    QMap<QDateTime, int> map_blood;//关联时间和血氧
    //数据图表
    QChart *chart_temperature;
    QChart *chart_heart;
    QChart *chart_blood;
    //坐标轴，为了避免三张表之间会有影响，就独立开来了
    QDateTimeAxis *timeAxisX;
    QValueAxis *valueAxisY;
    QDateTimeAxis *timeAxisX_heart;
    QValueAxis *valueAxisY_heart;
    QDateTimeAxis *timeAxisX_blood;
    QValueAxis *valueAxisY_blood;
    //数据曲线
    QSplineSeries *splineSeries;
    QSplineSeries *splineSeries_heart;
    QSplineSeries *splineSeries_blood;

private slots:
    void recviveMessages();
    void on_loginButton_clicked();
    void mStateChanged(QAbstractSocket::SocketState);
    void on_registButton_clicked();
    void on_quitButton_clicked();
    void family_infoface();
    void on_comboBox_currentIndexChanged(const QString &arg1);
    void on_pushButton_addmember_clicked();
    void on_pushButton_health_clicked();
    void on_pushButton_backfromadd_clicked();
    void on_pushButton_confirmadd_clicked();
};
#endif // WIDGET_H
