#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
//数据库
extern QSqlDatabase mydb;
//管理员信息
extern qint16 manid;
extern QString manname;
extern QString manposition;

//MQTT服务器的ip和通信端口
QString mqtt_ser_ip = "116.62.116.146";
quint16 mqtt_ser_port = 1883;
//主题topic和消息
QString mqtt_topic1 = "/xm/health"; //用于收发用户健康信息的主题
QString mqtt_topic_addmember = "/xm/addMember"; //用于添加新成员的主题
QString mqtt_msg = "test";

//TCP服务端ip和端口
//QString serverIp = "116.62.116.146";
/*  这里直接设为云服务器的公网ip，在实际测试时客户端无法连接这个公网ip，
    这或许是跟阿里云服务器的设计架构有关，也就是说这个公网ip实际上是个虚拟ip，
    用户一般通过将这个发起请求时会首先进入阿里云服务器的映射表，再实际转为服务器真实ip，
    具体情况得要去了解阿里云服务器的设计结构。
    所以后面调试我发现用0.0.0.0即主机所有ip，是可以连接的。阿里云服务器是有种机制能够让外网直连到公网ip的，但我尚未了解
*/

quint16 port = 9999;

Mainwindow::Mainwindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Mainwindow)
{
    ui->setupUi(this);
    //设置标题
    this->setWindowTitle(" ");
    this->setWindowIcon(QIcon(":/title_icon.png"));
    //部分界面数据初始化
    ui->label_manageid->setText(QString::number(manid));
    ui->label_managename->setText(manname);
    ui->label_manageposition->setText(manposition);

    //与mqtt服务器连接
    mqtt_client = new QMqttClient(this);
    mqtt_client->setHostname(mqtt_ser_ip);
    mqtt_client->setPort(mqtt_ser_port);
    mqtt_client->connectToHost();
    connect(mqtt_client, &QMqttClient::connected, this, &Mainwindow::brokerConnected);

    //创建TCP服务端
    tcpServer = new QTcpServer(this);
    connect(tcpServer,SIGNAL(newConnection()),this,SLOT(mNewconnection()));

    ui->start_listen->setEnabled(true);//按钮使能
    ui->stop_listen->setEnabled(false);//停止监听按钮->失能

    //数据库--创建家庭表，手机app用于登录和注册,添加成员等。(设备id(主键),家庭名,app登录密码,家庭成员名1,家庭成员名2,...)
    //首次创建表时成员名个数初始为5个，如果还有新添加成员，就再增加一列就行
    QString sql =QString("create table if not exists family_app("
                         "deviceid int PRIMARY KEY,"
                         "family varchar(32) ,"
                         "pswd varchar(32),"
                         "member1 varchar(32) default 0,"
                         "member2 varchar(32) default 0,"
                         "member3 varchar(32) default 0,"
                         "member4 varchar(32) default 0,"
                         "member5 varchar(32) default 0"
                         ")");
    QSqlQuery query;//用于执行sql语句的对象
    if (!query.exec(sql))
    {
        QMessageBox::warning(this,"创建家庭表",query.lastError().text());
        return;
    }
    //数据库--创建用户检测历史数据表(时间,设备id,家庭名，家庭成员名，家庭成员指纹id，心率，血氧，体温）
    //两张表在应用场景下的关系其实不大，家庭名也不算是外键，也不需要约束
    sql =QString("create table if not exists user("
                         "time DATETIME NOT NULL,"
                         "device int,"
                         "family varchar(32) ,"
                         "membername varchar(32) ,"
                         "memberfid int NOT NULL ,"
                         "heart int ,"
                         "blood int ,"
                         "temperature float"
                         ")");
    if (!query.exec(sql))
    {
        QMessageBox::warning(this,"创建用户历史数据表",query.lastError().text());
        return;
    }


}

Mainwindow::~Mainwindow()
{
    delete ui;
}

//json解析
void Mainwindow::getJsonValue(QByteArray msg,User &user)
{
    QJsonParseError error;
    QJsonDocument doc;
    doc = QJsonDocument::fromJson(msg, &error);
    if(error.error == QJsonParseError::NoError)
    {
        if(doc.isObject())
        {
            QJsonObject obj = doc.object();
            if(obj.contains("device")){
                QJsonValue value = obj.value("device");
                if(value.isDouble()){//整型的解析
                    user.setDevice(value.toVariant().toInt());
                }
            }
            if(obj.contains("family")){//字符串的解析
                QJsonValue value = obj.value("family");
                if(value.isString()){
                    user.setFamily(value.toString());

                }
            }
            if(obj.contains("membername")){//字符串的解析
                QJsonValue value = obj.value("membername");
                if(value.isString()){
                    user.setMembername(value.toString());

                }
            }
            if(obj.contains("memberfid")){
                QJsonValue value = obj.value("memberfid");
                if(value.isDouble()){//整型的解析
                    user.setMemberfid(value.toVariant().toInt());
                }
            }

            if(obj.contains("heart")){
                QJsonValue value = obj.value("heart");
                if(value.isDouble()){//整型的解析
                    user.setHeart(value.toVariant().toInt());
                }
            }
            if(obj.contains("blood")){
                QJsonValue value = obj.value("blood");
                if(value.isDouble()){//整型的解析
                    user.setBlood(value.toVariant().toInt());
                }
            }
            if(obj.contains("temperature")){
                QJsonValue value = obj.value("temperature");
                if(value.isDouble()){//double的解析
                    user.setTemperature(value.toDouble());
                }
            }
        }
    }

}
//接收app消息
void Mainwindow::receviveMessages()
{
    //获取该连接的连接套接字
    QTcpSocket *tmpTcpSocket = (QTcpSocket *)sender();
    QString recvmsg = tmpTcpSocket->readAll();
    ui->textBrowser_tcpserver->append(QLatin1String("IP:") + tmpTcpSocket->peerAddress().toString() + "发送消息:" + recvmsg);
    QString str1 = recvmsg.section(" ",0,0);
    QString str2 = recvmsg.section(" ",1,1);
    QString str3 = recvmsg.section(" ",2,2);
    QString str4 = recvmsg.section(" ",3,3);
    if(str1 == LOGIN)
    {
        loginctl(tmpTcpSocket,str2,str3,str4);
    }
    else if(str1 == REGIST)
    {
        registctl(tmpTcpSocket,str2,str3,str4);
    }
    else if(str1 == GETFAMILY)
    {
        getfamilyname(tmpTcpSocket,str2);
    }
    else if(str1 == GETMEMBERINFO)
    {
        getmemberinfo(tmpTcpSocket,recvmsg);
    }
    else if(str1 == ADDMEMBER)
    {
        addmember(tmpTcpSocket,recvmsg);
    }
}

//app家庭登录处理
void Mainwindow::loginctl(QTcpSocket *socket, QString str2, QString str3,QString str4)
{
    QString sql = QString("select * from family_app where deviceid=%1").arg(str4.toInt());
    QString res = LOGIN;
    QSqlQuery query;
    query.exec(sql);
    //QSqlQuery返回的数据集，record是停在第一条记录之前的。所以，你获得数据集后，必须执行next()或first()到第一条记录，这时候record才是有效的
    if (!query.first())
    {
        res += " " + QString::number(LOGIN_DEVID_ERR) + "\n";
    }
    else{
        //比较家庭名
        if(!QString::compare(str2,query.value(1).toString())){
            //比较密码
            if(!QString::compare(str3,query.value(2).toString())){
                res += " " + QString::number(LOGIN_SUCCESS)+ "\n";
            }
            else {
                res += " " + QString::number(LOGIN_KEY_ERR)+ "\n";
            }
        }
        else {
            res += " " + QString::number(LOGIN_FAMILY_ERR)+ "\n";
        }

    }
    socket->write(res.toUtf8());

}

//app家庭注册处理
void Mainwindow::registctl(QTcpSocket *socket, QString str2, QString str3,QString str4)
{
    QString sql = QString("select * from family_app where deviceid=%1").arg(str4.toInt());
    QString res = REGIST;
    QSqlQuery query;
    query.exec(sql);
    //QSqlQuery返回的数据集，record是停在第一条记录之前的。所以，你获得数据集后，必须执行next()或first()到第一条记录，这时候record才是有效的
    if (!query.first())
    {
        res += " " + QString::number(REGIST_SUCCESS) +"\n";
        //数据存入数据库
        sql = QString("insert into family_app('deviceid','family','pswd') values(%1,'%2','%3')").arg(str4.toInt()).arg(str2).arg(str3);
        if (!query.exec(sql))   //执行sql语句
        {

            qDebug() << "app 注册"<<query.lastError().text();
            return;
        }
    }
    else{
        res += " " + QString::number(REGIST_ERR) + "\n";
    }
    socket->write(res.toUtf8());
}

//获取对应家庭名之下的所有家庭成员名
void Mainwindow::getfamilyname(QTcpSocket *socket, QString str2)
{
    QString sql = QString("select * from family_app where deviceid=%1").arg(str2.toInt());
    QString res = GETFAMILY;
    QSqlQuery query;
    query.exec(sql);
    //QSqlQuery返回的数据集，record是停在第一条记录之前的。所以，你获得数据集后，必须执行next()或first()到第一条记录，这时候record才是有效的
    if (!query.first())
    {
//        res += " " + QString::number(GETFAMILY_IS_NULL);
        qDebug()<<"GETFAMILY_IS_NULL";
    }
    else{
        //获取成员名
        QSqlRecord rec = query.record();//获取列数
        int i = 3;
        while(i<rec.count()){
            res += " " + query.value(i).toString();
            i++;
        }
    }
    res += "\n";
    socket->write(res.toUtf8());
}

//获取成员健康信息
void Mainwindow::getmemberinfo(QTcpSocket *socket, QString str)
{
    QString deviceid = str.section(" ",1,1);
    QString memberName = str.section(" ",2,2);
    QDateTime oneWeekAgo = QDateTime::currentDateTime().addDays(-7);

    QSqlQuery query;
    query.prepare("SELECT * FROM user "
                  "WHERE device = :devid  AND membername = :member_name "
                  "AND time >= :one_week_ago");
    query.bindValue(":devid", deviceid.toInt());
    query.bindValue(":member_name", memberName);
    query.bindValue(":one_week_ago", oneWeekAgo);
    if (!query.exec()){   //执行sql语句
        qDebug() << query.lastError().text()<<" 查询成员信息语句出错";
        return;
    }
    while (query.next()) {
        QDateTime timestamp = query.value(0).toDateTime();
        memberName = query.value(3).toString();
        int mfid = query.value(4).toInt();
        float mtemperature = query.value(7).toFloat();
        int mheart = query.value(5).toInt();
        int mblood = query.value(6).toInt();
        QString message = QString("%1 %2 %3 %4 %5 %6 %7 %8\n")
                          .arg(QString(GETMEMBERINFO))
                          .arg(memberName)
                          .arg(QString(N))
                          .arg(timestamp.toString("yyyy-MM-dd,hh:mm:ss"))
                          .arg(mfid)
                          .arg(mtemperature)
                          .arg(mheart)
                          .arg(mblood);
        socket->write(message.toUtf8());
    }
    //发送结束标志
    QString res = QString("%1 %2 %3\n")
                      .arg(QString(GETMEMBERINFO))
                      .arg(memberName)
                      .arg(QString(Y));
    socket->write(res.toUtf8());
}
//添加家庭成员
void Mainwindow::addmember(QTcpSocket *socket, QString str)
{
    //接收协议头 设备id 家庭名 成员名 指纹id
    QString deviceid = str.section(" ",1,1);
    QString familyName = str.section(" ",2,2);
    QString memberName = str.section(" ",3,3);
    QString memberFid = str.section(" ",4,4);
    QString res = ADDMEMBER;
    QSqlQuery query;
    //查询指纹是否已被使用，限制只查询一条记录即可
    //使用 LIMIT 语句可以限制查询结果的数量，从而提高查询速度。
    //当数据表非常庞大时，使用 LIMIT 语句可以避免查询过多的数据，从而减少查询时间和资源消耗，提高查询效率
    query.prepare("SELECT * FROM user "
                  "WHERE device = :deviceid AND memberfid = :member_fid limit 1"
                  );
    query.bindValue(":deviceid", deviceid.toInt());
    query.bindValue(":member_fid", memberFid.toInt());
    if (!query.exec()){   //执行sql语句
        qDebug() << query.lastError().text()<<" 查询成员信息语句出错";
        return;
    }
    if (!query.first())
    {
//        //查询一条记录，获取设备id
//        query.prepare("SELECT * FROM user "
//                      "WHERE device = :deviceid limit 1"
//                      );
//        query.bindValue(":deviceid", deviceid);
//        query.exec();
//        if (query.next()) {
//            deviceid = query.value("device").toInt();
//            //qDebug()<<"device="<<deviceid;
//        } else {
//            qDebug() << "未查找到该家庭信息";
//            return;
//        }
        //向设备发送添加指纹消息，JSON格式
        //例如{"addmember": 567,"memberfid": 3}
        QJsonObject jsonObj;
        jsonObj.insert("addmember",deviceid.toInt());
        jsonObj.insert("memberfid",memberFid.toInt());
        //QJsonDocument将QJsonObject转换为Json格式的文档，toJson()方法将文档转换为QByteArray，最后使用fromUtf8()方法将QByteArray转换为QString
        QJsonDocument jsonDoc(jsonObj);
        QString mqttmsg = QString::fromUtf8(jsonDoc.toJson());

        mqtt_client->publish(mqtt_topic_addmember,mqttmsg.toUtf8());
        //存储添加的这名成员的信息
        //不直接把成员消息发给设备，一方面是考虑尽量不把用户私人信息进行传输以免泄露，另一方面是mqtt通信中文乱码我实在不好解决
        QString minfo = deviceid + "," + familyName + "," + memberName + "," + memberFid;
        addMemberList.append(minfo);
        //将设备id和tcp客户端套接字存入容器，不然到时候无法给对应客户端发送执行结果信息
        map_devforapp.insert(deviceid.toInt(),socket);
    }
    else{
        //该指纹id已被其他成员使用
        res += " " + QString::number(ADDMEMBER_REPEAT) + "\n";
        socket->write(res.toUtf8());
    } 
}

//当mqtt完成连接时
void Mainwindow::brokerConnected()
{

    if(mqtt_client->state() == QMqttClient::Connected){
        //订阅主题
        mqtt_client->subscribe(mqtt_topic1);
        mqtt_client->subscribe(mqtt_topic_addmember);
        connect(mqtt_client, SIGNAL(messageReceived(QByteArray,QMqttTopicName)), this, SLOT(mqttMsgReceived(QByteArray , QMqttTopicName)));
    }

}

//mqtt接收消息
void Mainwindow::mqttMsgReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString content;
    User user;
    user.setTime(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    content =  user.getTime() + QLatin1Char('\n');
    content += QLatin1String("Topic:") + topic.name() + QLatin1Char(',');
    content += QLatin1String("Message:") + message + QLatin1Char('\n');
    ui->textBrowser_mqttclient->insertPlainText(content);

    QByteArray msg = message;
    //用户健康信息主题
    if(!QString::compare(topic.name(),mqtt_topic1)){
        //健康信息存入数据库
        //解析json
        getJsonValue(msg,user);
        //根据设备id和指纹id，以及数据库已有记录，补全家庭名和成员名
        //成员首次注册,设备必须发送一次家庭名和成员名的，手机App->Qt服务端->emqx服务器->设备。在注册模式下设备会再把家庭名成员名发给Qt服务端
        //多条件查询（字段名1，字段名2）=（值1，值2）
        qDebug()<<"device:"<<user.getDevice();
        if(user.getDevice() !=0)//表示设备号为非默认值，是用户数据上传
        {
            QString sql =QString("select * from user where (device,memberfid) = (%1,%2)").arg(user.getDevice()).arg(user.getMemberfid());
            QSqlQuery query;
            query.exec(sql);
            if (!query.first())
            {              
                qDebug()<<"没有在历史数据表里查到该成员信息";
                return;
            }
            else{
                //设备在非注册模式下，不发送家庭名成员名，由Qt服务端根据数据库补齐
                //因为设备如果掉电，家庭名成员名是无法保存的。当然我可以给设备加个存储器，但是我觉得在网络传输中传输家庭和姓名有一定风险
                //特别是姓名和指纹id都传输的时候，可能会暴露用户指纹数据在指纹模块中的存储位置
                user.setFamily(query.value(2).toString());
                user.setMembername(query.value(3).toString());
            }
            //数据存入数据库
            //这里就不使用sql语句，而采用bindValue模式
            query.prepare("INSERT INTO user "
                          "VALUES(:time, :device, :family, :membername,:memberfid, :heart, :blood, :temperature)");
            query.bindValue(":time", user.getTime());
            query.bindValue(":device", user.getDevice());
            query.bindValue(":family", user.getFamily());
            query.bindValue(":membername", user.getMembername());
            query.bindValue(":memberfid", user.getMemberfid());
            query.bindValue(":heart", user.getHeart());
            query.bindValue(":blood", user.getBlood());
            query.bindValue(":temperature", user.getTemperature());

            if (!query.exec())   //执行sql语句
            {

                qDebug() << query.lastError().text()<<" ++++";
                return;
            }
        }
    }
    //添加成员主题
    else if(!QString::compare(topic.name(),mqtt_topic_addmember)){
        int devid ;
        QString protocol;
        //对消息进行json解析
        QJsonParseError error;
        QJsonDocument doc;
        doc = QJsonDocument::fromJson(msg, &error);
        if(error.error == QJsonParseError::NoError)
        {
            if(doc.isObject())
            {
                QJsonObject obj = doc.object();
                if(obj.contains("device")){
                    QJsonValue value = obj.value("device");
                    if(value.isDouble()){//整型的解析
                        devid  = value.toVariant().toInt();
                    }
                }
                if(obj.contains("protocol")){//字符串的解析
                    QJsonValue value = obj.value("protocol");
                    if(value.isString()){
                        protocol = value.toString();

                    }
                }
            }
        }
        //此条消息不是录入结果则结束
        if(protocol.isEmpty())return;

        qDebug()<<"dev="<<devid<<",pro="<<protocol;
        qDebug()<<"map:"<<map_devforapp;
        qDebug()<<"addmlist:"<<addMemberList;
        QString res = ADDMEMBER;
        if(!QString::compare(protocol,ADDERR)){
            //设备在添加指纹时出错，则向qpp端返回结果
            res += " " + QString::number(ADDMEMBER_ERROR) +"\n";
            //确保容器内该设备和客户端关联还存在，避免设备重复发送时出错
            if(map_devforapp.contains(devid)){
                map_devforapp.value(devid)->write(res.toUtf8());
                //在容器中删除套接字
                map_devforapp.remove(devid);
            }
            //删除该成员添加的信息
            for (QStringList::iterator it = addMemberList.begin(); it != addMemberList.end(); it++) {
                QStringList subList = it->split(","); // 将元素分割成一个QStringList
                if (subList[0].toInt() == devid) { // 判断分割后的第一个元素是否等于目标设备id
                    addMemberList.removeOne(*it);
                    //targetElement = *it; // 找到了目标元素
                    break;
                }
            }
        }
        else if (!QString::compare(protocol,ADDSUC)) {
            //设备添加指纹成功
            res += " " + QString::number(ADDMEMBER_SUCESS) +"\n";
            //查找该成员添加的信息，存入数据库后删除
            for (QStringList::iterator it = addMemberList.begin(); it != addMemberList.end(); it++) {
                QStringList subList = it->split(","); // 将元素分割成一个QStringList
                if (subList[0].toInt() == devid) { // 判断分割后的第一个元素是否等于目标设备id
                    QSqlQuery query;
                    //在用户表中添加记录
                    query.prepare("INSERT INTO user (time, device, family, membername, memberfid) "
                                  "VALUES (:time, :device, :family, :membername, :memberfid)");
                    query.bindValue(":time", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")); // 当前时间为插入时间
                    query.bindValue(":device", devid);
                    query.bindValue(":family", subList[1]);
                    query.bindValue(":membername", subList[2]);
                    query.bindValue(":memberfid", subList[3].toInt());
                    if (!query.exec()) {
                        QMessageBox::warning(this, "插入新添加的成员", query.lastError().text());
                        return;
                    }
                    //在家庭表中添加成员
                    //1.首先查询出这个家庭在family_app表中的记录
                    QString sql = QString("SELECT * FROM family_app WHERE deviceid=%1").arg(devid);
                    if (!query.exec(sql))
                    {
                        qDebug() << "Failed to query database.";
                        return ;
                    }
                    //2.判断family之后的字段是否都还是预设值，对还是预设值的首个字段的值修改。若都不是预设值则新添加一个字段再赋值
                    if(query.next()){
                        QSqlRecord rec = query.record();//获取该条记录
                        int i = 2;
                        while(i<rec.count()){//count获取列数,即字段个数
                            if(!QString::compare(query.value(i).toString(),"0")){
                                QString updateSql = QString("UPDATE family_app SET %1='%2' WHERE deviceid=%3").arg(rec.fieldName(i)).arg(QString(subList[2])).arg(devid);
                                qDebug()<<"sql1:"<<updateSql;
                                if (!query.exec(updateSql))
                                {
                                    qDebug() << "Failed to update database.";
                                    return ;
                                }
                                break;
                            }
                            i++;
                        }
                        if(i==rec.count()){//查询到最后一条后也没有找到值为"0"的字段
                            //添加新字段，
                            QString column_name = "member";
                            column_name += QString::number(i-2);//最后一列member编号比列数大3，所以-2就能衔接上
                            QString addSql = QString("ALTER TABLE family_app ADD COLUMN %1 varchar(32) default 0").arg(column_name);
                            qDebug()<<"sql2:"<<addSql;
                            if (!query.exec(addSql))
                            {
                                qDebug() << "Failed to add column to database.";
                                return ;
                            }
                            //并赋值
                            QString updateSql = QString("UPDATE family_app SET %1='%2' WHERE deviceid=%3").arg(column_name).arg(subList[2]).arg(devid);
                            qDebug()<<"sql3:"<<updateSql;
                            if (!query.exec(updateSql))
                            {
                                qDebug() << "Add_after failed to update database.";
                                return ;
                            }
                        }
                    }
                    //删除容器内临时数据
                    addMemberList.removeOne(*it);
                    break;
                }
            }
            //确保容器内该设备和客户端关联还存在，避免设备重复发送时出错
            if(map_devforapp.contains(devid)){
                map_devforapp.value(devid)->write(res.toUtf8());
                //在容器中删除套接字
                map_devforapp.remove(devid);
            }
        }
    }

}

//mqtt发布消息,主题默认/xm/health
void Mainwindow::on_pushButton_mqttsub_clicked()
{
    mqtt_msg = ui->lineEdit_mqttpub->text();
    if(mqtt_msg.isEmpty()){
        QMessageBox::warning(this,"mqtt发布","发布内容不能为空！");
        return;
    }

    //发布
    mqtt_client->publish(mqtt_topic1,mqtt_msg.toUtf8());
}

//tcp，当手机app客户端连接服务端时触发
void Mainwindow::mNewconnection()
{
    QTcpSocket *tmpTcpSocket = tcpServer->nextPendingConnection();
    connect(tmpTcpSocket,SIGNAL(readyRead()),this,SLOT(receviveMessages()));
    connect(tmpTcpSocket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this,SLOT(mStateChanged(QAbstractSocket::SocketState)));
}

//对tcp连接不同状态的处理
void Mainwindow::mStateChanged(QAbstractSocket::SocketState state)
{
    //获取该连接的连接套接字
    QTcpSocket *tmpTcpSocket = (QTcpSocket *)sender();
    //获取连接的客户端的ip，端口
    QString clientip = tmpTcpSocket->peerAddress().toString();
    //quint16 clientport = tmpTcpSocket->peerPort();
    //ui->textBrowser_tcpserver->append("num=" + QString::number(state));

    switch (state) {
    case QAbstractSocket::UnconnectedState :
        ui->textBrowser_tcpserver->append("IP:" + clientip + "已断开连接");
        tmpTcpSocket->deleteLater();//不能立即释放连接套接字，因为整个时间循环可能还没结束，其他地方还会用到这个指针
        break;

    case QAbstractSocket::ConnectingState :
        ui->textBrowser_tcpserver->append("IP:" + clientip + "已连接");
        break;

    default:
        break;
    }


}
//启动tcp服务端监听
void Mainwindow::on_start_listen_clicked()
{
    tcpServer->listen(QHostAddress::AnyIPv4,port);//AnyIPv4相当于"0.0.0.0"
    ui->start_listen->setEnabled(false);//按钮失能
    ui->stop_listen->setEnabled(true);//停止按钮，使能
}
//关闭监听
void Mainwindow::on_stop_listen_clicked()
{
    /* 如果是客户端连接上了也应该断开，如果不断开客户端还能继续发送信息，
     * 因为 socket 未断开，还在监听上一次端口 */
    QList <QTcpSocket *> socketlist = tcpServer->findChildren<QTcpSocket *>();
    foreach (QTcpSocket *tmpTcpSocket, socketlist) {
        tmpTcpSocket->disconnectFromHost();
    }
    ui->stop_listen->setEnabled(false);//按钮失能
    ui->start_listen->setEnabled(true);//开启按钮，使能

    tcpServer->close();
}


//tcp发送测试,服务端向所有已连接的客户端发送消息
void Mainwindow::on_pushButton_tcpsend_clicked()
{
    QString test = ui->lineEdit_tcpsend->text();
    if(test.isEmpty()){
        QMessageBox::warning(this,"tcp发布","发布内容不能为空！");
        return;
    }
    if(ui->start_listen->isEnabled()){
        QMessageBox::warning(this,"tcp发布","当前未开启监听,请开启服务后再尝试！");
        return;
    }
    QList <QTcpSocket *> socketlist = tcpServer->findChildren<QTcpSocket *>();
    if(socketlist.count() == 0){
        QMessageBox::warning(this,"tcp发布","当前没有手机app客户端连接，无法发送");
        return;
    }

    foreach (QTcpSocket *tmpTcpSocket, socketlist) {
        tmpTcpSocket->write(test.toUtf8());
    }
    QMessageBox::information(this,"tcp发布","系统消息已向所有已连接客户端发送");
}

//刷新显示
void Mainwindow::on_pushButton_refresh_clicked()
{
    //通过数据库创建数据模型并显示
    QSqlTableModel *model = new QSqlTableModel;
    //数据表model
    model = new QSqlTableModel(this, mydb);
    //设置数据表
    model->setTable("user");
    //查询数据
    if(!model->select())
    {
        QMessageBox::warning(this,"user表数据查询","数据查询失败");
        return;
    }

    //设置表头，如不设置则使用数据库中的默认表头
    model->setHeaderData(0, Qt::Horizontal, QStringLiteral("上传时间"));
    model->setHeaderData(1, Qt::Horizontal, QStringLiteral("设备ID"));
    model->setHeaderData(2, Qt::Horizontal, QStringLiteral("家庭名"));
    model->setHeaderData(3, Qt::Horizontal, QStringLiteral("家庭成员"));
    model->setHeaderData(4, Qt::Horizontal, QStringLiteral("指纹id"));
    model->setHeaderData(5, Qt::Horizontal, QStringLiteral("心率"));
    model->setHeaderData(6, Qt::Horizontal, QStringLiteral("血氧"));
    model->setHeaderData(7, Qt::Horizontal, QStringLiteral("体温"));

    while(model->canFetchMore())
        model->fetchMore();

    int column = model->columnCount(); //获取列数
    int row = model->rowCount(); //获取行数
    ui->label_columncount->setText(QString::number(column));
    ui->label_rowcount->setText(QString::number(row));

    ui->tableView->setModel(model);
    //ui->tableView->verticalHeader()->setHidden(true); //把QTableView中第一列的默认数字列去掉
    ui->tableView->resizeColumnsToContents();//根据内容自动调整所有列的列宽
}
