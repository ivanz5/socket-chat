#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    selectedUserId = 0;
    myPort = 1234;
    remotePort = 1234;
    ui->myPortLineEdit->setText(QString::number(myPort));
    ui->portLineEdit->setText(QString::number(remotePort));

    socket = new QUdpSocket(this);
    connect(socket, SIGNAL(readyRead()), this, SLOT(readSocket()));

    ui->plainTextEdit->setReadOnly(true);
    ui->plainTextEdit->setPlainText("localhost, port " + QString::number(myPort));

    connect(ui->messageLineEdit, SIGNAL(returnPressed()), this, SLOT(on_sendButton_clicked()));
    connect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onClientsListItemClicked(QListWidgetItem*)));

    // Get own address
    /*
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)){
            myAddress = address;
            qDebug() << address;
        }
    }
    ui->plainTextEdit->appendPlainText("My address: " + myAddress.toString());*/

    bindSocket();
    sendOnlineBroadcast();


    // Clients list
    QListWidgetItem* item = new QListWidgetItem();
    item->setText("All");
    nameList.push_back("All");
    addressList.push_back(nullptr);
    messages.push_back(QVector<Message*>());
    ui->listWidget->addItem(item);
}

MainWindow::~MainWindow()
{
    delete ui;
    socket->close();
}

void MainWindow::bindSocket()
{
    //myAddress = QHostAddress("192.168.0.105");
    myAddress = QHostAddress::Any;
    socket->bind(myAddress, myPort);
    while (socket->localPort() == 0) {
        myPort = rand()%10000;
        socket->close();
        socket->bind(myAddress, myPort);
    }
    ui->myPortLineEdit->setText(QString::number(myPort));
}

void MainWindow::on_changeNameButton_clicked()
{
    sendOnlineBroadcast();
}


void MainWindow::on_changePortButton_clicked()
{
    myPort = ui->myPortLineEdit->text().toInt();
    ui->plainTextEdit->appendPlainText("My port: " + QString::number(myPort));
    socket->close();
    socket->bind(QHostAddress::LocalHost, myPort);
}


void MainWindow::on_connectButton_clicked()
{
    remotePort = ui->portLineEdit->text().toInt();
    ui->plainTextEdit->appendPlainText("New remote port: " + QString::number(remotePort));
}

void MainWindow::on_sendButton_clicked()
{
    QString text = ui->messageLineEdit->text();
    if (text.isEmpty()) return; // do not send empty messages

    addMessage(text, nullptr, selectedUserId != 0, selectedUserId);
    // Public message
    if (selectedUserId == 0){
        QString data = "msg:" + text.toLatin1();
        socket->writeDatagram(data.toLatin1(), QHostAddress::Broadcast, remotePort);
    }
    // Private message
    else {
        QString data = "prv:" + ui->messageLineEdit->text().toLatin1();
        socket->writeDatagram(data.toLatin1(), *addressList[selectedUserId], remotePort);
    }
    ui->messageLineEdit->clear();
}

void MainWindow::readSocket()
{
    while (socket->hasPendingDatagrams()){
        QByteArray datagram;
        datagram.resize(socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QString dataStr = QString(datagram);
        if (dataStr.startsWith("msg:")) {
            QString text = dataStr.right(dataStr.length() - 4);
            addMessage(text, new QHostAddress(sender), false, 0);
        }
        else if (dataStr.startsWith("prv:")){
            QString text = dataStr.right(dataStr.length() - 4);
            addMessage(text, new QHostAddress(sender), true, 0);
        }
        else if (dataStr.startsWith("conn:")) {
            QString info = dataStr + " " + sender.toString();
            qDebug() << info;
            ui->plainTextEdit->appendPlainText(info);
            processNewClient(dataStr, sender);
        }
        else {
            qDebug() << dataStr;
        }
    }
}

void MainWindow::sendOnlineBroadcast(){
    QString name = ui->nameLineEdit->text();
    if (name.isEmpty()) {
        name = "Client";
        ui->nameLineEdit->setText(name);
    }
    QString message = "conn:" + name;
    socket->writeDatagram(message.toLatin1(), QHostAddress::Broadcast, defaultPort);
}

void MainWindow::processNewClient(QString connMessage, QHostAddress senderAddress)
{
    // Check if not me
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol){
            if (address == senderAddress){
                ui->plainTextEdit->appendPlainText("Me connected: " + connMessage);
                // Remove return for testing
                //return;
            }
        }
    }

    // Add to people list
    QString name = connMessage.right(connMessage.size() - 5);
    // Check if just name changed
    bool exists = false;
    for (int i = 0; i < addressList.size(); i++){
        if (addressList[i] == nullptr) continue; // Important
        QHostAddress nowAddr = *(addressList[i]);
        if (nowAddr == senderAddress){
            nameList[i] = name;
            exists = true;
            break;
        }
    }
    if (!exists){
        nameList.push_back(name);
        addressList.push_back(new QHostAddress(senderAddress));
        messages.push_back(QVector<Message*>());
    }

    // Update list in UI
    ui->listWidget->clear();
    for (int i = 0; i < nameList.size(); i++){
        QString text = nameList[i] + "\n" + (addressList[i] == nullptr ? "" : addressList[i]->toString());
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(text);
        ui->listWidget->addItem(item);
    }
}

void MainWindow::onClientsListItemClicked(QListWidgetItem* item){
    selectedUserId = ui->listWidget->row(item);
    qDebug() << selectedUserId;
    showMessageHistory(selectedUserId);
}

/**
 * @brief MainWindow::addMessage
 * @param text message text
 * @param address ip address of sender, nullptr for outgoing messages
 * @param privateMessage true if message is private, false otherwise
 * @param toId id (position in array) of user for outgoing private messages, 0 or less for incoming or non-private
 */
void MainWindow::addMessage(QString text, QHostAddress* address, bool privateMessage, int toId)
{
    int index = 0;
    qDebug() << (address == nullptr ? "address null" : address->toString());

    if (privateMessage){
        // Outgoing
        if (toId > 0){
            index = toId;
        }
        else if (address != nullptr){
            for (int i = 1; i < addressList.size(); i++){
                if (*address == *addressList[i]){
                    index = i;
                    break;
                }
            }
        }
        // Invalid parameters, either address or toId should be specified
        else return;
    }

    // Add to  messages
    Message* message = new Message(text, address);
    messages[index].push_back(message);
    // UI
    if (index == selectedUserId)
        ui->plainTextEdit->appendPlainText(message->getDisplayString(addressList, nameList));
}

void MainWindow::showMessageHistory(int userId)
{
    if (userId < 0 || userId >= messages.size()) return;
    ui->plainTextEdit->clear();
    for (int i = 0; i < messages[userId].size(); i++){
        ui->plainTextEdit->appendPlainText(messages[userId][i]->getDisplayString(addressList, nameList));
    }
}
