#include "message.h"

Message::Message()
{
}

Message::Message(QString text, QHostAddress* hostAddress)
{
    this->text = text;
    this->hostAddress = hostAddress;
}

QString Message::getText()
{
    return text;
}

QString Message::getDisplayName(const QVector<QHostAddress*>& addresses, const QVector<QString>& names)
{
    if (hostAddress == nullptr) return "Me";
    for (int i = 1; i < addresses.size(); i++){
        if (*hostAddress == *addresses[i]) return names[i];
    }
    return "Me";
}

QString Message::getDisplayString(const QVector<QHostAddress*>& addresses, const QVector<QString>& names)
{
    return (getDisplayName(addresses, names) + ": " + text);
}

void Message::setText(QString text)
{
    this->text = text;
}

void Message::setHostAddress(QHostAddress* hostAddress)
{
    this->hostAddress = hostAddress;
}
