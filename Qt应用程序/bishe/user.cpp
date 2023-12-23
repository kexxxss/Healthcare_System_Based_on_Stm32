#include "user.h"

User::User(QObject *parent) : QObject(parent)
{

}

void User::setTime(QString time)
{
    this->time = time;
}

void User::setDevice(uint16_t device)
{
    this->device = device;
}

void User::setFamily(QString family)
{
    this->family = family;
}

void User::setMembername(QString membername)
{
    this->membername = membername;
}

void User::setMemberfid(uint16_t memberfid)
{
    this->memberfid = memberfid;
}

void User::setHeart(int16_t heart)
{
    this->heart = heart;
}

void User::setBlood(int16_t blood)
{
    this->blood = blood;
}

void User::setTemperature(float temperature)
{
    this->temperature = temperature;
}

QString User::getTime()
{
    return this->time;
}

uint16_t User::getDevice()
{
    return this->device;
}

QString User::getFamily()
{
    return this->family;
}

QString User::getMembername()
{
    return this->membername;
}

uint16_t User::getMemberfid()
{
    return this->memberfid;
}

int16_t User::getHeart()
{
    return this->heart;
}

int16_t User::getBlood()
{
    return this->blood;
}

float User::getTemperature()
{
    return this->temperature;
}
