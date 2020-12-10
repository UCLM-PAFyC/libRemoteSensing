#ifndef PERSISTENCEMANAGER_H
#define PERSISTENCEMANAGER_H

#define PERSISTENCEMANAGER_CRS_PRECISION                -1 // cuando sea -1 lo calculo
#define PERSISTENCEMANAGER_CRS_PROJECTED_PRECISION      3 // cuando sea -1 lo calculo

//#include "spatialite.h"
#include "../libIGDAL/SpatiaLite.h"
#include "persistencemanager_definitions.h"

#include <QObject>
#include <QMap>

#include "libremotesensing_global.h"

namespace libCRS{
class CRSTools;
}

namespace IGDAL{
class libIGDALProcessMonitor;
class SpatiaLite;
}

namespace NestedGrid{
    class NestedGridTools;
}

namespace RemoteSensing{
class Algorithms;
class LIBREMOTESENSINGSHARED_EXPORT PersistenceManager : public QObject
{
    Q_OBJECT
public:
    explicit PersistenceManager(QObject *parent = 0);
    bool createDatabase(QString templateDb,
                        QString fileName,
                        QString proj4Text,
                        QString geoCrsBaseProj4Text,
                        QString nestedGridLocalParameters,
                        int& srid,
                        QString& strError,
                        QString sqlCreateFileName="");
    bool executeSql(QString sqlSentence,
                    QVector<QString> fieldsNamesToRetrieve,
                    QVector<QMap<QString,QString> > &fieldsValuesToRetrieve,
                    QString& strError);
    Algorithms* getAlgorithms(){return(mPtrAlgoritmhs);};
    QString getDatabaseFileName();
    bool getInitialDate(int& initialJd,
                        QString& strError);
    bool getFinalDate(int& finalJd,
                      QString& strError);
    bool getNdviDataByProject(QMap<QString,QMap<QString,QMap<int,QString> > >& tuplekeyFileNameByProjectCodeByTuplekeyByJd,
                              QMap<QString,QMap<QString,QMap<int,double> > >& gainByProjectCodeByTuplekeyByJd,
                              QMap<QString,QMap<QString,QMap<int,double> > >& offsetByProjectCodeByTuplekeyByJd,
                              QMap<QString,int>& lodByTuplekey,
                              int &maximumLod,
                              QString& strError);
    bool getNdviDataByTuplekeyByRoiCode(QMap<QString,QMap<QString,QMap<int,QString> > >& tuplekeyFileNameByTuplekeyByRoiCodeByJd,
                                        QMap<QString, int> &roiIdByRoiCode,
                                        QMap<QString, int> &tuplekeyIdByTuplekeyCode,
                                        QMap<QString,double> & gainByTuplekeyFileName,
                                        QMap<QString,double> & offsetByTuplekeyFileName,
                                        QMap<QString,int>& lodTilesByTuplekeyFileName,
                                        QMap<QString, int> &lodGsdByTuplekeyFileName,
                                        int &maximumLod,
                                        QString& strError);
    int getSRID(){return(mSRID);};
    bool getSRID(QString proj4Text,
                     int& srid,
                     QString& strError);
    bool getIsDatabaseDefined();
    bool getProjects(QMap<int,int>& initialDateByProjectId,
                     QMap<int,int>& finalDateByProjectId,
                     QMap<QString,int>& idByProjectCode,
                     QMap<int,QString>& outputPathByProjectId,
                     QMap<int, int> &outputSridByProjectId,
                     QVector<QString>& projectCodes,
                     QMap<QString, int> &intercalibrationTargetImagesById,
                     QString &strError);
    bool getProjectCodes(QVector<QString>& projectCodes,
                         QString& strError);
    bool getProjectEnvelopes(QMap<QString,double>& minFirstCoordinateByProjectCode,
                             QMap<QString,double>& maxSecondCoordinateByProjectCode,
                             QMap<QString,double>& maxFirstCoordinateByProjectCode,
                             QMap<QString,double>& minSecondCoordinateByProjectCode,
                             QString& strError);
    bool getProjectGeometries(QMap<QString, OGRGeometry *> &geometryByProjectCode,
                              QString& strError);
    bool getProjectTuplekeyIntersectionGeomety(int roiId,
                                               int tuplekeyId,
                                               OGRGeometry*& ptrGeometry,
                                               QString& strError);
    QString getCrsDescription(){return(mCrsDescription);};
    QString getGeographicCrsBaseProj4Text(){return(mGeographicCrsBaseProj4Text);};
    QString getNestedGridLocalParameters(){return(mNestedGridLocalParameters);};
    bool initializeAlgorithms(libCRS::CRSTools* ptrCrsTools,
                              NestedGrid::NestedGridTools* ptrNestedGridTools,
                              IGDAL::libIGDALProcessMonitor* ptrLibIGDALProcessMonitor,
                              QString& libPath,
                              QString& strError);
    bool insertIntercalibration(QString sceneId,
                                QString rasterFileReference,
                                QString bandId,
                                double gain,
                                double offset,
                                bool to8Bits,
                                bool toReflectance,
                                bool interpolated,
                                QString& strError);
    bool insertLandsat8Scene(QString sceneId,
                             int jd,
                             QString metadataFileName,
                             QVector<QString> metadataTags,
                             QVector<QString> metadataValues,
                             QString& strError);
    bool insertNdviTuplekeyFile(QString tuplekey,
                                QString rasterFile,
                                QString computationMethod,
                                QString rasterUnitConversion,
                                int lodTiles,
                                int lodGsd,
                                QString ndviFileName,
                                QString& strError);
    bool insertOrthoimage(QString orthoimageId,
                          int jd,
                          QString& strError);
    bool deletePiasFile(int piasFileId,
                        QString& strError);
    bool getExistsPiasTuplekeyFile(QString tuplekey,
                                   QVector<QString> rasterFiles,
                                   QString computationMethod,
                                   QString& piasFileName,
                                   int& piasFileId,
                                   QString& strError);
    IGDAL::SpatiaLite* getPtrDb(){return(mPtrDb);};
    bool insertPiasTuplekeyFile(QString tuplekey,
                                QVector<QString> rasterFiles,
                                QMap<QString,int> jdByRasterFiles,
                                QString computationMethod,
                                int piaValue,
                                QString piasFileName,
                                bool reprocessFiles,
                                QString& strError);
    bool insertProject(QString code,
                       QString resultsPath,
                       int initialJd,
                       int finalJd,
                       int srid,
                       QString strWktGeometry,
                       QString& strError);
    bool insertTuplekey(QString tuplekey,
                       NestedGrid::NestedGridTools* ptrNestedGridTools,
                       QString& strError);
    bool insertTuplekeyRasterFile(QString id, // orthoimage o scene
                                 QString tuplekey,
                                 QString tuplekeyRasterFile,
                                 QString bandId,
                                 QString& strError);
    bool insertRasterUnitConversion(QString conversion,
                                    double gain,
                                    double offset,
                                    QString& strError);
    bool insertRSProduct(QString rsProductId,
                         int jd,
                         QString rsProductDataType,
                         QString& strError);
    bool insertSentinel2Scene(QString sceneId,
                              int jd,
                              QString metadataFileName,
                              QVector<QString> metadataTags,
                              QVector<QString> metadataValues,
                              QString& strError);
    bool loadRasterUnitConversions(QString& strError);
    bool openDatabase(QString fileName,
                      QString& strError);
    bool processAlgorithm(QString algorithmCode,
                          QString zoneCode,
                          bool mergeFiles,
                          bool reprocessFiles,
                          bool reprojectFiles,
                          int mainScenceJd,
                          int intercalibrationReferenceImageId,
                          QString intercalibrationReferenceImageRasterId,
                          QString& strError);
    bool updateDatabase(QString sqlFileName,
                        QString& strError);
private:

signals:
    void operationFinished();

public slots:

private:
    IGDAL::SpatiaLite* mPtrDb;
    int mSRID;
    QString mCrsDescription;
    int mCrsPrecision;
    int mInitialJd;
    int mFinalJd;
    QString mGeographicCrsBaseProj4Text;
    QString mNestedGridLocalParameters;
    Algorithms* mPtrAlgoritmhs;
    QVector<QString> mZonesCodes;
    QMap<QString,QString> mResultsPathByZone;
    QMap<QString,int> mIdByZone;
    QMap<QString,int> mInitialDateByZone;
    QMap<QString,int> mFinalDateByZone;
    QMap<QString,int> mOutputSridByZone;
    QMap<QString,double> mRUCGains;
    QMap<QString,double> mRUCOffsets;
};
}
#endif // PERSISTENCEMANAGER_H
