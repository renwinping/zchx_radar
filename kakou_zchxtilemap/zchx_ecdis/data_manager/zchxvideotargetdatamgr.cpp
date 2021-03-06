#include "zchxvideotargetdatamgr.h"
#include "zchxmapframe.h"
#include <QDebug>

namespace qt {
zchxVideoTargetDataMgr::zchxVideoTargetDataMgr(zchxMapWidget* w,  QObject *parent) : zchxEcdisDataMgr(w, ZCHX::DATA_MGR_VIDEO_TARGET, parent)
{

}

void zchxVideoTargetDataMgr::show(QPainter* painter)
{
    if(!painter || !mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_WARNING_TARGET)) return ;
    QMap<QString, std::shared_ptr<VideoTargetElement>>::iterator it = mTargetMap.begin();
    for(; it != mTargetMap.end(); ++it)
    {
        std::shared_ptr<VideoTargetElement> item = it.value();
        if(item) item->drawElement(painter);
    }
}

bool zchxVideoTargetDataMgr::updateActiveItem(const QPoint &pt)
{
    if(!mDisplayWidget->getLayerMgr()->isLayerVisible(ZCHX::LAYER_WARNING_TARGET) || !isPickupAvailable()) return false;
    return false;
}

void zchxVideoTargetDataMgr::setData(const QList<ZCHX::Data::ITF_VideoTarget> &list)
{
   foreach (ZCHX::Data::ITF_VideoTarget data, list) {
        std::shared_ptr<VideoTargetElement> ele = mTargetMap[data.objectID];
        if(ele) {
            ele->setData(data);
        } else {
            mTargetMap[data.objectID] = std::shared_ptr<VideoTargetElement>(new VideoTargetElement(data, mDisplayWidget));
        }
    }
}

}


