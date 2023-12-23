#ifndef USER_H
#define USER_H

#include <QObject>

class User : public QObject
{
    Q_OBJECT
public:
    explicit User(QObject *parent = nullptr);
    ~User(){}
    void setTime(QString time);
    void setDevice(uint16_t device);
    void setFamily(QString family);
    void setMembername(QString membername);
    void setMemberfid(uint16_t memberfid);
    void setHeart(int16_t heart);
    void setBlood(int16_t blood);
    void setTemperature(float temperature);

    QString getTime(void);
    uint16_t getDevice(void);
    QString getFamily(void);
    QString getMembername(void);
    uint16_t getMemberfid(void);
    int16_t getHeart(void);
    int16_t getBlood(void);
    float getTemperature(void);

signals:

private:
    QString time;
    //给默认值
    uint16_t device = 0; //0不在上传的设备号内
    QString family;
    QString membername;
    uint16_t memberfid;
    //给默认值
    int16_t heart = 0;
    int16_t blood = 0;
    float temperature = 0;

};

#endif // USER_H
