#ifndef CLASSIFICATIONPROJECT_H
#define CLASSIFICATIONPROJECT_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QObject>
#include <QTextStream>
#include <QStringList>

#include "libremotesensing_global.h"

#define CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER                                  "#"
#define CLASSIFICATIONPROJECT_DATES_SEPARATOR_CHARACTER                            "-"
//#define CLASSIFICATIONPROJECT_NDVIS_SEPARATOR_CHARACTER                            "-"

#define CLASSIFICATIONPROJECT_DATE_FORMAT_1                                        "yyyy/MM/dd"
#define CLASSIFICATIONPROJECT_DATE_FORMAT_2                                        "yyyy:MM:dd"
#define CLASSIFICATIONPROJECT_DATE_FORMAT_3                                        "dd/MM/yyyy"
#define CLASSIFICATIONPROJECT_DATE_FORMAT_4                                        "dd:MM:yyyy"
#define CLASSIFICATIONPROJECT_DATE_MINIMUM_VALUE                                   "2001/01/01"
#define CLASSIFICATIONPROJECT_DATE_MAXIMUM_VALUE                                   "2030/01/01"

#define CLASSIFICATIONPROJECT_NDVI_MINIMUM_VALUE                                   0.001
#define CLASSIFICATIONPROJECT_NDVI_MAXIMUM_VALUE                                   1.0

//#define TONIPBPPROJECT_NDVI_MINIMUM_VALUE                                   0.001
//#define TONIPBPPROJECT_NDVI_MAXIMUM_VALUE                                   1.0

#define CLASSIFICATIONPROJECT_NODOUBLE                                             -99999.9
#define CLASSIFICATIONPROJECT_RASTER_NODATAVALUE                                   -999.0

#define CLASSIFICATIONPROJECT_ZONECODE_PREFIX                                      "zone_"

#define CLASSIFICATIONPROJECT_CREATE_ROISTABLE_SQL_FILENAME                        "/RemoteSensingTemplates/CreateROIsClassificationTable.sql"

#define CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME                       "rois_classification"
#define CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_ID                   "id"
#define CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_CROP                 "crop"
#define CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_GEOMETRY             "the_geom"

#define CLASSIFICATIONPROJECT_L8_TAG                                                "L8"
#define CLASSIFICATIONPROJECT_S2A_TAG                                               "S2A"

#define CLASSIFICATIONPROJECT_CROP_VALUES_ALL                                       "All"

#define CLASSIFICATIONPROJECT_MEAN_VALUES_PRINT_PRECISION                           3
#define CLASSIFICATIONPROJECT_STD_VALUES_PRINT_PRECISION                            3
#define CLASSIFICATIONPROJECT_VALUES_PRINT_WIDTH                                    8
#define CLASSIFICATIONPROJECT_ROI_ID_PRINT_WIDTH                                    10
#define CLASSIFICATIONPROJECT_ROI_CROP_PRINT_WIDTH                                  10
#define CLASSIFICATIONPROJECT_ROI_ID_PRINT_TAG                                      "ROI_id"
#define CLASSIFICATIONPROJECT_ROI_CROP_PRINT_TAG                                    "ROI_Crop"
#define CLASSIFICATIONPROJECT_ROI_AREA_PRECISION                                    1
#define CLASSIFICATIONPROJECT_ROI_AREA_PRINT_WIDTH                                  12
#define CLASSIFICATIONPROJECT_ROI_AREA_PRINT_TAG                                    "ROI_Area"

#define CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE                  10000.0 // Para almacecenar dos decimales

class OGRGeometry;

namespace NestedGrid{
    class NestedGridTools;
    class NestedGridProject;
}

namespace libCRS{
class CRSTools;
}

namespace IGDAL{
    class libIGDALProcessMonitor;
    class Shapefile;
}

namespace RemoteSensing{
class PersistenceManager;
class LIBREMOTESENSINGSHARED_EXPORT ClassificationProject : public QObject
{
    Q_OBJECT
public:
    ClassificationProject(libCRS::CRSTools* ptrCrsTools,
                          NestedGrid::NestedGridTools* ptrNestedGridTools,
                          IGDAL::libIGDALProcessMonitor* ptrLibIGDALProcessMonitor,
                          bool removeRubbish,
                          bool fromConsole,
                          QWidget *parent=NULL);
    bool createClassificationData(QString& strError);
    bool getApplyProcessCreateClassificationData(){return(mProcessCreateClassificationData);};
    bool setFromFile(QString& inputFileName,
                     QString& strError);
public slots:
    void multiProcessFinished();

private:
    void clear();
    bool getNdviDataByTuplekeyByRoi(int initialJd,
                                    int finalJd,
                                    QMap<QString, QMap<int, QMap<int, QVector<QString> > > > &tuplekeyFileNamesByTuplekeyByRoiCodeByJd,
                                    QMap<int, int> &roiCropByRoi,
                                    QMap<QString,int>& tuplekeyIdByTuplekeyCode,
                                    QMap<QString, double> &gainByTuplekeyFileName,
                                    QMap<QString, double> &offsetByTuplekeyFileName,
                                    QMap<QString, int> &lodTilesByTuplekeyFileName,
                                    QMap<QString, int> &lodGsdByTuplekeyFileName,
                                    QMap<QString, QString> &rasterIdByTuplekeyFileName,
                                    int &maximumLod,
                                    QString &strError);
    bool getIntersectsPixel(double rasterNwFc,
                            double rasterNwSc,
                            double rasterGsd,
                            int col,
                            int row,
                            OGRGeometry* ptrROIGeometry,
                            bool &intersects,
                            QString &strError);
    bool getROIArea(int roiId,
                    double& area,
                    QString &strError);
    bool getROITuplekeyIntersectionGeomety(int roiId,
                                           int tuplekeyId,
                                           OGRGeometry *&ptrGeometry,
                                           bool& roiContainsTuplekey,
                                           QString &strError);
    bool removeDir(QString dirName);
    bool removeRubbish(QString &strError);
    QTextStream* mStdOut;
    bool mFromConsole;
    bool mRemoveRubbish;
    QList<QString> mPathsToRemoveIfEmpty;
    QList<QString> mPathsToRemove;
    QList<QString> mFilesToRemove;
//    QVector<int> mROIsGids;
//    QVector<int> mROIsCrops;
//    QVector<OGRGeometry*> mROIsPtrGeometries;
    bool mProcessCreateClassificationData;
    IGDAL::Shapefile* mPtrROIsShapefile;
    int mInitialJd;
    int mFinalJd;
    double mStdThreshold;
    QString mReportFileName;
    QString mL8SpacecraftIdentifier;
    QString mS2ASpacecraftIdentifier;
    QString mL8ReportFileName;
    QString mS2AReportFileName;
    QString mResultsPath;
    bool mIsInitialized;
    QStringList mValidFormatsForDate;
    QWidget *mPtrParentWidget;
    libCRS::CRSTools* mPtrCrsTools;
    PersistenceManager *mPtrPersistenceManager;
    NestedGrid::NestedGridTools* mPtrNestedGridTools;
    NestedGrid::NestedGridProject* mPtrNestedGridProject;
    IGDAL::libIGDALProcessMonitor* mPtrLibIGDALProcessMonitor;
    int mROIIncrement;
};
}
#endif // CLASSIFICATIONPROJECT_H
