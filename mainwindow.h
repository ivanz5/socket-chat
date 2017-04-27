#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHostAddress>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QVector>
#include <QListWidgetItem>
#include "message.h"
#include <QFileDialog>
#include <QTcpSocket>
#include <QTcpServer>

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
    void on_sendButton_clicked();
    void on_changeNameButton_clicked();
    void onClientsListItemClicked(QListWidgetItem*);
    void on_fileButton_clicked();

    void filesSocketNewConnection();
    void filesSocketReadyRead();
    void filesSocketDisconnected();

private:
    static const quint16 defaultPort = 1234;

    Ui::MainWindow *ui;
    QUdpSocket* socket;
    QHostAddress myAddress;

    // Files
    QTcpServer* filesServer;
    QTcpSocket* filesSocket;
    QFile* receivedFile;
    QString fileName;
    bool receiving;
    int namesize;
    int filesize;

    // Users, messages
    int selectedUserId; // index in following vectors
    QVector<QString> nameList;
    QVector<QHostAddress*> addressList;
    QVector<QVector<Message*>> messages; // no no no

    bool isAddressLocal(QHostAddress hostAddress);
    void bindSocket();
    void sendOnlineBroadcast(bool shouldSendResponse);
    void processNewClient(QString connMessage, QHostAddress address, bool shouldSendResponse);
    void addMessage(QString text, QHostAddress* address, bool privateMessage, int toId);
    void showMessageHistory(int userId);
    void setupFilesSocket();
};

#endif // MAINWINDOW_H
