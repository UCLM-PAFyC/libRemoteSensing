#ifndef SCENESMANAGER_H
#define SCENESMANAGER_H

#include <QString>
#include <QVector>
#include <QMap>

#include "libremotesensing_global.h"
//#include "Scene.h"

class OGRGeometry;

namespace ProcessTools{
    class MultiProcess;
}

namespace libCRS{
    class CRSTools;
}

namespace NestedGrid{
    class NestedGridProject;
}

namespace RemoteSensing{
class Scene;
class LIBREMOTESENSINGSHARED_EXPORT ScenesManager
{
public:
    ScenesManager(libCRS::CRSTools* ptrCrsTools);
    ~ScenesManager();
    bool createNestedGrid(QString& sceneId,
//                          NestedGrid::NestedGridTools* ptrNestedGridTools,
                          NestedGrid::NestedGridProject* ptrNestedGridProject,
                          ProcessTools::MultiProcess* ptrMultiProcess,
                          bool reproject,
                          QString& strError);
    bool getTupleKeyBySceneId(QString sceneId,
                             QMap<QString,QVector<QString> >& quadkeysByBand,
                             QString& strError);
    QString getSceneType(QString sceneId,
                         QString& strError);
    bool getScenesPersistenceData(QMap<QString,QMap<QString,QString> >& scenesPersistenceData,
                                  QString& strError);
    bool insertSceneLandsat8UsgsFormat(QString& scenePath,
                                       QString& sceneBaseName,
                                       NestedGrid::NestedGridProject *ptrNestedGridProject,
                                       QString& strError);
    bool insertSceneSentinel2EsaZipFormat(QString sceneId,
                                          QString& scenePath,
                                          QString& sceneMetadataFile,
                                          NestedGrid::NestedGridProject *ptrNestedGridProject,
                                          QString& strError);
    bool setLandsat8Parameters(QString& landSat8PathRowShpFileName,
                               QString& landSat8PathRowShpPathField,
                               QString& landSat8PathRowShpRowField,
                               int& landSat8PathRowCrsEpsgCode,
                               QMap<int,QMap<int,OGRGeometry*> >& landsat8PathRowGeometries,
                               QVector<QString>& landsat8BandsToUse,
                               QString& strError);
    bool setSentinel2Parameters(QVector<QString>& landsat8BandsToUse,
                                QString& strError);
private:
    friend class Scene; // puede acceder a privados y protegidos
    QMap<QString,Scene*> mPtrScenes;
    libCRS::CRSTools* mPtrCrsTools;
    QString mLandSat8PathRowShpFileName;
    QString mLandSat8PathRowShpPathField;
    QString mLandSat8PathRowShpRowField;
    int mLandSat8PathRowCrsEpsgCode;
    QMap<int,QMap<int,OGRGeometry*> > mLandsat8PathRowGeometries;
    QVector<QString> mLandsat8BandsToUse;
    QVector<QString> mSentinel2BandsToUse;
};
}
#endif // SCENESMANAGER_H
