#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "user.h"
#include <QWidget>
#include <QChart>  //图表类
#include <QChartView>   //图表视图类
#include <QValueAxis>  //数值型坐标轴
#include <QSplineSeries> //曲线
#include <QLineSeries>  //折线
#include <QTcpServer>
#include <QTcpSocket>
#include <QtMqtt/qmqttclient.h>  //mqtt库
#include <QDateTime>
#include <QJsonObject> //封装JSON对象
#include <QJsonDocument> //JSON文档
#include <QSqlDatabase> //数据库
#include <QSqlQuery> //执行sql语句类
#include <QSqlError> //sql错误类
#include <QMessageBox> //消息对话框
#include <QSqlTableModel>  //sqlite再封装库，可显示数据库内容
#include <QSqlRecord>
#include <QMap> //关联式容器

//与客户端通信协议宏
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

//与设备mqtt通信宏
#define ADDERR "adderror"
#define ADDSUC "addsucess"

QT_CHARTS_USE_NAMESPACE //using namespace QT_CHARTS_NAMESPACE;


class MyThread;

namespace Ui {
class Mainwindow;
}

class Mainwindow : public QWidget
{
    Q_OBJECT

public:
    explicit Mainwindow(QWidget *parent = nullptr);
    ~Mainwindow();
    //json数据包解析
    void getJsonValue(QByteArray msg,User &user);
    void loginctl(QTcpSocket *socket,QString str2,QString str3,QString str4);
    void registctl(QTcpSocket *socket,QString str2,QString str3,QString str4);
    void getfamilyname(QTcpSocket *socket,QString str2);
    void getmemberinfo(QTcpSocket *socket,QString str);
    void addmember(QTcpSocket *socket,QString str);
private:
    Ui::Mainwindow *ui;
    QTcpServer *tcpServer;
    QMqttClient *mqtt_client;
    QSqlTableModel *model;
    QSplineSeries *splineSeries;

    QStringList addMemberList;//临时存储添加用户的信息
    QMap<int,QTcpSocket*> map_devforapp;//用于关联设备和app客户端

    //存储用户信息
    uint16_t usr_id;    //指纹id
    QString usr_name;   //用户名
    uint8_t usr_heart;  //心率
    uint8_t usr_blood;  //血氧
    float usr_temperature;//体温


private slots:
    void brokerConnected();
    void mqttMsgReceived(const QByteArray &, const QMqttTopicName &);
    void mNewconnection();
    void receviveMessages();
    void mStateChanged(QAbstractSocket::SocketState);
    void on_start_listen_clicked();
    void on_stop_listen_clicked();
    void on_pushButton_mqttsub_clicked();
    void on_pushButton_tcpsend_clicked();
    void on_pushButton_refresh_clicked();
};


#endif // MAINWINDOW_H
