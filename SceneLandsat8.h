#ifndef LANDSAT8SCENE_H
#define LANDSAT8SCENE_H

#include "SceneLandsat8_definitions.h"
#include "Scene.h"

#include <QDateTime>
#include <QTextStream>

class OGRGeometry;

namespace ProcessTools{
    class MultiProcess;
}

namespace RemoteSensing{
class LIBREMOTESENSINGSHARED_EXPORT SceneLandsat8 : public Scene
{
    Q_OBJECT
public:
    explicit SceneLandsat8(QString id,
                           libCRS::CRSTools* ptrCrsTools);
//    ~SceneLandsat8();
//    bool createMultibandFile(NestedGrid::NestedGridTools* ptrNestedGridTools,
//                             QVector<int>& landsat8BandsToUse,
//                             IGDAL::Raster* ptrMultibandRaster,
//                             QString& strError);
    bool createNestedGrid(NestedGrid::NestedGridProject* ptrNestedGridProject,
                          ProcessTools::MultiProcess* ptrMultiProcess,
                          int& landSat8PathRowCrsEpsgCode,
                          QMap<int,QMap<int,OGRGeometry*> >& landsat8PathRowGeometries,
                          QVector<QString>& landsat8BandsToUse,
                          bool reproject,
                          QString& strError);
    static bool getBandIdFromFileName(QString fileName,
                                      QString& bandId,
                                      QString& strError);
    static bool getMetadataValues(QString metadataFileName,
                                  QVector<QString> metadataTags,
                                  QVector<QString>& metadataValues,
                                  QString& strError);
    bool getPersistenceData(QMap<QString,QString>& persistenceData,
                            QString& strError);
    bool setFromUsgsFormat(QString& scenePath,
                           QString& sceneBaseName,
                           NestedGrid::NestedGridProject *ptrNestedGridProject,
                           QString& strError);
    bool setMetadataFromUsgsFormat(QString& fileName,
                                   QString& strError);
public slots:
    void onTuplekeyFirstProcessFinished();
    void onTuplekeySecondProcessFinished();
    void onReprojectionProcessFinished();
    void manageProccesStdOutput(QString data);
    void manageProccesErrorOutput(QString data);
private:
    QTextStream* mStdOut;
    QString mMetadataUsgsFormatFileName;
    QMap<QString,QString> mBandsUsgsFormatFileName;
    QMap<QString,QMap<QString,QString> > mUsgsMetadata;
    QMap<QString,QString> mBandCodeByBandUsgsFormatFileCompleteBaseName;
    int mPath;
    int mRow;
    QDateTime mDateTime;
    QVector<QString> mReprojectionProcessesOutputFileNames;
    QVector<QString> mTuplekeysOutputFileNames;
    QString mMetadataFileContent;
    QVector<QString> mLandsat8BandsCode;
    QString mStrBandsCodeValidDomain;
    QMap<QString,QVector<QString> > mLandsat8BandsCodeAliasByCode;
};
}
#endif // LANDSAT8SCENE_H
