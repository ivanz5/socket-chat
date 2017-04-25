#ifndef CLIENTITEM_H
#define CLIENTITEM_H

#include <QListWidgetItem>
#include <QHostAddress>
#include <QString>

class ClientItem : public QListWidgetItem
{
public:
    ClientItem();
    QHostAddress* getHostAddress();
    QString getName();
    void setHostAddress(QHostAddress*);
    void setName(QString);

private:
    QHostAddress* hostAddress;
    QString name;
};

#endif // CLIENTITEM_H
