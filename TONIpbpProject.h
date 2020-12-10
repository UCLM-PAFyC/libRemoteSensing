#ifndef TONIPBPPROJECT_H
#define TONIPBPPROJECT_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QObject>
#include <QTextStream>
#include <QStringList>

#include "libremotesensing_global.h"

#define TONIPBPPROJECT_SEPARATOR_CHARACTER                                  "#"
#define TONIPBPPROJECT_DATES_SEPARATOR_CHARACTER                            "-"
#define TONIPBPPROJECT_NDVIS_SEPARATOR_CHARACTER                            "-"

#define TONIPBPPROJECT_DATE_FORMAT_1                                        "yyyy/MM/dd"
#define TONIPBPPROJECT_DATE_FORMAT_2                                        "yyyy:MM:dd"
#define TONIPBPPROJECT_DATE_FORMAT_3                                        "dd/MM/yyyy"
#define TONIPBPPROJECT_DATE_FORMAT_4                                        "dd:MM:yyyy"
#define TONIPBPPROJECT_DATE_MINIMUM_VALUE                                   "2001/01/01"
#define TONIPBPPROJECT_DATE_MAXIMUM_VALUE                                   "2030/01/01"

#define TONIPBPPROJECT_NDVI_MINIMUM_VALUE                                   0.001
#define TONIPBPPROJECT_NDVI_MAXIMUM_VALUE                                   1.0

#define TONIPBPPROJECT_ETH0_MINIMUM_VALUE                                   0.001
#define TONIPBPPROJECT_ETH0_MAXIMUM_VALUE                                   100000.0

//#define TONIPBPPROJECT_NDVI_MINIMUM_VALUE                                   0.001
//#define TONIPBPPROJECT_NDVI_MAXIMUM_VALUE                                   1.0

#define TONIPBPPROJECT_NODOUBLE                                             -99999.9
#define TONIPBPPROJECT_RASTER_NODATAVALUE                                   -999.0

#define TONIPBPPROJECT_ZONECODE_PREFIX                                      "zone_"

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
class LIBREMOTESENSINGSHARED_EXPORT TONIpbpProject : public QObject
{
    Q_OBJECT
public:
    TONIpbpProject(libCRS::CRSTools* ptrCrsTools,
                   NestedGrid::NestedGridTools* ptrNestedGridTools,
                   IGDAL::libIGDALProcessMonitor* ptrLibIGDALProcessMonitor,
                   bool removeRubbish,
                   bool fromConsole,
                   QWidget *parent=NULL);
    bool loadETH0DataFromFile(QString fileName,
                              QString& strError);
    bool processAccumulatedPerspirationByRoi(QString& strError);
    bool processAccumulatedPerspirationByTuplekeyByRoi(QString& strError);
    bool processAccumulatedPerspirationByTuplekey(QString& strError);
    bool setFromFile(QString& inputFileName,
                     QString& strError);
public slots:
    void multiProcessFinished();

private:
    void clear();
    bool removeDir(QString dirName);
    bool removeRubbish(QString &strError);
    QTextStream* mStdOut;
    bool mFromConsole;
    bool mRemoveRubbish;
    QList<QString> mPathsToRemoveIfEmpty;
    QList<QString> mPathsToRemove;
    QList<QString> mFilesToRemove;
    QString mETH0FileName;
    QMap<int,double> mETHOValues;
    QVector<QString> mProjectCodes;
    IGDAL::Shapefile* mPtrROIsShapefile;
    int mInitialJd;
    int mFinalJd;
    double mInitialNdvi;
    double mFinalNdvi;
    double mKcbM;
    double mKcbN;
    QString mReportFileName;
//    QString mOutputRasterFileName;
    QString mResultsPath;
    bool mIsInitialized;
    QStringList mValidFormatsForDate;
    QWidget *mPtrParentWidget;
    libCRS::CRSTools* mPtrCrsTools;
    PersistenceManager *mPtrPersistenceManager;
    NestedGrid::NestedGridTools* mPtrNestedGridTools;
    NestedGrid::NestedGridProject* mPtrNestedGridProject;
    IGDAL::libIGDALProcessMonitor* mPtrLibIGDALProcessMonitor;
};
}
#endif // TONIPBPPROJECT_H
