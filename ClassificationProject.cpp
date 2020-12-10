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
#include "../../libs/libIGDAL/SpatiaLite.h"
#include "../../libs/libIGDAL/Raster.h"
#include "../../libs/libRemoteSensing/PersistenceManager.h"
#include "../../libs/libProcessTools/MultiProcess.h"
#include "../../libs/libProcessTools/ExternalProcess.h"

#include "ClassificationProject.h"

using namespace RemoteSensing;

ClassificationProject::ClassificationProject(libCRS::CRSTools* ptrCrsTools,
                                             NestedGrid::NestedGridTools* ptrNestedGridTools,
                                             IGDAL::libIGDALProcessMonitor* ptrLibIGDALProcessMonitor,
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
    mValidFormatsForDate.append(CLASSIFICATIONPROJECT_DATE_FORMAT_1);
    mValidFormatsForDate.append(CLASSIFICATIONPROJECT_DATE_FORMAT_2);
    mValidFormatsForDate.append(CLASSIFICATIONPROJECT_DATE_FORMAT_3);
    mValidFormatsForDate.append(CLASSIFICATIONPROJECT_DATE_FORMAT_4);
    mIsInitialized=false;
    mProcessCreateClassificationData=false;
}

bool ClassificationProject::createClassificationData(QString &strError)
{
    if(!mIsInitialized)
    {
        strError=QObject::tr("ClassificationProject::createClassificationData");
        strError+=QObject::tr("\nProject is not initialized");
        return(false);
    }
    if(mFromConsole)
    {
        QString msg=QObject::tr("        ... Recovering ROIs data from database");
        (*mStdOut) <<msg<< endl;
    }
    QString crsDescription=mPtrNestedGridTools->getCrsDescription();
    int numberOfPixels=mPtrNestedGridTools->getBaseWorldWidthAndHeightInPixels();
    QMap<QString,QMap<int,QMap<int,QVector<QString> > > > tuplekeyFileNamesByTuplekeyByRoiCodeByJd;
    QMap<int, int> roiCropByRoiCode;
    QMap<QString,int> tuplekeyIdByTuplekeyCode;
    QMap<QString,double> gainByTuplekeyFileName;
    QMap<QString,double> offsetByTuplekeyFileName;
    QMap<QString,int> lodTilesByTuplekeyFileName;
    QMap<QString,int> lodGsdByTuplekeyFileName;
    QMap<QString, QString> rasterIdByTuplekeyFileName;
    int maximumLod;
    QString strAuxError;
    if(mFromConsole)
    {
        QString msg=QObject::tr("        ... Recovering data from database");
        (*mStdOut) <<msg<< endl;
    }
    if(!getNdviDataByTuplekeyByRoi(mInitialJd,
                                   mFinalJd,
                                   tuplekeyFileNamesByTuplekeyByRoiCodeByJd,
                                   roiCropByRoiCode,
                                   tuplekeyIdByTuplekeyCode,
                                   gainByTuplekeyFileName,
                                   offsetByTuplekeyFileName,
                                   lodTilesByTuplekeyFileName,
                                   lodGsdByTuplekeyFileName,
                                   rasterIdByTuplekeyFileName,
                                   maximumLod,
                                   strAuxError))
    {
        strError=QObject::tr("ClassificationProject::createClassificationData");
        strError+=QObject::tr("\nError recovering ndvi data from database:\n%1").arg(strAuxError);
        return(false);
    }

    QVector<QString> tuplekeys;
    QMap<QString,QMap<int,QMap<int,QVector<QString> > > >::const_iterator iterTuplekeys=tuplekeyFileNamesByTuplekeyByRoiCodeByJd.begin();
    while(iterTuplekeys!=tuplekeyFileNamesByTuplekeyByRoiCodeByJd.end())
    {
        tuplekeys.push_back(iterTuplekeys.key());
        iterTuplekeys++;
    }
    QMap<int,double> minimunGsdByRoiCode;
    QMap<QString,QVector<double> > rasterBoundingBoxByFileName; // nwFc,nwSc,seFc,seSc -> puede no coincidir con georef porque en algunos casos georef va al centro del pixel
    QMap<QString,double> rasterGsdByFileName;
    QMap<QString,int> rasterRowsByFileName;
    QMap<QString,int> rasterColumnsByFileName;
    QMap<int,QVector<QString> > tuplekeysByRoiCode;
    QMap<int,QVector<bool> > tuplekeysProccesedByRoiCode;
    QMap<QString,QString> spacecraftIdByFileName;
    QMap<int, double> roiAreaByRoiCode;
    QMap<QString,QMap<int,QMap<int,QVector<QString> > > >::const_iterator iterTuplekeyFileNameByTuplekeyByRoiCodeByJd=tuplekeyFileNamesByTuplekeyByRoiCodeByJd.begin();
    if(mFromConsole)
    {
        QString msg=QObject::tr("    ... Computing tuplekeys previous data");
        (*mStdOut) <<msg<< endl;
    }
    while(iterTuplekeyFileNameByTuplekeyByRoiCodeByJd!=tuplekeyFileNamesByTuplekeyByRoiCodeByJd.end())
    {
        QString tuplekey=iterTuplekeyFileNameByTuplekeyByRoiCodeByJd.key();
        if(mFromConsole)
        {
            QString msg=QObject::tr("        ... Processing tuplekey: %1").arg(tuplekey);
            (*mStdOut) <<msg<< endl;
        }
        QMap<int,QMap<int,QVector<QString> > > tuplekeyFileNameByRoiCodeByJd=iterTuplekeyFileNameByTuplekeyByRoiCodeByJd.value();
        QMap<int,QMap<int,QVector<QString> > >::const_iterator iterTuplekeyFileNameByRoiCodeByJd=tuplekeyFileNameByRoiCodeByJd.begin();
        while(iterTuplekeyFileNameByRoiCodeByJd!=tuplekeyFileNameByRoiCodeByJd.end())
        {
            int roiCode=iterTuplekeyFileNameByRoiCodeByJd.key();
            if(!tuplekeysByRoiCode.contains(roiCode))
            {
                QVector<QString> auxVector;
                tuplekeysByRoiCode[roiCode]=auxVector;
                QVector<bool> auxVector2;
                tuplekeysProccesedByRoiCode[roiCode]=auxVector2;
            }
            tuplekeysByRoiCode[roiCode].push_back(tuplekey);
            tuplekeysProccesedByRoiCode[roiCode].push_back(false);
            if(mFromConsole)
            {
                QString msg=QObject::tr("            ... Processing ROI: %1").arg(QString::number(roiCode));
                (*mStdOut) <<msg<< endl;
            }
            QMap<int,QVector<QString> > tuplekeyFileNameByJd=iterTuplekeyFileNameByRoiCodeByJd.value();
            QMap<int,QVector<QString> >::const_iterator iterTuplekeyFileNameByJd=tuplekeyFileNameByJd.begin();
            while(iterTuplekeyFileNameByJd!=tuplekeyFileNameByJd.end())
            {
                QVector<QString> rasterFileNames=iterTuplekeyFileNameByJd.value();
                for(int nrf=0;nrf<rasterFileNames.size();nrf++)
                {
                    QString rasterFileName=rasterFileNames.at(nrf);
                    int lodTiles=lodTilesByTuplekeyFileName[rasterFileName];
                    int lodGsd=lodGsdByTuplekeyFileName[rasterFileName];
                    if(!rasterGsdByFileName.contains(rasterFileName))
                    {
                        double rasterGsd;
                        if(!mPtrNestedGridTools->getLodGsd(lodGsd,rasterGsd,strAuxError))
                        {
                            strError=QObject::tr("ClassificationProject::createClassificationData");
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
                            strError=QObject::tr("ClassificationProject::createClassificationData");
                            strError+=QObject::tr("\nError getting tileX and tileY from tuplekey for raster file:\n%1\nError:\n%2")
                                    .arg(rasterFileName).arg(strAuxError);
                            return(false);
                        }
                        double rasterNwFc,rasterNwSc,rasterSeFc,rasterSeSc;
                        if(!mPtrNestedGridTools->getBoundingBoxFromTile(lodTiles,tileX,tileY,crsDescription,
                                                                        rasterNwFc,rasterNwSc,rasterSeFc,rasterSeSc,
                                                                        strAuxError))
                        {
                            strError=QObject::tr("ClassificationProject::createClassificationData");
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
                        QString spaceCraftId=CLASSIFICATIONPROJECT_L8_TAG;
                        bool validSpacecraft=false;
                        if(rasterFileName.contains(mL8SpacecraftIdentifier,Qt::CaseInsensitive))
                        {
                            validSpacecraft=true;
                        }
                        else if(rasterFileName.contains(mS2ASpacecraftIdentifier,Qt::CaseInsensitive))
                        {
                            validSpacecraft=true;
                            spaceCraftId=CLASSIFICATIONPROJECT_S2A_TAG;
                        }
                        if(!validSpacecraft)
                        {
                            strError=QObject::tr("ClassificationProject::createClassificationData");
                            strError+=QObject::tr("\nInvalid spacecraft id in raster file name:\n%1").arg(rasterFileName);
                            return(false);
                        }
                        spacecraftIdByFileName[rasterFileName]=spaceCraftId;
                    }
                }
                iterTuplekeyFileNameByJd++;
            }
            iterTuplekeyFileNameByRoiCodeByJd++;
        }
        iterTuplekeyFileNameByTuplekeyByRoiCodeByJd++;
    }
    QFile reportFile(mReportFileName);
    if (!reportFile.open(QFile::WriteOnly |QFile::Text))
    {
        strError=QObject::tr("ClassificationProject::createClassificationData");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(mReportFileName);
        return(false);
    }
    QTextStream out(&reportFile);
    out<<"RESULTADOS DE PROCESAMIENTO\n";
    out<<"---------------------------\n";
    out<<"- Incrementoo a los id de los ROIs ..: "<<QString::number(mROIIncrement)<<"\n";
    if(mFromConsole)
    {
        QString msg=QObject::tr("    ... Processing tuplekeys");
        (*mStdOut) <<msg<< endl;
    }
    QMap<QString,int> rasterNoDataValueByFileName;
    QMap<int,QMap<QString,QMap<int,double> > >  meanValuesByRoiCodeBySpacecraftByJd;
    QMap<int,QMap<QString,QMap<int,double> > >  stdValuesByRoiCodeBySpacecraftByJd;
    QMap<int,QMap<QString,QMap<int,QVector<int> > > >  valuesByRoiCodeBySpacecraftByJd;
    int rasterBand=0;
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
        QMap<QString,QVector<QVector<int> > > rasterValuesByFileName;
        QMap<int,QMap<int,QVector<QString> > > tuplekeyFileNameByRoiCodeByJd=tuplekeyFileNamesByTuplekeyByRoiCodeByJd[tuplekey];
        QMap<int,QMap<int,QVector<QString>> >::const_iterator iterRoi=tuplekeyFileNameByRoiCodeByJd.begin();
        while(iterRoi!=tuplekeyFileNameByRoiCodeByJd.end())
        {
            int roiCode=iterRoi.key();
            if(mFromConsole)
            {
                QString msg=QObject::tr("            ... Processing ROI: %1").arg(QString::number(roiCode));
                (*mStdOut) <<msg<< endl;
            }
            out<<"  - ROI a procesar ..................: "<<QString::number(roiCode);
            out<<" ("<<QString::number(tuplekeysByRoiCode[roiCode].size())<<" tuplekeys)"<<"\n";
//            int roiId=roiIdByRoiCode[roiCode];
//            double minGsd=minimunGsdByRoiCode[roiCode];
            OGRGeometry *ptrRoiTuplekeyIntersectionGeometry;
            bool roiContainsTuplekey=false;
            if(!getROITuplekeyIntersectionGeomety(roiCode,
                                                  tuplekeyId,
                                                  ptrRoiTuplekeyIntersectionGeometry,
                                                  roiContainsTuplekey,
                                                  strAuxError))
            {
                strError=QObject::tr("ClassificationProject::createClassificationData");
                strError+=QObject::tr("\nError recovering geometries of projects from database:\n%1").arg(strAuxError);
                reportFile.close();
                return(false);
            }
            double roiNwFc,roiNwSc, roiSeFc,roiSeSc;
            OGREnvelope roiEnvelope;
            ptrRoiTuplekeyIntersectionGeometry->getEnvelope(&roiEnvelope);
            roiNwFc=roiEnvelope.MinX;
            roiNwSc=roiEnvelope.MaxY;
            roiSeFc=roiEnvelope.MaxX;
            roiSeSc=roiEnvelope.MinY;
            if(!roiAreaByRoiCode.contains(roiCode))
            {
                double roiArea;
                if(!getROIArea(roiCode,roiArea,strAuxError))
                {
                    strError=QObject::tr("ClassificationProject::createClassificationData");
                    strError+=QObject::tr("\nError recovering area for ROI: %1\nError:\n%2")
                            .arg(QString::number(roiCode)).arg(strAuxError);
                    reportFile.close();
                    return(false);
                }
                roiAreaByRoiCode[roiCode]=roiArea;
            }
            QMap<int,QVector<QString> > tuplekeyFileNameByJd=iterRoi.value();
            QMap<int,QVector<QString> >::const_iterator iterTuplekeyFileNameByJd=tuplekeyFileNameByJd.begin();
            while(iterTuplekeyFileNameByJd!=tuplekeyFileNameByJd.end())
            {
                int jd=iterTuplekeyFileNameByJd.key();
                out<<"    - Fecha a procesar ..............: "<<QString::number(jd);
                out<<" - "<<QDate::fromJulianDay(jd).toString(CLASSIFICATIONPROJECT_DATE_FORMAT_1)<<"\n";
                QVector<QString> rasterFileNames=iterTuplekeyFileNameByJd.value();
                for(int nrf=0;nrf<rasterFileNames.size();nrf++)
                {
                    QString rasterFileName=rasterFileNames.at(nrf);
                    QString spaceCraftId=spacecraftIdByFileName[rasterFileName];
                    out<<"      - Fichero a procesar ..........: "<<rasterFileName<<"\n";
                    double gain=gainByTuplekeyFileName[rasterFileName];
                    double offset=offsetByTuplekeyFileName[rasterFileName];
                    int lodTiles=lodTilesByTuplekeyFileName[rasterFileName];
                    int lodGsd=lodGsdByTuplekeyFileName[rasterFileName];
                    double rasterGsd=rasterGsdByFileName[rasterFileName];
                    double rasterNwFc=rasterBoundingBoxByFileName[rasterFileName][0];
                    double rasterNwSc=rasterBoundingBoxByFileName[rasterFileName][1];
                    int rasterColumns=rasterColumnsByFileName[rasterFileName];
                    int rasterRows=rasterRowsByFileName[rasterFileName];
                    int rasterNoDataValue;
                    QVector<QVector<int> > rasterValues(rasterRows);
                    if(!rasterValuesByFileName.contains(rasterFileName))
                    {
                        IGDAL::Raster* ptrRaster=new IGDAL::Raster(mPtrCrsTools);
                        if(!ptrRaster->setFromFile(rasterFileName,strAuxError))
                        {
                            strError=QObject::tr("ClassificationProject::createClassificationData");
                            strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                                    .arg(rasterFileName).arg(strAuxError);
                            reportFile.close();
                            return(false);
                        }
                        QVector<QVector<double> > dblRasterValues;
                        if(!ptrRaster->readValues(rasterBand,
                                                  0,0,
                                                  rasterColumns,rasterRows,
                                                  dblRasterValues,
                                                  strAuxError)) // values[row][column]
                        {
                            strError=QObject::tr("ClassificationProject::createClassificationData");
                            strError+=QObject::tr("\nError getting dimensions from raster file:\n%1\nError:\n%2")
                                    .arg(rasterFileName).arg(strAuxError);
                            delete(ptrRaster);
                            reportFile.close();
                            return(false);
                        }
                        for(int row=0;row<rasterRows;row++)
                        {
                            QVector<int> rowRasterValues(rasterColumns);
                            for(int column=0;column<rasterColumns;column++)
                            {
                                int intValue=qRound(dblRasterValues[row][column]*CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE);
                                rowRasterValues[column]=intValue;
                            }
                            rasterValues[row]=rowRasterValues;
                        }
                        double dblRasterNoDataValue;
                        if(!ptrRaster->getNoDataValue(rasterBand,
                                                      dblRasterNoDataValue,
                                                      strAuxError))
                        {
                            strError=QObject::tr("ClassificationProject::createClassificationData");
                            strError+=QObject::tr("\nError getting no data value from raster file:\n%1\nError:\n%2")
                                    .arg(rasterFileName).arg(strAuxError);
                            delete(ptrRaster);
                            reportFile.close();
                            return(false);
                        }
                        delete(ptrRaster);
                        rasterNoDataValue=qRound(dblRasterNoDataValue*CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE);
                        rasterValuesByFileName[rasterFileName]=rasterValues;
                        rasterNoDataValueByFileName[rasterFileName]=rasterNoDataValue;
                    }
                    else
                    {
                        rasterValues=rasterValuesByFileName[rasterFileName];
                        rasterNoDataValue=rasterNoDataValueByFileName[rasterFileName];
                    }
                    if(roiContainsTuplekey)
                    {
                        for(int nRow=0;nRow<rasterRows;nRow++)
                        {
                            for(int nColumn=0;nColumn<rasterColumns;nColumn++)
                            {
                                if(fabs(rasterValues[nRow][nColumn]-rasterNoDataValue)<1.0)
                                {
                                    continue;
                                }
                                double ndvi=((double)rasterValues[nRow][nColumn])/CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE*gain+offset;
                                int intNdvi=qRound(ndvi*CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE);
                                bool insertedValue=false;
                                if(valuesByRoiCodeBySpacecraftByJd.contains(roiCode))
                                {
                                    if(valuesByRoiCodeBySpacecraftByJd[roiCode].contains(spaceCraftId))
                                    {
                                        if(valuesByRoiCodeBySpacecraftByJd[roiCode][spaceCraftId].contains(jd))
                                        {
                                            valuesByRoiCodeBySpacecraftByJd[roiCode][spaceCraftId][jd].push_back(intNdvi);
                                            insertedValue=true;
                                        }
                                    }
                                }
                                if(!insertedValue)
                                {
                                    QVector<int> values;
                                    values.push_back(intNdvi);
                                    valuesByRoiCodeBySpacecraftByJd[roiCode][spaceCraftId][jd]=values;
                                }
                            }
                        }
                    }
                    else
                    {
                        int rasterColumn=floor((roiNwFc-rasterNwFc)/rasterGsd);
                        bool controlColumn=true;
                        while(controlColumn)
                        {
                            int rasterRow=floor((rasterNwSc-roiNwSc)/rasterGsd);
                            bool controlRow=true;
                            while(controlRow)
                            {
                                if(fabs(rasterValues[rasterRow][rasterColumn]-rasterNoDataValue)<1.0)
                                {
                                    double scSRasterPixel=rasterNwSc-(rasterRow+1)*rasterGsd;
                                    if(scSRasterPixel<=roiSeSc)
                                    {
                                        controlRow=false;
                                    }
                                    else
                                    {
                                        rasterRow++;
                                    }
                                    continue;
                                }
                                bool intersects=false;
                                if(!getIntersectsPixel(rasterNwFc,
                                                       rasterNwSc,
                                                       rasterGsd,
                                                       rasterColumn,
                                                       rasterRow,
                                                       ptrRoiTuplekeyIntersectionGeometry,
                                                       intersects,
                                                       strAuxError))
                                {
                                    strError=QObject::tr("ClassificationProject::createClassificationData");
                                    strError+=QObject::tr("\nError getting intersects for tuplekey: %1, for roi: %2, for date: %32")
                                            .arg(tuplekey).arg(QString::number(roiCode)).arg(QDate::fromJulianDay(jd).toString(CLASSIFICATIONPROJECT_DATE_FORMAT_1));
                                    strError+=QObject::tr("\nfor raster file:\n%1").arg(rasterFileName);
                                    strError+=QObject::tr("\nfor column: %1 and row: %2")
                                            .arg(QString::number(rasterColumn)).arg(QString::number(rasterRow));
                                    reportFile.close();
                                    return(false);
                                }
                                if(intersects)
                                {
                                    double ndvi=((double)rasterValues[rasterRow][rasterColumn])/CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE*gain+offset;
                                    int intNdvi=qRound(ndvi*CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE);
                                    bool insertedValue=false;
                                    if(valuesByRoiCodeBySpacecraftByJd.contains(roiCode))
                                    {
                                        if(valuesByRoiCodeBySpacecraftByJd[roiCode].contains(spaceCraftId))
                                        {
                                            if(valuesByRoiCodeBySpacecraftByJd[roiCode][spaceCraftId].contains(jd))
                                            {
                                                valuesByRoiCodeBySpacecraftByJd[roiCode][spaceCraftId][jd].push_back(intNdvi);
                                                insertedValue=true;
                                            }
                                        }
                                    }
                                    if(!insertedValue)
                                    {
                                        QVector<int> values;
                                        values.push_back(intNdvi);
                                        valuesByRoiCodeBySpacecraftByJd[roiCode][spaceCraftId][jd]=values;
                                    }
                                }
                                double scSRasterPixel=rasterNwSc-(rasterRow+1)*rasterGsd;
                                if(scSRasterPixel<=roiSeSc)
                                {
                                    controlRow=false;
                                }
                                else
                                {
                                    rasterRow++;
                                }
                            }
                            double fcWRasterPixel=rasterNwFc+(rasterColumn+1)*rasterGsd;
                            if(fcWRasterPixel>=roiSeFc)
                            {
                                controlColumn=false;
                            }
                            else
                            {
                                rasterColumn++;
                            }
                        }
                    }
                }
                if(tuplekeysByRoiCode[roiCode].size()==1) // lo mas frecuente
                {
                    QMap<QString,QMap<int,QVector<int> > > valuesBySpacecraftByJd=valuesByRoiCodeBySpacecraftByJd[roiCode];
                    QMap<QString,QMap<int,QVector<int> > >::const_iterator iterSpacecraft=valuesBySpacecraftByJd.begin();
                    QVector<QString> spacecraftIdValuesToRemove;
                    while(iterSpacecraft!=valuesBySpacecraftByJd.end())
                    {
                        QString spacecraftId=iterSpacecraft.key();
                        QMap<int,QVector<int> > valuesByJd=iterSpacecraft.value();
                        QMap<int,QVector<int> >::const_iterator iterJd=valuesByJd.begin();
                        QVector<int> values=iterJd.value();
                        double meanValue=0.0;
                        for(int nv=0;nv<values.size();nv++)
                        {
                            meanValue+=values[nv];
                        }
                        meanValue/=values.size();
                        double stdValue=0.0;
                        if(values.size()>1)
                        {
                            for(int nv=0;nv<values.size();nv++)
                            {
                                stdValue+=pow(meanValue-values[nv],2.0);
                            }
                            stdValue=sqrt(stdValue/(values.size()-1));
                        }
                        //                    QMap<int,QMap<QString,QMap<int,double> > >  meanValuesByRoiCodeBySpacecraftByJd;
                        //                    QMap<int,QMap<QString,QMap<int,double> > >  stdValuesByRoiCodeBySpacecraftByJd;
                        //                    QMap<int,QMap<QString,QMap<int,QVector<double> > > >  valuesByRoiCodeBySpacecraftByJd;
                        meanValuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftId][jd]=meanValue/CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE;
                        stdValuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftId][jd]=stdValue/CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE;
                        spacecraftIdValuesToRemove.push_back(spacecraftId);
                        iterSpacecraft++;
                    }
                    for(int nvtr=0;nvtr<spacecraftIdValuesToRemove.size();nvtr++)
                    {
                        QString spacecraftIdToRemove=spacecraftIdValuesToRemove.at(nvtr);
                        valuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftIdToRemove].remove(jd);
                    }
                    for(int nvtr=0;nvtr<spacecraftIdValuesToRemove.size();nvtr++)
                    {
                        QString spacecraftIdToRemove=spacecraftIdValuesToRemove.at(nvtr);
                        if(valuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftIdToRemove].size()==0)
                        {
                            valuesByRoiCodeBySpacecraftByJd[roiCode].remove(spacecraftIdToRemove);
                        }
                    }
                    if(valuesByRoiCodeBySpacecraftByJd[roiCode].size()==0)
                    {
                        valuesByRoiCodeBySpacecraftByJd.remove(roiCode);
                    }
                }
                iterTuplekeyFileNameByJd++;
            }
            OGRGeometryFactory::destroyGeometry(ptrRoiTuplekeyIntersectionGeometry);
            int tuplekeyPositionInTuplekeysByRoiCode=tuplekeysByRoiCode[roiCode].indexOf(tuplekey);
            tuplekeysProccesedByRoiCode[roiCode][tuplekeyPositionInTuplekeysByRoiCode]=true;
            bool proccesedAllTuplekeysForRoiCode=true;
            for(int nn=0;nn<tuplekeysProccesedByRoiCode[roiCode].size();nn++)
            {
                if(!tuplekeysProccesedByRoiCode[roiCode][nn])
                {
                    proccesedAllTuplekeysForRoiCode=false;
                    break;
                }
            }
            // Si se han procesado todos obtengo las estadísticas
            if(proccesedAllTuplekeysForRoiCode)
            {
                QMap<QString,QMap<int,QVector<int> > > valuesBySpacecraftByJd=valuesByRoiCodeBySpacecraftByJd[roiCode];
                QMap<QString,QMap<int,QVector<int> > >::const_iterator iterSpacecraft=valuesBySpacecraftByJd.begin();
                while(iterSpacecraft!=valuesBySpacecraftByJd.end())
                {
                    QString spacecraftId=iterSpacecraft.key();
                    QMap<int,QVector<int> > valuesByJd=iterSpacecraft.value();
                    QMap<int,QVector<int> >::const_iterator iterJd=valuesByJd.begin();
                    while(iterJd!=valuesByJd.end())
                    {
                        int jd=iterJd.key();
                        QVector<int> values=iterJd.value();
                        double meanValue=0.0;
                        for(int nv=0;nv<values.size();nv++)
                        {
                            meanValue+=values[nv];
                        }
                        meanValue/=values.size();
                        double stdValue=0.0;
                        if(values.size()>1)
                        {
                            for(int nv=0;nv<values.size();nv++)
                            {
                                stdValue+=pow(meanValue-values[nv],2.0);
                            }
                        }
                        stdValue=sqrt(stdValue/(values.size()-1));
                        //                    QMap<int,QMap<QString,QMap<int,double> > >  meanValuesByRoiCodeBySpacecraftByJd;
                        //                    QMap<int,QMap<QString,QMap<int,double> > >  stdValuesByRoiCodeBySpacecraftByJd;
                        //                    QMap<int,QMap<QString,QMap<int,QVector<double> > > >  valuesByRoiCodeBySpacecraftByJd;
                        meanValuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftId][jd]=meanValue/CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE;
                        stdValuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftId][jd]=stdValue/CLASSIFICATIONPROJECT_SCALE_FACTOR_TO_NDVI_INTEGER_STORAGE;
                        iterJd++;
                    }
                    iterSpacecraft++;
                }
                valuesByRoiCodeBySpacecraftByJd.remove(roiCode);
            }
            iterRoi++;
        }
    }
    reportFile.close();
    QFile reportL8File(mL8ReportFileName);
    if (!reportL8File.open(QFile::WriteOnly |QFile::Text))
    {
        strError=QObject::tr("ClassificationProject::createClassificationData");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(mL8ReportFileName);
        return(false);
    }
    QTextStream outL8(&reportL8File);
    QFile reportS2AFile(mS2AReportFileName);
    if (!reportS2AFile.open(QFile::WriteOnly |QFile::Text))
    {
        strError=QObject::tr("ClassificationProject::createClassificationData");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(mS2AReportFileName);
        return(false);
    }
    QTextStream outS2A(&reportS2AFile);

    QString strRoiIdTag=CLASSIFICATIONPROJECT_ROI_ID_PRINT_TAG;
    QString strRoiCropTag=CLASSIFICATIONPROJECT_ROI_CROP_PRINT_TAG;
    QString strRoiAreaTag=CLASSIFICATIONPROJECT_ROI_AREA_PRINT_TAG;

    outL8<<"L8 CLASSIFICATION DATA, Time interval: ";
    outL8<<QDate::fromJulianDay(mInitialJd).toString(CLASSIFICATIONPROJECT_DATE_FORMAT_1);
    outL8<<" to ";
    outL8<<QDate::fromJulianDay(mInitialJd).toString(CLASSIFICATIONPROJECT_DATE_FORMAT_1);
    outL8<<"\n";
    outL8<<"- Increment to ROIs ids: "<<QString::number(mROIIncrement)<<"\n";
    outL8<<strRoiIdTag.rightJustified(CLASSIFICATIONPROJECT_ROI_ID_PRINT_WIDTH);
    outL8<<strRoiCropTag.rightJustified(CLASSIFICATIONPROJECT_ROI_CROP_PRINT_WIDTH);
    outL8<<strRoiAreaTag.rightJustified(CLASSIFICATIONPROJECT_ROI_AREA_PRINT_WIDTH);

    outS2A<<"S2A CLASSIFICATION DATA, Time interval: ";
    outS2A<<QDate::fromJulianDay(mInitialJd).toString(CLASSIFICATIONPROJECT_DATE_FORMAT_1);
    outS2A<<" to ";
    outS2A<<QDate::fromJulianDay(mInitialJd).toString(CLASSIFICATIONPROJECT_DATE_FORMAT_1);
    outS2A<<"\n";
    outS2A<<"- Increment to ROIs ids: "<<QString::number(mROIIncrement)<<"\n";
    outS2A<<strRoiIdTag.rightJustified(CLASSIFICATIONPROJECT_ROI_ID_PRINT_WIDTH);
    outS2A<<strRoiCropTag.rightJustified(CLASSIFICATIONPROJECT_ROI_CROP_PRINT_WIDTH);
    outS2A<<strRoiAreaTag.rightJustified(CLASSIFICATIONPROJECT_ROI_AREA_PRINT_WIDTH);

    for(int njd=mInitialJd;njd<=mFinalJd;njd++)
    {
        QString strDate=QDate::fromJulianDay(njd).toString(CLASSIFICATIONPROJECT_DATE_FORMAT_1);
        outL8<<strDate.rightJustified(CLASSIFICATIONPROJECT_VALUES_PRINT_WIDTH*2);
        outS2A<<strDate.rightJustified(CLASSIFICATIONPROJECT_VALUES_PRINT_WIDTH*2);
    }
    outL8<<"\n";
    outS2A<<"\n";

    QMap<int,QMap<QString,QMap<int,double> > >::const_iterator iterROIs=meanValuesByRoiCodeBySpacecraftByJd.begin();
    while(iterROIs!=meanValuesByRoiCodeBySpacecraftByJd.end())
    {
        int roiCode=iterROIs.key();
        QMap<QString,QMap<int,double> > meanValuesBySpacecraftByJd=iterROIs.value();
        QMap<QString,QMap<int,double> >::const_iterator iterSpacecraft=meanValuesBySpacecraftByJd.begin();
        while(iterSpacecraft!=meanValuesBySpacecraftByJd.end())
        {
            QString spacecraftId=iterSpacecraft.key();
            if(spacecraftId.compare(CLASSIFICATIONPROJECT_L8_TAG)==0)
            {
                outL8<<QString::number(roiCode).rightJustified(CLASSIFICATIONPROJECT_ROI_ID_PRINT_WIDTH);
                outL8<<QString::number(roiCropByRoiCode[roiCode]).rightJustified(CLASSIFICATIONPROJECT_ROI_CROP_PRINT_WIDTH);
                outL8<<QString::number(roiAreaByRoiCode[roiCode],'f',CLASSIFICATIONPROJECT_ROI_AREA_PRECISION).rightJustified(CLASSIFICATIONPROJECT_ROI_AREA_PRINT_WIDTH);
            }
            else if(spacecraftId.compare(CLASSIFICATIONPROJECT_S2A_TAG)==0)
            {
                outS2A<<QString::number(roiCode).rightJustified(CLASSIFICATIONPROJECT_ROI_ID_PRINT_WIDTH);
                outS2A<<QString::number(roiCropByRoiCode[roiCode]).rightJustified(CLASSIFICATIONPROJECT_ROI_CROP_PRINT_WIDTH);
                outS2A<<QString::number(roiAreaByRoiCode[roiCode],'f',CLASSIFICATIONPROJECT_ROI_AREA_PRECISION).rightJustified(CLASSIFICATIONPROJECT_ROI_AREA_PRINT_WIDTH);
            }
            for(int njd=mInitialJd;njd<=mFinalJd;njd++)
            {
                QString strMeanValue="NaN";
                QString strStdValue="NaN";
                if(meanValuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftId].contains(njd))
                {
                    strMeanValue=QString::number(meanValuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftId][njd],'f',CLASSIFICATIONPROJECT_MEAN_VALUES_PRINT_PRECISION);
                    strStdValue=QString::number(stdValuesByRoiCodeBySpacecraftByJd[roiCode][spacecraftId][njd],'f',CLASSIFICATIONPROJECT_MEAN_VALUES_PRINT_PRECISION);
                }
                if(spacecraftId.compare(CLASSIFICATIONPROJECT_L8_TAG)==0)
                {
                    outL8<<strMeanValue.rightJustified(CLASSIFICATIONPROJECT_VALUES_PRINT_WIDTH);
                    outL8<<strStdValue.rightJustified(CLASSIFICATIONPROJECT_VALUES_PRINT_WIDTH);
                }
                else if(spacecraftId.compare(CLASSIFICATIONPROJECT_S2A_TAG)==0)
                {
                    outS2A<<strMeanValue.rightJustified(CLASSIFICATIONPROJECT_VALUES_PRINT_WIDTH);
                    outS2A<<strStdValue.rightJustified(CLASSIFICATIONPROJECT_VALUES_PRINT_WIDTH);
                }
            }
            if(spacecraftId.compare(CLASSIFICATIONPROJECT_L8_TAG)==0)
            {
                outL8<<"\n";
            }
            else if(spacecraftId.compare(CLASSIFICATIONPROJECT_S2A_TAG)==0)
            {
                outS2A<<"\n";
            }
            iterSpacecraft++;
        }
        iterROIs++;
    }
    reportL8File.close();
    reportS2AFile.close();
    return(true);
}

bool ClassificationProject::setFromFile(QString &inputFileName,
                                        QString &strError)
{
    clear();
    if(!QFile::exists(inputFileName))
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nNot exists input file:\n%1").arg(inputFileName);
        return(false);
    }
    QFile fileInput(inputFileName);
    if (!fileInput.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
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

    // Lectura del fichero informe de resultados
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
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

    // Lectura de la ruta de resultados
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
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
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nError making results path:\n%1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }
    }
    mResultsPath=strValue;

    // Lectura del fichero de base de datos del proyecto
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    QString projectDbFileName=strValue;

    // Lectura del valor a incrementar al id
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    if(strValue.isEmpty())
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nGID field is empty");
        fileInput.close();
        clear();
        return(false);
    }
    okToInt=false;
    mROIIncrement=strValue.toInt(&okToInt);
    if(!okToInt)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nRoi increment: %1 is not an integer").arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }

    // Lectura de proceso para crear la base de datos
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    okToInt=false;
    bool createProjectDb=false;
    int intCreateDb=strValue.toInt(&okToInt);
    if(!okToInt)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nIn line %1 value: %2 is not an integer").arg(QString::number(nline)).arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    if(intCreateDb<0||intCreateDb>1)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nIn line %1 value: %2 is not 0 or 1").arg(QString::number(nline)).arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    if(intCreateDb==1)
    {
        createProjectDb=true;
    }

    int sridROIsShapefile;
    QVector<int> roisGids;
    QVector<int> roisCrops;
    QVector<OGRGeometry*> roisPtrGeometries;

    if(createProjectDb)
    {
        // Lectura del fichero de base de datos original
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(!QFile::exists(strValue))
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nScenes database file not exists:\n%1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }
        QString originalDbFileName=strValue;
        if(QFile::exists(projectDbFileName))
        {
            if(!QFile::remove(projectDbFileName))
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nError removing existing scenes database copy file:\n%1").arg(projectDbFileName);
                fileInput.close();
                clear();
                return(false);
            }
        }
        if(!QFile::copy(originalDbFileName,projectDbFileName))
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nError copying intial scenes database file:\n%1\nto file:\n%2").arg(originalDbFileName).arg(projectDbFileName);
            fileInput.close();
            clear();
            return(false);
        }

        // Se ignora la siguiente linea
        nline++;
        strLine=in.readLine();

        // Lectura del fichero shapefile de ROIs
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(!QFile::exists(strValue))
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
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
            strError=QObject::tr("ClassificationProject::setFromFile");
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
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nInvalid geometry type in ROIs shapefile from file:\n%1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }
        if(!mPtrROIsShapefile->existsCrs())
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nNo CRS information in ROIs shapefile from file:\n%1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }
        sridROIsShapefile=mPtrROIsShapefile->getCrsEpsgCode();
        int numberOfROIs=mPtrROIsShapefile->getNumberOfFeatures();
        if(numberOfROIs<1)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are no features in ROIs shapefile from file:\n%1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }

        // Lectura del campo GID
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(strValue.isEmpty())
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nGID field is empty");
            fileInput.close();
            clear();
            return(false);
        }
        QString gidFieldName=strValue;

        // Se ignora la siguiente linea
        nline++;
        strLine=in.readLine();

        // Lectura del campo tipo de cultivo
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(strValue.isEmpty())
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nCrop field is empty");
            fileInput.close();
            clear();
            return(false);
        }
        QString cropFieldName=strValue;

        // Lectura de los codigos de tipo de cultivo
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(strValue.isEmpty())
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nCrop values is empty");
            fileInput.close();
            clear();
            return(false);
        }
        QVector<int> validCropTypes;
        if(strValue.compare(CLASSIFICATIONPROJECT_CROP_VALUES_ALL,Qt::CaseInsensitive)!=0)
        {
            QStringList strValues=strValue.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);
            for(int nv=0;nv<strValues.size();nv++)
            {
                QString strCropValue=strValues.at(nv);
                okToInt=false;
                int cropType=strCropValue.toInt(&okToInt);
                if(!okToInt)
                {
                    strError=QObject::tr("ClassificationProject::setFromFile");
                    strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                    strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                    strError+=QObject::tr("\nCrop valid value: %1 is not an integer").arg(strCropValue);
                    fileInput.close();
                    clear();
                    return(false);
                }
                validCropTypes.push_back(cropType);
            }
        }
        QStringList roisFieldNames;
        roisFieldNames.push_back(gidFieldName);
        roisFieldNames.push_back(cropFieldName);
        QVector<QStringList> roisFieldsValues;
        QVector<OGRGeometry*> ptrROIsGeometries; // son clonadas
        if(!mPtrROIsShapefile->getFeaturesValues(roisFieldNames,
                                                 roisFieldsValues,
                                                 ptrROIsGeometries,
                                                 strAuxError))
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nError getting gids and geometries in ROIs shapefile from file:\n%1").arg(strValue);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            fileInput.close();
            clear();
            return(false);
        }
        for(int nf=0;nf<ptrROIsGeometries.size();nf++)
        {
            QString strRoiGid=roisFieldsValues[nf][0].trimmed();
            okToInt=false;
            int roiGid=strRoiGid.toInt(&okToInt);
            if(!okToInt)
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nGID: %1 is not an integer").arg(strRoiGid);
                fileInput.close();
                clear();
                return(false);
            }
            roiGid+=mROIIncrement;
            QString strRoiCrop=roisFieldsValues[nf][1].trimmed();
            okToInt=false;
            int roiCrop=strRoiCrop.toInt(&okToInt);
            if(!okToInt)
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nCrop type: %1 is not an integer").arg(strRoiGid);
                fileInput.close();
                clear();
                return(false);
            }
            if(validCropTypes.size()>0)
            {
                if(validCropTypes.indexOf(roiCrop)==-1) // se ignoran los que no son tipos de cultivo valido
                {
                    continue;
                }
            }
            OGRGeometry* ptrROIGeometry=ptrROIsGeometries[nf];
            roisGids.push_back(roiGid);
            roisCrops.push_back(roiCrop);
            roisPtrGeometries.push_back(ptrROIGeometry);
        }
    }
    else
    {
        int numberOfLinesToIgnore=7;
        for(int nlti=0;nlti<numberOfLinesToIgnore;nlti++)
        {
            nline++;
            strLine=in.readLine();
        }
        if(!QFile::exists(projectDbFileName))
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nNot exists project database file:\n%1").arg(projectDbFileName);
            fileInput.close();
            clear();
            return(false);
        }
    }

    // Lectura de proceso para crear la datos para clasificacion
    nline++;
    strLine=in.readLine();
    strLine=strLine.trimmed();
    strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
    if(strList.size()!=2)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        fileInput.close();
        clear();
        return(false);
    }
    strValue=strList.at(1).trimmed();
    okToInt=false;
    bool createClassificationData=false;
    int intCreateClassificationData=strValue.toInt(&okToInt);
    if(!okToInt)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nIn line %1 value: %2 is not an integer").arg(QString::number(nline)).arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    if(intCreateClassificationData<0||intCreateClassificationData>1)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
        strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
        strError+=QObject::tr("\nIn line %1 value: %2 is not 0 or 1").arg(QString::number(nline)).arg(strValue);
        fileInput.close();
        clear();
        return(false);
    }
    if(intCreateClassificationData==1)
    {
        createClassificationData=true;
    }

    if(createClassificationData)
    {
        // Lectura de la fecha inicial y de la fecha final
        int minimumValueDate=QDate::fromString(CLASSIFICATIONPROJECT_DATE_MINIMUM_VALUE,CLASSIFICATIONPROJECT_DATE_FORMAT_1).toJulianDay();
        int maximumValueDate=QDate::fromString(CLASSIFICATIONPROJECT_DATE_MAXIMUM_VALUE,CLASSIFICATIONPROJECT_DATE_FORMAT_1).toJulianDay();
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        strAuxList=strValue.split(CLASSIFICATIONPROJECT_DATES_SEPARATOR_CHARACTER);
        if(strAuxList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nIn dates string: %1\nthere are not two dates separated by %2")
                    .arg(strValue).arg(CLASSIFICATIONPROJECT_DATES_SEPARATOR_CHARACTER);
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
            strError=QObject::tr("ClassificationProject::setFromFile");
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
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nInitial date, %1, is out of valid domain:[%2,%3]")
                        .arg(strInitialDate).arg(CLASSIFICATIONPROJECT_DATE_MINIMUM_VALUE).arg(CLASSIFICATIONPROJECT_DATE_MAXIMUM_VALUE);
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
            strError=QObject::tr("ClassificationProject::setFromFile");
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
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("Final date, %1, is out of valid domain:[%2,%3]")
                        .arg(strFinalDate).arg(CLASSIFICATIONPROJECT_DATE_MINIMUM_VALUE).arg(CLASSIFICATIONPROJECT_DATE_MAXIMUM_VALUE);
                fileInput.close();
                clear();
                return(false);
            }
        }
        if(mInitialJd>=mFinalJd)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("Initial date, %1, is not greather than final date:%2")
                    .arg(strInitialDate).arg(strFinalDate);
            fileInput.close();
            clear();
            return(false);
        }

        // Lectura del umbral de desviacion tipica para deteccion de errores groseros, si es 0 no se aplica
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        okToDouble=false;
        mStdThreshold=strValue.toDouble(&okToDouble);
        if(!okToDouble)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("Parameter std threshold is not a double: %1").arg(strValue);
            fileInput.close();
            clear();
            return(false);
        }

        // Lectura del identificador para escenas ndvi de L8
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(QFile::exists(strValue))
        {
            if(!QFile::remove(strValue))
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nError removing existing L8 results file:\n%1").arg(strValue);
                fileInput.close();
                clear();
                return(false);
            }
        }
        mL8SpacecraftIdentifier=strValue;

        // Lectura del identificador para escenas ndvi de S2A
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(QFile::exists(strValue))
        {
            if(!QFile::remove(strValue))
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nError removing existing L8 results file:\n%1").arg(strValue);
                fileInput.close();
                clear();
                return(false);
            }
        }
        mS2ASpacecraftIdentifier=strValue;

        // Lectura del fichero de resultados para L8
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(QFile::exists(strValue))
        {
            if(!QFile::remove(strValue))
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nError removing existing L8 results file:\n%1").arg(strValue);
                fileInput.close();
                clear();
                return(false);
            }
        }
        mL8ReportFileName=strValue;

        // Lectura del fichero de resultados para S2
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        strList=strLine.split(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
        if(strList.size()!=2)
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(CLASSIFICATIONPROJECT_SEPARATOR_CHARACTER);
            fileInput.close();
            clear();
            return(false);
        }
        strValue=strList.at(1).trimmed();
        if(QFile::exists(strValue))
        {
            if(!QFile::remove(strValue))
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nError removing existing S2 results file:\n%1").arg(strValue);
                fileInput.close();
                clear();
                return(false);
            }
        }
        mS2AReportFileName=strValue;
    }
    else
    {
        int numberOfLinesToIgnore=4;
        for(int nlti=0;nlti<numberOfLinesToIgnore;nlti++)
        {
            nline++;
            strLine=in.readLine();
        }
    }
    fileInput.close();

    if(!createProjectDb
            &&!createClassificationData)
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nThere is not any proccess to do");
        fileInput.close();
        clear();
        return(false);
    }
    mProcessCreateClassificationData=createClassificationData;

    mPtrPersistenceManager = new RemoteSensing::PersistenceManager();
    QDir programDir=qApp->applicationDirPath();
    QString programPath=programDir.absolutePath();
    if(!mPtrPersistenceManager->initializeAlgorithms(mPtrCrsTools,
                                                     mPtrNestedGridTools,
                                                     mPtrLibIGDALProcessMonitor,
                                                     programPath,
                                                     strAuxError))
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError initializing database:\n%1\nError:\n%2").arg(projectDbFileName).arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    if(!mPtrPersistenceManager->openDatabase(projectDbFileName,strError))
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError setting database:\n%1\nError:\n%2").arg(projectDbFileName).arg(strAuxError);
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
        strError=QObject::tr("ClassificationProject::setFromFile");
        strError+=QObject::tr("\nError setting CRS in NestedGridTools: %1").arg(strAuxError);
        fileInput.close();
        clear();
        return(false);
    }
    if(!mPtrNestedGridTools->setGeographicBaseCrs(geographicCrsBaseProj4Text,
                                                  strAuxError))
    {
        strError=QObject::tr("ClassificationProject::setFromFile");
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
            strError=QObject::tr("ClassificationProject::setFromFile");
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
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError setting Whole Earth Parameters:\n%1").arg(strAuxError);
            fileInput.close();
            clear();
            return(false);
        }
    }

    if(createProjectDb)
    {
        if(mFromConsole)
        {
            QString msg=QObject::tr("    ... Inserting ROIs data from shapefile");
            (*mStdOut) <<msg<< endl;
        }
        QString createCropTableSqlFileName=programPath+CLASSIFICATIONPROJECT_CREATE_ROISTABLE_SQL_FILENAME;
        if(!QFile::exists(createCropTableSqlFileName))
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nNot exists file: %1").arg(createCropTableSqlFileName);
            clear();
            return(false);
        }
        QFile sqlFile(createCropTableSqlFileName);
        if(!sqlFile.open(QIODevice::ReadOnly))
        {
            strError=QObject::tr("ClassificationProject::setFromFile");
            strError+=QObject::tr("\nError reading sql file:\n%1").arg(createCropTableSqlFileName);
            clear();
            return(false);
        }
        QTextStream sqlIn(&sqlFile);
        QString sqlSentence;
        while(!sqlIn.atEnd())
        {
            QString line = sqlIn.readLine().trimmed();
            if(line.trimmed().isEmpty())
                continue;
            sqlSentence+=line;
            if(line.contains(";"))
            {
                QVector<QString> fieldsNamesToRetrieve;
                QVector<QMap<QString,QString> > fieldsValuesToRetrieve;
    //            if(sqlSentence.contains(SPATIALITE_CRS_PROJ4TEXT_VALUE))
    //            {
    //                sqlSentence=sqlSentence.replace(SPATIALITE_CRS_PROJ4TEXT_VALUE,proj4Text);
    //            }
                if(sqlSentence.contains(SPATIALITE_CRS_SRID_TAG_VALUE))
                {
                    sqlSentence=sqlSentence.replace(SPATIALITE_CRS_SRID_TAG_VALUE,QString::number(sridROIsShapefile));
                    fieldsNamesToRetrieve.append("result");
                }
                if(!mPtrPersistenceManager->executeSql(sqlSentence,
                                                       fieldsNamesToRetrieve,
                                                       fieldsValuesToRetrieve,
                                                       strAuxError))
                {
                    strError=QObject::tr("ClassificationProject::setFromFile");
                    strError+=QObject::tr("\nIn spatialite database: %1\nIn sql:\n%2\nError: %3")
                            .arg(mPtrPersistenceManager->getDatabaseFileName())
                            .arg(sqlSentence).arg(strAuxError);
                    clear();
                    return(false);
                }
                sqlSentence="";
            }
        }
        sqlFile.close();

        for(int nRoi=0;nRoi<roisGids.size();nRoi++)
        {
            int roiGid=roisGids.at(nRoi);
            int roiCrop=roisCrops.at(nRoi);
            OGRGeometry* ptrROIGeometry=roisPtrGeometries.at(nRoi);
            char *charsWktGeometry;
            if(OGRERR_NONE!=ptrROIGeometry->exportToWkt(&charsWktGeometry))
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
                strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
                strError+=QObject::tr("\nError exporting to wkt geometry in ROIs shapefile from file:\n%1").arg(strValue);
                clear();
                return(false);
            }
            QString roiWktGeometry=QString::fromLatin1(charsWktGeometry);
            QString sqlSentence="INSERT INTO ";
            sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
            sqlSentence+=" (";
            sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_ID;
            sqlSentence+=",";
            sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_CROP;
            sqlSentence+=",";
            sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_GEOMETRY;
            sqlSentence+=") VALUES (";
            sqlSentence+=QString::number(roiGid);
            sqlSentence+=",";
            sqlSentence+=QString::number(roiCrop);
            sqlSentence+=",";
            sqlSentence+="GeomFromText('";
            sqlSentence+=roiWktGeometry;
            sqlSentence+="',";
            sqlSentence+=QString::number(sridROIsShapefile);
            sqlSentence+="));";
            if(sqlSentence.contains("POLYGON (("))
            {
                sqlSentence=sqlSentence.replace("POLYGON ((","POLYGON((");
            }
            QVector<QString> fieldsNamesToRetrieve;
            QVector<QMap<QString,QString> > fieldsValuesToRetrieve;
            if(!mPtrPersistenceManager->executeSql(sqlSentence,
                                                   fieldsNamesToRetrieve,
                                                   fieldsValuesToRetrieve,
                                                   strAuxError))
            {
                strError=QObject::tr("ClassificationProject::setFromFile");
                strError+=QObject::tr("\nIn spatialite database: %1\nIn sql:\n%2\nError: %3")
                        .arg(mPtrPersistenceManager->getDatabaseFileName())
                        .arg(sqlSentence).arg(strAuxError);
                clear();
                return(false);
            }
        }
    }
    mIsInitialized=true;

    return(true);
}

void ClassificationProject::clear()
{
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
    mReportFileName.clear();
//    mOutputRasterFileName.clear();
    mResultsPath.clear();
//    mROIsGids.clear();
//    mROIsCrops.clear();
//    mROIsPtrGeometries.clear();
    mIsInitialized=false;
    mStdThreshold=CLASSIFICATIONPROJECT_NODOUBLE;
    mL8ReportFileName.clear();
    mS2AReportFileName.clear();
    mProcessCreateClassificationData=false;
    mS2ASpacecraftIdentifier.clear();
    mL8SpacecraftIdentifier.clear();
    mROIIncrement=0;
    //    mProjectCodes.clear();
}

bool ClassificationProject::getROITuplekeyIntersectionGeomety(int roiId,
                                                              int tuplekeyId,
                                                              OGRGeometry *&ptrGeometry,
                                                              bool& roiContainsTuplekey,
                                                              QString &strError)
{
    ptrGeometry=NULL;
    QString sqlSentence;
    // Primer compruebo si la zona contiene al tuplekey
    roiContainsTuplekey=false;
    {
        sqlSentence+="SELECT contains(";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_GEOMETRY;
        sqlSentence+=",";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM;
        sqlSentence+=") FROM ";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence+=",";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=" WHERE ";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_ID;
        sqlSentence+="=";
        sqlSentence+=QString::number(roiId);
        sqlSentence+=" AND ";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
        sqlSentence+="=";
        sqlSentence+=QString::number(tuplekeyId);
        QVector<QString> fieldsNamesToRetrieve;
        QString containsFieldName="contains";
        fieldsNamesToRetrieve.push_back(containsFieldName);
        QVector<QMap<QString, QString> > fieldsValuesToRetrieve;
        QString strAuxError;
        if(!mPtrPersistenceManager->getPtrDb()->executeSqlQuery(sqlSentence,
                                                                fieldsNamesToRetrieve,
                                                                fieldsValuesToRetrieve,
                                                                strAuxError))
        {
            strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
            strError+=QObject::tr("\nError recovering data for project id: %1 an tuplekey id: %2")
                    .arg(QString::number(roiId)).arg(QString::number(tuplekeyId));
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
        {
            QString strContains=fieldsValuesToRetrieve[nr][containsFieldName];
            int intContains=strContains.toInt();
            if(intContains==1)
            {
                roiContainsTuplekey=true;
            }
        }
    }
    if(!roiContainsTuplekey)
    {
        sqlSentence="SELECT upper(asText(intersection(";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_GEOMETRY;
        sqlSentence+=",";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM;
        sqlSentence+=")))";
        sqlSentence+=" FROM ";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence+=",";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=" WHERE ";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_ID;
        sqlSentence+="=";
        sqlSentence+=QString::number(roiId);
        sqlSentence+=" AND ";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
        sqlSentence+="=";
        sqlSentence+=QString::number(tuplekeyId);
        QVector<QString> fieldsNamesToRetrieve;
        QString wktGeometryFieldName="wktGeometry";
        fieldsNamesToRetrieve.push_back(wktGeometryFieldName);
        QVector<QMap<QString, QString> > fieldsValuesToRetrieve;
        QString strAuxError;
        if(!mPtrPersistenceManager->getPtrDb()->executeSqlQuery(sqlSentence,
                                                                fieldsNamesToRetrieve,
                                                                fieldsValuesToRetrieve,
                                                                strAuxError))
        {
            strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
            strError+=QObject::tr("\nError recovering data for project id: %1 an tuplekey id: %2")
                    .arg(QString::number(roiId)).arg(QString::number(tuplekeyId));
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
        {
            QString wktGeometry=fieldsValuesToRetrieve[nr][wktGeometryFieldName];
            bool validGeometry=false;
            QByteArray byteArrayWktGeometry = wktGeometry.toUtf8();
            char *charsWktGeometry = byteArrayWktGeometry.data();
            if(wktGeometry.contains("POLYGON"))
            {
                ptrGeometry=OGRGeometryFactory::createGeometry(wkbPolygon);
                validGeometry=true;
            }
            if(wktGeometry.contains("MULTIPOLYGON"))
            {
                ptrGeometry=OGRGeometryFactory::createGeometry(wkbMultiPolygon);
                validGeometry=true;
            }
            if(!validGeometry)
            {
                strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
                strError+=QObject::tr("\nError recovering data for project id: %1 an tuplekey id: %2")
                        .arg(QString::number(roiId)).arg(QString::number(tuplekeyId));
                strError+=QObject::tr("\nInvalid wkt geometry\n%1").arg(wktGeometry);
                return(false);
            }
            if(OGRERR_NONE!=ptrGeometry->importFromWkt(&charsWktGeometry))
            {
                strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
                strError+=QObject::tr("\nError recovering data for project id: %1 an tuplekey id: %2")
                        .arg(QString::number(roiId)).arg(QString::number(tuplekeyId));
                strError+=QObject::tr("\nError making geometry from WKT: %1").arg(wktGeometry);
                return(false);
            }
        }
    }
    else
    {
        sqlSentence="SELECT upper(asText(";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM;
        sqlSentence+="))";
        sqlSentence+=" FROM ";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=" WHERE ";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence+=".";
        sqlSentence+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
        sqlSentence+="=";
        sqlSentence+=QString::number(tuplekeyId);
        QVector<QString> fieldsNamesToRetrieve;
        QString wktGeometryFieldName="wktGeometry";
        fieldsNamesToRetrieve.push_back(wktGeometryFieldName);
        QVector<QMap<QString, QString> > fieldsValuesToRetrieve;
        QString strAuxError;
        if(!mPtrPersistenceManager->getPtrDb()->executeSqlQuery(sqlSentence,
                                                                fieldsNamesToRetrieve,
                                                                fieldsValuesToRetrieve,
                                                                strAuxError))
        {
            strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
            strError+=QObject::tr("\nError recovering data for tuplekey id: %1")
                    .arg(QString::number(tuplekeyId));
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
        {
            QString wktGeometry=fieldsValuesToRetrieve[nr][wktGeometryFieldName];
            bool validGeometry=false;
            QByteArray byteArrayWktGeometry = wktGeometry.toUtf8();
            char *charsWktGeometry = byteArrayWktGeometry.data();
            if(wktGeometry.contains("POLYGON"))
            {
                ptrGeometry=OGRGeometryFactory::createGeometry(wkbPolygon);
                validGeometry=true;
            }
            if(wktGeometry.contains("MULTIPOLYGON"))
            {
                ptrGeometry=OGRGeometryFactory::createGeometry(wkbMultiPolygon);
                validGeometry=true;
            }
            if(!validGeometry)
            {
                strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
                strError+=QObject::tr("\nError recovering data for project id: %1 an tuplekey id: %2")
                        .arg(QString::number(roiId)).arg(QString::number(tuplekeyId));
                strError+=QObject::tr("\nInvalid wkt geometry\n%1").arg(wktGeometry);
                return(false);
            }
            if(OGRERR_NONE!=ptrGeometry->importFromWkt(&charsWktGeometry))
            {
                strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
                strError+=QObject::tr("\nError recovering data for project id: %1 an tuplekey id: %2")
                        .arg(QString::number(roiId)).arg(QString::number(tuplekeyId));
                strError+=QObject::tr("\nError making geometry from WKT: %1").arg(wktGeometry);
                return(false);
            }
        }
    }
    return(true);
}

void ClassificationProject::multiProcessFinished()
{
    QString strError;
    if(mRemoveRubbish)
    {
        if(!removeRubbish(strError))
            (*mStdOut) <<strError<< endl;
    }
    exit(0);
}

bool ClassificationProject::removeDir(QString dirName)
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

bool ClassificationProject::removeRubbish(QString &strError)
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

bool ClassificationProject::getNdviDataByTuplekeyByRoi(int initialJd,
                                                       int finalJd,
                                                       QMap<QString, QMap<int, QMap<int, QVector<QString> > > > &tuplekeyFileNamesByTuplekeyByRoiCodeByJd,
                                                       QMap<int,int>& roiCropByRoi,
                                                       QMap<QString,int>& tuplekeyIdByTuplekeyCode,
                                                       QMap<QString, double> &gainByTuplekeyFileName,
                                                       QMap<QString, double> &offsetByTuplekeyFileName,
                                                       QMap<QString, int> &lodTilesByTuplekeyFileName,
                                                       QMap<QString, int> &lodGsdByTuplekeyFileName,
                                                       QMap<QString, QString> &rasterIdByTuplekeyFileName,
                                                       int &maximumLod,
                                                       QString &strError)
{
    QString strAuxError;
    tuplekeyFileNamesByTuplekeyByRoiCodeByJd.clear();
    roiCropByRoi.clear();
    tuplekeyIdByTuplekeyCode.clear();
    gainByTuplekeyFileName.clear();
    offsetByTuplekeyFileName.clear();
    lodTilesByTuplekeyFileName.clear();
    lodGsdByTuplekeyFileName.clear();
    maximumLod=-1;
    // Get ROIs data
    QString tableName=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
    QVector<QString> fieldsNames;
    fieldsNames.push_back(CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_ID);
    fieldsNames.push_back(CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_CROP);
    QVector<QVector<QString> > fieldsValues;
    QVector<QString> whereFieldsNames;
    QVector<QString> whereFieldsValues;
    QVector<QString> whereFieldsTypes;
    IGDAL::SpatiaLite* mPtrDb=mPtrPersistenceManager->getPtrDb();
    if(!mPtrDb->select(tableName,fieldsNames,fieldsValues,
                       whereFieldsNames,whereFieldsValues,whereFieldsTypes,
                       strAuxError))
    {
        strError=QObject::tr("ClassificationProject::getNdviDataByTuplekeyByRoi");
        strError+=QObject::tr("\nError getting ROIs data from database:\n%1\nError:\n%2")
                .arg(mPtrDb->getFileName()).arg(strAuxError);
        return(false);
    }
    for(int i=0;i<fieldsValues.size();i++)
    {
        int roiGid=fieldsValues[i][0].toInt();
        int roiCrop=fieldsValues[i][1].toInt();
        roiCropByRoi[roiGid]=roiCrop;
    }
    if(roiCropByRoi.size()==0)
    {
        return(true);
    }
    QMap<int,int>::const_iterator iterROIs=roiCropByRoi.begin();
    int contROIs=0;
    while(iterROIs!=roiCropByRoi.end())
    {
        int roiGid=iterROIs.key();
        contROIs++;
        if(mFromConsole)
        {
            QString msg=QObject::tr("            ... Recovering data for ROI: %1, there are %2 ROIs left")
                    .arg(QString::number(roiGid)).arg(QString::number(roiCropByRoi.size()-contROIs));
            (*mStdOut) <<msg<< endl;
        }
//        SELECT distinct(ndvi_files.file_name),tuplekeys.tuplekey,raster_unit_conversions.gain,raster_unit_conversions.offset,
//        raster_files.jd,tuplekeys.id,ndvi_files.lod_tiles,ndvi_files.lod_gsd
//        FROM ndvi_files,raster_files,tuplekeys,rois_classification,raster_unit_conversions
//        WHERE ndvi_files.raster_file_id=raster_files.id
//        AND ndvi_files.tuplekey_id=tuplekeys.id
//        AND ndvi_files.ruc_id=raster_unit_conversions.id
//        AND raster_files.jd>=2457814
//        AND raster_files.jd<=2458058 AND tuplekeys.id
//         IN (SELECT tuplekeys.id FROM tuplekeys,rois_classification
//          WHERE INTERSECTS(tuplekeys.the_geom,rois_classification.the_geom) AND rois_classification.id=0) ORDER BY raster_files.jd
        QString sqlSentence1;
        sqlSentence1+="SELECT distinct(";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_FILE_NAME;
        sqlSentence1+="),";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD;
//        sqlSentence1+=",";
//        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
//        sqlSentence1+=".";
//        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_LOD;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_LOD_TILES;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_LOD_GSD;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
        sqlSentence1+=" FROM ";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence1+=",";
        sqlSentence1+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence1+=",";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS;
        sqlSentence1+=" WHERE ";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID;
        sqlSentence1+="=";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID;
        sqlSentence1+=" AND ";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID;
        sqlSentence1+="=";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
        sqlSentence1+=" AND ";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID;
        sqlSentence1+="=";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_ID;
        sqlSentence1+=" AND ";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD;
        sqlSentence1+=">=";
        sqlSentence1+=QString::number(initialJd);
        sqlSentence1+=" AND ";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD;
        sqlSentence1+="<=";
        sqlSentence1+=QString::number(finalJd);
        sqlSentence1+=" AND ";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence1+=".";
        sqlSentence1+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
        QString sqlSentence2=" IN (SELECT ";
        sqlSentence2+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence2+=".";
        sqlSentence2+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
        sqlSentence2+=" FROM ";
        sqlSentence2+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence2+=",";
        sqlSentence2+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence2+=" WHERE INTERSECTS(";
        sqlSentence2+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        sqlSentence2+=".";
        sqlSentence2+=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM;
        sqlSentence2+=",";
        sqlSentence2+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence2+=".";
        sqlSentence2+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_GEOMETRY;
        sqlSentence2+=")";
        sqlSentence2+=" AND ";
        sqlSentence2+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
        sqlSentence2+=".";
        sqlSentence2+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_ID;
        sqlSentence2+="=";
        sqlSentence2+=QString::number(roiGid);
        sqlSentence2+=")";
        sqlSentence2+=" ORDER BY ";
        sqlSentence2+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        sqlSentence2+=".";
        sqlSentence2+=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD;
        QString sqlSentence=sqlSentence1+sqlSentence2;
        // select nf.file_name,t.tuplekey,ruc.gain,ruc.offset,rf.jd
        QVector<QString> fieldsNamesToRetrieve;
        QString fileNameFieldName="file_name";
        QString tuplekeyFieldName="tuplekey";
        QString gainFieldName="gain";
        QString offsetFieldName="offset";
        QString jdFieldName="jd";
        QString lodFieldName="lod";
        QString tuplekeyIdFieldName="tuplekeyId";
        QString lodTilesFieldName="lodTiles";
        QString lodGsdFieldName="lodGsd";
        QString rasterIdFieldName="rasterId";
        fieldsNamesToRetrieve.push_back(fileNameFieldName);
        fieldsNamesToRetrieve.push_back(tuplekeyFieldName);
        fieldsNamesToRetrieve.push_back(gainFieldName);
        fieldsNamesToRetrieve.push_back(offsetFieldName);
        fieldsNamesToRetrieve.push_back(jdFieldName);
//        fieldsNamesToRetrieve.push_back(lodFieldName);
        fieldsNamesToRetrieve.push_back(tuplekeyIdFieldName);
        fieldsNamesToRetrieve.push_back(lodTilesFieldName);
        fieldsNamesToRetrieve.push_back(lodGsdFieldName);
        fieldsNamesToRetrieve.push_back(rasterIdFieldName);
        QVector<QMap<QString, QString> > fieldsValuesToRetrieve;
        if(!mPtrDb->executeSqlQuery(sqlSentence,
                                    fieldsNamesToRetrieve,
                                    fieldsValuesToRetrieve,
                                    strAuxError))
        {
            strError=QObject::tr("ClassificationProject::getNdviDataByTuplekeyByRoi");
            strError+=QObject::tr("\nError recovering data for project: %1").arg(QString::number(roiGid));
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        else if(fieldsValuesToRetrieve.size()==0)
        {
            strError=QObject::tr("ClassificationProject::getNdviDataByTuplekeyByRoi");
            strError+=QObject::tr("\nError recovering data for project: %1").arg(QString::number(roiGid));
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
        {
            QString fileName=fieldsValuesToRetrieve[nr][fileNameFieldName];
            QString tuplekey=fieldsValuesToRetrieve[nr][tuplekeyFieldName];
            double gain=fieldsValuesToRetrieve[nr][gainFieldName].toDouble();
            double offset=fieldsValuesToRetrieve[nr][offsetFieldName].toDouble();
            int jd=fieldsValuesToRetrieve[nr][jdFieldName].toInt();
//            int lod=fieldsValuesToRetrieve[nr][lodFieldName].toInt();
            int tuplekeyId=fieldsValuesToRetrieve[nr][tuplekeyIdFieldName].toInt();
            int lodTiles=fieldsValuesToRetrieve[nr][lodTilesFieldName].toInt();
            int lodGsd=fieldsValuesToRetrieve[nr][lodGsdFieldName].toInt();
            QString rasterId=fieldsValuesToRetrieve[nr][rasterIdFieldName];
            if(!tuplekeyFileNamesByTuplekeyByRoiCodeByJd[tuplekey][roiGid].contains(jd))
            {
                QVector<QString> auxVector;
                tuplekeyFileNamesByTuplekeyByRoiCodeByJd[tuplekey][roiGid][jd]=auxVector;
            }
            tuplekeyFileNamesByTuplekeyByRoiCodeByJd[tuplekey][roiGid][jd].push_back(fileName);
            tuplekeyIdByTuplekeyCode[tuplekey]=tuplekeyId;
            if(!gainByTuplekeyFileName.contains(fileName))
            {
                gainByTuplekeyFileName[fileName]=gain;
                offsetByTuplekeyFileName[fileName]=offset;
            }
            if(!lodTilesByTuplekeyFileName.contains(fileName))
            {
                lodTilesByTuplekeyFileName[fileName]=lodTiles;
                if(lodTiles>maximumLod)
                {
                    maximumLod=lodTiles;
                }
                lodGsdByTuplekeyFileName[fileName]=lodGsd;
            }
            rasterIdByTuplekeyFileName[fileName]=rasterId;
        }
        iterROIs++;
    }
    return(true);
}

bool ClassificationProject::getIntersectsPixel(double rasterNwFc,
                                               double rasterNwSc,
                                               double rasterGsd,
                                               int col,
                                               int row,
                                               OGRGeometry* ptrROIGeometry,
                                               bool &intersects,
                                               QString &strError)
{
    double firstCoordinate=rasterNwFc+col*rasterGsd+0.5*rasterGsd;
    double secondCoordinate=rasterNwSc-row*rasterGsd-0.5*rasterGsd;
    QString wktPointGeometry="POINT(";
    wktPointGeometry+=QString::number(firstCoordinate,'f',12);
    wktPointGeometry+=" ";
    wktPointGeometry+=QString::number(secondCoordinate,'f',12);
    wktPointGeometry+=")";
    QByteArray byteArrayWktGeometry = wktPointGeometry.toUtf8();
    char *charsWktGeometry = byteArrayWktGeometry.data();
    OGRGeometry* ptrPointGeometry;
    ptrPointGeometry=OGRGeometryFactory::createGeometry(wkbPoint);
    if(OGRERR_NONE!=ptrPointGeometry->importFromWkt(&charsWktGeometry))
    {
        strError=QObject::tr("ClassificationProject::getRoiIntersectsPixel");
        strError+=QObject::tr("\nError making point for column: %1 and row: %2")
                .arg(QString::number(col)).arg(QString::number(row));
        return(false);
    }
    intersects=false;
    if(ptrROIGeometry->Contains(ptrPointGeometry))
    {
        OGRGeometryFactory::destroyGeometry(ptrPointGeometry);
        intersects=true;
    }
    else
    {
        double distance=ptrPointGeometry->Distance(ptrROIGeometry);
        OGRGeometryFactory::destroyGeometry(ptrPointGeometry);
        if(distance<=(0.5*rasterGsd*sqrt(2.0)))
        {
            double pixelNwFc=rasterNwFc+rasterGsd*col;
            double pixelNwSc=rasterNwSc-rasterGsd*row;
            double pixelSeFc=pixelNwFc+rasterGsd;
            double pixelSeSc=pixelNwSc-rasterGsd;
            QString pixelWktGeometry="POLYGON((";
            pixelWktGeometry+=QString::number(pixelNwFc,'f',12);
            pixelWktGeometry+=" ";
            pixelWktGeometry+=QString::number(pixelNwSc,'f',12);
            pixelWktGeometry+=",";
            pixelWktGeometry+=QString::number(pixelSeFc,'f',12);
            pixelWktGeometry+=" ";
            pixelWktGeometry+=QString::number(pixelNwSc,'f',12);
            pixelWktGeometry+=",";
            pixelWktGeometry+=QString::number(pixelSeFc,'f',12);
            pixelWktGeometry+=" ";
            pixelWktGeometry+=QString::number(pixelSeSc,'f',12);
            pixelWktGeometry+=",";
            pixelWktGeometry+=QString::number(pixelNwFc,'f',12);
            pixelWktGeometry+=" ";
            pixelWktGeometry+=QString::number(pixelSeSc,'f',12);
            pixelWktGeometry+=",";
            pixelWktGeometry+=QString::number(pixelNwFc,'f',12);
            pixelWktGeometry+=" ";
            pixelWktGeometry+=QString::number(pixelNwSc,'f',12);
            pixelWktGeometry+="))";
            QByteArray byteArrayPixelWktGeometry = pixelWktGeometry.toUtf8();
            char *charsPixelWktGeometry = byteArrayPixelWktGeometry.data();
            OGRGeometry* ptrPixelGeometry;
            ptrPixelGeometry=OGRGeometryFactory::createGeometry(wkbPolygon);
            if(OGRERR_NONE!=ptrPixelGeometry->importFromWkt(&charsPixelWktGeometry))
            {
                strError=QObject::tr("ClassificationProject::getRoiIntersectsPixel");
                strError+=QObject::tr("\nError making pixel for column: %1 and row: %2")
                        .arg(QString::number(col)).arg(QString::number(row));
                OGRGeometryFactory::destroyGeometry(ptrPixelGeometry);
                return(false);
            }
            if(ptrPixelGeometry->Intersects(ptrROIGeometry))
            {
                intersects=true;
            }
            OGRGeometryFactory::destroyGeometry(ptrPixelGeometry);
        }
    }
    return(true);
}

bool ClassificationProject::getROIArea(int roiId,
                                       double &area,
                                       QString &strError)
{
    QString sqlSentence;
    sqlSentence+="SELECT Area(";
    sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
    sqlSentence+=".";
    sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_GEOMETRY;
    sqlSentence+=") FROM ";
    sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
    sqlSentence+=" WHERE ";
    sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_NAME;
    sqlSentence+=".";
    sqlSentence+=CLASSIFICATIONPROJECT_ROIS_CLASSIFICATION_TABLE_FIELD_ID;
    sqlSentence+="=";
    sqlSentence+=QString::number(roiId);
    QVector<QString> fieldsNamesToRetrieve;
    QString fieldName="area";
    fieldsNamesToRetrieve.push_back(fieldName);
    QVector<QMap<QString, QString> > fieldsValuesToRetrieve;
    QString strAuxError;
    if(!mPtrPersistenceManager->getPtrDb()->executeSqlQuery(sqlSentence,
                                                            fieldsNamesToRetrieve,
                                                            fieldsValuesToRetrieve,
                                                            strAuxError))
    {
        strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
        strError+=QObject::tr("\nError recovering area for project id: %1").arg(QString::number(roiId));
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
    {
        QString strArea=fieldsValuesToRetrieve[nr][fieldName];
        bool oktoDbl=false;
        area=strArea.toDouble(&oktoDbl);
        if(!oktoDbl)
        {
            strError=QObject::tr("ClassificationProject::getROITuplekeyIntersectionGeomety");
            strError+=QObject::tr("\nError recovering area for project id: %1").arg(QString::number(roiId));
            strError+=QObject::tr("\nError:\nArea: %1 is not a dobule").arg(strArea);
            return(false);
        }
    }
    return(true);
}
