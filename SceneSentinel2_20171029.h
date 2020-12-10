#ifndef SCENESENTINEL2_H
#define SCENESENTINEL2_H

#include "SceneSentinel2_definitions.h"
#include "Scene.h"

#include <QDateTime>
#include <QTextStream>


namespace PW{
    class MultiProcess;
}

namespace RemoteSensing{
class LIBREMOTESENSINGSHARED_EXPORT SceneSentinel2 : public Scene
{
    Q_OBJECT
public:
    explicit SceneSentinel2(QString id,
                            libCRS::CRSTools* ptrCrsTools);
//    ~SceneLandsat8();
//    bool createMultibandFile(NestedGrid::NestedGridTools* ptrNestedGridTools,
//                             QVector<int>& landsat8BandsToUse,
//                             IGDAL::Raster* ptrMultibandRaster,
//                             QString& strError);
    bool createNestedGrid(NestedGrid::NestedGridProject* ptrNestedGridProject,
                          PW::MultiProcess* ptrMultiProcess,
                          QVector<QString>& sentinel2BandsToUse,
                          bool reproject,
                          QString& strError);
//    bool getPersistenceData(QMap<QString,QString>& persistenceData,
//                            QString& strError);
    static bool getBandIdFromFileName(QString fileName,
                                      QString& bandId,
                                      QString& strError);
    bool setFromEsaZipFormat(QString& scenePath,
                             QString& sceneMetadataFile,
                             NestedGrid::NestedGridProject *ptrNestedGridProject,
                             QString& strError);
    //    bool setMetadataFromUsgsFormat(QString& fileName,
//                                   QString& strError);
public slots:
    void onTuplekeyFirstProcessFinished();
    void onTuplekeySecondProcessFinished();
    void onReprojectionProcessFinished();
    void manageProccesStdOutput(QString data);
    void manageProccesErrorOutput(QString data);
private:
    QTextStream* mStdOut;
    QString mMetadataEsaZipFormatFileName;
    QMap<QString,QString> mBandsEsaZipFormatFileName;
//    QMap<QString,QMap<QString,QString> > mUsgsMetadata;
    QMap<QString,QString> mBandCodeByBandEsaZipFormatFileCompleteBaseName;
//    int mPath;
//    int mRow;
//    QDateTime mDateTime;
    QVector<QString> mReprojectionProcessesOutputFileNames;
    QVector<QString> mTuplekeysOutputFileNames;
//    QString mMetadataFileContent;
    QVector<QString> mSentinel2BandsCode;
    QString mStrBandsCodeValidDomain;
};
}
#endif // SCENESENTINEL2_H
