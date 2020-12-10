#include <ogr_geos.h>
#include <ogr_geometry.h>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QTime>
#include <QDateTime>

#include "../../libs/libPW/libPW.h"
#include "../../libs/libPW/Process.h"
#include "../../libs/libPW/ExternalProcess.h"
#include "../../libs/libPW/MultiProcess.h"

#include "Raster.h"
#include "CRSTools.h"
#include "NestedGridTools.h"
#include "NestedGridProject.h"
#include "SceneLandsat8.h"
#include "libIGDALProcessMonitor.h"
#include "nestedgrid_definitions.h"

using namespace RemoteSensing;

SceneLandsat8::SceneLandsat8(QString id,
                             libCRS::CRSTools *ptrCrsTools):
    Scene(id,
          ptrCrsTools)
{
    mGdalDataType=RASTER_GDALDATATYPE_NULL_VALUE;
    mStdOut = new QTextStream(stdout);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B1_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B2_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B3_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B4_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B5_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B6_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B7_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B8_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B9_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B10_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B11_CODE);
    mLandsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_BQA_CODE);
    for(int nb=0;nb<mLandsat8BandsCode.size();nb++)
    {
        mStrBandsCodeValidDomain+=mLandsat8BandsCode[nb];
        if(nb<(mLandsat8BandsCode.size()-1))
            mStrBandsCodeValidDomain+="-";
    }
    mSceneType=Landsat8;
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B1_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[0]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B2_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[1]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B3_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[2]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B4_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[3]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B5_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[4]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B6_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[5]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B7_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[6]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B8_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[7]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B9_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[8]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B10_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[9]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_B11_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[10]]=auxAlias;
    }
    {
        QVector<QString> auxAlias;
        auxAlias.push_back(REMOTESENSING_LANDSAT8_BAND_BQA_ALIAS_1_CODE);
        mLandsat8BandsCodeAliasByCode[mLandsat8BandsCode[11]]=auxAlias;
    }
}

void SceneLandsat8::manageProccesStdOutput(QString data)
{
    //*mStdOut<<data;
    fprintf(stdout, data.toLatin1());
}

void SceneLandsat8::manageProccesErrorOutput(QString data)
{
//   *mStdOut<<data;
    fprintf(stdout, data.toLatin1());
}

/*
bool SceneLandsat8::createMultibandFile(NestedGrid::NestedGridTools *ptrNestedGridTools,
                                        QVector<int> &landsat8BandsToUse,
                                        IGDAL::Raster *ptrMultibandRaster,
                                        QString &strError)
{
    if(ptrMultibandRaster!=NULL)
        delete(ptrMultibandRaster);

    return(true);
}
*/
bool SceneLandsat8::setFromUsgsFormat(QString& scenePath,
                                      QString& sceneBaseName,
                                      NestedGrid::NestedGridProject *ptrNestedGridProject,
                                      QString& strError)
{
    QString sceneMetadataFileName=scenePath+"/"+sceneBaseName+REMOTESENSING_LANDSAT8_USGS_FORMAT_METADATA_FILE_SUFFIX;
    sceneMetadataFileName+=REMOTESENSING_LANDSAT8_USGS_FORMAT_METADATA_FILE_EXTENSION;
    if(!QFile::exists(sceneMetadataFileName))
    {
        strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
        strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
        strError+=QObject::tr("\nnot exists metadata file:\n%1").arg(sceneMetadataFileName);
        return(false);
    }
    QString strAuxError;
    if(!setMetadataFromUsgsFormat(sceneMetadataFileName,
                                  strAuxError))
    {
        strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
        strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
        strError+=QObject::tr("\nError setting metadata from file:\n%1\nError:\n%2")
                .arg(sceneMetadataFileName).arg(strAuxError);
        return(false);
    }
    mPtrNestedGridProject=ptrNestedGridProject;
    if(!mPtrNestedGridProject->insertLandsat8MetadataFile(sceneMetadataFileName,
                                                          strAuxError))
    {
        strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
        strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
        strError+=QObject::tr("\nError inserting metadata file:\n%1\nError:\n%2")
                .arg(sceneMetadataFileName).arg(strAuxError);
        return(false);
    }
    mMetadataUsgsFormatFileName=sceneMetadataFileName;
    QString sceneCrsDescription;
    for(int nb=0;nb<mLandsat8BandsCode.size();nb++)
    {
        QString bandCode=mLandsat8BandsCode[nb];
        QString bandFileName=scenePath+"/"+sceneBaseName+REMOTESENSING_LANDSAT8_USGS_FORMAT_BAND_FILE_SUFFIX;
//        bandFileName+=QString::number(nb);
        bandFileName+=bandCode;
        bandFileName+=REMOTESENSING_LANDSAT8_USGS_FORMAT_BAND_FILE_EXTENSION;
        if(!QFile::exists(bandFileName))
        {
            bool success=false;
            for(int nAlias=0;nAlias<mLandsat8BandsCodeAliasByCode[bandCode].size();nAlias++)
            {
                QString aliasBandFileName=scenePath+"/"+sceneBaseName+REMOTESENSING_LANDSAT8_USGS_FORMAT_BAND_FILE_SUFFIX;
                aliasBandFileName+=mLandsat8BandsCodeAliasByCode[bandCode][nAlias];
                aliasBandFileName+=REMOTESENSING_LANDSAT8_USGS_FORMAT_BAND_FILE_EXTENSION;
                if(QFile::exists(aliasBandFileName))
                {
                    if(!QFile::rename(aliasBandFileName,bandFileName))
                    {
                        strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
                        strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
                        strError+=QObject::tr("\nerror renaming file for band file:\n%1").arg(aliasBandFileName);
                        return(false);
                    }
                }
                else
                {
                    success=true;
                    break;
                }
            }
            if(!success)
            {
                strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
                strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
                strError+=QObject::tr("\nnot exists band file:\n%1").arg(bandFileName);
            }
        }
//        QString bandId=QString::number(nb);
        QString bandId=bandCode;
        mBandsUsgsFormatFileName[bandId]=bandFileName;
        QFileInfo bandFileInfo(bandFileName);
        QString bandFileBaseName=bandFileInfo.completeBaseName();
//        mBandCodeByBandUsgsFormatFileCompleteBaseName[bandFileBaseName]=nb;
        mBandCodeByBandUsgsFormatFileCompleteBaseName[bandFileBaseName]=bandCode;
        IGDAL::Raster* ptrRasterBand=new IGDAL::Raster(mPtrCrsTools);
        QString strAuxError;
        if(!ptrRasterBand->setFromFile(bandFileName,
                                       strAuxError))
        {
            strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
            strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
            strError+=QObject::tr("\nError setting band:\n%1").arg(bandFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        QString bandCrsDescription=ptrRasterBand->getCrsDescription();
        if(!sceneCrsDescription.isEmpty())
        {
            if(sceneCrsDescription!=bandCrsDescription)
            {
                strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
                strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
                strError+=QObject::tr("\nError setting band:\n%1").arg(bandFileName);
                strError+=QObject::tr("\nEPSG code: %1 is different to previous: %2")
                        .arg(sceneCrsDescription).arg(bandCrsDescription);
                return(false);
            }
        }
        else
            sceneCrsDescription=bandCrsDescription;
        GDALDataType bandGDALDataType;
        if(!ptrRasterBand->getDataType(bandGDALDataType,
                                      strAuxError))
        {
            strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
            strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
            strError+=QObject::tr("\nError setting band:\n%1").arg(bandFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(mGdalDataType!=RASTER_GDALDATATYPE_NULL_VALUE)
        {
            if(mGdalDataType!=bandGDALDataType)
            {
                strError=QObject::tr("SceneLandsat8::setFromUsgsFormat");
                strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
                strError+=QObject::tr("\nError setting band:\n%1").arg(bandFileName);
                strError+=QObject::tr("\nGDALDataType: %1 is different to previous: %2")
                        .arg(QString::number(bandGDALDataType)).arg(QString::number(mGdalDataType));
                return(false);
            }
        }
        else
            mGdalDataType=bandGDALDataType;
        ptrRasterBand->closeDataset();
        mPtrRasterBands[bandId]=ptrRasterBand;
    }
    mCrsDescription=sceneCrsDescription;
    return(true);
}

bool SceneLandsat8::setMetadataFromUsgsFormat(QString &inputFileName,
                                              QString &strError)
{
    mUsgsMetadata.clear();
    if(!QFile::exists(inputFileName))
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nNot exists input file:\n%1").arg(inputFileName);
        return(false);
    }
    QFile fileInput(inputFileName);
    if (!fileInput.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nError opening file: \n%1").arg(inputFileName);
        return(false);
    }
    QTextStream in(&fileInput);

    int intValue,nline=0;
    double dblValue;
    bool okToInt,okToDouble;
    QString strLine,strValue,strTag;
    QStringList strList;
    QStringList strAuxList;
    QDir currentDir=QDir::current();

    // Se ignora la cabecera
//    nline++;
//    strLine=in.readLine();
    QString tagGroup;
    QMap<QString,QString> metadataGroup;
    while(!in.atEnd())
    {
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        mMetadataFileContent+=strLine;
        mMetadataFileContent+="\n";
        if(strLine.compare(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_END,Qt::CaseInsensitive)==0) // línea final
            continue;
        strList=strLine.split(REMOTESENSING_LANDSAT8_USGS_METADATA_STRING_SEPARATOR);
        if(strList.size()!=2)
        {
            strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(REMOTESENSING_LANDSAT8_USGS_METADATA_STRING_SEPARATOR);
            fileInput.close();
            mUsgsMetadata.clear();
            return(false);
        }
        strTag=strList.at(0).trimmed();
        if(strTag.compare(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_BEGIN_GROUP,Qt::CaseInsensitive)==0)
        {
            tagGroup=strList.at(1).trimmed();
            // Comparar con los valores admitidos ... por hacer
            metadataGroup.clear();
            continue;
        }
        else if(strTag.compare(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_END_GROUP,Qt::CaseInsensitive)==0)
        {
            mUsgsMetadata[tagGroup]=metadataGroup;
        }
        else
        {
            strValue=strList.at(1).trimmed();
            metadataGroup[strTag]=strValue;
        }
    }
    fileInput.close();
    if(!mUsgsMetadata.contains(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA))
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nMetadata group not found: %1").arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA);
        mUsgsMetadata.clear();
        return(false);
    }
    if(!mUsgsMetadata[REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA].contains(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_PATH))
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nTag: %1 not found in metadata group: %2")
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_PATH)
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA);
        mUsgsMetadata.clear();
        return(false);
    }
    QString strPath=mUsgsMetadata[REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA][REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_PATH];
    if(strPath.contains("\"")) // desprecio la parte decimal de segundo porque no se el número de decimales y hay un carácter z final
    {
        strPath=strPath.remove("\"");
    }
    int path=strPath.toInt(&okToInt);
    if(!okToInt)
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nTag: %1 in metadata group: %2 is not an integer: %3")
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_PATH)
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA)
                .arg(strPath);
        mUsgsMetadata.clear();
        return(false);
    }
    if(!mUsgsMetadata[REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA].contains(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_ROW))
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nTag: %1 not found in metadata group: %2")
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_PATH)
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA);
        mUsgsMetadata.clear();
        return(false);
    }
    QString strRow=mUsgsMetadata[REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA][REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_ROW];
    if(strRow.contains("\"")) // desprecio la parte decimal de segundo porque no se el número de decimales y hay un carácter z final
    {
        strRow=strRow.remove("\"");
    }
    int row=strRow.toInt(&okToInt);
    if(!okToInt)
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nTag: %1 in metadata group: %2 is not an integer: %3")
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_ROW)
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA)
                .arg(strRow);
        mUsgsMetadata.clear();
        return(false);
    }
    if(!mUsgsMetadata[REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA].contains(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_DATE_ACQUIRED))
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nTag: %1 not found in metadata group: %2")
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_PATH)
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA);
        mUsgsMetadata.clear();
        return(false);
    }
    QString strDate=mUsgsMetadata[REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA][REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_DATE_ACQUIRED];
    if(strDate.contains("\"")) // desprecio la parte decimal de segundo porque no se el número de decimales y hay un carácter z final
    {
        strDate=strDate.remove("\"");
    }
    QDate date=QDate::fromString(strDate,"yyyy-MM-dd");
    if(!date.isValid())
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nTag: %1 in metadata group: %2 is not a valid date: %3")
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_DATE_ACQUIRED)
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA)
                .arg(strDate);
        mUsgsMetadata.clear();
        return(false);
    }
    if(!mUsgsMetadata[REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA].contains(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_CENTER_TIME))
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nTag: %1 not found in metadata group: %2")
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_WRS_PATH)
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA);
        mUsgsMetadata.clear();
        return(false);
    }
    QString strTime=mUsgsMetadata[REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA][REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_CENTER_TIME];
    if(strTime.contains(".")) // desprecio la parte decimal de segundo porque no se el número de decimales y hay un carácter z final
    {
        QStringList strTimeList=strTime.split(".");
        strTime=strTimeList.at(0);
    }
    if(strTime.contains("\"")) // desprecio la parte decimal de segundo porque no se el número de decimales y hay un carácter z final
    {
        strTime=strTime.remove("\"");
    }
    QTime time=QTime::fromString(strTime,"hh:mm:ss");
    if(!time.isValid())
    {
        strError=QObject::tr("SceneLandsat8::setMetadataFromUsgsFormat");
        strError+=QObject::tr("\nTag: %1 in metadata group: %2 is not a valid time: %3")
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA_CENTER_TIME)
                .arg(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_GROUP_PRODUCT_METADATA)
                .arg(strDate);
        mUsgsMetadata.clear();
        return(false);
    }
    mDateTime=QDateTime(date,time);
    mPath=path;
    mRow=row;

    return(true);
}

bool SceneLandsat8::createNestedGrid(NestedGrid::NestedGridProject *ptrNestedGridProject,
                                     PW::MultiProcess *ptrMultiProcess,
                                     int& landSat8PathRowCrsEpsgCode,
                                     QMap<int,QMap<int,OGRGeometry*> >& landsat8PathRowGeometries,
                                     QVector<QString>& landsat8BandsToUse,
                                     bool reproject,
                                     QString &strError)
{
    if(!landsat8PathRowGeometries.contains(mPath))
    {
        strError=QObject::tr("SceneLandsat8::createNestedGrid");
        strError+=QObject::tr("\nThere are not geometry for path: %1")
                .arg(QString::number(mPath));
        return(false);
    }
    if(!landsat8PathRowGeometries[mPath].contains(mRow))
    {
        strError=QObject::tr("SceneLandsat8::createNestedGrid");
        strError+=QObject::tr("\nThere are not geometry for path: %1 and row: %2")
                .arg(QString::number(mPath)).arg(QString::number(mRow));
        return(false);
    }
    for(int i=0;i<landsat8BandsToUse.size();i++)
    {
//        QString bandId=QString::number(landsat8BandsToUse.at(i));
        QString bandId=landsat8BandsToUse.at(i);
        if(!mPtrRasterBands.contains(bandId))
        {
            strError=QObject::tr("SceneLandsat8::createNestedGrid");
            strError+=QObject::tr("\nThere are not band id: %1 in scene: %2")
                    .arg(bandId).arg(mId);
            return(false);
        }
    }
    mPtrNestedGridProject=ptrNestedGridProject;
    mPtrMultiProcess=ptrMultiProcess;

    QString strAuxError;
    NestedGrid::NestedGridTools* ptrNestedGridTools=mPtrNestedGridProject->getNestedGridTools();
    IGDAL::libIGDALProcessMonitor* ptrLibIGDALProcessMonitor=ptrNestedGridTools->getIGDALProcessMonitor();
    QString resamplingMethod=ptrNestedGridProject->getResamplingMethod();
    for(int i=0;i<landsat8BandsToUse.size();i++)
    {
//        QString bandId=QString::number(landsat8BandsToUse.at(i));
        QString bandId=landsat8BandsToUse.at(i);
        IGDAL::Raster* ptrRasterBand=mPtrRasterBands[bandId];
        QString fileName=ptrRasterBand->getFileName();

        // 1. Reproyectar las bandas al CRS de NestedGrid
        QString inputProj4Crs=mCrsDescription;
        QString outputProj4Crs=ptrNestedGridTools->getCrsDescription();
        bool areTheSameCRSs=false;
        if(!mPtrNestedGridProject->getNestedGridTools()->getCrsTools()->compareCRSs(inputProj4Crs,
                                                                                    outputProj4Crs,
                                                                                    areTheSameCRSs,
                                                                                    strAuxError))
        {
            strError=QObject::tr("SceneLandsat8::createNestedGrid");
            strError+=QObject::tr("\nFor Landsat8:\n%1\nband id: %1").arg(mId).arg(bandId);
            strError+=QObject::tr("\nError comparing input and output CRSs");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        mReprojectionProcessesOutputFileNames.push_back(fileName);
        // siempre reproyecto porque además se pasa a GeoTiff y establece el valor nulo. Tarda poco
//        if(!areTheSameCRSs)
//        {
//            if(reproject)
//            {
                QString command;
                QStringList arguments;
                QString outputFileName;
                QFileInfo fileInfo(fileName);
                outputFileName=fileInfo.absolutePath()+"/"+fileInfo.completeBaseName()+"_rpy."+fileInfo.suffix();
                if(QFile::exists(outputFileName))
                {
                    QFlags<QFile::Permission> permissionsImage=QFile::permissions(outputFileName);
                    if(permissionsImage.testFlag(QFile::ReadUser))
                    {
                        if(!QFile::setPermissions(outputFileName,QFile::WriteUser))
                        {
                            strError=QObject::tr("SceneLandsat8::createNestedGrid");
                            strError+=QObject::tr("\nError deleting existing file:\n%1").arg(outputFileName);
                            return(false);
                        }
                    }
                    if(!QFile::remove(outputFileName))
                    {
                        strError=QObject::tr("SceneLandsat8::createNestedGrid");
                        strError+=QObject::tr("\nError deleting existing file:\n%1").arg(outputFileName);
                        return(false);
                    }
                }
                bool forceReprojection=true;
                if(!ptrLibIGDALProcessMonitor->rasterReprojectionGetProcessDefinition(fileName,
                                                                                      inputProj4Crs,
                                                                                      outputFileName,
                                                                                      outputProj4Crs,
                                                                                      resamplingMethod,
                                                                                      command,
                                                                                      arguments,
                                                                                      strAuxError,
                                                                                      forceReprojection))
                {
                    strError=QObject::tr("SceneLandsat8::createNestedGrid");
                    strError+=QObject::tr("\nError getting reprojection process definition:\nError: %1").arg(strAuxError);
                    return(false);
                }
                arguments<<"-srcnodata";
                arguments<<QString::number(REMOTESENSING_LANDSAT8_NULL_VALUE);
                arguments<<"-dstnodata";
                arguments<<QString::number(REMOTESENSING_LANDSAT8_NULL_VALUE);
                PW::ExternalProcess* ptrReprojectionProcess=new PW::ExternalProcess(command);
                QString programsPath=mPtrNestedGridProject->getProgramsPath();
                ptrReprojectionProcess->appendEnvironmentValue("PATH",programsPath);
        //        ptrReprojectionRasterProcess->appendEnvironmentValue("PATH",micmacBinaryAuxPath);
        //        ptrReprojectionRasterProcess->setWorkingDir();
                ptrReprojectionProcess->addIntputs(arguments);
                QObject::connect(ptrReprojectionProcess, SIGNAL(newStdData(QString)),this,SLOT(manageProccesStdOutput(QString)));
                QObject::connect(ptrReprojectionProcess, SIGNAL(newErrorData(QString)),this,SLOT(manageProccesErrorOutput(QString)));
                QObject::connect(ptrReprojectionProcess, SIGNAL(newStdData(QString)),this,SLOT(manageProccesStdOutput(QString)));
                QObject::connect(ptrReprojectionProcess, SIGNAL(newErrorData(QString)),this,SLOT(manageProccesErrorOutput(QString)));
                QObject::connect(ptrReprojectionProcess, SIGNAL(finished()),this,SLOT(onReprojectionProcessFinished()));
                mPtrMultiProcess->appendProcess(ptrReprojectionProcess);

//            }
//            else
//            {
//                onReprojectionProcessFinished();
//            }
//        }
    }
    return(true);
}

bool SceneLandsat8::getBandIdFromFileName(QString fileName,
                                          QString &bandId,
                                          QString &strError)
{
    bandId="";
    QVector<QString> landsat8BandsCode;
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B0_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B1_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B2_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B3_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B4_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B5_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B6_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B7_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B8_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B9_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B10_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_B11_CODE);
    landsat8BandsCode.append(REMOTESENSING_LANDSAT8_BAND_BQA_CODE);
    for(int i=0;i<landsat8BandsCode.size();i++)
    {
        if(fileName.contains(landsat8BandsCode[i],Qt::CaseInsensitive))
        {
            bandId=landsat8BandsCode[i];
            break;
        }
    }
    if(bandId.isEmpty())
    {
        strError=QObject::tr("SceneLandsat8::getBandIdFromFileName");
        strError+=QObject::tr("\nBand id not found for scene file name:\n%1").arg(fileName);
        return(false);
    }
    return(true);
}

bool SceneLandsat8::getMetadataValues(QString inputFileName,
                                      QVector<QString> metadataTags,
                                      QVector<QString> &metadataValues,
                                      QString &strError)
{
    metadataValues.clear();
    if(!QFile::exists(inputFileName))
    {
        strError=QObject::tr("SceneLandsat8::getMetadataValues");
        strError+=QObject::tr("\nFor metadata file name: %1").arg(inputFileName);
        strError+=QObject::tr("\nnot exists metadata file:\n%1").arg(inputFileName);
        return(false);
    }
    QFile fileInput(inputFileName);
    if (!fileInput.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        strError=QObject::tr("SceneLandsat8::getMetadataValues");
        strError+=QObject::tr("\nError opening file: \n%1").arg(inputFileName);
        return(false);
    }
    QTextStream in(&fileInput);

    int nline=0;
    QString strLine,strTag;
    QStringList strList;
//    int intValue;
//    double dblValue;
//    bool okToInt,okToDouble;
//    QStringList strAuxList;
//    QDir currentDir=QDir::current();

    // Se ignora la cabecera
//    nline++;
//    strLine=in.readLine();
    QString tagGroup;
    QMap<QString,QString> metadataValueByTag;
    while(!in.atEnd())
    {
        nline++;
        strLine=in.readLine();
        strLine=strLine.trimmed();
        if(strLine.compare(REMOTESENSING_LANDSAT8_USGS_METADATA_TAG_END,Qt::CaseInsensitive)==0) // línea final
            continue;
        strList=strLine.split(REMOTESENSING_LANDSAT8_USGS_METADATA_STRING_SEPARATOR);
        if(strList.size()!=2)
        {
            strError=QObject::tr("SceneLandsat8::getMetadataValues");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nError reading line: %1").arg(QString::number(nline));
            strError+=QObject::tr("\nThere are not two fields separated by %1").arg(REMOTESENSING_LANDSAT8_USGS_METADATA_STRING_SEPARATOR);
            fileInput.close();
            return(false);
        }
        strTag=strList.at(0).trimmed();
        if(metadataTags.contains(strTag)
                ||metadataTags.contains(strTag.toLower())
                ||metadataTags.contains(strTag.toUpper()))
        {
            metadataValueByTag[strTag]=strList.at(1).trimmed();
        }
    }
    fileInput.close();
    for(int i=0;i<metadataTags.size();i++)
    {
        QString metadataTag=metadataTags[i];
        if(metadataValueByTag.contains(metadataTag)
                ||metadataTags.contains(metadataTag.toLower())
                ||metadataTags.contains(metadataTag.toUpper()))
        {
            metadataValues.push_back(metadataValueByTag[metadataTag]);
        }
        else
        {
            strError=QObject::tr("SceneLandsat"
                                 "8::getMetadataValues");
            strError+=QObject::tr("\nError reading file: %1").arg(inputFileName);
            strError+=QObject::tr("\nTag %1 not found").arg(metadataTag);
            fileInput.close();
            return(false);
        }
    }
    return(true);
}

bool SceneLandsat8::getPersistenceData(QMap<QString, QString> &persistenceData,
                                       QString &strError)
{
    persistenceData[REMOTESENSING_LANDSAT8_USGS_METADATA_FILE_PERSISTENCE_TAG]=mMetadataFileContent;
    return(true);
}

void SceneLandsat8::onTuplekeyFirstProcessFinished()
{
    QString fileName=mTuplekeysOutputFileNames[0];
//    if(fileName.contains("0313333323"))
//    {
//        int yo=1;
//        yo++;
//    }
}

void SceneLandsat8::onTuplekeySecondProcessFinished()
{
    QString fileName=mTuplekeysOutputFileNames[0];
    IGDAL::Raster* ptrRaster=new IGDAL::Raster(mPtrCrsTools);
    QString strError;
//    if(fileName.contains("0313332321"))
//    {
//        int yo=1;
//        yo++;
//    }
    if(!ptrRaster->setFromFile(fileName,
                               strError))
    {
        QString msg=QObject::tr("SceneLandsat8::onQuadkeyProcessFinished");
        msg+=QObject::tr("\nError openning raster file:\n%1\nError:\n%2")
                .arg(fileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    double minValue,maxValue;
    if(!ptrRaster->getMinimumValue(0,minValue,strError))
    {
        QString msg=QObject::tr("SceneLandsat8::onQuadkeyProcessFinished");
        msg+=QObject::tr("\nError getting min value in raster file:\n%1\nError:\n%2")
                .arg(fileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    if(!ptrRaster->getMaximumValue(0,maxValue,strError))
    {
        QString msg=QObject::tr("SceneLandsat8::onQuadkeyProcessFinished");
        msg+=QObject::tr("\nError getting min value in raster file:\n%1\nError:\n%2")
                .arg(fileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    if(fabs(minValue-maxValue)<0.1
            &&fabs(minValue-REMOTESENSING_LANDSAT8_NULL_VALUE)<0.1)
    {
        mPtrNestedGridProject->addFileToRemove(fileName);
        QFileInfo fileInfo(fileName);
        mPtrNestedGridProject->addPathToRemoveIfEmpty(fileInfo.absolutePath());
    }
    else
    {
        QFileInfo fileInfo(fileName);
        QString completeBaseName=fileInfo.completeBaseName();
//        int numberOfBand=mBandCodeByBandUsgsFormatFileCompleteBaseName[completeBaseName];
        QString bandId=mBandCodeByBandUsgsFormatFileCompleteBaseName[completeBaseName];
        QDir fileDir=fileInfo.dir();
        QString quadkey=fileDir.dirName();
        if(!mTuplekeysByBand.contains(bandId))
        {
            QVector<QString> quadkeys;
            mTuplekeysByBand[bandId]=quadkeys;
        }
//        if(!mQuadkeysByBand.contains(numberOfBand))
//        {
//            QVector<QString> quadkeys;
//            mQuadkeysByBand[numberOfBand]=quadkeys;
//        }
//        mQuadkeysByBand[numberOfBand].push_back(quadkey);
        mTuplekeysByBand[bandId].push_back(quadkey);
    }
    ptrRaster->closeDataset();
    mTuplekeysOutputFileNames.remove(0);
}

void SceneLandsat8::onReprojectionProcessFinished()
{
    QString fileName=mReprojectionProcessesOutputFileNames[0];
    QFileInfo fileInfo(fileName);
    QString completeBaseName=fileInfo.completeBaseName();
    QString bandId=mBandCodeByBandUsgsFormatFileCompleteBaseName[completeBaseName];
//    int numberOfBand=mBandCodeByBandUsgsFormatFileCompleteBaseName[completeBaseName];
    QString outputFileName;
    outputFileName=fileInfo.absolutePath()+"/"+fileInfo.completeBaseName()+"_rpy."+fileInfo.suffix();
    if(!QFile::exists(outputFileName))
    {
        QString msg=QObject::tr("SceneLandsat8::onReprojectionProcessFinished\nOutput file not exists:\n%1")
                .arg(outputFileName);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    QDir tmpDir(fileInfo.absolutePath());
    QStringList filters;
    QString filter=fileInfo.completeBaseName()+"_rpy.*";
    filters << filter;
    QFileInfoList fileInfoList=tmpDir.entryInfoList(filters,QDir::Files);
    for(int i=0;i<fileInfoList.size();i++)
    {
        QString fileName=fileInfoList[i].absoluteFilePath();
        mPtrNestedGridProject->addFileToRemove(fileName);
    }
    QString strError;
    IGDAL::Raster* ptrReprojectedRasterBand=new IGDAL::Raster(mPtrCrsTools);
    if(!ptrReprojectedRasterBand->setFromFile(outputFileName,
                                              strError))
    {
        QString msg=QObject::tr("SceneLandsat8::onReprojectionProcessFinished");
        msg+=QObject::tr("\nError openning reprojected raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    NestedGrid::NestedGridTools* ptrNestedGridTools=mPtrNestedGridProject->getNestedGridTools();
    QString reprojectedProj4Crs=ptrNestedGridTools->getCrsDescription();
    double nwFc,nwSc,seFc,seSc;
    if(!ptrReprojectedRasterBand->getBoundingBox(nwFc,
                                                 nwSc,
                                                 seFc,
                                                 seSc,
                                                 strError))
    {
        QString msg=QObject::tr("SceneLandsat8::onReprojectionProcessFinished");
        msg+=QObject::tr("\nError recovering bounding box from reprojected raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    ptrReprojectedRasterBand->closeDataset();
    int lodStorage=mPtrNestedGridProject->getLandsat8LODStorage(bandId);
    int lodSpatialResolution=mPtrNestedGridProject->getLandsat8LODSpatialResolution(bandId);
//    int lodStorage=mPtrNestedGridProject->getLandsatLODStorage(numberOfBand);
//    int lodSpatialResolution=mPtrNestedGridProject->getLandsatLODSpatialResolution(numberOfBand);
    int basePixelSize=ptrNestedGridTools->getBaseWorldWidthAndHeightInPixels();
    int ratioInLods=ptrNestedGridTools->getRatioInLODs();
    for(int lod=lodStorage+1;lod<=lodSpatialResolution;lod++)
    {
        //        basePixelSize*=2;
                basePixelSize*=ratioInLods;
    }
    QVector<QString> tuplekeys;
    QVector<int> tilesX;
    QVector<int> tilesY;
    QVector<QVector<double> > boundingBoxes;
    if(!ptrNestedGridTools->getTiles(lodStorage,reprojectedProj4Crs,nwFc,nwSc,seFc,seSc,
                                     tuplekeys,tilesX,tilesY,boundingBoxes,
                                     strError))
    {
        QString msg=QObject::tr("SceneLandsat8::onReprojectionProcessFinished");
        msg+=QObject::tr("\nError recovering tiles for reprojected raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    QDir auxDir=QDir::currentPath();
//    IGDAL::libIGDALProcessMonitor* ptrLibIGDALProcessMonitor=ptrNestedGridTools->getIGDALProcessMonitor();
    QString resamplingMethod=mPtrNestedGridProject->getResamplingMethod();
    QString compressionMethod=mPtrNestedGridProject->getCompressMethod();
    if(compressionMethod.compare(NESTED_GRID_COMPRESSION_METHOD_JPEG)==0)
    {
        compressionMethod=REMOTESENSING_LANDSAT8_COMPRESSION_METHOD_JPEG_ALTERNATIVE;
    }
    QString strCompressionArgument="COMPRESS="+compressionMethod;
    bool createTiledRaster=mPtrNestedGridProject->getCreateTiledRaster();
    for(int nTile=0;nTile<tuplekeys.size();nTile++)
    {
        QString tuplekey=tuplekeys.at(nTile);
        int tileX=tilesX.at(nTile);
        int tileY=tilesY.at(nTile);
        double ulx=boundingBoxes.at(nTile)[0];
        double uly=boundingBoxes.at(nTile)[1];
        double lrx=boundingBoxes.at(nTile)[2];
        double lry=boundingBoxes.at(nTile)[3];
        QString tuplekeyPath=mPtrNestedGridProject->getStoragePath()+"/"+tuplekey;
        if(!auxDir.exists(tuplekeyPath))
        {
            if(!auxDir.mkpath(tuplekeyPath))
            {
                QString msg=QObject::tr("SceneLandsat8::onReprojectionProcessFinished");
                msg+=QObject::tr("\nError making tuplekey path:\n%1").arg(tuplekeyPath);
                (*mStdOut)<<msg<<"\n";
                (*mStdOut).flush();
                QCoreApplication::exit();
            }
        }
        QString outputTuplekeyFileName=tuplekeyPath+"/"+fileInfo.baseName()+"."+fileInfo.suffix();
        if(QFile::exists(outputTuplekeyFileName))
        {
            if(!QFile::remove(outputTuplekeyFileName))
            {
                QString msg=QObject::tr("SceneLandsat8::onReprojectionProcessFinished");
                msg+=QObject::tr("\nError removing tuplekey output file:\n%1").arg(outputTuplekeyFileName);
                (*mStdOut)<<msg<<"\n";
                (*mStdOut).flush();
                QCoreApplication::exit();
            }
        }
//        if(quadkey.compare("0313333323")==0)
//        {
//            int yo=1;
//            yo++;
//        }
        mTuplekeysOutputFileNames.push_back(outputTuplekeyFileName);
        {
            QString command=LIBIGDAL_GDALTRANSLATE_COMMAND;
            QStringList arguments;
            arguments<<"-of";
            arguments<<"GTiff";
            arguments<<"-outsize";
            arguments<<QString::number(basePixelSize);
            arguments<<QString::number(basePixelSize);
            arguments<<"-r";
//            arguments<<"nearest";//resamplingMethod;
            arguments<<resamplingMethod;
            arguments<<"-a_nodata";
            arguments<<QString::number(REMOTESENSING_LANDSAT8_NULL_VALUE);
            arguments<<"-projwin";
            arguments<<QString::number(ulx,'f',NESTED_GRID_DEFINITION_PROJECTED_COORDINATES_PRECISION);
            arguments<<QString::number(uly,'f',NESTED_GRID_DEFINITION_PROJECTED_COORDINATES_PRECISION);
            arguments<<QString::number(lrx,'f',NESTED_GRID_DEFINITION_PROJECTED_COORDINATES_PRECISION);
            arguments<<QString::number(lry,'f',NESTED_GRID_DEFINITION_PROJECTED_COORDINATES_PRECISION);
            if(createTiledRaster)
            {
                arguments<<"-co";
                arguments<<"TILED=YES";
            }
            //    arguments<<"-co";
            //    arguments<<"TFW=YES";
            //    arguments<<"-mo";
            //    arguments<<("TIFFTAG_SOFTWARE="+mSoftware);
            //    arguments<<"-mo";
            //    arguments<<("TIFFTAG_COPYRIGHT="+mCopyright);
            arguments<<"-co";
            arguments<<strCompressionArgument;
            arguments<<outputFileName;
            arguments<<outputTuplekeyFileName;

            PW::ExternalProcess* ptrProcess=new PW::ExternalProcess(command);
            QString programsPath=mPtrNestedGridProject->getProgramsPath();
            ptrProcess->appendEnvironmentValue("PATH",programsPath);
    //        ptrReprojectionRasterProcess->appendEnvironmentValue("PATH",micmacBinaryAuxPath);
    //        ptrReprojectionRasterProcess->setWorkingDir();
            ptrProcess->addIntputs(arguments);
            QObject::connect(ptrProcess, SIGNAL(newStdData(QString)),this,SLOT(manageProccesStdOutput(QString)));
            QObject::connect(ptrProcess, SIGNAL(newErrorData(QString)),this,SLOT(manageProccesErrorOutput(QString)));
            QObject::connect(ptrProcess, SIGNAL(newStdData(QString)),this,SLOT(manageProccesStdOutput(QString)));
            QObject::connect(ptrProcess, SIGNAL(newErrorData(QString)),this,SLOT(manageProccesErrorOutput(QString)));
            if(lodSpatialResolution>lodStorage)
            {
                QObject::connect(ptrProcess, SIGNAL(finished()),this,SLOT(onTuplekeyFirstProcessFinished()));
            }
            else
            {
                QObject::connect(ptrProcess, SIGNAL(finished()),this,SLOT(onTuplekeySecondProcessFinished()));
            }
            mPtrMultiProcess->appendProcess(ptrProcess);
        }
        if(lodSpatialResolution>lodStorage)
        {
            QString command=LIBIGDAL_GDALADDO_COMMAND;
            QStringList arguments;
            arguments<<"-r";
//            arguments<<"nearest";//resamplingMethod;
            arguments<<resamplingMethod;
            arguments<<outputTuplekeyFileName;
            int overview=1;
            for(int lod=lodStorage+1;lod<=lodSpatialResolution;lod++)
            {
                overview*=2;
                arguments<<QString::number(overview);
            }

            PW::ExternalProcess* ptrProcess=new PW::ExternalProcess(command);
            QString programsPath=mPtrNestedGridProject->getProgramsPath();
            ptrProcess->appendEnvironmentValue("PATH",programsPath);
    //        ptrReprojectionRasterProcess->appendEnvironmentValue("PATH",micmacBinaryAuxPath);
    //        ptrReprojectionRasterProcess->setWorkingDir();
            ptrProcess->addIntputs(arguments);
            QObject::connect(ptrProcess, SIGNAL(newStdData(QString)),this,SLOT(manageProccesStdOutput(QString)));
            QObject::connect(ptrProcess, SIGNAL(newErrorData(QString)),this,SLOT(manageProccesErrorOutput(QString)));
            QObject::connect(ptrProcess, SIGNAL(newStdData(QString)),this,SLOT(manageProccesStdOutput(QString)));
            QObject::connect(ptrProcess, SIGNAL(newErrorData(QString)),this,SLOT(manageProccesErrorOutput(QString)));
            QObject::connect(ptrProcess, SIGNAL(finished()),this,SLOT(onTuplekeySecondProcessFinished()));
            mPtrMultiProcess->appendProcess(ptrProcess);
        }
    }
    mReprojectionProcessesOutputFileNames.remove(0);
}
