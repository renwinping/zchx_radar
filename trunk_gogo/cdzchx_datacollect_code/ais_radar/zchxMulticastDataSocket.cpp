#include "zchxMulticastDataSocket.h"
#include "common.h"
#include <QThread>
#include "../profiles.h"

zchxMulticastDataScoket::zchxMulticastDataScoket(const QString& host, int port, const QString& tag,
                                                 bool join, int data_size, int mode, QObject *parent)
    : QObject(parent),
      mHost(host),
      mPort(port),
      mTag(tag),
      mIsJoin(join),
      mDataSize(data_size),
      mMode(mode),
      mSocket(Q_NULLPTR),
      mInit(false)
{
    init();
}

void zchxMulticastDataScoket::slotWriteData(const QByteArray &data)
{
    if(!mSocket) return ;
    mSocket->writeDatagram(data, QHostAddress(mHost), mPort);
}

void zchxMulticastDataScoket::init()
{
    if(mHost.length() == 0 || mPort == 0) return;
    mSocket = new QUdpSocket();
    //选择网卡
    QString ip_str = Utils::Profiles::instance()->value("Radar_Control","Mac_IP").toString();
    if(!mSocket->bind(QHostAddress(ip_str), mPort,QAbstractSocket::ShareAddress))
    {
        qDebug()<<"bind port failed:"<<mPort;
        return;
    }

    mSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);//禁止本机接收
    if(mIsJoin && !mSocket->joinMulticastGroup(QHostAddress(mHost)))
    {
         qDebug()<<"joinMuticastGroup host failed:"<<mHost;
         return;
    }
    if(mMode == ModeAsyncRecv)
    {
        connect(mSocket, SIGNAL(readyRead()),this, SLOT(slotReadyReadMulticastData()));
        connect(mSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(slotDisplayUdpReportError(QAbstractSocket::SocketError)));
    }

    mInit = true;

    qDebug()<<"init multicast succeed."<<mHost<<":"<<mPort;

    return;
}

void zchxMulticastDataScoket::processRecvData(const QByteArray &data)
{
    LOG_FUNC_DBG<<" nothing...";
}

void zchxMulticastDataScoket::slotReadyReadMulticastData()
{
    if(!mInit || !mSocket) return;

    QByteArray datagram;
    // 让datagram的大小为等待处理的数据报的大小，这样才能接收到完整的数据
    qint64 dataSize = mSocket->pendingDatagramSize();

//    qDebug()<<"recv data from socket:"<<dataSize<<QDateTime::currentDateTime().toString("hh:mm:ss zzz");
    datagram.resize(dataSize);//pendingDatagramSize() 当前数据包大小
    // 接收数据报，将其存放到datagram中
    mSocket->readDatagram(datagram.data(), dataSize);
    qint64 utc = QDateTime::currentMSecsSinceEpoch();
    emit signalRecvMulticastData(utc, mTag, QString("data size:%1").arg(dataSize));
    if(mDataSize == 0)
    {
        processRecvData(datagram);
    } else
    {
        mRecvData.append(datagram);
        if(mRecvData.size() >= mDataSize)
        {
            processRecvData(mRecvData.mid(0, mDataSize));
            mRecvData = mRecvData.right(mRecvData.size() - mDataSize);
        }
    }
}

void zchxMulticastDataScoket::startRecv()
{
    if((!mInit) || mMode == ModeAsyncRecv) return;
    while(1){
        if(mSocket->hasPendingDatagrams())
        {
            slotReadyReadMulticastData();
        } else
        {
            LOG_FUNC_DBG<<"no data received";
        }
        QThread::msleep(100);
    }
}

void zchxMulticastDataScoket::slotDisplayUdpReportError(QAbstractSocket::SocketError e)
{
    LOG_FUNC_DBG<<"socket error:"<<e<<mSocket->errorString();
}
