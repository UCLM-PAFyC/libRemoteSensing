#ifndef LIB_REMOTE_SENSING_ALGORITHMS_H
#define LIB_REMOTE_SENSING_ALGORITHMS_H

#define LIB_REMOTE_SENSING_ALGORITHMS_PARAMETERS_FILE_NAME                 "parameters_rs.xml"

#include <QTextStream>
#include <QVector>
#include <QMap>
#include <QWidget>
#include <QFile>

#include "libremotesensing_global.h"

class ParametersManager;

namespace PW{
class MultiProcess;
}

namespace NestedGrid{
    class NestedGridTools;
//    class NestedGridProject;
}

namespace IGDAL{
    class libIGDALProcessMonitor;
    class Raster;
}

namespace RemoteSensing{
class PersistenceManager;
}

namespace libCRS{
class CRSTools;
}

namespace RemoteSensing{
class LIBREMOTESENSINGSHARED_EXPORT Algorithms
{
public:
    Algorithms(libCRS::CRSTools* ptrCrsTools,
               NestedGrid::NestedGridTools* ptrNestedGridTools,
               IGDAL::libIGDALProcessMonitor* ptrLibIGDALProcessMonitor,
               RemoteSensing::PersistenceManager *ptrPersistenceManager,
               QWidget* ptrWidgetParent=NULL);
    bool applyIntercalibration(QString inputFileName,
                               QString outputFileName,
                               bool outputTo8bits,
                               double gain,
                               double offset,
                               bool parametersIn8Bits,
                               bool parametersInReflectance,
                               double toReflectanceMultValue,
                               double toReflectanceAddValue,
                               QString& strError);
    bool cloudRemoval(QVector<QString>& rasterFiles,
                      QMap<QString,QString>& rasterTypesByRasterFile,
                      QMap<QString,QVector<QString> >& rasterFilesByTuplekey,
                      QMap<QString,QMap<QString,QMap<QString,QString> > >& tuplekeysRasterFilesByQuadkeyByRasterFileAndByBand,
                      QMap<QString,int>& jdByRasterFile,
                      QMap<QString, double> &sunAzimuthByRasterFile,
                      QMap<QString, double> &sunElevationByRasterFile,
                      QMap<QString, QMap<QString, double> > &reflectanceAddValueByRasterFileAndByBand,
                      QMap<QString, QMap<QString, double> > &reflectanceMultValueByRasterFileAndByBand,
                      QMap<QString, QMap<QString, double> > &intercalibrationGainByRasterFileAndByBand,
                      QMap<QString, QMap<QString, double> > &intercalibrationOffsetByRasterFileAndByBand,
                      QMap<QString, QMap<QString, bool> > &intercalibrationTo8BitsByRasterFileAndByBand,
                      QMap<QString, QMap<QString, bool> > &intercalibrationToReflectanceByRasterFileAndByBand,
                      QMap<QString, QMap<QString, bool> > &intercalibrationInterpolatedByRasterFileAndByBand,
                      QString intercalibrationReferenceImageRasterId,
                      bool mergeFiles,
                      bool reprocessFiles,
                      bool reprojectFiles,
                      QFile &resultsFile,
                      QMap<QString,QVector<QString> >& bandsInAlgorithmBySpaceCraft,
                      QString& strError);
    bool cloudRemovalByCloudFreeImprovement(QVector<QString>& rasterFiles,
                                            QMap<QString,QString>& rasterTypesByRasterFile,
                                            QMap<QString,QVector<QString> >& rasterFilesByTuplekey,
                                            QMap<QString,QMap<QString,QMap<QString,QString> > >& tuplekeysRasterFilesByQuadkeyByRasterFileAndByBand,
                                            QMap<QString,int>& jdByRasterFile,
                                            QMap<QString, double> &sunAzimuthByRasterFile,
                                            QMap<QString, double> &sunElevationByRasterFile,
                                            QMap<QString, QMap<QString, double> > &reflectanceAddValueByRasterFileAndByBand,
                                            QMap<QString, QMap<QString, double> > &reflectanceMultValueByRasterFileAndByBand,
                                            QMap<QString, QMap<QString, double> > &intercalibrationGainByRasterFileAndByBand,
                                            QMap<QString, QMap<QString, double> > &intercalibrationOffsetByRasterFileAndByBand,
                                            QMap<QString, QMap<QString, bool> > &intercalibrationTo8BitsByRasterFileAndByBand,
                                            QMap<QString, QMap<QString, bool> > &intercalibrationToReflectanceByRasterFileAndByBand,
                                            QMap<QString, QMap<QString, bool> > &intercalibrationInterpolatedByRasterFileAndByBand,
                                            QString intercalibrationReferenceImageRasterId,
                                            bool mergeFiles,
                                            bool reprocessFiles,
                                            bool reprojectFiles,
                                            QFile &resultsFile,
                                            QMap<QString,QVector<QString> >& bandsInAlgorithmBySpaceCraft,
                                            QString& strError);
    bool getAlgorithmsGuiTags(QVector<QString> &algorithmsCodes,
                              QMap<QString,QString>& algorithmsGuiTags,
                              QString& strError);
    bool getParameterValue(QString algorithm,
                           QString code,
                           QString &value,
                           QString &strError);
    ParametersManager* getParametersManager(){return(mPtrParametersManager);};
    bool getParametersTagAndValues(QString command,
                                   QVector<QString> &codes,
                                   QVector<QString> &tags,
                                   QVector<QString> &values,
                                   QString &strError);
    bool initialize(QString libPath,
                    QString& strError);
    bool intercalibrationComputation(QVector<QString>& rasterFiles,
                                     QMap<QString,QString>& rasterTypesByRasterFile,
                                     QMap<QString,QVector<QString> >& rasterFilesByQuadkey,
                                     QMap<QString,QMap<QString,QMap<QString,QString> > >& quadkeysRasterFilesByQuadkeyByRasterFileAndByBand,
                                     QMap<QString,int>& jdByRasterFile,
                                     QMap<QString, double> &sunAzimuthByRasterFile,
                                     QMap<QString, double> &sunElevationByRasterFile,
                                     QMap<QString, QMap<QString, double> > &reflectanceAddValueByRasterFileAndByBand,
                                     QMap<QString, QMap<QString, double> > &reflectanceMultValueByRasterFileAndByBand,
                                     QVector<int>& piasFilesIds,
                                     QMap<int,QString>& piasFileNameByPiasFilesId,
                                     QMap<int,int>& piasValueByPiasFilesId,
                                     QMap<int,QString>& piasTuplekeyByPiasFilesId,
                                     QMap<int,QVector<QString> >& rasterFilesByPiasFilesIds,
                                     bool mergeFiles,
                                     bool reprocessFiles,
                                     bool reprojectFiles,
                                     QFile &resultsFile,
                                     int mainScenceJd,
                                     QString& strError);
    bool ndviComputation(QString quadkey,
                         QString rasterFile,
                         QString computationMethod,
                         bool to8Bits, // solo tiene sentido si es en DN
                         QString rasterUnitConversion,
                         QString redFileName,
                         QString nirFileName,
                         QString outputFileName,
                         bool reprocess,
                         QString& strError,
                         QVector<QVector<float> > &values,
                         bool readValues=false);
    bool piasComputation(QVector<QString>& rasterFiles,
                         QMap<QString,QString>& rasterTypesByRasterFile,
                         QMap<QString,QVector<QString> >& rasterFilesByQuadkey,
                         QMap<QString,QMap<QString,QMap<QString,QString> > >& quadkeysRasterFilesByQuadkeyByRasterFileAndByBand,
                         QMap<QString,int>& jdByRasterFile,
                         QMap<QString,double>& sunAzimuthByRasterFile,
                         QMap<QString,double>& sunElevationByRasterFile,
                         QMap<QString,QMap<QString,double> >& reflectanceAddValueByRasterFileAndByBand,
                         QMap<QString,QMap<QString,double> >& reflectanceMultValueByRasterFileAndByBand,
                         bool mergeFiles,
                         bool reprocessFiles,
                         bool reprojectFiles,
                         QFile &resultsFile,
                         QString& strError);
    bool readMaskRasterFile(QString maskBandRasterFile,
                            int maskValue,
                            QVector<double>& georef,
                            QVector<QVector<bool> > &values,
                            float &maskValuesPercentage,
                            QString& strError);
    bool readMaskRasterFile(QString maskBandRasterFile,
                            int maskValue,
                            QVector<double>& georef,
                            QMap<int,QVector<int> > &values,
                            float &maskValuesPercentage,
                            QString& strError);
    bool reflectanceComputation(QString inputFileName,
                                QString outputFileName,
                                double sunElevation,
                                double addValue,
                                double multValue,
                                QString& strError);
    bool setAlgorithms(QString& strError);
    bool setParametersForAlgorithm(QString command,
                                   QString &strError,
                                   QWidget *ptrWidget);
    bool setParametersManager(QString fileName,
                              QString& strError);
    bool writeCloudRemovedFile(QString inputFileName,
                               QString outputFileName,
                               QVector<QVector<double> >& bandData,
                               QString& strError);
private:
    QString mFileName;
    QString mStrExecution;
    PW::MultiProcess *mPtrMultiProcess;
    libCRS::CRSTools* mPtrCrsTools;
    RemoteSensing::PersistenceManager *mPtrPersistenceManager;
    NestedGrid::NestedGridTools* mPtrNestedGridTools;
    IGDAL::libIGDALProcessMonitor* mPtrLibIGDALProcessMonitor;
    ParametersManager* mPtrParametersManager;
    bool mIsInitialized;
    QMap<QString,QString> mAlgorithmsGuiTagsByCode;
    QVector<QString> mAlgorithmsCodes;
    QWidget* mPtrWidgetParent;
};
}
#endif // LIB_REMOTE_SENSING_ALGORITHMS_H
