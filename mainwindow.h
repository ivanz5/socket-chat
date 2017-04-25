#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHostAddress>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QVector>
#include <QListWidgetItem>
#include "message.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void readSocket();

private slots:
    void on_connectButton_clicked();
    void on_sendButton_clicked();
    void on_changePortButton_clicked();
    void on_changeNameButton_clicked();
    void onClientsListItemClicked(QListWidgetItem*);

private:
    static const quint16 defaultPort = 1234;

    Ui::MainWindow *ui;
    QUdpSocket* socket;
    QHostAddress myAddress;
    quint16 myPort;
    quint16 remotePort;

    // Users, messages
    int selectedUserId; // index in following vectors
    QVector<QString> nameList;
    QVector<QHostAddress*> addressList;
    QVector<QVector<Message*>> messages; // no no no

    void bindSocket();
    void sendOnlineBroadcast();
    void processNewClient(QString connMessage, QHostAddress address);
    void addMessage(QString text, QHostAddress* address, bool privateMessage, int toId);
    void showMessageHistory(int userId);

};

#endif // MAINWINDOW_H