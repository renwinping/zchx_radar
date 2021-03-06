#ifndef ZCHXECDISDATAMGR_H
#define ZCHXECDISDATAMGR_H

#include <QObject>
#include "element/drawelement.hpp"
#include "map_layer/zchxmaplayermgr.h"
#include "../zchxutils.hpp"
#include "../zchxMapDatautils.h"
#include "../profiles.h"
#include "../zchxmapwidget.h"
#include <QPainter>
#include <QAction>

namespace qt {
enum DataDisplayMode{
    Data_RealTime = 0,
    Data_History,
};

#if 0
enum    ZCHX_DATA_MGR_TYPE{     //这里值的安排和图元绘制时有关系  值越小 就在最底层
    DATA_MGR_UNKNOWN = 0,
    DATA_MGR_RADAR_VIDEO = 1,
    DATA_MGR_AIS,
    DATA_MGR_RADAR,
    DATA_MGR_CAMERA,
    DATA_MGR_CAMERA_VIEW,    
    DATA_MGR_ROD,
    DATA_MGR_WARNING_ZONE,
    DATA_MGR_COAST,
    DATA_MGR_SEABEDIPLINE,
    DATA_MGR_CHANNEL,
    DATA_MGR_CAMERA_NET_GRID,
    DATA_MGR_STRUCTURE,
    DATA_MGR_AREANET,
    DATA_MGR_MOOR,
    DATA_MGR_CARDMOUTH,
    DATA_MGR_LOCAL_MARK,
    DATA_MGR_DANGEROUS,
    DATA_MGR_PASTROLSTATION,
    DATA_MGR_ISLANDLINE,
    DATA_MGR_SHIPALARM_ASCEND,
    DATA_MGR_VIDEO_TARGET,
    DATA_MGR_RADAR_FEATURE_ZONE,
    DATA_MGR_USER_DEFINE,

};
#endif

class zchxEcdisDataMgr : public QObject
{
    Q_OBJECT
public:
    explicit zchxEcdisDataMgr(zchxMapWidget* w, int type, QObject *parent = 0);
    int     getType() const {return mType;}
    virtual void    show(QPainter* painter);

    //设定当前数据是否可以鼠标选择
    void    setPickUpAvailable(bool sts) {mIsPickUp = sts;}
    bool    isPickupAvailable() const {return mIsPickUp;}

    //关注列表操作
    int             getMaxConcernNum() const {return mMaxConcernNum;}
    virtual void    removeConcern(const QString& id) {mConcernList.removeOne(id);}
    bool            isConcern(const QString& id) {return mConcernList.contains(id);}
    QStringList     getConcernList() const {return mConcernList;}
    void            appendConcernList(const QStringList& list, bool fromMenu);
    void            removeConcernList(const QStringList& list);

    //实时尾迹列表操作
    int             getMaxRealtimeTailTrackNum() const {return mMaxRealtimeTailTrackNum;}
    virtual void    removeRealtimeTailTrack(const QString& id) {mRealtimeTailTrackList.removeOne(id);}
    bool            isRealtimeTailTrack(const QString& id) {return mRealtimeTailTrackList.contains(id);}
    QStringList     getRealtimeTailTrackList() const {return mRealtimeTailTrackList;}
    void            clearRealtimeTailTrackList() {mRealtimeTailTrackList.clear();}

    void            appendRealtimeTailTrackList(const QStringList& list, bool fromMenu);
    void            removeRealtimeTailTrackList(const QStringList& list);

    //历史轨迹列表操作
    int             getMaxHistoryTrackNum() const {return mMaxHistoryTrackNum;}
    virtual void    removeHistoryTrack(const QString& id) {mHistoryTrackList.removeOne(id);}
    bool            isHistoryTrack(const QString& id) {return mHistoryTrackList.contains(id);}
    QStringList     getHistoryTrackList() const {return mHistoryTrackList;}
    void            appendHistoryTrackList(const QStringList& list, bool fromMenu);
    void            removeHistoryTrackList(const QStringList& list);
    //模拟外推操作
    int             getMaxExtrapolationNum() const {return mMaxExtrapolationNum;}
    virtual void    removeExtrapolation(const QString& id) {mExtrapolationList.removeOne(ZCHX::Data::ExtrapolateParam(id));}
    bool            isExtrapolation(const QString& id) {return mExtrapolationList.contains(ZCHX::Data::ExtrapolateParam(id));}
    ZCHX::Data::ExtrapolateList     getExtrapolationList() const;
    double          getExtrapolationTime(const QString& id) const;
    void            appendExtrapolationList(const QStringList& list, bool fromMenu);
    void            removeExtrapolationList(const QStringList& list);
    void            updateExtrapolationTime(const QString& id, double time);

    //鼠标操作
    virtual bool        updateActiveItem(const QPoint& pt);  //鼠标左键点击选择目标
    virtual Element*    selectItem(const QPoint& pt);
    virtual QList<QAction*> getRightMenuActions(const QPoint& pt);
    //创建菜单关联
    QAction *addAction(const QString &text, const QObject *obj, const char* slot, void* userData = 0);
    //获取菜单信号对象的数据
    Element* getElementOfSender();



    void    setDataDisplayMode(int mode);
    int     getDataDisplayMode() const;
    virtual void clearDisplayData() {}
    //设置聚焦的目标
    void    setFocusID(const QString& id) {mFocusID = id;}

signals:
    //外推
    void signalShipExtrapolation(const QString& id, bool add);
    //关注
    void signalSetConcern(const int type, const QString& id);
public slots:
    virtual void setPictureInPicture();
    virtual void setSimulationExtrapolation();
    virtual void setHistoryTraces();
    virtual void setRealTimeTraces();
    virtual void invokeLinkageSpot();
    virtual void setConcern();
protected:
    int                 mMaxConcernNum;                             //最大关注列表数
    int                 mMaxRealtimeTailTrackNum;                   //最大尾迹数据数
    int                 mMaxHistoryTrackNum;                        //最大历史轨迹数
    int                 mMaxExtrapolationNum;                       //最大模拟外推数

    bool                mReplaceRealtimeTailTrackWhenOver;          //是否自动覆盖轨迹点显示当超出最大轨迹线数目
    bool                mReplaceConcernWhenOver;                    //是否自动取消一个关注点
    bool                mReplaceHistoryTrackWhenOver;
    bool                mReplaceExtrapolationWhenOver;

    int                 mDataDisplayMode;
    zchxMapWidget       *mDisplayWidget;

    QStringList         mConcernList;               //关注列表
    QStringList         mRealtimeTailTrackList;     //实时尾迹列表
    QStringList         mHistoryTrackList;          //历史轨迹列表
    ZCHX::Data::ExtrapolateList      mExtrapolationList;         //模拟外推列表            这里注意这里设定有外推时间,默认为0, 用户设定

    int                 mType;                      //数据管理类型
    QString             mFocusID;
    bool                mIsPickUp;
};
}

#endif // ZCHXECDISDATAMGR_H

