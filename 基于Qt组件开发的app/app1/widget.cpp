#include "widget.h"
#include "ui_widget.h"

//TCP服务端ip和端口
QString server_ip = "116.62.116.146";
//QString server_ip = "192.168.43.231"; //自己电脑上测试用
quint16 server_port = 9999;

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //初始化三张图表
    initchart();
    //为适应Android 智能手机各种各样的屏幕尺寸，就不能再使用px了，而是dpi
    //获取设备DPI
    QScreen *screen = qApp->primaryScreen();
    QSize screenSize = screen->size();
    //qDebug()<<"size:"<<screenSize;
    //控件的大小。且在布局中设为固定大小.其值跟随设备DPI适配
    ui->loginButton->setFixedSize(screenSize.width()/4,screenSize.height()/18);
    ui->registButton->setFixedSize(screenSize.width()/4,screenSize.height()/18);
    ui->quitButton->setFixedSize(screenSize.width()/4,screenSize.height()/18);
    ui->pushButton_health->setFixedHeight(screenSize.height()/22);
    ui->pushButton_addmember->setFixedHeight(screenSize.height()/22);
    ui->pushButton_confirmadd->setFixedSize(screenSize.width()/3,screenSize.height()/16);
    ui->pushButton_backfromadd->setText("");
    QPixmap pixmap(":/back.png");
    QPixmap fitpixmap = pixmap.scaled(screenSize.height()/16, screenSize.height()/16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->pushButton_backfromadd->setIcon(QIcon(fitpixmap));
    ui->pushButton_backfromadd->setIconSize(QSize(screenSize.height()/16, screenSize.height()/16));
    ui->pushButton_backfromadd->setStyleSheet("border: 5px");
    // 开启背景设置
    this->setAutoFillBackground(true);
    // 创建调色板对象
    QPalette p = this->palette();
    // 加载图片
    QPixmap pix(":/applogin.jpg");
    QPixmap fitpix = pix.scaled(screenSize);
    // 设置图片
    p.setBrush(QPalette::Window, QBrush(fitpix));
    this->setPalette(p);

    //堆叠窗口，初始化显示登录界面
    ui->stackedWidget->setCurrentIndex(0);//登录窗口
    //界面美化和优化部分
    ui->pushButton_confirmadd->setText("确认添加");
    //tcp
    tcpSocket = new QTcpSocket(this);
    //连接服务端，参数服务端ip和端口
    tcpSocket->connectToHost(QHostAddress(server_ip),server_port);
    connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(recviveMessages()));
    connect(tcpSocket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(mStateChanged(QAbstractSocket::SocketState)));



}

Widget::~Widget()
{
    delete ui;
    tcpSocket->disconnectFromHost();//断开tcp连接

}

void Widget::recviveMessages()
{
    QString data = tcpSocket->readAll();
    qDebug()<<"服务端:" + data;
    QStringList lines = data.split("\n");//分割出每一行数据
    lines.removeLast();//调试的时候发现总会有个空字符串，暂时不知道为啥
    foreach(QString line,lines) {
        qDebug()<<"line:"<<line;
        QStringList fields = line.split(" ");//分割出每一个子串
        //下面只取信息里的协议头和返回值
        QString str1 = fields[0];
        QString str2 = fields[1];

        if(str1 == LOGIN){
            loginctl(str2);
        }
        else if(str1 == REGIST){
            registctl(str2);
        }
        else if(str1 == GETFAMILY){
            //当前窗口为家庭信息窗口时才设置
            if(ui->stackedWidget->currentIndex() == 1){
                QStringList list = line.split(" ");
                list.removeAt(0);//移除协议头
                list.removeAll("0");//表中成员名默认初始值为"0",所以需要移除
                //qDebug()<<"list="<<list;
                ui->comboBox->addItems(list);
            }
        }
        else if(str1 == GETMEMBERINFO){
            memberinfoctl(line);
        }
        else if(str1 == ADDMEMBER){
            if(str2.toInt() == ADDMEMBER_REPEAT){
                QMessageBox::warning(this,"添加新成员","该指纹id已被其他成员使用，添加失败");
            }
            else if(str2.toInt() == ADDMEMBER_ERROR){
                QMessageBox::warning(this,"添加新成员","设备在添加指纹时出错，添加失败");
            }
            else if (str2.toInt() == ADDMEMBER_SUCESS) {
                QMessageBox::information(this,"添加新成员","添加成功!");
                //下拉条增加该成员
                ui->comboBox->addItem(ui->lineEdit_addmname->text());
            }
            //恢复控件使能
            ui->pushButton_confirmadd->setText("确认添加");
            ui->pushButton_confirmadd->setEnabled(true);
            ui->pushButton_backfromadd->setEnabled(true);
            ui->lineEdit_addmname->setEnabled(true);
            ui->lineEdit_addmfid->setEnabled(true);
        }


    }


}
//登录结果处理
void Widget::loginctl(QString str)
{
    int mode = str.toInt();
    switch(mode)
    {
        case LOGIN_FAMILY_ERR:
            QMessageBox::warning(this,"登录","设备已注册但家庭名输入错误");
            ui->UserName->clear();
            ui->Userpwd->clear();
            break;
        case LOGIN_DEVID_ERR:
            QMessageBox::warning(this,"登录","该设备尚未完成注册");
            break;
        case LOGIN_KEY_ERR:
            QMessageBox::warning(this,"登录","密码错误");
            ui->Userpwd->clear();
            break;
        case LOGIN_SUCCESS:
            QMessageBox::information(this,"登录","登陆成功");
            //跳转到家庭健康信息页面--page2--索引1
            ui->stackedWidget->setCurrentIndex(1);
            this->family_infoface();

            break;
        default:
            qDebug()<<"login error";

    }
}
//注册结果处理
void Widget::registctl(QString str2)
{
    int mode = str2.toInt();
    qDebug()<<"mode="<<mode;
    switch(mode)
    {
        case REGIST_ERR:
            QMessageBox::warning(this,"注册","该设备已被注册，注册失败！");
//            ui->UserName->clear();
//            ui->Userpwd->clear();
            break;
        case REGIST_SUCCESS:
            QMessageBox msgBox;
            msgBox.setText("注册成功！");
            msgBox.setInformativeText("是否自动登录？");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setButtonText(QMessageBox::Ok, "是"); //修改按钮上的文字
            msgBox.setButtonText(QMessageBox::Cancel, "否");
            msgBox.setDefaultButton(QMessageBox::Cancel);  //设置默认按钮
            int ret = msgBox.exec();  //模态对话框
            switch (ret) {
                case QMessageBox::Ok:
                    //跳转到家庭健康信息页面--page2--索引1
                    ui->stackedWidget->setCurrentIndex(1);
                    this->family_infoface();
                    break;
                case QMessageBox::Cancel:
                    break;
                default:
                    qDebug()<<"regist success error";
                    break;
            }
            break;
        //default:
            //qDebug()<<"regist error";
    }
}

//获取成员健康信息
void Widget::memberinfoctl(QString str)
{
    //协议头 成员名 发送状态 时间 指纹id 体温 心率 血氧
    QDateTime time;
    int memberid,heart,blood;
    float temperature;
    //判断发送状态Y为发送完毕，N未发送完
    qDebug()<<"flag:"<<str.section(" ",2,2);
    if(!QString::compare(str.section(" ",2,2),QString(Y))){
        paintchart();
    }
    else {
        time = QDateTime::fromString(str.section(" ",3,3),"yyyy-MM-dd,hh:mm:ss");
        memberid = str.section(" ",4,4).toInt();
        temperature = str.section(" ",5,5).toFloat();
        heart = str.section(" ",6,6).toInt();
        blood = str.section(" ",7,7).toInt();

        //未测量的值不录入
        if(temperature == 0){
            map_heart.insert(time,heart);
            map_blood.insert(time,blood);
        }
        if(heart == 0 || blood == 0){
            map_temperature.insert(time,temperature);
        }

        //将指纹id显示在界面上，并设置格式如1，“001”
        ui->label_memberid->setText(QString("  %1").arg(memberid,3,10,QChar('0')));
        qDebug()<<"time:"<<time.toString()<<",fid:"<<memberid<<",heart:"<<heart<<",blood:"<<blood;
        qDebug()<<"temp:"<<temperature;

    }

}

void Widget::initchart()
{
    //起始时间
    QDate m_BaseDate = QDate::currentDate().addDays(-6);
    QTime m_BaseTime = QTime(0, 0, 0);
    //1.创建图表视图
    //QChartView *chartview1 = new QChartView(); //ui文件中已创建
    //2.实例化图表
    chart_temperature = new QChart();
    //3.实例坐标轴
    timeAxisX = new QDateTimeAxis(); //时间轴
    valueAxisY = new QValueAxis();
    //4.设置坐标轴范围
    QDateTime temp_StartTime(m_BaseDate,m_BaseTime);
    timeAxisX->setRange(temp_StartTime,temp_StartTime.addDays(6));
    valueAxisY->setRange(35,40);
    //5.设置坐标轴标题和显示格式
    //valueAxisX->setTitleText("时间");
    //valueAxisY->setTitleText("体温/℃");
    timeAxisX->setFormat("M.d");
    valueAxisY->setLabelFormat("%d");
    //设置分隔数
    timeAxisX->setTickCount(7);
    valueAxisY->setTickCount(6);
    //6.图表添加坐标轴
    chart_temperature->createDefaultAxes();
    chart_temperature->addAxis(timeAxisX,Qt::AlignBottom);
    chart_temperature->addAxis(valueAxisY,Qt::AlignLeft);
    //7.设置图表标题和图例是否显示
    chart_temperature->setTitle("最近一周体温数据");
    chart_temperature->legend()->setVisible(false);

    //1.创建图表视图
    //QChartView *chartview1 = new QChartView(); //ui文件中已创建
    //2.实例化图表
    chart_heart = new QChart();
    //3.实例坐标轴
    //时间轴,之前三个图表共用一条时间轴，结果报了使用纯虚函数的错，干脆三张表独立开来
    timeAxisX_heart = new QDateTimeAxis();
    valueAxisY_heart = new QValueAxis();
    //4.设置坐标轴范围
    QDateTime temp_StartTime1(m_BaseDate,m_BaseTime);
    timeAxisX_heart->setRange(temp_StartTime1,QDateTime::currentDateTime());
    valueAxisY_heart->setRange(60,90);
    //5.设置坐标轴标题和显示格式
    timeAxisX_heart->setFormat("M.d");
    valueAxisY_heart->setLabelFormat("%d");
    //设置分隔数
    timeAxisX_heart->setTickCount(7);
    valueAxisY_heart->setTickCount(7);
    //6.图表添加坐标轴
    chart_heart->createDefaultAxes();
    chart_heart->addAxis(timeAxisX_heart,Qt::AlignBottom);
    chart_heart->addAxis(valueAxisY_heart,Qt::AlignLeft);
    //7.设置图表标题和图例是否显示
    chart_heart->setTitle("最近一周心率数据");
    chart_heart->legend()->setVisible(false);

    //1.创建图表视图
    //QChartView *chartview1 = new QChartView(); //ui文件中已创建
    //2.实例化图表
    chart_blood = new QChart();
    //3.实例坐标轴
    //时间轴,之前三个图表共用一条时间轴，结果报了使用纯虚函数的错，干脆三张表独立开来
    timeAxisX_blood = new QDateTimeAxis();
    valueAxisY_blood = new QValueAxis();
    //4.设置坐标轴范围
    QDateTime temp_StartTime2(m_BaseDate,m_BaseTime);
    timeAxisX_blood->setRange(temp_StartTime2,QDateTime::currentDateTime());
    valueAxisY_blood->setRange(88,100);
    //5.设置坐标轴标题和显示格式
    timeAxisX_blood->setFormat("M.d");
    valueAxisY_blood->setLabelFormat("%d");
    //设置分隔数
    timeAxisX_blood->setTickCount(7);
    valueAxisY_blood->setTickCount(7);
    //6.图表添加坐标轴
    chart_blood->createDefaultAxes();
    chart_blood->addAxis(timeAxisX_blood,Qt::AlignBottom);
    chart_blood->addAxis(valueAxisY_blood,Qt::AlignLeft);
    //7.设置图表标题和图例是否显示
    chart_blood->setTitle("最近一周血氧数据");
    chart_blood->legend()->setVisible(false);

    //实例化曲线
    splineSeries = new QSplineSeries();
    splineSeries_heart = new QSplineSeries();
    splineSeries_blood = new QSplineSeries();
    //设置曲线颜色
    QPen pen(QColor(0xff5566));
    pen.setWidth(5);
    splineSeries->setPen(pen);
    //图表添加曲线
    chart_temperature->addSeries(splineSeries);
    //将曲线的数据附属到坐标轴上，注意这要在图表添加曲线之后
    splineSeries->attachAxis(timeAxisX);
    splineSeries->attachAxis(valueAxisY);
    //将图表放置于图表视图
    ui->chartView_temp->setChart(chart_temperature);

    QPen pen1(QColor(0xff5566));
    pen1.setWidth(5);
    splineSeries_heart->setPen(pen1);
    //图表添加曲线
    chart_heart->addSeries(splineSeries_heart);
    //将曲线的数据附属到坐标轴上，注意这要在图表添加曲线之后
    splineSeries_heart->attachAxis(timeAxisX_heart);
    splineSeries_heart->attachAxis(valueAxisY_heart);
    //将图表放置于图表视图
    ui->chartView_heart->setChart(chart_heart);

    QPen pen2(QColor(0xff5566));
    pen2.setWidth(5);
    splineSeries_blood->setPen(pen2);
    //图表添加曲线
    chart_blood->addSeries(splineSeries_blood);
    //将曲线的数据附属到坐标轴上，注意这要在图表添加曲线之后
    splineSeries_blood->attachAxis(timeAxisX_blood);
    splineSeries_blood->attachAxis(valueAxisY_blood);
    //将图表放置于图表视图
    ui->chartView_blood->setChart(chart_blood);
}

void Widget::paintchart()
{
    //擦除图表上原有曲线
    //如何判断曲线上是否有点，并没有直接方法，可以通过获取所有点判断是否为空表
    QList<QPointF> points = splineSeries->points(); // 获取所有点坐标
    if (!points.isEmpty()) {
        //清空曲线上的点
        splineSeries->clear();
    }
    QList<QPointF> points1 = splineSeries_heart->points(); // 获取所有点坐标
    if (!points1.isEmpty()) {
        //清空曲线上的点
        splineSeries_heart->clear();
    }
    QList<QPointF> points2 = splineSeries_blood->points(); // 获取所有点坐标
    if (!points2.isEmpty()) {
        //清空曲线上的点
        splineSeries_blood->clear();
    }
    //体温曲线对象添加点
    //遍历容器，采用java类型迭代，将时间转为时间戳，关联上坐标轴
    QMapIterator<QDateTime, float> iterator(map_temperature);
    while (iterator.hasNext()) {
        iterator.next();
        qDebug() << iterator.key() << ":" << iterator.value();
        splineSeries->append(iterator.key().toMSecsSinceEpoch(),iterator.value());
    }
    //心率曲线对象添加点
    QMapIterator<QDateTime, int> iterator1(map_heart);
    while (iterator1.hasNext()) {
        iterator1.next();
        qDebug() << iterator1.key() << ":" << iterator1.value();
        splineSeries_heart->append(iterator1.key().toMSecsSinceEpoch(),iterator1.value());
    }
    //血氧曲线对象添加点
    QMapIterator<QDateTime, int> iterator2(map_blood);
    while (iterator2.hasNext()) {
        iterator2.next();
        qDebug() << iterator2.key() << ":" << iterator2.value();
        splineSeries_blood->append(iterator2.key().toMSecsSinceEpoch(),iterator2.value());
    }

}
void Widget::mStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        qDebug()<<"断开连接";
        break;

    case QAbstractSocket::ConnectedState:
        qDebug()<<"已连接";
        break;

    default:
        break;

    }
}

void Widget::on_loginButton_clicked()
{
    int devid = ui->lineEdit_devid->text().toInt();
    if (ui->UserName->text().isEmpty() || ui->Userpwd->text().isEmpty()||ui->lineEdit_devid->text().isEmpty()){
        QMessageBox::warning(this, "家庭登录", "设备、家庭名和密码不能为空!");
        return;
    }
    if(devid < 0){
        QMessageBox::warning(this, "家庭登录", "设备id不能为负数!");
        return;
    }

    QString msg = LOGIN;
    msg += " " + ui->UserName->text() + " " + ui->Userpwd->text() + " " + QString::number(devid);
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
        tcpSocket->write(msg.toUtf8());
    else {
        QMessageBox::warning(this,"登录","连接服务器失败，请检查您的网络");
    }
}

void Widget::on_registButton_clicked()
{
    int devid = ui->lineEdit_devid->text().toInt();
    if (ui->UserName->text().isEmpty() || ui->Userpwd->text().isEmpty()||ui->lineEdit_devid->text().isEmpty()){
        QMessageBox::warning(this, "家庭注册", "家庭名,密码,设备id不能为空!");
        return;
    }
    if(devid < 0){
        QMessageBox::warning(this, "家庭注册", "设备id不能为负!");
        return;
    }
    QString msg = REGIST;
    msg +=" "+ ui->UserName->text() + " " + ui->Userpwd->text() + " " + QString::number(devid);
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
        tcpSocket->write(msg.toUtf8());
    else {
        QMessageBox::warning(this,"注册","连接服务器失败，请检查您的网络");
    }
}

void Widget::on_quitButton_clicked()
{
    qApp->quit();
}

//对家庭信息窗口初始化以及向服务端发送数据获取请求
void Widget::family_infoface()
{
    ui->label_fname->setText(ui->UserName->text());
    ui->label_devid->setText(ui->lineEdit_devid->text());
    QString msg = GETFAMILY;
    msg += " " + ui->lineEdit_devid->text();
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
        tcpSocket->write(msg.toUtf8());
    else {
        QMessageBox::warning(this,"通信异常","连接服务器失败，请检查您的网络");
    }


}

void Widget::on_comboBox_currentIndexChanged(const QString &arg1)
{
    qDebug()<<"box:"<<arg1;

    //清空容器原有数据
    map_temperature.clear();
    map_heart.clear();
    map_blood.clear();
    //重置指纹标签
    ui->label_memberid->setText("---");
    //发送获取数据请求
    QString membername = arg1;
    QString msg = GETMEMBERINFO;
    msg += " " + ui->label_devid->text();
    msg += " " + membername;
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
        tcpSocket->write(msg.toUtf8());
    else {
        QMessageBox::warning(this,"通信异常","连接服务器失败，请检查您的网络");
    }
}

//跳转到添加新成员页面--pqge3--索引2
void Widget::on_pushButton_addmember_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);

}

//退出登录,返回家庭登录页面--page--索引0
void Widget::on_pushButton_health_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
    //清空下拉条
    ui->comboBox->clear();
}
//从添加成员页面返回
void Widget::on_pushButton_backfromadd_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);


}
//确认添加
void Widget::on_pushButton_confirmadd_clicked()
{
    if (ui->lineEdit_addmname->text().isEmpty() || ui->lineEdit_addmfid->text().isEmpty()){
        QMessageBox::warning(this, "添加新成员", "姓名和指纹id不能为空!");
        return;
    }
    QString membername = ui->lineEdit_addmname->text();
    int memberfid = ui->lineEdit_addmfid->text().toInt();
    if(memberfid >299 || memberfid < 0){
        QMessageBox::warning(this, "添加新成员", "指纹id超过可设置范围");
        ui->lineEdit_addmfid->clear();
        return;
    }
    //向服务端发送添加请求
    QString msg = ADDMEMBER;
    msg += " " + ui->label_devid->text();
    msg += " " + ui->label_fname->text();
    msg += " " + membername;
    msg += " " + QString::number(memberfid);
    if(tcpSocket->state() == QAbstractSocket::ConnectedState)
        tcpSocket->write(msg.toUtf8());
    else {
        QMessageBox::warning(this,"通信异常","连接服务器失败，请检查您的网络");
    }

    ui->pushButton_confirmadd->setText("执行中..");
    ui->pushButton_confirmadd->setEnabled(false);
    ui->pushButton_backfromadd->setEnabled(false);
    //在执行期间除了按钮失能，输入框也要失能
    ui->lineEdit_addmname->setEnabled(false);
    ui->lineEdit_addmfid->setEnabled(false);

}
