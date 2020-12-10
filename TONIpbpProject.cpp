#include <QCoreApplication>
#include <QDir>
#include <QProgressDialog>
#include <QWidget>
#include <QFile>
#include <QDate>

#include "../../libs/libCRS/CRSTools.h"
#include "../../libs/libNestedGrid/NestedGridTools.h"
#include "../../libs/libIGDAL/libIGDALProcessMonitor.h"
#include "../../libs/libIGDAL/Shapefile.h"
#include "../../libs/libIGDAL/Raster.h"
#include "../../libs/libRemoteSensing/PersistenceManager.h"
#include "../../libs/libProcessTools/MultiProcess.h"
#include "../../libs/libProcessTools/ExternalProcess.h"

#include "TONIpbpProject.h"

using namespace RemoteSensing;

TONIpbpProject::TONIpbpProject(libCRS::CRSTools* ptrCrsTools,
                               NestedGrid::NestedGridTools* ptrNestedGridTools,
                               IGDAL::libIGDALProcessMonitor *ptrLibIGDALProcessMonitor,
                               bool removeRubbish,
                               bool fromConsole,
                               QWidget *parent):
    mPtrCrsTools(ptrCrsTools),
    mPtrNestedGridTools(ptrNestedGridTools),
    mPtrLibIGDALProcessMonitor(ptrLibIGDALProcessMonitor),
    mRemoveRubbish(removeRubbish)
{
    mStdOut = new QTextStream(stdout);
    mFromConsole=fromConsole;
    mPtrParentWidget=parent;
    if(mPtrParentWidget==NULL&&!fromConsole)
    {
        mPtrParentWidget=new QWidget();
    }
    mPtrROIsShapefile=NULL;
    mPtrPersistenceManager=NULL;
    mValidFormatsForDate.append(TONIPBPPROJECT_DATE_FORMAT_1);
    mValidFormatsForDate.append(TONIPBPPROJECT_DATE_FORMAT_2);
    mValidFormatsForDate.append(TONIPBPPROJECT_DATE_FORMAT_3);
    mValidFormatsForDate.append(TONIPBPPROJECT_DATE_FORMAT_4);
    mInitialNdvi=TONIPBPPROJECT_NODOUBLE;
    mFinalNdvi=TONIPBPPROJECT_NODOUBLE;
    mKcbM=TONIPBPPROJECT_NODOUBLE;
    mKcbN=TONIPBPPROJECT_NODOUBLE;
    mIsInitialized=false;
}

bool TONIpbpProject::loadETH0DataFromFile(QString fileName,
                                          QString &strError)
{
    if(!QFile::exists(fileName))
    {
        strError=QObject::tr("TONIpbpProject::loadETH0DataFromFile");
        strError+=QObject::tr("\nNot exists input file:\n%1").arg(fileName);
        return(false);
    }
    QFile fileInput(fileName);
    if (!fileInput.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        strError=QObject::tr("TONIpbpProject::loadETH0DataFromFile");
        strError+=QObject::tr("\nError opening file: \n%1").arg(fileName);
        return(false);
    }
    QFileInfo fileInputInfo(fileName);
    QTextStream in(&fileInput);

    int intValue,nline=0;
    double dblValue;
    bool okToInt,okToDouble;
    QString strLine,strValue,strAuxError;
    QStringList strList;
    QStringList strAuxList;
    QDir currentDir=QDir::current();
    int minimumValueDate=QDate::fromString(TONIPBPPROJECT_DATE_MINIMUM_VALUE,TONIPBPPROJECT_DATE_FORMAT_1).toJulianDay();
    int maximumValueDate=QDate::fromString(TONIPBPPROJECT_DATE_MAXIMUM_VALUE,TONIPBPPROJECT_DATE_FORMAT_1).toJulianDay();
    QMap<int,double> ethOValues;
    int previousJd=-1;
    do{
        strLine = in.readLine().trimmed();
        nline++;
        if(strLine.isEmpty())
            continue;
        strList=strLine.split(QRegExp("\\s"));
        if(strList.size()!=2)
        {
            strError=QObject::tr("TONIpbpProject::loadETH0DataFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(fileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            return(false);
        }
        QString strDate=strList.at(0).trimmed();
        int jd=0;
        for(int nfd=0;nfd<mValidFormatsForDate.size();nfd++)
        {
            QDate date=QDate::fromString(strDate,mValidFormatsForDate.at(nfd));
            if(date.isValid())
            {
                jd=date.toJulianDay();
                break;
            }
        }
        if(jd==0)
        {
            strError=QObject::tr("TONIpbpProject::loadETH0DataFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(fileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nInvalid date format for initial date in string: %1").arg(strDate);
            fileInput.close();
            return(false);
        }
        else
        {
            if(jd<minimumValueDate||jd>maximumValueDate)
            {
                strError=QObject::tr("TONIpbpProject::loadETH0DataFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(fileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nDate, %1, is out of valid domain:[%2,%3]")
                        .arg(strDate).arg(TONIPBPPROJECT_DATE_MINIMUM_VALUE).arg(TONIPBPPROJECT_DATE_MAXIMUM_VALUE);
                fileInput.close();
                return(false);
            }
        }
        if(jd<previousJd)
        {
            strError=QObject::tr("TONIpbpProject::loadETH0DataFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(fileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nDate, %1, is less than previous date").arg(strDate);
            fileInput.close();
            return(false);
        }
        QString strETH0Value=strList.at(1).trimmed();
        okToDouble=false;
        double eth0Value=strETH0Value.toDouble(&okToDouble);
        if(!okToDouble)
        {
            strError=QObject::tr("TONIpbpProject::loadETH0DataFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(fileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nInitial NDVI value is not a double: %1").arg(strETH0Value);
            fileInput.close();
            return(false);
        }
        if(eth0Value<TONIPBPPROJECT_ETH0_MINIMUM_VALUE||eth0Value>TONIPBPPROJECT_ETH0_MAXIMUM_VALUE)
        {
            strError=QObject::tr("TONIpbpProject::loadETH0DataFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(fileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("ETH0, %1, is out of valid domain:[%2,%3]")
                    .arg(strETH0Value).arg(QString::number(TONIPBPPROJECT_ETH0_MINIMUM_VALUE,'f',3))
                    .arg(QString::number(TONIPBPPROJECT_ETH0_MAXIMUM_VALUE,'f',3));
            fileInput.close();
            return(false);
        }
        previousJd=jd;
        ethOValues[jd]=eth0Value;
    }while (!strLine.isNull());    // Se ignora la cabecera, integrada por dos lineas
    mETHOValues=ethOValues;
    return(true);
}

bool TONIpbpProject::processAccumulatedPerspirationByRoi(QString &strError)
{
    if(!mIsInitialized)
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
        strError+=QObject::tr("\nProject is not initialized");
        return(false);
    }
    QMap<QString,QMap<QString,QMap<int,QString> > > tuplekeyFileNameByProjectCodeByTuplekeyByJd;
    QMap<QString,QMap<QString,QMap<int,double> > > gainByProjectCodeByTuplekeyByJd;
    QMap<QString,QMap<QString,QMap<int,double> > > offsetByProjectCodeByTuplekeyByJd;
    QMap<QString,int> lodByTuplekey;
    int globalMaximumLod;
    QString strAuxError;
    if(!mPtrPersistenceManager->getNdviDataByProject(tuplekeyFileNameByProjectCodeByTuplekeyByJd,
                                                     gainByProjectCodeByTuplekeyByJd,
                                                     offsetByProjectCodeByTuplekeyByJd,
                                                     lodByTuplekey,
                                                     globalMaximumLod,
                                                     strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
        strError+=QObject::tr("\nError recovering ndvi data from database:\n%1").arg(strAuxError);
        return(false);
    }
//    QMap<QString, double> minFirstCoordinateByProjectCode;
//    QMap<QString, double> maxSecondCoordinateByProjectCode;
//    QMap<QString, double> maxFirstCoordinateByProjectCode;
//    QMap<QString, double> minSecondCoordinateByProjectCode;
//    if(!mPtrPersistenceManager->getProjectEnvelopes(minFirstCoordinateByProjectCode,
//                                                    maxSecondCoordinateByProjectCode,
//                                                    maxFirstCoordinateByProjectCode,
//                                                    minSecondCoordinateByProjectCode,
//                                                    strAuxError))
//    {
//        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
//        strError+=QObject::tr("\nError recovering envelopes of projects from database:\n%1").arg(strAuxError);
//        return(false);
//    }
    QMap<QString, OGRGeometry *> geometryByProjectCode;
    if(!mPtrPersistenceManager->getProjectGeometries(geometryByProjectCode,
                                                     strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
        strError+=QObject::tr("\nError recovering geometries of projects from database:\n%1").arg(strAuxError);
        return(false);
    }
    QFile reportFile(mReportFileName);
    if (!reportFile.open(QFile::WriteOnly |QFile::Text))
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(mReportFileName);
        return(false);
    }
    QTextStream out(&reportFile);
    out<<"RESULTADOS DE PROCESAMIENTO\n";
    out<<"---------------------------\n";
    QMap<QString,QMap<QString,QMap<int,QString> > >::const_iterator iter1=tuplekeyFileNameByProjectCodeByTuplekeyByJd.begin();
    QString crsDescription=mPtrNestedGridTools->getCrsDescription();
    int numberOfPixels=mPtrNestedGridTools->getBaseWorldWidthAndHeightInPixels();
    int rasterBand=0;
    while(iter1!=tuplekeyFileNameByProjectCodeByTuplekeyByJd.end())
    {
        QString projectCode=iter1.key();
        OGRGeometry* ptrROIGeometry=geometryByProjectCode[projectCode];
        QMap<QString,QMap<int,QString> > tuplekeyFileNameByTuplekeyByJd=iter1.value();
        QMap<QString,QMap<int,QString> >::const_iterator iter2=tuplekeyFileNameByTuplekeyByJd.begin();
        // Obtener el gsd de cada fichero que intersecte
        double minGsd=1000000.0;
        int maximumLod=0;
        QMap<QString,IGDAL::Raster*> ptrRasterFileByFileName; // si no se inserta es porque no intersecta con la geometria de la ROI
        QMap<QString,QVector<double> > georefRasterFileByFileName;
        while(iter2!=tuplekeyFileNameByTuplekeyByJd.end())
        {
            QString tuplekey=iter2.key();
            int lod=lodByTuplekey[tuplekey];
            QMap<int,QString> tuplekeyFileNameByJd=iter2.value();
            QMap<int,QString>::const_iterator iter3=tuplekeyFileNameByJd.begin();
            bool existSomeRasterFile=false;
            while(iter3!=tuplekeyFileNameByJd.end())
            {
                QString rasterFileName=iter3.value();
                IGDAL::Raster* ptrRasterFile=NULL;
                ptrRasterFile=new IGDAL::Raster(mPtrCrsTools);
                if(!ptrRasterFile->setFromFile(rasterFileName,strAuxError))
                {
                    strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
                    strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                            .arg(rasterFileName).arg(strAuxError);
                    QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
                    while(iter!=geometryByProjectCode.end())
                    {
                        OGRGeometryFactory::destroyGeometry(iter.value());
                        iter.value()=NULL;
                        iter++;
                    }
                    return(false);
                }
                OGRGeometry *ptrRasterEnvelopeGeometry=ptrRasterFile->getEnvelopeGeometry();
                if(!ptrROIGeometry->Intersects(ptrRasterEnvelopeGeometry))
                {
                    iter3++;
                    continue;
                }
                existSomeRasterFile=true;
                QVector<double> georef;
                if(!ptrRasterFile->getGeoRef(georef,
                                             strAuxError))
                {
                    strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
                    strError+=QObject::tr("\nError getting georef from raster file:\n%1\nError:\n%2")
                            .arg(rasterFileName).arg(strAuxError);
                    QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
                    while(iter!=geometryByProjectCode.end())
                    {
                        OGRGeometryFactory::destroyGeometry(iter.value());
                        iter.value()=NULL;
                        iter++;
                    }
                    QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
                    while(iterRf!=ptrRasterFileByFileName.end())
                    {
                        if(iterRf.value()!=NULL)
                        {
                            delete(iterRf.value());
                            iterRf.value()=NULL;
                        }
                        iterRf++;
                    }
                    return(false);
                }
                ptrRasterFileByFileName[rasterFileName]=ptrRasterFile;
                georefRasterFileByFileName[rasterFileName]=georef;
                double rasterGsd=georef[1];
                if(rasterGsd<minGsd)
                {
                    minGsd=rasterGsd;
                }

                iter3++;
            }
            if(existSomeRasterFile)
            {
                if(lod>maximumLod)
                {
                    maximumLod=lod;
                }
            }
            iter2++;
        }
        double roiNwFc,roiNwSc;
        QMap<QString, QMap<int, QVector<int> > > roiColumnsByRowByTuplekey;
        int roiColumns,roiRows;
        if(!mPtrNestedGridTools->getRoiPixelsFromPolygon(ptrROIGeometry,
                                                         crsDescription,
                                                         maximumLod,
                                                         minGsd,
                                                         roiNwFc,
                                                         roiNwSc,
                                                         roiColumnsByRowByTuplekey,
                                                         roiColumns,
                                                         roiRows,
                                                         strAuxError))
        {
            strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
            strError+=QObject::tr("\nError getting pixels for project: %1\nError:\n%2")
                    .arg(projectCode).arg(strAuxError);
            QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
            while(iter!=geometryByProjectCode.end())
            {
                OGRGeometryFactory::destroyGeometry(iter.value());
                iter.value()=NULL;
                iter++;
            }
            QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
            while(iterRf!=ptrRasterFileByFileName.end())
            {
                if(iterRf.value()!=NULL)
                {
                    delete(iterRf.value());
                    iterRf.value()=NULL;
                }
                iterRf++;
            }
            return(false);
        }
        out<<"- Procesamiento de la zona .........................: "<<projectCode<<"\n";
        out<<"  - LOD maximo .....................................: "<<QString::number(maximumLod)<<"\n";
        out<<"  - GSD minimo .....................................: "<<QString::number(minGsd,'f',2)<<"\n";
//        out<<"  - Dimensiones del raster (columnas,filas) ........: ("<<QString::number(roiColumns)<<","<<QString::number(roiRows)<<")\n";
        // Se crea el raster
        int roiNumberOfPixels=roiRows*roiColumns;
        float* pData=(float*)malloc(roiNumberOfPixels*sizeof(GDT_Float32));
        QVector<double> roiGeoref;
        double roiSeFc=roiNwFc+minGsd*roiColumns;
        double roiSeSc=roiNwSc-minGsd*roiRows;
        bool closeAfterCreate=false;
        IGDAL::ImageTypes roiImageType=IGDAL::TIFF;
        GDALDataType roiGdalDataType=GDT_Float32;
        QString rasterOutputFileName=mResultsPath+"/"+projectCode+"."+RASTER_TIFF_FILE_EXTENSION;
        int numberOfBands=1;
        bool internalGeoRef=true;
        bool externalGeoRef=false;
        double noDataValue=TONIPBPPROJECT_RASTER_NODATAVALUE;
        IGDAL::Raster* ptrOutputRasterFile=new IGDAL::Raster(mPtrCrsTools);
        QMap<QString, QString> refImageOptions; // por implementar
        if(!ptrOutputRasterFile->createRaster(rasterOutputFileName, // Se le añade la extension
                                              roiImageType,roiGdalDataType,
                                              numberOfBands,
                                              roiColumns,roiRows,
                                              internalGeoRef,externalGeoRef,crsDescription,
                                              roiNwFc,roiNwSc,roiSeFc,roiSeSc,
                                              roiGeoref, // vacío si se georeferencia con las esquinas
                                              noDataValue,
                                              closeAfterCreate,
                                              refImageOptions,
                                              //                                               buildOverviews,
                                              strAuxError))
        {
            strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
            strError+=QObject::tr("\nError creating raster file:\n%1\nfor project: %2\nError:\n%2")
                    .arg(rasterOutputFileName).arg(projectCode).arg(strAuxError);
            QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
            while(iter!=geometryByProjectCode.end())
            {
                OGRGeometryFactory::destroyGeometry(iter.value());
                iter.value()=NULL;
                iter++;
            }
            QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
            while(iterRf!=ptrRasterFileByFileName.end())
            {
                if(iterRf.value()!=NULL)
                {
                    delete(iterRf.value());
                    iterRf.value()=NULL;
                }
                iterRf++;
            }
            free(pData);
            return(false);
        }
        for(int row=0;row<roiRows;row++)
        {
            for(int column=0;column<roiColumns;column++)
            {
                pData[row*roiColumns+column]=noDataValue;
            }
        }

        QMap<QString, QMap<int, QVector<int> > >::const_iterator iterRoiColumnsByRowByTuplekey=roiColumnsByRowByTuplekey.begin();
        while(iterRoiColumnsByRowByTuplekey!=roiColumnsByRowByTuplekey.end())
        {
            QString tuplekey=iterRoiColumnsByRowByTuplekey.key();
            out<<"  - Procesamiento del tuplekey .....................: "<<tuplekey<<"\n";
            if(!tuplekeyFileNameByTuplekeyByJd.contains(tuplekey))
            {
                out<<"    No hay informacion para este tuplekey"<<"\n";
                iterRoiColumnsByRowByTuplekey++;
            }
            QVector<int> jds;
            QVector<QString> fileNames;
            QVector<double> gains;
            QVector<double> offsets;
            QMap<int,QString> tuplekeyFileNameByJd=tuplekeyFileNameByTuplekeyByJd[tuplekey];
            QMap<int,QString>::const_iterator iter3=tuplekeyFileNameByJd.begin();
            while(iter3!=tuplekeyFileNameByJd.end())
            {
                int jd=iter3.key();
                QString fileName=iter3.value();
                if(ptrRasterFileByFileName.contains(fileName)) // siempre se tiene que cumplir
                {
                    jds.push_back(jd);
                    fileNames.push_back(fileName);
                    double gain=gainByProjectCodeByTuplekeyByJd[projectCode][tuplekey][jd];
                    double offset=offsetByProjectCodeByTuplekeyByJd[projectCode][tuplekey][jd];
                    gains.push_back(gain);
                    offsets.push_back(offset);
                }
                iter3++;
            }
            out<<"  - Procesamiento del tuplekey .....................: "<<tuplekey<<"\n";
            out<<"    - Escenas NDVI que intervienenr: "<<QString::number(jds.size())<<"\n";
            for(int njd=0;njd<jds.size();njd++)
            {
                out<<"    - Fichero ......................................: "<<fileNames[njd]<<"\n";
                out<<"      - Fecha ......................................: "<<QDate::fromJulianDay(jds[njd]).toString(TONIPBPPROJECT_DATE_FORMAT_1)<<"\n";
                out<<"      - Gain y Offset ..............................: ";
                out<<QString::number(gains[njd],'f',3).rightJustified(10);
                out<<" y ";
                out<<QString::number(offsets[njd],'f',3).rightJustified(10);
                out<<"\n";
            }
            QMap<int, QVector<int> > roiColumnsByRow=roiColumnsByRowByTuplekey[tuplekey];
            QMap<int, QVector<int> >::const_iterator iterRow=roiColumnsByRow.begin();
            QVector<IGDAL::Raster*> rastersToClose;
            QMap<QString,QVector<QVector<double> > > rasterValuesByFileName;
            QMap<QString,QVector<double> > rasterBoundingBoxByFileName; // nwFc,nwSc,seFc,seSc -> puede no coincidir con georef porque en algunos casos georef va al centro del pixel
            while(iterRow!=roiColumnsByRow.end())
            {
                int row=iterRow.key();
                for(int nc=0;nc<iterRow.value().size();nc++)
                {
                    int column=iterRow.value()[nc];
                    double centerPixelFc=roiNwFc+column*minGsd+0.5*minGsd;
                    double centerPixelSc=roiNwSc-row*minGsd-0.5*minGsd;
                    double nwPixelFc=roiNwFc+column*minGsd;
                    double nwPixelSc=roiNwSc-row*minGsd;
                    double nePixelFc=nwPixelFc+minGsd;
                    double nePixelSc=nwPixelSc;
                    double sePixelFc=nwPixelFc+minGsd;
                    double sePixelSc=nwPixelSc-minGsd;
                    double swPixelFc=nwPixelFc;
                    double swPixelSc=nwPixelSc-minGsd;
                    bool findInitialNdvi=false;
                    bool findFinalNdvid=false;
                    int previousJd=0;
                    double previousKcb=0.0;
                    double accumulatedValue=0.0;
                    for(int njd=0;njd<jds.size();njd++)
                    {
                        int jd=jds[njd];
                        if(jd<mInitialJd||jd>mFinalJd)
                        {
                            continue;
                        }
                        QString rasterFileName=fileNames[njd];
                        QVector<double> rasterGeoref=georefRasterFileByFileName[rasterFileName];
                        QVector<QVector<double> > rasterValues;
                        QVector<double> rasterBoundingBox;
                        int rasterColumns,rasterRows;
                        if(!rasterValuesByFileName.contains(rasterFileName))
                        {
                            IGDAL::Raster* ptrRaster=ptrRasterFileByFileName[rasterFileName];
                            if(!ptrRaster->getSize(rasterColumns,rasterRows,strAuxError))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
                                strError+=QObject::tr("\nError getting dimensions from raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
                                while(iter!=geometryByProjectCode.end())
                                {
                                    OGRGeometryFactory::destroyGeometry(iter.value());
                                    iter.value()=NULL;
                                    iter++;
                                }
                                QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
                                while(iterRf!=ptrRasterFileByFileName.end())
                                {
                                    if(iterRf.value()!=NULL)
                                    {
                                        delete(iterRf.value());
                                        iterRf.value()=NULL;
                                    }
                                    iterRf++;
                                }
                                free(pData);
                                delete(ptrOutputRasterFile);
                                return(false);
                            }
                            double rasterNwFc,rasterNwSc,rasterSeFc,rasterSeSc;
                            if(!ptrRaster->getBoundingBox(rasterNwFc,rasterNwSc,rasterSeFc,rasterSeSc,strAuxError))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
                                strError+=QObject::tr("\nError getting bounding box from raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
                                while(iter!=geometryByProjectCode.end())
                                {
                                    OGRGeometryFactory::destroyGeometry(iter.value());
                                    iter.value()=NULL;
                                    iter++;
                                }
                                QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
                                while(iterRf!=ptrRasterFileByFileName.end())
                                {
                                    if(iterRf.value()!=NULL)
                                    {
                                        delete(iterRf.value());
                                        iterRf.value()=NULL;
                                    }
                                    iterRf++;
                                }
                                free(pData);
                                delete(ptrOutputRasterFile);
                                return(false);
                            }
                            rasterBoundingBox.push_back(rasterNwFc);
                            rasterBoundingBox.push_back(rasterNwSc);
                            rasterBoundingBox.push_back(rasterSeFc);
                            rasterBoundingBox.push_back(rasterSeSc);
                            rasterBoundingBoxByFileName[rasterFileName]=rasterBoundingBox;
                            if(!ptrRaster->readValues(rasterBand,
                                                      0,0,
                                                      rasterColumns,rasterRows,
                                                      rasterValues,
                                                      strAuxError)) // values[row][column]
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
                                strError+=QObject::tr("\nError getting dimensions from raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
                                while(iter!=geometryByProjectCode.end())
                                {
                                    OGRGeometryFactory::destroyGeometry(iter.value());
                                    iter.value()=NULL;
                                    iter++;
                                }
                                QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
                                while(iterRf!=ptrRasterFileByFileName.end())
                                {
                                    if(iterRf.value()!=NULL)
                                    {
                                        delete(iterRf.value());
                                        iterRf.value()=NULL;
                                    }
                                    iterRf++;
                                }
                                free(pData);
                                delete(ptrOutputRasterFile);
                                return(false);
                            }
                            rasterValuesByFileName[rasterFileName]=rasterValues;
                        }
                        else
                        {
                            rasterValues=rasterValuesByFileName[rasterFileName];
                            rasterBoundingBox=rasterBoundingBoxByFileName[rasterFileName];
                            rasterRows=rasterValues.size();
                            rasterColumns=rasterValues[0].size();
                        }
                        double rasterGsd=rasterGeoref[1];
                        double rasterValue=0.0;
                        if(fabs(rasterGsd-minGsd)<0.001)
                        {
                            int rasterColumn=floor((centerPixelFc-rasterBoundingBox[0])/rasterGsd);
                            int rasterRow=floor((rasterBoundingBox[1]-centerPixelSc)/rasterGsd);
                            rasterValue=rasterValues[rasterRow][rasterColumn];
//                            rasterValue=rasterValues[row][column];
                        }
                        else
                        {
                            if(minGsd<=rasterGsd) // en este caso siempre se cumple
                            {
                                int rasterColumn=floor((centerPixelFc-rasterBoundingBox[0])/rasterGsd);
                                int rasterRow=floor((rasterBoundingBox[1]-centerPixelSc)/rasterGsd);
                                rasterValue=rasterValues[rasterRow][rasterColumn];
                            }
                            else
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
                                strError+=QObject::tr("\nError invalid GSD in [%1,%2] for raster file:\n%3")
                                        .arg(QString::number(column)).arg(QString::number(row)).arg(rasterFileName);
                                QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
                                while(iter!=geometryByProjectCode.end())
                                {
                                    OGRGeometryFactory::destroyGeometry(iter.value());
                                    iter.value()=NULL;
                                    iter++;
                                }
                                QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
                                while(iterRf!=ptrRasterFileByFileName.end())
                                {
                                    if(iterRf.value()!=NULL)
                                    {
                                        delete(iterRf.value());
                                        iterRf.value()=NULL;
                                    }
                                    iterRf++;
                                }
                                free(pData);
                                delete(ptrOutputRasterFile);
                                return(false);
                                /*
                                int rasterNwColumn=floor((nwPixelFc-rasterBoundingBox[0])/rasterGsd);
                                int rasterNwRow=floor((rasterBoundingBox[1]-nwPixelSc)/rasterGsd);
                                double rasterNwValue=0.0;
                                if(rasterNwColumn>0&&rasterNwColumn<rasterColumns
                                        &&rasterNwRow>0&&rasterNwRow<rasterRows)
                                {
                                    rasterNwValue=rasterValues[rasterNwRow][rasterNwColumn];
                                }
                                int rasterNeColumn=floor((nePixelFc-rasterBoundingBox[0])/rasterGsd);
                                int rasterNeRow=floor((rasterBoundingBox[1]-nePixelSc)/rasterGsd);
                                double rasterNeValue=0.0;
                                if(rasterNeColumn>0&&rasterNeColumn<rasterColumns
                                        &&rasterNeRow>0&&rasterNeRow<rasterRows)
                                {
                                    rasterNeValue=rasterValues[rasterNeRow][rasterNeColumn];
                                }
                                int rasterSeColumn=floor((sePixelFc-rasterBoundingBox[0])/rasterGsd);
                                int rasterSeRow=floor((rasterBoundingBox[1]-sePixelSc)/rasterGsd);
                                double rasterSeValue=0.0;
                                if(rasterSeColumn>0&&rasterSeColumn<rasterColumns
                                        &&rasterSeRow>0&&rasterSeRow<rasterRows)
                                {
                                    rasterSeValue=rasterValues[rasterSeRow][rasterSeColumn];
                                }
                                int rasterSwColumn=floor((swPixelFc-rasterBoundingBox[0])/rasterGsd);
                                int rasterSwRow=floor((rasterBoundingBox[1]-swPixelSc)/rasterGsd);
                                double rasterSwValue=0.0;
                                if(rasterSwColumn>0&&rasterSwColumn<rasterColumns
                                        &&rasterSwRow>0&&rasterSwRow<rasterRows)
                                {
                                    rasterSwValue=rasterValues[rasterSwRow][rasterSwColumn];
                                }
                                // Interpolacion bilineal
                                */
                            }
                        }
                        double offset=offsets[njd];
                        double gain=gains[njd];
                        double ndvi=(rasterValue+offsets[njd])*gains[njd];
                        if(ndvi>=mInitialNdvi&&ndvi<=mFinalNdvi)
                        {
                            double kcb=ndvi*mKcbM+mKcbN;
                            if(!mETHOValues.contains(jd))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
                                strError+=QObject::tr("\nError no ETH0 for date %1 for raster file:\n%2")
                                        .arg(QDate::fromJulianDay(jd).toString(TONIPBPPROJECT_DATE_FORMAT_1)).arg(rasterFileName);
                                QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
                                while(iter!=geometryByProjectCode.end())
                                {
                                    OGRGeometryFactory::destroyGeometry(iter.value());
                                    iter.value()=NULL;
                                    iter++;
                                }
                                QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
                                while(iterRf!=ptrRasterFileByFileName.end())
                                {
                                    if(iterRf.value()!=NULL)
                                    {
                                        delete(iterRf.value());
                                        iterRf.value()=NULL;
                                    }
                                    iterRf++;
                                }
                                free(pData);
                                delete(ptrOutputRasterFile);
                                return(false);
                            }
                            double eth0=mETHOValues[jd];
                            accumulatedValue+=(kcb*eth0);
                            if(previousJd>0)
                            {
                                int interpolatedJd=previousJd+1;
                                while(interpolatedJd<jd)
                                {
                                    if(!mETHOValues.contains(interpolatedJd))
                                    {
                                        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
                                        strError+=QObject::tr("\nError no ETH0 for date %1 for raster file:\n%2")
                                                .arg(QDate::fromJulianDay(jd).toString(TONIPBPPROJECT_DATE_FORMAT_1)).arg(rasterFileName);
                                        QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
                                        while(iter!=geometryByProjectCode.end())
                                        {
                                            OGRGeometryFactory::destroyGeometry(iter.value());
                                            iter.value()=NULL;
                                            iter++;
                                        }
                                        QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
                                        while(iterRf!=ptrRasterFileByFileName.end())
                                        {
                                            if(iterRf.value()!=NULL)
                                            {
                                                delete(iterRf.value());
                                                iterRf.value()=NULL;
                                            }
                                            iterRf++;
                                        }
                                        free(pData);
                                        delete(ptrOutputRasterFile);
                                        return(false);
                                    }
                                    double interpolatedKcb=previousKcb+(kcb-previousKcb)/(jd-previousJd)*(interpolatedJd-previousJd);
                                    double interpolatedEth0=mETHOValues[interpolatedJd];
                                    accumulatedValue+=(interpolatedKcb*interpolatedEth0);
                                    interpolatedJd++;
                                }
                            }
                            previousJd=jd;
                            previousKcb=kcb;
                            if(!findInitialNdvi)
                            {
                                findInitialNdvi=true;
                            }
                        }
                        else
                        {
                            if(findInitialNdvi) // se supone que
                            {
                                findFinalNdvid=true;
                                break;
                            }
                        }
                    }
                    double finalRasterValue=noDataValue;
                    if(findInitialNdvi&&findFinalNdvid) // caso normal
                    {
                        finalRasterValue=accumulatedValue;
                    }
                    if(findInitialNdvi&&!findFinalNdvid)
                    {
                        finalRasterValue=accumulatedValue;//??
                    }
                    int posInPData=(row)*roiColumns+column;
                    pData[posInPData]=finalRasterValue;
                }
                iterRow++;
            }
            iterRoiColumnsByRowByTuplekey++;
        }
        int initialColumn=0;
        int initialRow=0;
        int numberOfBand=0;
        if(!ptrOutputRasterFile->writeValues(numberOfBand, // desde 0
                                             initialColumn,
                                             initialRow,
                                             roiColumns,
                                             roiRows,
                                             pData,
                                             strError))
        {
            strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByProject");
            strError+=QObject::tr("\nError writting raster file:\n%1\nfor project: %2\nError:\n%2")
                    .arg(rasterOutputFileName).arg(projectCode).arg(strAuxError);
            QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
            while(iter!=geometryByProjectCode.end())
            {
                OGRGeometryFactory::destroyGeometry(iter.value());
                iter.value()=NULL;
                iter++;
            }
            QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
            while(iterRf!=ptrRasterFileByFileName.end())
            {
                if(iterRf.value()!=NULL)
                {
                    delete(iterRf.value());
                    iterRf.value()=NULL;
                }
                iterRf++;
            }
            free(pData);
            delete(ptrOutputRasterFile);
            return(false);
        }
        free(pData);
        delete(ptrOutputRasterFile);
        QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
        while(iterRf!=ptrRasterFileByFileName.end())
        {
            if(iterRf.value()!=NULL)
            {
                delete(iterRf.value());
                iterRf.value()=NULL;
            }
            iterRf++;
        }
        iter1++;
    }

    reportFile.close();
    QMap<QString, OGRGeometry *>::iterator iter=geometryByProjectCode.begin();
    while(iter!=geometryByProjectCode.end())
    {
        OGRGeometryFactory::destroyGeometry(iter.value());
        iter.value()=NULL;
        iter++;
    }
    return(true);
}

bool TONIpbpProject::processAccumulatedPerspirationByTuplekeyByRoi(QString &strError)
{
    if(!mIsInitialized)
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
        strError+=QObject::tr("\nProject is not initialized");
        return(false);
    }
    QMap<QString,QMap<QString,QMap<int,QString> > > tuplekeyFileNameByRoiCodeByTuplekeyByJd;
    QMap<QString,QMap<QString,QMap<int,double> > > gainByRoiCodeByTuplekeyByJd;
    QMap<QString,QMap<QString,QMap<int,double> > > offsetByRoiCodeByTuplekeyByJd;
    QMap<QString,int> lodByTuplekey;
    int globalMaximumLod;
    QString strAuxError;
    if(!mPtrPersistenceManager->getNdviDataByProject(tuplekeyFileNameByRoiCodeByTuplekeyByJd,
                                                     gainByRoiCodeByTuplekeyByJd,
                                                     offsetByRoiCodeByTuplekeyByJd,
                                                     lodByTuplekey,
                                                     globalMaximumLod,
                                                     strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
        strError+=QObject::tr("\nError recovering ndvi data from database:\n%1").arg(strAuxError);
        return(false);
    }
//    QMap<QString, double> minFirstCoordinateByProjectCode;
//    QMap<QString, double> maxSecondCoordinateByProjectCode;
//    QMap<QString, double> maxFirstCoordinateByProjectCode;
//    QMap<QString, double> minSecondCoordinateByProjectCode;
//    if(!mPtrPersistenceManager->getProjectEnvelopes(minFirstCoordinateByProjectCode,
//                                                    maxSecondCoordinateByProjectCode,
//                                                    maxFirstCoordinateByProjectCode,
//                                                    minSecondCoordinateByProjectCode,
//                                                    strAuxError))
//    {
//        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
//        strError+=QObject::tr("\nError recovering envelopes of projects from database:\n%1").arg(strAuxError);
//        return(false);
//    }
    QMap<QString, OGRGeometry *> geometryByRoiCode;
    if(!mPtrPersistenceManager->getProjectGeometries(geometryByRoiCode,
                                                     strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
        strError+=QObject::tr("\nError recovering geometries of projects from database:\n%1").arg(strAuxError);
        return(false);
    }
    QString crsDescription=mPtrNestedGridTools->getCrsDescription();
    int numberOfPixels=mPtrNestedGridTools->getBaseWorldWidthAndHeightInPixels();
    int rasterBand=0;
    double noDataValue=TONIPBPPROJECT_RASTER_NODATAVALUE;
    QMap<QString,QMap<QString, QMap<int, QVector<int> > > > roiColumnsByRowByTuplekeyByRoiCode;
    QMap<QString, double> roiNwFcByRoiCode;
    QMap<QString, double> roiNwScByRoiCode;
    QMap<QString, int> roiColumnsByRoiCode;
    QMap<QString, int> roiRowsByRoiCode;
    QMap<QString,double> minimunGsdByRoiCode;
    QMap<QString,int> maximunLodByRoiCode;
    QMap<QString,QString> outputRasterFileNameByRoiCode; // si no se inserta es porque no intersecta con la geometria de la ROI
    QMap<QString,IGDAL::Raster*> ptrRasterFileByFileName; // si no se inserta es porque no intersecta con la geometria de la ROI
    QMap<QString,QVector<double> > georefRasterFileByFileName;
    QMap<QString,QMap<QString,QMap<int,QString> > >::const_iterator iter1=tuplekeyFileNameByRoiCodeByTuplekeyByJd.begin();
    while(iter1!=tuplekeyFileNameByRoiCodeByTuplekeyByJd.end())
    {
        QString roiCode=iter1.key();
        OGRGeometry* ptrRoiGeometry=geometryByRoiCode[roiCode];
        QMap<QString,QMap<int,QString> > tuplekeyFileNameByTuplekeyByJd=iter1.value();
        QMap<QString,QMap<int,QString> >::const_iterator iter2=tuplekeyFileNameByTuplekeyByJd.begin();
        // Obtener el gsd de cada fichero que intersecte
        double minGsd=1000000.0;
        int maximumLod=0;
        while(iter2!=tuplekeyFileNameByTuplekeyByJd.end())
        {
            QString tuplekey=iter2.key();
            int lod=lodByTuplekey[tuplekey];
            QMap<int,QString> tuplekeyFileNameByJd=iter2.value();
            QMap<int,QString>::const_iterator iter3=tuplekeyFileNameByJd.begin();
            bool existSomeRasterFile=false;
            while(iter3!=tuplekeyFileNameByJd.end())
            {
                QString rasterFileName=iter3.value();
                IGDAL::Raster* ptrRasterFile=NULL;
                if(!ptrRasterFileByFileName.contains(rasterFileName))
                {
                    ptrRasterFile=new IGDAL::Raster(mPtrCrsTools);
                    if(!ptrRasterFile->setFromFile(rasterFileName,strAuxError))
                    {
                        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                        strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                                .arg(rasterFileName).arg(strAuxError);
                        QMap<QString, OGRGeometry *>::iterator iterGeometryByRoiCode=geometryByRoiCode.begin();
                        while(iterGeometryByRoiCode!=geometryByRoiCode.end())
                        {
                            OGRGeometryFactory::destroyGeometry(iterGeometryByRoiCode.value());
                            iterGeometryByRoiCode.value()=NULL;
                            iterGeometryByRoiCode++;
                        }
                        return(false);
                    }
                }
                else
                {
                    ptrRasterFile=ptrRasterFileByFileName[rasterFileName];
                }
                OGRGeometry *ptrRasterEnvelopeGeometry=ptrRasterFile->getEnvelopeGeometry();
                if(!ptrRoiGeometry->Intersects(ptrRasterEnvelopeGeometry))
                {
                    iter3++;
                    continue;
                }
                existSomeRasterFile=true;
                if(!ptrRasterFileByFileName.contains(rasterFileName))
                {
                    QVector<double> georef;
                    if(!ptrRasterFile->getGeoRef(georef,
                                                 strAuxError))
                    {
                        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                        strError+=QObject::tr("\nError getting georef from raster file:\n%1\nError:\n%2")
                                .arg(rasterFileName).arg(strAuxError);
                        QMap<QString, OGRGeometry *>::iterator iterGeometryByRoiCode=geometryByRoiCode.begin();
                        while(iterGeometryByRoiCode!=geometryByRoiCode.end())
                        {
                            OGRGeometryFactory::destroyGeometry(iterGeometryByRoiCode.value());
                            iterGeometryByRoiCode.value()=NULL;
                            iterGeometryByRoiCode++;
                        }
                        QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
                        while(iterRf!=ptrRasterFileByFileName.end())
                        {
                            if(iterRf.value()!=NULL)
                            {
                                delete(iterRf.value());
                                iterRf.value()=NULL;
                            }
                            iterRf++;
                        }
                        return(false);
                    }
                    ptrRasterFileByFileName[rasterFileName]=ptrRasterFile;
                    georefRasterFileByFileName[rasterFileName]=georef;
                    double rasterGsd=georef[1];
                    if(rasterGsd<minGsd)
                    {
                        minGsd=rasterGsd;
                    }
                }
                iter3++;
            }
            if(existSomeRasterFile)
            {
                if(lod>maximumLod)
                {
                    maximumLod=lod;
                }
            }
            iter2++;
        }
        minimunGsdByRoiCode[roiCode]=minGsd;
        maximunLodByRoiCode[roiCode]=maximumLod;
        double roiNwFc,roiNwSc;
        QMap<QString, QMap<int, QVector<int> > > roiColumnsByRowByTuplekey;
        int roiColumns,roiRows;
        if(!mPtrNestedGridTools->getRoiPixelsFromPolygon(ptrRoiGeometry,
                                                         crsDescription,
                                                         maximumLod,
                                                         minGsd,
                                                         roiNwFc,
                                                         roiNwSc,
                                                         roiColumnsByRowByTuplekey,
                                                         roiColumns,
                                                         roiRows,
                                                         strAuxError))
        {
            strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
            strError+=QObject::tr("\nError getting pixels for project: %1\nError:\n%2")
                    .arg(roiCode).arg(strAuxError);
            QMap<QString, OGRGeometry *>::iterator iterGeometryByRoiCode=geometryByRoiCode.begin();
            while(iterGeometryByRoiCode!=geometryByRoiCode.end())
            {
                OGRGeometryFactory::destroyGeometry(iterGeometryByRoiCode.value());
                iterGeometryByRoiCode.value()=NULL;
                iterGeometryByRoiCode++;
            }
            QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
            while(iterRf!=ptrRasterFileByFileName.end())
            {
                if(iterRf.value()!=NULL)
                {
                    delete(iterRf.value());
                    iterRf.value()=NULL;
                }
                iterRf++;
            }
            return(false);
        }
        roiNwFcByRoiCode[roiCode]=roiNwFc;
        roiNwScByRoiCode[roiCode]=roiNwSc;
        roiColumnsByRoiCode[roiCode]=roiColumns;
        roiRowsByRoiCode[roiCode]=roiRows;
        // Se crea el raster
        int roiNumberOfPixels=roiRows*roiColumns;
        float* pData=(float*)malloc(roiNumberOfPixels*sizeof(GDT_Float32));
        QVector<double> roiGeoref;
        double roiSeFc=roiNwFc+minGsd*roiColumns;
        double roiSeSc=roiNwSc-minGsd*roiRows;
        bool closeAfterCreate=false;
        IGDAL::ImageTypes roiImageType=IGDAL::TIFF;
        GDALDataType roiGdalDataType=GDT_Float32;
        QString rasterOutputFileName=mResultsPath+"/"+roiCode+"."+RASTER_TIFF_FILE_EXTENSION;
        int numberOfBands=1;
        bool internalGeoRef=true;
        bool externalGeoRef=false;
        IGDAL::Raster* ptrOutputRasterFile=new IGDAL::Raster(mPtrCrsTools);
        QMap<QString, QString> refImageOptions; // por implementar
        if(!ptrOutputRasterFile->createRaster(rasterOutputFileName, // Se le añade la extension
                                              roiImageType,roiGdalDataType,
                                              numberOfBands,
                                              roiColumns,roiRows,
                                              internalGeoRef,externalGeoRef,crsDescription,
                                              roiNwFc,roiNwSc,roiSeFc,roiSeSc,
                                              roiGeoref, // vacío si se georeferencia con las esquinas
                                              noDataValue,
                                              closeAfterCreate,
                                              refImageOptions,
                                              //                                               buildOverviews,
                                              strAuxError))
        {
            strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
            strError+=QObject::tr("\nError creating raster file:\n%1\nfor project: %2\nError:\n%2")
                    .arg(rasterOutputFileName).arg(roiCode).arg(strAuxError);
            QMap<QString, OGRGeometry *>::iterator iterGeometryByRoiCode=geometryByRoiCode.begin();
            while(iterGeometryByRoiCode!=geometryByRoiCode.end())
            {
                OGRGeometryFactory::destroyGeometry(iterGeometryByRoiCode.value());
                iterGeometryByRoiCode.value()=NULL;
                iterGeometryByRoiCode++;
            }
            QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
            while(iterRf!=ptrRasterFileByFileName.end())
            {
                if(iterRf.value()!=NULL)
                {
                    delete(iterRf.value());
                    iterRf.value()=NULL;
                }
                iterRf++;
            }
            free(pData);
            return(false);
        }
        for(int row=0;row<roiRows;row++)
        {
            for(int column=0;column<roiColumns;column++)
            {
                pData[row*roiColumns+column]=noDataValue;
            }
        }
        int initialColumn=0;
        int initialRow=0;
        int numberOfBand=0;
        if(!ptrOutputRasterFile->writeValues(numberOfBand, // desde 0
                                             initialColumn,
                                             initialRow,
                                             roiColumns,
                                             roiRows,
                                             pData,
                                             strError))
        {
            strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
            strError+=QObject::tr("\nError writting raster file:\n%1\nfor project: %2\nError:\n%2")
                    .arg(rasterOutputFileName).arg(roiCode).arg(strAuxError);
            QMap<QString, OGRGeometry *>::iterator iterGeometryByRoiCode=geometryByRoiCode.begin();
            while(iterGeometryByRoiCode!=geometryByRoiCode.end())
            {
                OGRGeometryFactory::destroyGeometry(iterGeometryByRoiCode.value());
                iterGeometryByRoiCode.value()=NULL;
                iterGeometryByRoiCode++;
            }
            QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
            while(iterRf!=ptrRasterFileByFileName.end())
            {
                if(iterRf.value()!=NULL)
                {
                    delete(iterRf.value());
                    iterRf.value()=NULL;
                }
                iterRf++;
            }
            free(pData);
            delete(ptrOutputRasterFile);
            return(false);
        }
        free(pData);
        delete(ptrOutputRasterFile);
        outputRasterFileNameByRoiCode[roiCode]=rasterOutputFileName;
        QMap<QString, QMap<int, QVector<int> > >::const_iterator iterRoiColumnsByRowByTuplekey=roiColumnsByRowByTuplekey.begin();
        while(iterRoiColumnsByRowByTuplekey!=roiColumnsByRowByTuplekey.end())
        {
            QString tuplekey=iterRoiColumnsByRowByTuplekey.key();
            QMap<int, QVector<int> > roiColumnsByRow=roiColumnsByRowByTuplekey[tuplekey];
            roiColumnsByRowByTuplekeyByRoiCode[tuplekey][roiCode]=roiColumnsByRow;
            iterRoiColumnsByRowByTuplekey++;
        }
        iter1++;
    }
    QMap<QString,IGDAL::Raster*>::iterator iterRf=ptrRasterFileByFileName.begin();
    while(iterRf!=ptrRasterFileByFileName.end())
    {
        if(iterRf.value()!=NULL)
        {
            delete(iterRf.value());
            iterRf.value()=NULL;
        }
        iterRf++;
    }
    QMap<QString, OGRGeometry *>::iterator iterGeometryByRoiCode=geometryByRoiCode.begin();
    while(iterGeometryByRoiCode!=geometryByRoiCode.end())
    {
        OGRGeometryFactory::destroyGeometry(iterGeometryByRoiCode.value());
        iterGeometryByRoiCode.value()=NULL;
        iterGeometryByRoiCode++;
    }
    QFile reportFile(mReportFileName);
    if (!reportFile.open(QFile::WriteOnly |QFile::Text))
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(mReportFileName);
        return(false);
    }
    QTextStream out(&reportFile);
    out<<"RESULTADOS DE PROCESAMIENTO\n";
    out<<"---------------------------\n";
    QMap<QString,QMap<QString, QMap<int, QVector<int> > > >::const_iterator iterRoiColumnsByRowByTuplekeyByRoiCode=roiColumnsByRowByTuplekeyByRoiCode.begin();
    while(iterRoiColumnsByRowByTuplekeyByRoiCode!=roiColumnsByRowByTuplekeyByRoiCode.end())
    {
        QString tuplekey=iterRoiColumnsByRowByTuplekeyByRoiCode.key();
        out<<"- Tuplekey a procesar ...............: "<<tuplekey<<"\n";
//        QMap<QString,IGDAL::Raster*> ptrTuplekeyRasterFileByFileName; // si no se inserta es porque no intersecta con la geometria de la ROI
        QMap<QString,QVector<QVector<double> > > rasterValuesByFileName;
        QMap<QString,QVector<double> > rasterBoundingBoxByFileName; // nwFc,nwSc,seFc,seSc -> puede no coincidir con georef porque en algunos casos georef va al centro del pixel
        QMap<QString, QMap<int, QVector<int> > >::const_iterator iterRoiColumnsByRowByByRoiCode=iterRoiColumnsByRowByTuplekeyByRoiCode.value().begin();
        while(iterRoiColumnsByRowByByRoiCode!=iterRoiColumnsByRowByTuplekeyByRoiCode.value().end())
        {
            QString roiCode=iterRoiColumnsByRowByByRoiCode.key();
            QMap<int,QString> tuplekeyFileNameByJd=tuplekeyFileNameByRoiCodeByTuplekeyByJd[roiCode][tuplekey];
            QVector<int> jds;
            QVector<QString> fileNames;
            QVector<double> gains;
            QVector<double> offsets;
            QMap<int,QString>::const_iterator iterTuplekeyFileNameByJd=tuplekeyFileNameByJd.begin();
            while(iterTuplekeyFileNameByJd!=tuplekeyFileNameByJd.end())
            {
                int jd=iterTuplekeyFileNameByJd.key();
                QString fileName=iterTuplekeyFileNameByJd.value();
                jds.push_back(jd);
                fileNames.push_back(fileName);
                double gain=gainByRoiCodeByTuplekeyByJd[roiCode][tuplekey][jd];
                double offset=offsetByRoiCodeByTuplekeyByJd[roiCode][tuplekey][jd];
                gains.push_back(gain);
                offsets.push_back(offset);
                iterTuplekeyFileNameByJd++;
            }
            double roiNwFc=roiNwFcByRoiCode[roiCode];
            double roiNwSc=roiNwScByRoiCode[roiCode];
            int roiColumns=roiColumnsByRoiCode[roiCode];
            int roiRows=roiRowsByRoiCode[roiCode];
            int roiNumberOfPixels=roiRows*roiColumns;
            QString roiOutputRasterFileName=outputRasterFileNameByRoiCode[roiCode];
            IGDAL::Raster* ptrRoiRaster=new IGDAL::Raster(mPtrCrsTools);
            bool updateFile=true;
            if(!ptrRoiRaster->setFromFile(roiOutputRasterFileName,strAuxError,updateFile))
            {
                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                        .arg(roiOutputRasterFileName).arg(strAuxError);
                return(false);
            }
            float* pData=(float*)malloc(roiNumberOfPixels*sizeof(GDT_Float32));
            { // para que se borre roiRasterValues
                QVector<QVector<double> > roiRasterValues;
                if(!ptrRoiRaster->readValues(rasterBand,
                                          0,0,
                                          roiColumns,roiRows,
                                          roiRasterValues,
                                          strAuxError)) // values[row][column]
                {
                    strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                    strError+=QObject::tr("\nError reading values from raster file:\n%1\nError:\n%2")
                            .arg(roiOutputRasterFileName).arg(strAuxError);
                    free(pData);
                    delete(ptrRoiRaster);
                    return(false);
                }
                for(int row=0;row<roiRows;row++)
                {
                    for(int column=0;column<roiColumns;column++)
                    {
                        pData[row*roiColumns+column]=roiRasterValues[row][column];
                    }
                }
            }
            double minGsd=minimunGsdByRoiCode[roiCode];
            int maximumLod=maximunLodByRoiCode[roiCode];
            QMap<int, QVector<int> > roiColumnsByRow=iterRoiColumnsByRowByByRoiCode.value();
            QMap<int, QVector<int> >::const_iterator iterRow=roiColumnsByRow.begin();
            while(iterRow!=roiColumnsByRow.end())
            {
                int row=iterRow.key();
                for(int nc=0;nc<iterRow.value().size();nc++)
                {
                    int column=iterRow.value()[nc];
                    double centerPixelFc=roiNwFc+column*minGsd+0.5*minGsd;
                    double centerPixelSc=roiNwSc-row*minGsd-0.5*minGsd;
                    double nwPixelFc=roiNwFc+column*minGsd;
                    double nwPixelSc=roiNwSc-row*minGsd;
                    double nePixelFc=nwPixelFc+minGsd;
                    double nePixelSc=nwPixelSc;
                    double sePixelFc=nwPixelFc+minGsd;
                    double sePixelSc=nwPixelSc-minGsd;
                    double swPixelFc=nwPixelFc;
                    double swPixelSc=nwPixelSc-minGsd;
                    bool findInitialNdvi=false;
                    bool findFinalNdvid=false;
                    int previousJd=0;
                    double previousKcb=0.0;
                    double accumulatedValue=0.0;
                    for(int njd=0;njd<jds.size();njd++)
                    {
                        int jd=jds[njd];
                        if(jd<mInitialJd||jd>mFinalJd)
                        {
                            continue;
                        }
                        QString rasterFileName=fileNames[njd];
                        QVector<double> rasterGeoref=georefRasterFileByFileName[rasterFileName];
                        QVector<QVector<double> > rasterValues;
                        QVector<double> rasterBoundingBox;
                        int rasterColumns,rasterRows;
                        if(!rasterValuesByFileName.contains(rasterFileName))
                        {
                            IGDAL::Raster* ptrRaster=new IGDAL::Raster(mPtrCrsTools);
                            if(!ptrRaster->setFromFile(rasterFileName,strAuxError))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                                strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                delete(ptrRoiRaster);
                                free(pData);
                                return(false);
                            }
                            if(!ptrRaster->getSize(rasterColumns,rasterRows,strAuxError))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                                strError+=QObject::tr("\nError getting dimensions from raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                delete(ptrRaster);
                                delete(ptrRoiRaster);
                                free(pData);
                                return(false);
                            }
                            double rasterNwFc,rasterNwSc,rasterSeFc,rasterSeSc;
                            if(!ptrRaster->getBoundingBox(rasterNwFc,rasterNwSc,rasterSeFc,rasterSeSc,strAuxError))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                                strError+=QObject::tr("\nError getting bounding box from raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                delete(ptrRaster);
                                delete(ptrRoiRaster);
                                free(pData);
                                return(false);
                            }
                            rasterBoundingBox.push_back(rasterNwFc);
                            rasterBoundingBox.push_back(rasterNwSc);
                            rasterBoundingBox.push_back(rasterSeFc);
                            rasterBoundingBox.push_back(rasterSeSc);
                            rasterBoundingBoxByFileName[rasterFileName]=rasterBoundingBox;
                            if(!ptrRaster->readValues(rasterBand,
                                                      0,0,
                                                      rasterColumns,rasterRows,
                                                      rasterValues,
                                                      strAuxError)) // values[row][column]
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                                strError+=QObject::tr("\nError getting dimensions from raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                delete(ptrRaster);
                                delete(ptrRoiRaster);
                                free(pData);
                                return(false);
                            }
                            delete(ptrRaster);
                            rasterValuesByFileName[rasterFileName]=rasterValues;
                        }
                        else
                        {
                            rasterValues=rasterValuesByFileName[rasterFileName];
                            rasterBoundingBox=rasterBoundingBoxByFileName[rasterFileName];
                            rasterRows=rasterValues.size();
                            rasterColumns=rasterValues[0].size();
                        }
                        double rasterGsd=rasterGeoref[1];
                        double rasterValue=0.0;
                        if(fabs(rasterGsd-minGsd)<0.001)
                        {
                            int rasterColumn=floor((centerPixelFc-rasterBoundingBox[0])/rasterGsd);
                            int rasterRow=floor((rasterBoundingBox[1]-centerPixelSc)/rasterGsd);
                            rasterValue=rasterValues[rasterRow][rasterColumn];
//                            rasterValue=rasterValues[row][column];
                        }
                        else
                        {
                            if(minGsd<=rasterGsd) // en este caso siempre se cumple
                            {
                                int rasterColumn=floor((centerPixelFc-rasterBoundingBox[0])/rasterGsd);
                                int rasterRow=floor((rasterBoundingBox[1]-centerPixelSc)/rasterGsd);
                                rasterValue=rasterValues[rasterRow][rasterColumn];
                            }
                            else
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                                strError+=QObject::tr("\nError invalid GSD in [%1,%2] for raster file:\n%3")
                                        .arg(QString::number(column)).arg(QString::number(row)).arg(rasterFileName);
                                delete(ptrRoiRaster);
                                free(pData);
                                return(false);
                            }
                        }
                        double offset=offsets[njd];
                        double gain=gains[njd];
                        double ndvi=(rasterValue+offsets[njd])*gains[njd];
                        if(ndvi>=mInitialNdvi&&ndvi<=mFinalNdvi)
                        {
                            double kcb=ndvi*mKcbM+mKcbN;
                            if(!mETHOValues.contains(jd))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                                strError+=QObject::tr("\nError no ETH0 for date %1 for raster file:\n%2")
                                        .arg(QDate::fromJulianDay(jd).toString(TONIPBPPROJECT_DATE_FORMAT_1)).arg(rasterFileName);
                                delete(ptrRoiRaster);
                                free(pData);
                                return(false);
                            }
                            double eth0=mETHOValues[jd];
                            accumulatedValue+=(kcb*eth0);
                            if(previousJd>0)
                            {
                                int interpolatedJd=previousJd+1;
                                while(interpolatedJd<jd)
                                {
                                    if(!mETHOValues.contains(interpolatedJd))
                                    {
                                        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                                        strError+=QObject::tr("\nError no ETH0 for date %1 for raster file:\n%2")
                                                .arg(QDate::fromJulianDay(jd).toString(TONIPBPPROJECT_DATE_FORMAT_1)).arg(rasterFileName);
                                        delete(ptrRoiRaster);
                                        free(pData);
                                        return(false);
                                    }
                                    double interpolatedKcb=previousKcb+(kcb-previousKcb)/(jd-previousJd)*(interpolatedJd-previousJd);
                                    double interpolatedEth0=mETHOValues[interpolatedJd];
                                    accumulatedValue+=(interpolatedKcb*interpolatedEth0);
                                    interpolatedJd++;
                                }
                            }
                            previousJd=jd;
                            previousKcb=kcb;
                            if(!findInitialNdvi)
                            {
                                findInitialNdvi=true;
                            }
                        }
                        else
                        {
                            if(findInitialNdvi) // se supone que
                            {
                                findFinalNdvid=true;
                                break;
                            }
                        }
                    }
                    double finalRasterValue=noDataValue;
                    if(findInitialNdvi&&findFinalNdvid) // caso normal
                    {
                        finalRasterValue=accumulatedValue;
                    }
                    if(findInitialNdvi&&!findFinalNdvid)
                    {
                        finalRasterValue=accumulatedValue;//??
                    }
                    int posInPData=(row)*roiColumns+column;
                    pData[posInPData]=finalRasterValue;
                }
                iterRow++;
            }
            int initialColumn=0;
            int initialRow=0;
            if(!ptrRoiRaster->writeValues(rasterBand, // desde 0
                                          initialColumn,
                                          initialRow,
                                          roiColumns,
                                          roiRows,
                                          pData,
                                          strError))
            {
                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
                strError+=QObject::tr("\nError writting values to raster file:\n%1\nError:\n%2")
                        .arg(roiOutputRasterFileName).arg(strAuxError);
                return(false);
            }
            free(pData);
            delete(ptrRoiRaster);
            out<<"  - ROI a procesar ..................: "<<roiCode<<"\n";
            iterRoiColumnsByRowByByRoiCode++;
        }
        iterRoiColumnsByRowByTuplekeyByRoiCode++;
    }
    reportFile.close();
    return(true);
}

bool TONIpbpProject::processAccumulatedPerspirationByTuplekey(QString &strError)
{
    if(!mIsInitialized)
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspiration");
        strError+=QObject::tr("\nProject is not initialized");
        return(false);
    }
    QString crsDescription=mPtrNestedGridTools->getCrsDescription();
    int numberOfPixels=mPtrNestedGridTools->getBaseWorldWidthAndHeightInPixels();
    QMap<QString,QMap<QString,QMap<int,QString> > > tuplekeyFileNameByTuplekeyByRoiCodeByJd;
    QMap<QString, int> roiIdByRoiCode;
    QMap<QString,int> tuplekeyIdByTuplekeyCode;
    QMap<QString,double> gainByTuplekeyFileName;
    QMap<QString,double> offsetByTuplekeyFileName;
    QMap<QString,int> lodTilesByTuplekeyFileName;
    QMap<QString,int> lodGsdByTuplekeyFileName;
    int maximumLod;
    QString strAuxError;
    if(mFromConsole)
    {
        QString msg=QObject::tr("    ... Recovering data from database");
        (*mStdOut) <<msg<< endl;
    }
    if(!mPtrPersistenceManager->getNdviDataByTuplekeyByRoiCode(tuplekeyFileNameByTuplekeyByRoiCodeByJd,
                                                               roiIdByRoiCode,
                                                               tuplekeyIdByTuplekeyCode,
                                                               gainByTuplekeyFileName,
                                                               offsetByTuplekeyFileName,
                                                               lodTilesByTuplekeyFileName,
                                                               lodGsdByTuplekeyFileName,
                                                               maximumLod,
                                                               strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
        strError+=QObject::tr("\nError recovering ndvi data from database:\n%1").arg(strAuxError);
        return(false);
    }
    QVector<QString> tuplekeys;
    QMap<QString,QMap<QString,QMap<int,QString> > >::const_iterator iterTuplekeys=tuplekeyFileNameByTuplekeyByRoiCodeByJd.begin();
    while(iterTuplekeys!=tuplekeyFileNameByTuplekeyByRoiCodeByJd.end())
    {
        tuplekeys.push_back(iterTuplekeys.key());
        iterTuplekeys++;
    }
    QMap<QString,double> minimunGsdByRoiCode;
    QMap<QString,QVector<double> > rasterBoundingBoxByFileName; // nwFc,nwSc,seFc,seSc -> puede no coincidir con georef porque en algunos casos georef va al centro del pixel
    QMap<QString,double> rasterGsdByFileName;
    QMap<QString,int> rasterRowsByFileName;
    QMap<QString,int> rasterColumnsByFileName;
    QMap<QString,QMap<QString,QMap<int,QString> > >::const_iterator iterTuplekeyFileNameByTuplekeyByRoiCodeByJd=tuplekeyFileNameByTuplekeyByRoiCodeByJd.begin();
    if(mFromConsole)
    {
        QString msg=QObject::tr("    ... Computing tuplekeys previous data");
        (*mStdOut) <<msg<< endl;
    }
    while(iterTuplekeyFileNameByTuplekeyByRoiCodeByJd!=tuplekeyFileNameByTuplekeyByRoiCodeByJd.end())
    {
        QString tuplekey=iterTuplekeyFileNameByTuplekeyByRoiCodeByJd.key();
        if(mFromConsole)
        {
            QString msg=QObject::tr("        ... Processing tuplekey: %1").arg(tuplekey);
            (*mStdOut) <<msg<< endl;
        }
        QMap<QString,QMap<int,QString> > tuplekeyFileNameByRoiCodeByJd=iterTuplekeyFileNameByTuplekeyByRoiCodeByJd.value();
        QMap<QString,QMap<int,QString> >::const_iterator iterTuplekeyFileNameByRoiCodeByJd=tuplekeyFileNameByRoiCodeByJd.begin();
        while(iterTuplekeyFileNameByRoiCodeByJd!=tuplekeyFileNameByRoiCodeByJd.end())
        {
            QString roiCode=iterTuplekeyFileNameByRoiCodeByJd.key();
            if(mFromConsole)
            {
                QString msg=QObject::tr("            ... Processing ROI: %1").arg(roiCode);
                (*mStdOut) <<msg<< endl;
            }
            QMap<int,QString> tuplekeyFileNameByJd=iterTuplekeyFileNameByRoiCodeByJd.value();
            QMap<int,QString>::const_iterator iterTuplekeyFileNameByJd=tuplekeyFileNameByJd.begin();
            while(iterTuplekeyFileNameByJd!=tuplekeyFileNameByJd.end())
            {
                QString rasterFileName=iterTuplekeyFileNameByJd.value();
                int lodTiles=lodTilesByTuplekeyFileName[rasterFileName];
                int lodGsd=lodGsdByTuplekeyFileName[rasterFileName];
                if(!rasterGsdByFileName.contains(rasterFileName))
                {
                    double rasterGsd;
                    if(!mPtrNestedGridTools->getLodGsd(lodGsd,rasterGsd,strAuxError))
                    {
                        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                        strError+=QObject::tr("\nError getting lod gds for raster file:\n%1\nError:\n%2")
                                .arg(rasterFileName).arg(strAuxError);
                        return(false);
                    }
                    rasterGsdByFileName[rasterFileName]=rasterGsd;
                    if(!minimunGsdByRoiCode.contains(roiCode))
                    {
                        minimunGsdByRoiCode[roiCode]=rasterGsd;
                    }
                    else
                    {
                        if(rasterGsd<minimunGsdByRoiCode[roiCode])
                        {
                            minimunGsdByRoiCode[roiCode]=rasterGsd;
                        }
                    }
                    int tileX,tileY,lodAux;
                    if(!mPtrNestedGridTools->conversionTuplekeyToTileCoordinates(tuplekey,lodAux,tileX,tileY,strAuxError))
                    {
                        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                        strError+=QObject::tr("\nError getting tileX and tileY from tuplekey for raster file:\n%1\nError:\n%2")
                                .arg(rasterFileName).arg(strAuxError);
                        return(false);
                    }
                    double rasterNwFc,rasterNwSc,rasterSeFc,rasterSeSc;
                    if(!mPtrNestedGridTools->getBoundingBoxFromTile(lodTiles,tileX,tileY,crsDescription,
                                                                    rasterNwFc,rasterNwSc,rasterSeFc,rasterSeSc,
                                                                    strAuxError))
                    {
                        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                        strError+=QObject::tr("\nError getting bounding box from tuplekey for raster file:\n%1\nError:\n%2")
                                .arg(rasterFileName).arg(strAuxError);
                        return(false);
                    }
                    QVector<double> rasterBoundingBox;
                    rasterBoundingBox.push_back(rasterNwFc);
                    rasterBoundingBox.push_back(rasterNwSc);
                    rasterBoundingBox.push_back(rasterSeFc);
                    rasterBoundingBox.push_back(rasterSeSc);
                    rasterBoundingBoxByFileName[rasterFileName]=rasterBoundingBox;
                    int rasterColumns,rasterRows;
                    rasterColumns=qRound((rasterSeFc-rasterNwFc)/rasterGsdByFileName[rasterFileName]);
                    rasterRows=qRound((rasterNwSc-rasterSeSc)/rasterGsdByFileName[rasterFileName]);
                    rasterRowsByFileName[rasterFileName]=rasterRows;
                    rasterColumnsByFileName[rasterFileName]=rasterColumns;
                }
                iterTuplekeyFileNameByJd++;
            }
            iterTuplekeyFileNameByRoiCodeByJd++;
        }
        iterTuplekeyFileNameByTuplekeyByRoiCodeByJd++;
    }

    int rasterBand=0;
    double noDataValue=TONIPBPPROJECT_RASTER_NODATAVALUE;
    QFile reportFile(mReportFileName);
    if (!reportFile.open(QFile::WriteOnly |QFile::Text))
    {
        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(mReportFileName);
        return(false);
    }
    QTextStream out(&reportFile);
    out<<"RESULTADOS DE PROCESAMIENTO\n";
    out<<"---------------------------\n";
    if(mFromConsole)
    {
        QString msg=QObject::tr("    ... Processing tuplekeys");
        (*mStdOut) <<msg<< endl;
    }
    for(int nt=0;nt<tuplekeys.size();nt++)
    {
        QString tuplekey=tuplekeys[nt];
        if(mFromConsole)
        {
            QString msg=QObject::tr("        ... Processing tuplekey: %1").arg(tuplekey);
            (*mStdOut) <<msg<< endl;
        }
        int tuplekeyId=tuplekeyIdByTuplekeyCode[tuplekey];
        out<<"- Tuplekey a procesar ...............: "<<tuplekey<<"\n";
        QMap<QString,QVector<QVector<double> > > rasterValuesByFileName;
        QMap<QString,QMap<int,QString> > tuplekeyFileNameByRoiCodeByJd=tuplekeyFileNameByTuplekeyByRoiCodeByJd[tuplekey];
        QMap<QString,QMap<int,QString> >::const_iterator iterRoi=tuplekeyFileNameByRoiCodeByJd.begin();
        while(iterRoi!=tuplekeyFileNameByRoiCodeByJd.end())
        {
            QString roiCode=iterRoi.key();
            if(mFromConsole)
            {
                QString msg=QObject::tr("            ... Processing ROI: %1").arg(roiCode);
                (*mStdOut) <<msg<< endl;
            }
            int roiId=roiIdByRoiCode[roiCode];
            double minGsd=minimunGsdByRoiCode[roiCode];
            OGRGeometry *ptrRoiTuplekeyIntersectionGeometry;
            if(!mPtrPersistenceManager->getProjectTuplekeyIntersectionGeomety(roiId,
                                                                              tuplekeyId,
                                                                              ptrRoiTuplekeyIntersectionGeometry,
                                                                              strAuxError))
            {
                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                strError+=QObject::tr("\nError recovering geometries of projects from database:\n%1").arg(strAuxError);
                return(false);
            }
            double roiNwFc,roiNwSc, roiSeFc,roiSeSc;
            QMap<int, QVector<int> > roiColumnsByRow;
            int roiColumns,roiRows;
            bool existsRoiGeoref=false;
            if(ptrRoiTuplekeyIntersectionGeometry!=NULL) // si es NULL es porque el ROI contiene al tuplekey
            {
                if(!mPtrNestedGridTools->getRoiPixelsFromPolygonInTuplekey(ptrRoiTuplekeyIntersectionGeometry,
                                                                           crsDescription,
                                                                           tuplekey,
                                                                           maximumLod,
                                                                           minGsd,
                                                                           roiNwFc,
                                                                           roiNwSc,
                                                                           roiColumnsByRow,
                                                                           roiColumns,
                                                                           roiRows,
                                                                           strAuxError))
                {
                    strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                    strError+=QObject::tr("\nError recovering roi pixels for project: %1 and tuplekye: %2\nError:\n%3")
                            .arg(roiCode).arg(tuplekey).arg(strAuxError);
                    return(false);
                }
                roiSeFc=roiNwFc+roiColumns*minGsd;
                roiSeSc=roiNwSc-roiRows*minGsd;
                existsRoiGeoref=true;
            }
            if(ptrRoiTuplekeyIntersectionGeometry!=NULL)
            {
                OGRGeometryFactory::destroyGeometry(ptrRoiTuplekeyIntersectionGeometry);
                ptrRoiTuplekeyIntersectionGeometry=NULL;
            }
            QMap<int,QString> tuplekeyFileNameByJd=iterRoi.value();
            QVector<int> jds;
            QVector<QString> fileNames;
            QVector<double> gains;
            QVector<double> offsets;
            QMap<int,QString>::const_iterator iterTuplekeyFileNameByJd=tuplekeyFileNameByJd.begin();
            while(iterTuplekeyFileNameByJd!=tuplekeyFileNameByJd.end())
            {
                int jd=iterTuplekeyFileNameByJd.key();
                QString fileName=iterTuplekeyFileNameByJd.value();
                jds.push_back(jd);
                fileNames.push_back(fileName);
                double gain=gainByTuplekeyFileName[fileName];
                double offset=offsetByTuplekeyFileName[fileName];
                gains.push_back(gain);
                offsets.push_back(offset);
                if(!existsRoiGeoref)
                {
                    roiNwFc=rasterBoundingBoxByFileName[fileName][0];
                    roiNwSc=rasterBoundingBoxByFileName[fileName][1];
                    roiSeFc=rasterBoundingBoxByFileName[fileName][2];
                    roiSeSc=rasterBoundingBoxByFileName[fileName][3];
                    roiColumns=qRound((roiSeFc-roiNwFc)/minGsd);
                    roiRows=qRound((roiNwSc-roiSeSc)/minGsd);
                    for(int contRow=0;contRow<roiRows;contRow++)
                    {
                        QVector<int> auxColumns(roiColumns);
                        for(int contColumn=0;contColumn<roiColumns;contColumn++)
                        {
                            auxColumns[contColumn]=contColumn;
                        }
                        roiColumnsByRow[contRow]=auxColumns;
                    }
                    existsRoiGeoref=true;
                }
                iterTuplekeyFileNameByJd++;
            }
            int roiNumberOfPixels=roiRows*roiColumns;
            float* pData=(float*)malloc(roiNumberOfPixels*sizeof(GDT_Float32));
            for(int row=0;row<roiRows;row++)
            {
                for(int column=0;column<roiColumns;column++)
                {
                    pData[row*roiColumns+column]=noDataValue;
                }
            }
            QMap<int, QVector<int> >::const_iterator iterRow=roiColumnsByRow.begin();
            while(iterRow!=roiColumnsByRow.end())
            {
                int row=iterRow.key();
                for(int nc=0;nc<iterRow.value().size();nc++)
                {
                    int column=iterRow.value()[nc];
                    double centerPixelFc=roiNwFc+column*minGsd+0.5*minGsd;
                    double centerPixelSc=roiNwSc-row*minGsd-0.5*minGsd;
                    double nwPixelFc=roiNwFc+column*minGsd;
                    double nwPixelSc=roiNwSc-row*minGsd;
                    double nePixelFc=nwPixelFc+minGsd;
                    double nePixelSc=nwPixelSc;
                    double sePixelFc=nwPixelFc+minGsd;
                    double sePixelSc=nwPixelSc-minGsd;
                    double swPixelFc=nwPixelFc;
                    double swPixelSc=nwPixelSc-minGsd;
                    bool findInitialNdvi=false;
                    bool findFinalNdvid=false;
                    int previousJd=0;
                    double previousKcb=0.0;
                    double accumulatedValue=0.0;
                    for(int njd=0;njd<jds.size();njd++)
                    {
                        int jd=jds[njd];
                        if(jd<mInitialJd||jd>mFinalJd)
                        {
                            continue;
                        }
                        QString rasterFileName=fileNames[njd];
                        QVector<QVector<double> > rasterValues;
                        double rasterNwFc=rasterBoundingBoxByFileName[rasterFileName][0];
                        double rasterNwSc=rasterBoundingBoxByFileName[rasterFileName][1];
                        int rasterColumns=rasterColumnsByFileName[rasterFileName];
                        int rasterRows=rasterRowsByFileName[rasterFileName];
                        if(!rasterValuesByFileName.contains(rasterFileName))
                        {
                            IGDAL::Raster* ptrRaster=new IGDAL::Raster(mPtrCrsTools);
                            if(!ptrRaster->setFromFile(rasterFileName,strAuxError))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                                strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                free(pData);
                                return(false);
                            }
                            if(!ptrRaster->readValues(rasterBand,
                                                      0,0,
                                                      rasterColumns,rasterRows,
                                                      rasterValues,
                                                      strAuxError)) // values[row][column]
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                                strError+=QObject::tr("\nError getting dimensions from raster file:\n%1\nError:\n%2")
                                        .arg(rasterFileName).arg(strAuxError);
                                delete(ptrRaster);
                                free(pData);
                                return(false);
                            }
                            delete(ptrRaster);
                            rasterValuesByFileName[rasterFileName]=rasterValues;
                        }
                        else
                        {
                            rasterValues=rasterValuesByFileName[rasterFileName];
//                            rasterBoundingBox=rasterBoundingBoxByFileName[rasterFileName];
//                            rasterRows=rasterValues.size();
//                            rasterColumns=rasterValues[0].size();
                        }
                        double rasterGsd=rasterGsdByFileName[rasterFileName];
                        double rasterValue=0.0;
                        if(fabs(rasterGsd-minGsd)<0.001)
                        {
                            int rasterColumn=floor((centerPixelFc-rasterNwFc)/rasterGsd);
                            int rasterRow=floor((rasterNwSc-centerPixelSc)/rasterGsd);
                            rasterValue=rasterValues[rasterRow][rasterColumn];
//                            rasterValue=rasterValues[row][column];
                        }
                        else
                        {
                            if(minGsd<=rasterGsd) // en este caso siempre se cumple
                            {
                                int rasterColumn=floor((centerPixelFc-rasterNwFc)/rasterGsd);
                                int rasterRow=floor((rasterNwSc-centerPixelSc)/rasterGsd);
                                rasterValue=rasterValues[rasterRow][rasterColumn];
                            }
                            else
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                                strError+=QObject::tr("\nError invalid GSD in [%1,%2] for raster file:\n%3")
                                        .arg(QString::number(column)).arg(QString::number(row)).arg(rasterFileName);
                                free(pData);
                                return(false);
                            }
                        }
                        double offset=offsets[njd];
                        double gain=gains[njd];
                        double ndvi=(rasterValue+offsets[njd])*gains[njd];
                        if(ndvi>=mInitialNdvi&&ndvi<=mFinalNdvi)
                        {
                            double kcb=ndvi*mKcbM+mKcbN;
                            if(!mETHOValues.contains(jd))
                            {
                                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                                strError+=QObject::tr("\nError no ETH0 for date %1 for raster file:\n%2")
                                        .arg(QDate::fromJulianDay(jd).toString(TONIPBPPROJECT_DATE_FORMAT_1)).arg(rasterFileName);
                                free(pData);
                                return(false);
                            }
                            double eth0=mETHOValues[jd];
                            accumulatedValue+=(kcb*eth0);
                            if(previousJd>0)
                            {
                                int interpolatedJd=previousJd+1;
                                while(interpolatedJd<jd)
                                {
                                    if(!mETHOValues.contains(interpolatedJd))
                                    {
                                        strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                                        strError+=QObject::tr("\nError no ETH0 for date %1 for raster file:\n%2")
                                                .arg(QDate::fromJulianDay(jd).toString(TONIPBPPROJECT_DATE_FORMAT_1)).arg(rasterFileName);
                                        free(pData);
                                        return(false);
                                    }
                                    double interpolatedKcb=previousKcb+(kcb-previousKcb)/(jd-previousJd)*(interpolatedJd-previousJd);
                                    double interpolatedEth0=mETHOValues[interpolatedJd];
                                    accumulatedValue+=(interpolatedKcb*interpolatedEth0);
                                    interpolatedJd++;
                                }
                            }
                            previousJd=jd;
                            previousKcb=kcb;
                            if(!findInitialNdvi)
                            {
                                findInitialNdvi=true;
                            }
                        }
                        else
                        {
                            if(findInitialNdvi) // se supone que
                            {
                                findFinalNdvid=true;
                                break;
                            }
                        }
                    }
                    double finalRasterValue=noDataValue;
                    if(findInitialNdvi&&findFinalNdvid) // caso normal
                    {
                        finalRasterValue=accumulatedValue;
                    }
                    if(findInitialNdvi&&!findFinalNdvid)
                    {
                        finalRasterValue=accumulatedValue;//??
                    }
                    int posInPData=(row)*roiColumns+column;
                    pData[posInPData]=finalRasterValue;
                }
                iterRow++;
            }
            bool closeAfterCreate=false;
            IGDAL::ImageTypes roiImageType=IGDAL::TIFF;
            GDALDataType roiGdalDataType=GDT_Float32;
            QString rasterOutputFileName=mResultsPath+"/"+tuplekey+"_"+roiCode+"."+RASTER_TIFF_FILE_EXTENSION;
            int numberOfBands=1;
            bool internalGeoRef=true;
            bool externalGeoRef=false;
            QVector<double> roiGeoref;// vacio si se georeferencia con las esquinas
            IGDAL::Raster* ptrOutputRasterFile=new IGDAL::Raster(mPtrCrsTools);
            QMap<QString, QString> refImageOptions; // por implementar
            if(!ptrOutputRasterFile->createRaster(rasterOutputFileName, // Se le añade la extension
                                                  roiImageType,roiGdalDataType,
                                                  numberOfBands,
                                                  roiColumns,roiRows,
                                                  internalGeoRef,externalGeoRef,crsDescription,
                                                  roiNwFc,roiNwSc,roiSeFc,roiSeSc,
                                                  roiGeoref,
                                                  noDataValue,
                                                  closeAfterCreate,
                                                  refImageOptions,
                                                  //                                               buildOverviews,
                                                  strAuxError))
            {
                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                strError+=QObject::tr("\nError creating raster file:\n%1\nfor tuplekey: %2 and project: %3\nError:\n%4")
                        .arg(rasterOutputFileName).arg(tuplekey).arg(roiCode).arg(strAuxError);
                free(pData);
                return(false);
            }
            int initialColumn=0;
            int initialRow=0;
            int numberOfBand=0;
            if(!ptrOutputRasterFile->writeValues(numberOfBand, // desde 0
                                                 initialColumn,
                                                 initialRow,
                                                 roiColumns,
                                                 roiRows,
                                                 pData,
                                                 strError))
            {
                strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
                strError+=QObject::tr("\nError writting raster file:\n%1\nfor tuplekey: %2 and project: %3\nError:\n%4")
                        .arg(rasterOutputFileName).arg(tuplekey).arg(roiCode).arg(strAuxError);
                return(false);
                free(pData);
                delete(ptrOutputRasterFile);
                return(false);
            }
            free(pData);
            delete(ptrOutputRasterFile);
            iterRoi++;
        }
    }
    reportFile.close();
    QMap<QString, int>::const_iterator iterRoi=roiIdByRoiCode.begin();
    ProcessTools::MultiProcess *multiProcess=new ProcessTools::MultiProcess();
    if(mFromConsole)
    {
        QString msg=QObject::tr("    ... Writting merged raster");
        (*mStdOut) <<msg<< endl;
    }
    while(iterRoi!=roiIdByRoiCode.end())
    {
        QString roiCode=iterRoi.key();
//        if(mFromConsole)
//        {
//            QString msg=QObject::tr("        ... Processing ROI: %1").arg(roiCode);
//            (*mStdOut) <<msg<< endl;
//        }
        QStringList mergeArguments;
        QString mergeCommand;
        QString outputFileName=mResultsPath+"/"+roiCode+"."+RASTER_TIFF_FILE_EXTENSION;
        QString inputProj4Crs,outputProj4Crs;
        QStringList inputFileNames;
        QString patternFiles="*_"+roiCode+"."+RASTER_TIFF_FILE_EXTENSION;
        QString resamplingMethod;
        if(!mPtrLibIGDALProcessMonitor->rasterMergeProcessDefinition(inputFileNames,
                                                                     patternFiles,
                                                                     inputProj4Crs,
                                                                     outputFileName,
                                                                     outputProj4Crs,
                                                                     resamplingMethod,
                                                                     mergeCommand,
                                                                     mergeArguments,
                                                                     strAuxError))
        {
            strError=QObject::tr("TONIpbpProject::processAccumulatedPerspirationByTuplekey");
            strError+=QObject::tr("\nError getting merge process definition for project: %1\nError:\n%2")
                    .arg(roiCode).arg(strAuxError);
            return(false);
        }
        ProcessTools::ExternalProcess* ptrMergeRasterProcess=new ProcessTools::ExternalProcess(mergeCommand);
//        ptrReprojectionRasterProcess->appendEnvironmentValue("PATH",micmacBinaryPath);
//        ptrReprojectionRasterProcess->appendEnvironmentValue("PATH",micmacBinaryAuxPath);
        ptrMergeRasterProcess->setWorkingDir(mResultsPath);
        ptrMergeRasterProcess->addIntputs(mergeArguments);
        QObject::connect(ptrMergeRasterProcess, SIGNAL(finished()),mPtrLibIGDALProcessMonitor,SLOT(onMergeProcessFinished()));
        multiProcess->appendProcess(ptrMergeRasterProcess);
        iterRoi++;
    }
    QObject::connect(multiProcess, SIGNAL(newStdData(QString)),mPtrLibIGDALProcessMonitor,SLOT(manageProccesStdOutput(QString)));
    QObject::connect(multiProcess, SIGNAL(newErrorData(QString)),mPtrLibIGDALProcessMonitor,SLOT(manageProccesErrorOutput(QString)));
    QObject::connect(multiProcess, SIGNAL(finished()),mPtrLibIGDALProcessMonitor,SLOT(multiProcessFinished()));
    multiProcess->start();

    return(true);
}

bool TONIpbpProject::setFromFile(QString &inputFileName,
                                 QString &strError)
{
    clear();
    if(!QFile::exists(inputFileName))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nNot exists input file:\n%1").arg(inputFileName);
        return(false);
    }
    QFile fileInput(inputFileName);
    if (!fileInput.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError opening file: \n%1").arg(inputFileName);
        return(false);
    }
    QFileInfo fileInputInfo(inputFileName);
    QTextStream in(&fileInput);

    int intValue,nline=0;
    double dblValue;
    bool okToInt,okToDouble;
    QString strLine,strValue,strAuxError;
    QStringList strList;
    QStringList strAuxList;
    QDir currentDir=QDir::current();

    // Se ignora la cabecera, integrada por dos lineas
    nline++;
    strLine=in.readLine();
    nline++;
    strLine=in.readLine();

    // Lectura del fichero de datos de ETH0
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    if(!QFile::exists(strValue))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nETH0 file not exists:\n%1").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    if(!loadETH0DataFromFile(strValue,strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nError reading ETH0 file:\n%1\nError:\n%2")
                .arg(strValue).arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    mETH0FileName=strValue;

    // Lectura del fichero shapefile de ROIs
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    if(!QFile::exists(strValue))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nROIs shapefile not exists:\n%1").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    mPtrROIsShapefile=new IGDAL::Shapefile(mPtrCrsTools);
    if(!mPtrROIsShapefile->setFromFile(strValue,strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nError making ROIs shapefile from file:\n%1\nError:\n%2")
                .arg(strValue).arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    OGRwkbGeometryType roiShapefileGeometryType=mPtrROIsShapefile->getGeometryType();
    if(roiShapefileGeometryType!=wkbPolygon
            &&roiShapefileGeometryType!=wkbMultiPolygon)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nInvalid geometry type in ROIs shapefile from file:\n%1").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    if(!mPtrROIsShapefile->existsCrs())
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nNo CRS information in ROIs shapefile from file:\n%1").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    int srid=mPtrROIsShapefile->getCrsEpsgCode();
    int numberOfROIs=mPtrROIsShapefile->getNumberOfFeatures();
    if(numberOfROIs<1)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are no features in ROIs shapefile from file:\n%1").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    QVector<QString> roisWktGeometries;
    {
        QStringList roisFieldNames;
        QVector<QStringList> roisFieldsValues;
        QVector<OGRGeometry*> ptrROIsGeometries; // son clonadas
        if(!mPtrROIsShapefile->getFeaturesValues(roisFieldNames,
                                                 roisFieldsValues,
                                                 ptrROIsGeometries,
                                                 strAuxError))
        {
            strError=QObject::tr("TONIpbpProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nError getting geometries in ROIs shapefile from file:\n%1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }
        for(int nf=0;nf<ptrROIsGeometries.size();nf++)
        {
            OGRGeometry* ptrROIGeometry=ptrROIsGeometries[nf];
            char *charsWktGeometry;
            if(OGRERR_NONE!=ptrROIGeometry->exportToWkt(&charsWktGeometry))
            {
                strError=QObject::tr("TONIpbpProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nError exporting to wkt geometry in ROIs shapefile from file:\n%1").arg(strValue);
                fileInput.close();
                clear();
                return(false);
            }
            QString roiWktGeometry=QString::fromLatin1(charsWktGeometry);
            roisWktGeometries.push_back(roiWktGeometry);
        }
    }
    // Lectura de la fecha inicial y de la fecha final
    int minimumValueDate=QDate::fromString(TONIPBPPROJECT_DATE_MINIMUM_VALUE,TONIPBPPROJECT_DATE_FORMAT_1).toJulianDay();
    int maximumValueDate=QDate::fromString(TONIPBPPROJECT_DATE_MAXIMUM_VALUE,TONIPBPPROJECT_DATE_FORMAT_1).toJulianDay();
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    strAuxList=strValue.split(TONIPBPPROJECT_DATES_SEPARATOR_CHARACTER);
    if(strAuxList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nIn dates string: %1\nthere are not two dates separated by %2")
                .arg(strValue).arg(TONIPBPPROJECT_DATES_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    QString strInitialDate=strAuxList.at(0).trimmed();
    mInitialJd=0;
    for(int nfd=0;nfd<mValidFormatsForDate.size();nfd++)
    {
        QDate date=QDate::fromString(strInitialDate,mValidFormatsForDate.at(nfd));
        if(date.isValid())
        {
            mInitialJd=date.toJulianDay();
            break;
        }
    }
    if(mInitialJd==0)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nInvalid date format for initial date in string: %1").arg(strInitialDate);
        fileInput.close();
        clear();
        return(false);
    }
    else
    {
        if(mInitialJd<minimumValueDate||mInitialJd>maximumValueDate)
        {
            strError=QObject::tr("TONIpbpProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nInitial date, %1, is out of valid domain:[%2,%3]")
                    .arg(strInitialDate).arg(TONIPBPPROJECT_DATE_MINIMUM_VALUE).arg(TONIPBPPROJECT_DATE_MAXIMUM_VALUE);
            fileInput.close();
            clear();
            return(false);
        }
    }
    QString strFinalDate=strAuxList.at(1).trimmed();
    mFinalJd=0;
    for(int nfd=0;nfd<mValidFormatsForDate.size();nfd++)
    {
        QDate date=QDate::fromString(strFinalDate,mValidFormatsForDate.at(nfd));
        if(date.isValid())
        {
            mFinalJd=date.toJulianDay();
            break;
        }
    }
    if(mFinalJd==0)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nInvalid date format for final date in string: %1").arg(strFinalDate);
        fileInput.close();
        clear();
        return(false);
    }
    else
    {
        if(mFinalJd<minimumValueDate||mFinalJd>maximumValueDate)
        {
            strError=QObject::tr("TONIpbpProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("Final date, %1, is out of valid domain:[%2,%3]")
                    .arg(strFinalDate).arg(TONIPBPPROJECT_DATE_MINIMUM_VALUE).arg(TONIPBPPROJECT_DATE_MAXIMUM_VALUE);
            fileInput.close();
            clear();
            return(false);
        }
    }
    if(mInitialJd>=mFinalJd)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("Initial date, %1, is not greather than final date:%2")
                .arg(strInitialDate).arg(strFinalDate);
        fileInput.close();
        clear();
        return(false);
    }

    // Lectura de los valores extremos de ndvi
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    strAuxList=strValue.split(TONIPBPPROJECT_NDVIS_SEPARATOR_CHARACTER);
    if(strAuxList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nIn dates string: %1\nthere are not two dates separated by %2")
                .arg(strValue).arg(TONIPBPPROJECT_NDVIS_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    QString strInitialNdvi=strAuxList.at(0).trimmed();
    okToDouble=false;
    mInitialNdvi=strInitialNdvi.toDouble(&okToDouble);
    if(!okToDouble)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nInitial NDVI value is not a double: %1").arg(strInitialNdvi);
        fileInput.close();
        clear();
        return(false);
    }
    if(mInitialNdvi<TONIPBPPROJECT_NDVI_MINIMUM_VALUE||mInitialNdvi>TONIPBPPROJECT_NDVI_MAXIMUM_VALUE)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("Initial NDVI, %1, is out of valid domain:[%2,%3]")
                .arg(strInitialNdvi).arg(QString::number(TONIPBPPROJECT_NDVI_MINIMUM_VALUE,'f',3))
                .arg(QString::number(TONIPBPPROJECT_NDVI_MAXIMUM_VALUE,'f',3));
        fileInput.close();
        clear();
        return(false);
    }
    QString strFinalNdvi=strAuxList.at(1).trimmed();
    okToDouble=false;
    mFinalNdvi=strFinalNdvi.toDouble(&okToDouble);
    if(!okToDouble)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("Final NDVI value is not a double: %1").arg(strInitialNdvi);
        fileInput.close();
        clear();
        return(false);
    }
    if(mFinalNdvi<TONIPBPPROJECT_NDVI_MINIMUM_VALUE||mFinalNdvi>TONIPBPPROJECT_NDVI_MAXIMUM_VALUE)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("Final NDVI, %1, is out of valid domain:[%2,%3]")
                .arg(strInitialNdvi).arg(QString::number(TONIPBPPROJECT_NDVI_MINIMUM_VALUE,'f',3))
                .arg(QString::number(TONIPBPPROJECT_NDVI_MAXIMUM_VALUE,'f',3));
        fileInput.close();
        clear();
        return(false);
    }
    if(mInitialNdvi>=mFinalNdvi)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("Initial NDVI, %1, is not greather than final NDVI:%2")
                .arg(strInitialNdvi).arg(strFinalNdvi);
        fileInput.close();
        clear();
        return(false);
    }

    // Lectura del parametro m en el calculo de kcb (kcb=m*ndvi+b)
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    okToDouble=false;
    mKcbM=strValue.toDouble(&okToDouble);
    if(!okToDouble)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("Parameter M for equation of Kcb is not a double: %1").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }

    // Lectura del parametro n en el calculo de kcb (kcb=m*ndvi+b)
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    okToDouble=false;
    mKcbN=strValue.toDouble(&okToDouble);
    if(!okToDouble)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("Parameter N for equation of Kcb is not a double: %1").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }

    // Lectura del fichero informe de resultados
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
//    if(!QFile::exists(strValue))
//    {
//        strError=QObject::tr("TONIpbpProject::setFromFile");
//        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
//        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
//        strError+=QObject::tr("\nETH0 file not exists:\n%1").arg(strValue);
//        fileInput.close();
//        clear();
//        return(false);
//    }
    mReportFileName=strValue;

    /*
    // Lectura del fichero raster de salida
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
//    if(!QFile::exists(strValue))
//    {
//        strError=QObject::tr("TONIpbpProject::setFromFile");
//        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
//        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
//        strError+=QObject::tr("\nETH0 file not exists:\n%1").arg(strValue);
//        fileInput.close();
//        clear();
//        return(false);
//    }
    mOutputRasterFileName=strValue;
    */

    // Lectura de la ruta de resultados
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    QDir auxDir=QDir::currentPath();
    if(auxDir.exists(strValue))
    {
        if(!auxDir.mkpath(strValue))
        {
            strError=QObject::tr("TONIpbpProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nError making results path:\n%1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }
    }
    mResultsPath=strValue;

    // Lectura del fichero de base de datos original
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    if(!QFile::exists(strValue))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nScenes database file not exists:\n%1").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    QString originalDbFileName=strValue;

    // Lectura del fichero copia de base de datos original
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(TONIPBPPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(TONIPBPPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    if(QFile::exists(strValue))
    {
        if(!QFile::remove(strValue))
        {
            strError=QObject::tr("TONIpbpProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nError removing existing scenes database copy file:\n%1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }
    }
    if(!QFile::copy(originalDbFileName,strValue))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nError copying intial scenes database file:\n%1\nto file:\n%2").arg(originalDbFileName).arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    mPtrPersistenceManager = new RemoteSensing::PersistenceManager();
    QDir programDir=qApp->applicationDirPath();
    QString programPath=programDir.absolutePath();
    if(!mPtrPersistenceManager->initializeAlgorithms(mPtrCrsTools,
                                                     mPtrNestedGridTools,
                                                     mPtrLibIGDALProcessMonitor,
                                                     programPath,
                                                     strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError initializing database:\n%1\nError:\n%2").arg(strValue).arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    if(!mPtrPersistenceManager->openDatabase(strValue,strError))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError setting database:\n%1\nError:\n%2").arg(strValue).arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    QString crsDescription=mPtrPersistenceManager->getCrsDescription();
    QString geographicCrsBaseProj4Text=mPtrPersistenceManager->getGeographicCrsBaseProj4Text();
    QString nestedGridLocalParameters=mPtrPersistenceManager->getNestedGridLocalParameters();
    if(!mPtrNestedGridTools->setCrs(crsDescription,
                                    strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError setting CRS in NestedGridTools: %1").arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    if(!mPtrNestedGridTools->setGeographicBaseCrs(geographicCrsBaseProj4Text,
                                                  strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError setting Geographic CRS in NestedGridTools: %1").arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    QString nestedGridDefinitionResultsFileBasename;
    bool createLODsShapefiles=false;
    if(!nestedGridLocalParameters.isEmpty())
    {
        if(!mPtrNestedGridTools->setLocalParameters(nestedGridLocalParameters,
                                                    nestedGridDefinitionResultsFileBasename,
                                                    strAuxError,
                                                    createLODsShapefiles))
        {
            strError=QObject::tr("TONIpbpProject::setFromFile");
            strError+=QObject::tr("\nError setting Local Parameters:\n%1").arg(strAuxError);
            fileInput.close();
            clear();
            return(false);
        }
    }
    else
    {
        if(!mPtrNestedGridTools->setWholeEarthStandardParameters(strAuxError))
        {
            strError=QObject::tr("TONIpbpProject::setFromFile");
            strError+=QObject::tr("\nError setting Whole Earth Parameters:\n%1").arg(strAuxError);
            fileInput.close();
            clear();
            return(false);
        }
    }

    QMap<int,int> initialDateByProjectId;
    QMap<int,int> finalDateByProjectId;
    QMap<QString,int> idByProjectCode;
    QMap<int,QString> outputPathByProjectId;
    QMap<int,int> outputSridByProjectId;
    QVector<QString> projectCodes;
    QMap<QString,int> intercalibrationReferenceImagesById;
    if(!mPtrPersistenceManager->getProjects(initialDateByProjectId,
                                            finalDateByProjectId,
                                            idByProjectCode,
                                            outputPathByProjectId,
                                            outputSridByProjectId,
                                            projectCodes,
                                            intercalibrationReferenceImagesById,
                                            strAuxError))
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nError recovering projects from database:\n%1\nError:\n%2")
                .arg(mPtrPersistenceManager->getDatabaseFileName()).arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    if(projectCodes.size()!=0)
    {
        strError=QObject::tr("TONIpbpProject::setFromFile");
        strError+=QObject::tr("\nThere are previous projects in database:\n%1")
                .arg(mPtrPersistenceManager->getDatabaseFileName());
        fileInput.close();
        clear();
        return(false);
    }
    // Insertar una zona para cada feature del fichero shapefile
    for(int nf=0;nf<roisWktGeometries.size();nf++)
    {
        QString roiWktGeometry=roisWktGeometries[nf];
        QString zoneCode=TONIPBPPROJECT_ZONECODE_PREFIX+QString::number(nf+1);
        if(!mPtrPersistenceManager->insertProject(zoneCode,mResultsPath,mInitialJd,mFinalJd,
                                                  srid,roiWktGeometry,strAuxError))
        {
            strError=QObject::tr("TONIpbpProject::setFromFile");
            strError+=QObject::tr("\nError inserting project code: %1\nError:\n%2")
                    .arg(strAuxError).arg(mPtrPersistenceManager->getDatabaseFileName());
            fileInput.close();
            clear();
            return(false);
        }
        mProjectCodes.push_back(zoneCode);
    }
    mIsInitialized=true;
    return(true);
}

void TONIpbpProject::multiProcessFinished()
{
    QString strError;
    if(mRemoveRubbish)
    {
        if(!removeRubbish(strError))
            (*mStdOut) <<strError<< endl;
    }
    exit(0);
}

void TONIpbpProject::clear()
{
    mETH0FileName.clear();
    mETHOValues.clear();
    if(mPtrROIsShapefile!=NULL)
    {
        delete(mPtrROIsShapefile);
        mPtrROIsShapefile=NULL;
    }
    if(mPtrPersistenceManager!=NULL)
    {
        delete(mPtrPersistenceManager);
        mPtrPersistenceManager=NULL;
    }
    mInitialJd=0;
    mFinalJd=0;
    mInitialNdvi=TONIPBPPROJECT_NODOUBLE;
    mFinalNdvi=TONIPBPPROJECT_NODOUBLE;
    mKcbM=TONIPBPPROJECT_NODOUBLE;
    mKcbN=TONIPBPPROJECT_NODOUBLE;
    mReportFileName.clear();
//    mOutputRasterFileName.clear();
    mResultsPath.clear();
    mIsInitialized=false;
    mProjectCodes.clear();
}

bool TONIpbpProject::removeDir(QString dirName)
{
    bool result = true;
    QDir dir(dirName);
    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }
            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }
    return result;
}

bool TONIpbpProject::removeRubbish(QString &strError)
{
    if(!mRemoveRubbish)
        return(true);
    QDir auxDir=QDir::currentPath();
    for(int i=0;i<mFilesToRemove.size();i++)
    {
        QString file=mFilesToRemove[i];
        if(QFile::exists(file))
        {
            if(!QFile::remove(file))
            {
                strError=tr("Error removing file:\n %1\n").arg(file);
                return(false);
            }
        }
    }
    for(int i=0;i<mPathsToRemove.size();i++)
    {
        QString path=mPathsToRemove[i];
        if(auxDir.exists(path))
        {
            if(!removeDir(path))
            {
                strError=tr("Error removing directory:\n %1\n").arg(path);
                return(false);
            }
        }
    }
    for(int i=0;i<mPathsToRemoveIfEmpty.size();i++)
    {
        QString path=mPathsToRemoveIfEmpty[i];
        if(auxDir.exists(path))
        {
            bool isEmtpy=false;
            {
                QDir dir(path);
                if(dir.count()<3)
                {
                    isEmtpy=true;
                }
            }
            if(isEmtpy)
            {
                if(!removeDir(path))
                {
                    strError=tr("Error removing directory:\n %1\n").arg(path);
                    return(false);
                }
            }
        }
    }
    return(true);
}
