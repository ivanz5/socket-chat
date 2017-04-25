#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QHostAddress>
#include <QVector>

class Message
{
private:
    QString text;
    QHostAddress* hostAddress;
    bool out;

public:
    Message();
    Message(QString text, QHostAddress* hostAddress);
    QString getText();
    QString getDisplayName(const QVector<QHostAddress*>& addresses, const QVector<QString>& names);
    QString getDisplayString(const QVector<QHostAddress*>& addresses, const QVector<QString>& names);
    void setText(QString text);
    void setHostAddress(QHostAddress* hostAddress);
};

#endif // MESSAGE_H
