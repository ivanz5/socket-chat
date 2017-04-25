#include "clientitem.h"

ClientItem::ClientItem()
{
}

QHostAddress* ClientItem::getHostAddress()
{
    return hostAddress;
}

QString ClientItem::getName()
{
    return name;
}

void ClientItem::setHostAddress(QHostAddress* address)
{
    this->hostAddress = address;
    this->setText(name + "\n" + (hostAddress == nullptr ? "" : hostAddress->toString()));
}

void ClientItem::setName(QString name)
{
    this->name = name;
    this->setText(name + "\n" + (hostAddress == nullptr ? "" : hostAddress->toString()));
}
