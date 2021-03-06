#include "zchxaischartworker.h"
#include <QPainter>
#include <QTimer>
#include <QSvgGenerator>
#include <QBuffer>

#define     IMG_RADIUS          512

zchxAisChartWorker::zchxAisChartWorker(double lat, double lon,  bool fixed_radius, int radius, QObject *parent)
    : QObject(parent)
    , mCenterLat(lat)
    , mCenterLon(lon)
    , mCurRange(0.0)
    , mMaxRange(0)
    , mRangeFactor(0.0)
    , mRadiusFixed(fixed_radius)
    , mFixedRadius(radius)
{
    mImgWidth = IMG_RADIUS * 2 -1;
    mImgHeight = IMG_RADIUS * 2 -1;
    mHeartTimer = new QTimer;
    mHeartTimer->setInterval(20 *1000);
    connect(mHeartTimer, SIGNAL(timeout()), this, SLOT(slotUpdateWithList()));

    moveToThread(&mWorkThread);
    mWorkThread.start();
    mHeartTimer->start();
}

zchxAisChartWorker::~zchxAisChartWorker()
{
    if(mHeartTimer) delete mHeartTimer;
    mWorkThread.quit();
}

void zchxAisChartWorker::slotAppendAisLatLon(double lat, double lon)
{
    QMutexLocker locker(&mMutex);
    mWorkLL.append(Latlon(lat, lon));
}

QList<Latlon> zchxAisChartWorker::getWorkLL()
{
    QMutexLocker locker(&mMutex);
    QList<Latlon> list(mWorkLL);
    mWorkLL.clear();
    return list;
}

void zchxAisChartWorker::paintImg(QPaintDevice* dev)
{
    if(!dev) return;
    QPainter objPainter(dev);
    objPainter.setRenderHint(QPainter::Antialiasing,true);

    objPainter.save();
    objPainter.setPen(QPen(Qt::darkMagenta, 1));
    objPainter.setBrush(Qt::darkMagenta);
    //将保存的像素点的坐标进行更新
    QStringList allKeys = mPixPosMap.keys();
    foreach (QString key, allKeys) {
        //解析对应的像素点坐标
        QStringList posList = key.split(",");
        if(posList.size() != 2) continue;
        int x = posList[0].toInt();
        int y = posList[1].toInt();
        objPainter.drawEllipse(QPoint(x, y), 3, 3);
    }
    objPainter.restore();
    //开始画目标的扫描范围
    objPainter.translate(mImgWidth / 2, mImgHeight / 2);
    //画基站中心位置
    objPainter.setPen(QPen(Qt::red, 1));
    objPainter.setBrush(Qt::red);
    objPainter.drawEllipse(QPoint(0, 0), 3, 3);

    //开始画范围
    double radius = mCurRange / mRangeFactor;
    objPainter.setPen(QPen(Qt::red, 1, Qt::SolidLine));    
    objPainter.setBrush(Qt::NoBrush);
    int last_pos = 0;
    for(int i=1; i<=5; i++)
    {
        int range = mCurRange * i / 5;
        //计算对应的像素位置
        double radius = range / mRangeFactor;
        //扫描的圆圈
        objPainter.drawEllipse(QPoint(0, 0), radius, radius);
//        int range = qRound(radius * mRangeFactor);
        QString text = QString("%1 m").arg(range);
        int wid = objPainter.fontMetrics().width(text);
        int hei = objPainter.fontMetrics().height();
        objPainter.drawText(QPointF(radius - wid - 2, 0.5*hei*(-1)), text);
        if(i==5) last_pos = radius;
    }
    objPainter.drawLine(QPoint(0, 0), QPoint(last_pos, 0));

}

void zchxAisChartWorker::slotMakePixMap()
{
    QString save_path = QApplication::applicationDirPath();
    save_path += "/temp/ais";
    QDir dir(save_path);
    if(!dir.exists()) {dir.mkpath(dir.absolutePath());}
    bool drawSvg = true;
    QByteArray bytes;
    QString fileName = QString("%1/%2").arg(save_path).arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    if(drawSvg)
    {
        fileName.append("SVG");
        QSvgGenerator generator;
        generator.setFileName(fileName);
        generator.setSize(QSize(mImgWidth, mImgHeight));
        generator.setViewBox(QRect(0, 0, mImgHeight, mImgHeight));
        generator.setTitle("Ais SVG Chart Sample");
        generator.setDescription("This SVG file is generated by Qt.");

        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        generator.setOutputDevice(&buffer);

        paintImg(&generator);

        buffer.close();
        emit signalSendPixmap(bytes, mImgWidth, mImgHeight,mMaxRange, "SVG");
    } else
    {
        fileName.append("PNG");
        QPixmap objPixmap(mImgWidth, mImgHeight);
        objPixmap.fill(Qt::transparent);//用透明色填充
        paintImg(&objPixmap);
        bool sts = objPixmap.save(fileName, "PNG");
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        objPixmap.save(&buffer ,"PNG");
        buffer.close();

        emit signalSendPixmap(bytes, mImgWidth, mImgHeight, mMaxRange, "PNG");
    }
    //检查目录的文件数,删除以前的  保留最近的10个文件
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    int del_num = list.size() - 2;
    if(del_num > 0)
    {
        int i = 0;
        foreach(QFileInfo info, list)
        {
            QFile::remove(info.absoluteFilePath());
            i++;
            if(i == del_num) break;
        }
    }
    return;
}

void zchxAisChartWorker::slotUpdateCenter(double lat, double lon)
{
    mCenterLat = lat;
    mCenterLon = lon;
}

void zchxAisChartWorker::slotUpdateWithList()
{
//    qDebug()<<__FUNCTION__<<__LINE__;
    QList<Latlon> list = getWorkLL();
    if(list.size() == 0) return;
    if(!mRadiusFixed)
    {
        //没有固定数据时,数据在动态变化
        double max_distance = 0;
        foreach (Latlon ll, list) {
            //计算当前点到中心的距离
            double distance = getDisDeg(mCenterLat, mCenterLon, ll.lat, ll.lon);
            if(distance > max_distance) max_distance = distance;
        }
        if(max_distance > mCurRange)
        {
            mCurRange = max_distance;
            mMaxRange = mCurRange * 1.1;
            mRangeFactor = mMaxRange / IMG_RADIUS;
            QMap<QString, QList<Latlon>> tempMap;
            //半径发生了变化,图形需要重绘
            zchxPosConverter convert(QPointF(mImgWidth/2, mImgHeight/2), Latlon(mCenterLat, mCenterLon), mRangeFactor);
            //将保存的像素点的坐标进行更新
            QStringList allKeys = mPixPosMap.keys();
            foreach (QString key, allKeys) {
                //解析对应的像素点坐标
                QStringList posList = key.split(",");
                if(posList.size() != 2) continue;
                int x = posList[0].toInt();
                int y = posList[1].toInt();
                QList<Latlon> list = mPixPosMap.value(key);
                foreach (Latlon ll , list) {
                    //计算经纬度点对应的像素点
                    QPoint pt = convert.Latlon2Pixel(ll).toPoint();
                    QString key = QString("%1,%2").arg(pt.x()).arg(pt.y());
                    tempMap[key].append(ll);
                }
            }

            mPixPosMap = tempMap;
        }
    } else
    {
        mCurRange = mFixedRadius;
        mMaxRange = mCurRange * 1.1;
        mRangeFactor = mMaxRange / IMG_RADIUS;
    }
//    qDebug()<<__FUNCTION__<<__LINE__;
    //将新的点更新到当前的位置关系图
    foreach (Latlon ll, list) {
        zchxPosConverter convert(QPointF(mImgWidth/2, mImgHeight/2), Latlon(mCenterLat, mCenterLon), mRangeFactor);
        //计算经纬度点对应的像素点
        QPoint pt = convert.Latlon2Pixel(ll).toPoint();
        QString key = QString("%1,%2").arg(pt.x()).arg(pt.y());
        if(mPixPosMap[key].size() >= 20) continue;
        mPixPosMap[key].append(ll);
    }

    //开始更新图片
    slotMakePixMap();

}
