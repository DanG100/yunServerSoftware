#include "host.h"

Host::Host()
{
    this->broadCastSock = new QUdpSocket();;
    this->commSock = new  QUdpSocket();
    this->clients = new QList<ConnectedClient>;
    this->commSock->bind(HOST_LISTENING_PORT); 
    connect(this->commSock, SIGNAL(readyRead()), this, SLOT(readData()));
}
Host::~Host()
{
    this->broadCastSock->close();
    this->commSock->close();
    delete this->commSock;
    delete this->broadCastSock;
}
void Host::sendBroadcast()
{
    QByteArray datagram;
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) //send a broadcast on all network connections
    {
        //check that connection is not a loopback and supports our protcol
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
        {
             datagram.append(address.toString()); //write connection ip to datagram
             this->broadCastSock->writeDatagram(datagram, QHostAddress::Broadcast, BROADCAST_PORT); //broadcast datagram
             datagram.clear(); //clear the datagram for the next iteration
        }
    }
}

void Host::sendGameSync(QByteArray dgram)
{
    for(int i = 0;i<this->clients->size();i++)
    {
        this->commSock->writeDatagram(dgram,this->clients->at(i).addr,this->clients->at(i).port);
    }
}

void Host::readData()
{
    QByteArray datagram;
    datagram.resize(this->commSock->pendingDatagramSize());
    QHostAddress sender;
    quint16 senderPort;

    this->commSock->readDatagram(datagram.data(), datagram.size(),&sender, &senderPort);
    D_MSG(datagram.data());
    this->checkValidDgram(datagram,sender,senderPort);
}
bool Host::checkValidDgram(QByteArray dgram, QHostAddress sender, quint16 senderPort)
{
    if(dgram.startsWith("gmd")) //checks for game synce flag
    {
        emit receivedValidDgram(dgram);
        return true;
    }
    else if(dgram.startsWith("CLI")) //checks for client flag
    {
        ConnectedClient cli;
        cli.addr = sender;
        cli.port = senderPort;
        QString name = QString::fromUtf8(dgram.data());
        cli.name = name.section(':',1);
        bool dupCli = false;
        for(int i = 0;i<clients->size();i++) //checks that the client is not already registered
        {
            if(clients->at(i).name==cli.name)
            {
                D_MSG("DUPLICATE CLIENT");
            }
        }
        if(!dupCli)
        {
            clients->append(cli);
            QByteArray dgram = "CLI:connected";
            this->broadCastSock->writeDatagram(dgram,sender,BROADCAST_PORT);
            emit clientAdded();
            return true;
        }
    }
    return false;
}
QList<QString> Host::getClientNames()
{
    QList<QString> names;
    for(int i =0;i<this->clients->size();i++)
    {
        D_MSG(clients->at(i).name);
        names.append(clients->at(i).name);

    }
    return names;
}
