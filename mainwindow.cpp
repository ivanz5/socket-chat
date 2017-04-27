#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    receiving = false;
    selectedUserId = 0;

    socket = new QUdpSocket(this);
    connect(socket, SIGNAL(readyRead()), this, SLOT(readSocket()));

    ui->plainTextEdit->setReadOnly(true);
    ui->fileButton->setEnabled(false);

    connect(ui->messageLineEdit, SIGNAL(returnPressed()), this, SLOT(on_sendButton_clicked()));
    connect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onClientsListItemClicked(QListWidgetItem*)));

    // Sockets setup
    bindSocket();
    sendOnlineBroadcast(true);
    setupFilesSocket();    

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

bool MainWindow::isAddressLocal(QHostAddress hostAddress)
{
    QString s = hostAddress.toString();
    int lastColon = s.lastIndexOf(':');
    if (lastColon >= 0){
        s = s.right(s.length() - lastColon - 1);
        hostAddress = QHostAddress(s);
    }

    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address == hostAddress) return true;
    }
    return false;
}

void MainWindow::bindSocket()
{
    myAddress = QHostAddress::Any;
    socket->bind(myAddress, defaultPort);
    if (socket->localPort() == 0){
        ui->plainTextEdit->appendPlainText("Port error: 1234. Try another port.");
    }
    /*while (socket->localPort() == 0) {
        myPort = rand()%10000;
        socket->close();
        socket->bind(myAddress, myPort);
    }*/
}

void MainWindow::on_changeNameButton_clicked()
{
    sendOnlineBroadcast(false);
}

void MainWindow::on_sendButton_clicked()
{
    QString text = ui->messageLineEdit->text();
    if (text.isEmpty()) return; // do not send empty messages

    addMessage(text, nullptr, selectedUserId != 0, selectedUserId);
    // Public message
    if (selectedUserId == 0){
        QString data = "msg:" + text;
        socket->writeDatagram(data.toUtf8(), QHostAddress::Broadcast, defaultPort);
    }
    // Private message
    else {
        QString data = "prv:" + ui->messageLineEdit->text().toLatin1();
        socket->writeDatagram(data.toUtf8(), *addressList[selectedUserId], defaultPort);
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
        else if (dataStr.startsWith("conn:") || dataStr.startsWith("online:")) {
            QString info = dataStr + " " + sender.toString();
            qDebug() << info;
            bool shouldSendResponse = dataStr.startsWith("conn:");
            if (shouldSendResponse) processNewClient(dataStr, sender, true);
            else processNewClient(dataStr, sender, false);
        }
        else {
            qDebug() << dataStr;
        }
    }
}

void MainWindow::sendOnlineBroadcast(bool shouldSendResponse){
    QString name = ui->nameLineEdit->text();
    if (name.isEmpty()) {
        name = "Client";
        ui->nameLineEdit->setText(name);
    }
    QString message = (shouldSendResponse ? "conn:" : "online:") + name;
    socket->writeDatagram(message.toLatin1(), QHostAddress::Broadcast, defaultPort);
}

void MainWindow::processNewClient(QString connMessage, QHostAddress senderAddress, bool shouldSendResponse)
{
    // Check if not me
    bool myself = false;
    if (isAddressLocal(senderAddress)){
        // Remove return for testing
        myself = true;
        //return;
    }

    // Add to people list
    QString name;
    if (connMessage.startsWith("conn:")) name = connMessage.right(connMessage.size() - 5);
    else name = connMessage.right(connMessage.size() - 7);
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

    // Send online broadcast for newly connected client
    if (!myself && shouldSendResponse) sendOnlineBroadcast(false);
}

void MainWindow::onClientsListItemClicked(QListWidgetItem* item){
    selectedUserId = ui->listWidget->row(item);
    showMessageHistory(selectedUserId);
    ui->fileButton->setEnabled(selectedUserId != 0);
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
    qDebug() << "addMessage: " + (address == nullptr ? "address null" : address->toString());

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

void MainWindow::on_fileButton_clicked()
{
    if (selectedUserId == 0) return;

    QString filename = QFileDialog::getOpenFileName(this, "Select a file");
    qDebug() << "Sending file: " + filename;
    QFile* file = new QFile(filename);
    file->open(QIODevice::ReadOnly);
    QByteArray data = file->readAll();

    filesSocket = new QTcpSocket(this);
    filesSocket->connectToHost(*addressList[selectedUserId], 5678);
    filesSocket->waitForConnected(3000);

    QString nameStr = filename.split("/").last();
    QByteArray infoArray;
    QDataStream stream(&infoArray, QIODevice::WriteOnly);
    stream << nameStr.size();
    stream << data.size();

    filesSocket->write(infoArray);
    filesSocket->write(nameStr.toLatin1());
    quint64 bytesWritten = filesSocket->write(data);
    filesSocket->flush();
    filesSocket->waitForBytesWritten(3000);
    filesSocket->close();

    QString msgText;
    if (bytesWritten == -1) msgText = "file transfer failed: " + nameStr;
    else msgText = "file sent: " + nameStr;
    addMessage(msgText, nullptr, true, selectedUserId);
}

void MainWindow::setupFilesSocket()
{
    filesServer = new QTcpServer(this);
    connect(filesServer, SIGNAL(newConnection()), this, SLOT(filesSocketNewConnection()));

    if (!filesServer->listen(QHostAddress::Any, 5678)){
        qDebug() << "TCP server error";
    }
    else {
        qDebug() << "TCP server started";
    }
}

void MainWindow::filesSocketNewConnection()
{
    qDebug() << "new connection";
    QTcpSocket* socket = filesSocket = filesServer->nextPendingConnection();
    connect(socket, SIGNAL(readyRead()), this, SLOT(filesSocketReadyRead()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(filesSocketDisconnected()));
}

void MainWindow::filesSocketReadyRead()
{
    // First bunch of data
    // namesize (4b), filesize (4b), name, data
    if (!receiving){
        QByteArray sizesBytes = filesSocket->read(8);
        QDataStream sizesStream(sizesBytes);
        sizesStream >> namesize >> filesize;
        QByteArray nameBytes = filesSocket->read(namesize);
        fileName = QString(nameBytes);

        QString path = QDir::homePath() + "/" + fileName;
        qDebug() << path;
        qDebug() << namesize << " " << filesize;
        receivedFile = new QFile(path);
        receivedFile->open(QIODevice::WriteOnly);

        receiving = true;
    }

    QByteArray data = filesSocket->readAll();
    qDebug() << "data received:\n" << data.size();

    receivedFile->write(data);
}

void MainWindow::filesSocketDisconnected()
{
    receivedFile->flush();
    receivedFile->close();
    receivedFile->deleteLater();
    disconnect(filesSocket, SIGNAL(readyRead()), this, SLOT(filesSocketReadyRead()));
    disconnect(filesSocket, SIGNAL(disconnected()), this, SLOT(filesSocketDisconnected()));
    qDebug() << "disconnected";
    receiving = false;

    // Indicate file received with message
    QString text = "file received: " + QDir::homePath() + "/" + fileName;
    addMessage(text, new QHostAddress(filesSocket->peerAddress()), true, 0);

    delete filesSocket;
    filesSocket = nullptr;
}
