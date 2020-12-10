#ifndef SCENE_H
#define SCENE_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QObject>
#include <QTextStream>

#include "libremotesensing_global.h"

#include <gdal_priv.h>
#include <ogrsf_frmts.h>

namespace ProcessTools{
    class MultiProcess;
}

namespace IGDAL{
    class Raster;
}

namespace libCRS{
    class CRSTools;
}

namespace NestedGrid{
    class NestedGridProject;
}

namespace RemoteSensing{
enum SceneType {Landsat8,Sentinel2};
class LIBREMOTESENSINGSHARED_EXPORT Scene : public QObject
{
    Q_OBJECT
public:
    explicit Scene(QString id,
                   libCRS::CRSTools* ptrCrsTools);
    void getQuadkeys(QMap<QString,QVector<QString> >& quadkeysByBand){quadkeysByBand=mTuplekeysByBand;}
    QString getType();
    bool isLandsat8();
    bool isSentinel2();
    ~Scene();
protected:
    QTextStream* mStdOut;
    QString mId;
    libCRS::CRSTools* mPtrCrsTools;
    NestedGrid::NestedGridProject* mPtrNestedGridProject;
    QString mCrsDescription;
    GDALDataType mGdalDataType;
    QMap<QString,IGDAL::Raster*> mPtrRasterBands;
    ProcessTools::MultiProcess* mPtrMultiProcess;
    QMap<QString,QVector<QString> > mTuplekeysByBand;
    SceneType mSceneType;
};
}
#endif // SCENE_H
