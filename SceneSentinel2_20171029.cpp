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
#include "SceneSentinel2.h"
#include "libIGDALProcessMonitor.h"
#include "nestedgrid_definitions.h"

using namespace RemoteSensing;

SceneSentinel2::SceneSentinel2(QString id,
                               libCRS::CRSTools* ptrCrsTools):
    Scene(id,
          ptrCrsTools)
{
    mGdalDataType=RASTER_GDALDATATYPE_NULL_VALUE;
    mStdOut = new QTextStream(stdout);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B1_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B2_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B3_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B4_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B5_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B6_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B7_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B8_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B8A_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B9_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B10_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B11_CODE);
    mSentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B12_CODE);
    for(int nb=0;nb<mSentinel2BandsCode.size();nb++)
    {
        mStrBandsCodeValidDomain+=mSentinel2BandsCode[nb];
        if(nb<(mSentinel2BandsCode.size()-1))
            mStrBandsCodeValidDomain+="-";
    }
    mSceneType=Sentinel2;
}

bool SceneSentinel2::createNestedGrid(NestedGrid::NestedGridProject *ptrNestedGridProject,
                                      PW::MultiProcess *ptrMultiProcess,
                                      QVector<QString> &sentinel2BandsToUse,
                                      bool reproject,
                                      QString &strError)
{
    for(int i=0;i<sentinel2BandsToUse.size();i++)
    {
//        QString bandId=QString::number(landsat8BandsToUse.at(i));
        QString bandId=sentinel2BandsToUse.at(i);
        if(!mPtrRasterBands.contains(bandId))
        {
            strError=QObject::tr("SceneSentinel2::createNestedGrid");
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
    for(int i=0;i<sentinel2BandsToUse.size();i++)
    {
//        QString bandId=QString::number(landsat8BandsToUse.at(i));
        QString bandId=sentinel2BandsToUse.at(i);
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
            strError=QObject::tr("SceneSentinel2::createNestedGrid");
            strError+=QObject::tr("\nFor Scene2:\n%1\nband id: %1").arg(mId).arg(bandId);
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
                QString outputFileExtension=ptrNestedGridProject->getRasterFormat();
                if(outputFileExtension.compare(NESTED_GRID_RASTER_FORMAT_GTIFF,Qt::CaseInsensitive)!=0)
                {
                    strError=QObject::tr("SceneSentinel2::createNestedGrid");
                    strError+=QObject::tr("\For band id: %1 in scene: %2\nnot valid output format:%3")
                            .arg(bandId).arg(mId).arg(outputFileExtension);
                    return(false);
                }
                else
                {
                    outputFileExtension=NESTED_GRID_RASTER_FORMAT_GTIFF_FILE_EXTENSION;
                }
//                outputFileName=fileInfo.absolutePath()+"/"+fileInfo.completeBaseName()+"_rpy."+fileInfo.suffix();
                outputFileName=fileInfo.absolutePath()+"/"+fileInfo.completeBaseName()+"_rpy."+outputFileExtension;
                if(QFile::exists(outputFileName))
                {
                    QFlags<QFile::Permission> permissionsImage=QFile::permissions(outputFileName);
                    if(permissionsImage.testFlag(QFile::ReadUser))
                    {
                        if(!QFile::setPermissions(outputFileName,QFile::WriteUser))
                        {
                            strError=QObject::tr("SceneSentinel2::createNestedGrid");
                            strError+=QObject::tr("\nError deleting existing file:\n%1").arg(outputFileName);
                            return(false);
                        }
                    }
                    if(!QFile::remove(outputFileName))
                    {
                        strError=QObject::tr("SceneSentinel2::createNestedGrid");
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
                    strError=QObject::tr("SceneSentinel2::createNestedGrid");
                    strError+=QObject::tr("\nError getting reprojection process definition:\nError: %1").arg(strAuxError);
                    return(false);
                }
                arguments<<"-srcnodata";
                arguments<<QString::number(REMOTESENSING_SENTINEL2_NULL_VALUE);
                arguments<<"-dstnodata";
                arguments<<QString::number(REMOTESENSING_SENTINEL2_NULL_VALUE);
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
//        else
//        {
//            onReprojectionProcessFinished();
//        }
    }
    return(true);
}

bool SceneSentinel2::getBandIdFromFileName(QString fileName,
                                           QString &bandId,
                                           QString &strError)
{
    bandId="";
    QVector<QString> sentinel2BandsCode;
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B1_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B2_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B3_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B4_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B5_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B6_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B7_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B8_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B8A_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B9_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B10_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B11_CODE);
    sentinel2BandsCode.append(REMOTESENSING_SENTINEL2_BAND_B12_CODE);
    for(int i=0;i<sentinel2BandsCode.size();i++)
    {
        if(fileName.contains(sentinel2BandsCode[i],Qt::CaseInsensitive))
        {
            bandId=sentinel2BandsCode[i];
            break;
        }
    }
    if(bandId.isEmpty())
    {
        strError=QObject::tr("SceneSentinel2::getBandIdFromFileName");
        strError+=QObject::tr("\nBand id not found for scene file name:\n%1").arg(fileName);
        return(false);
    }
    return(true);
}

void SceneSentinel2::manageProccesStdOutput(QString data)
{
    //*mStdOut<<data;
    fprintf(stdout, data.toLatin1());
}

void SceneSentinel2::manageProccesErrorOutput(QString data)
{
//   *mStdOut<<data;
    fprintf(stdout, data.toLatin1());
}

bool SceneSentinel2::setFromEsaZipFormat(QString &scenePath,
                                         QString &sceneMetadataFile,
                                         NestedGrid::NestedGridProject *ptrNestedGridProject,
                                         QString &strError)
{
    QDir auxDir=QDir::currentPath();
    if(!auxDir.exists(scenePath))
    {
        strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
        strError+=QObject::tr("\nFor scene: %1").arg(mId);
        strError+=QObject::tr("\nnot exists path:\n%1").arg(scenePath);
        return(false);
    }
    if(!QFile::exists(sceneMetadataFile))
    {
        strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
        strError+=QObject::tr("\nFor scene: %1").arg(mId);
        strError+=QObject::tr("\nnot exists metadata file:\n%1").arg(sceneMetadataFile);
        return(false);
    }
    QString strAuxError;
//    if(!setMetadataFromUsgsFormat(sceneMetadataFileName,
//                                  strAuxError))
//    {
//        strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
//        strError+=QObject::tr("\nFor scene: %1").arg(sceneBaseName);
//        strError+=QObject::tr("\nError setting metadata from file:\n%1\nError:\n%2")
//                .arg(sceneMetadataFileName).arg(strAuxError);
//        return(false);
//    }
    mPtrNestedGridProject=ptrNestedGridProject;
    if(!mPtrNestedGridProject->insertSentinel2MetadataFile(mId,
                                                           sceneMetadataFile,
                                                           strAuxError)) // se copia a la carpeta metadata de store
    {
        strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
        strError+=QObject::tr("\nFor scene: %1").arg(mId);
        strError+=QObject::tr("\nError inserting metadata file:\n%1\nError:\n%2")
                .arg(sceneMetadataFile).arg(strAuxError);
        return(false);
    }
    mMetadataEsaZipFormatFileName=sceneMetadataFile;
    QString sceneCrsDescription;
    for(int nb=0;nb<mSentinel2BandsCode.size();nb++)
    {
        QString bandCode=mSentinel2BandsCode[nb];
        QString bandFileName=scenePath+"/"+mId+REMOTESENSING_SENTINEL2_ESA_ZIP_FORMAT_BAND_FILE_SUFFIX;
//        bandFileName+=QString::number(nb);
        bandFileName+=bandCode;
        bandFileName+=REMOTESENSING_SENTINEL2_ESA_ZIP_FORMAT_BAND_FILE_EXTENSION;
        if(!QFile::exists(bandFileName))
        {
            strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
            strError+=QObject::tr("\nFor scene: %1").arg(mId);
            strError+=QObject::tr("\nnot exists band file:\n%1").arg(bandFileName);
            return(false);
        }
//        QString bandId=QString::number(nb);
        QString bandId=bandCode;
        mBandsEsaZipFormatFileName[bandId]=bandFileName;
        QFileInfo bandFileInfo(bandFileName);
        QString bandFileBaseName=bandFileInfo.completeBaseName();
//        mBandCodeByBandUsgsFormatFileCompleteBaseName[bandFileBaseName]=nb;
        mBandCodeByBandEsaZipFormatFileCompleteBaseName[bandFileBaseName]=bandCode;
        IGDAL::Raster* ptrRasterBand=new IGDAL::Raster(mPtrCrsTools);
        QString strAuxError;
        if(!ptrRasterBand->setFromFile(bandFileName,
                                       strAuxError))
        {
            strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
            strError+=QObject::tr("\nFor scene: %1").arg(mId);
            strError+=QObject::tr("\nError setting band:\n%1").arg(bandFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        QString bandCrsDescription=ptrRasterBand->getCrsDescription();
        if(!sceneCrsDescription.isEmpty())
        {
            if(sceneCrsDescription!=bandCrsDescription)
            {
                strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
                strError+=QObject::tr("\nFor scene: %1").arg(mId);
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
            strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
            strError+=QObject::tr("\nFor scene: %1").arg(mId);
            strError+=QObject::tr("\nError setting band:\n%1").arg(bandFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(mGdalDataType!=RASTER_GDALDATATYPE_NULL_VALUE)
        {
            if(mGdalDataType!=bandGDALDataType)
            {
                strError=QObject::tr("SceneSentinel2::setFromEsaZipFormat");
                strError+=QObject::tr("\nFor scene: %1").arg(mId);
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

void SceneSentinel2::onTuplekeyFirstProcessFinished()
{
    QString fileName=mTuplekeysOutputFileNames[0];
//    if(fileName.contains("0313333323"))
//    {
//        int yo=1;
//        yo++;
//    }
}

void SceneSentinel2::onTuplekeySecondProcessFinished()
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
        QString msg=QObject::tr("SceneSentinel2::onQuadkeyProcessFinished");
        msg+=QObject::tr("\nError openning raster file:\n%1\nError:\n%2")
                .arg(fileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    bool addToRemove=false;
    bool existsNoNullValue=true;
    if(!ptrRaster->getExitsNotNullValues(existsNoNullValue,strError))
    {
        QString msg=QObject::tr("SceneSentinel2::onQuadkeyProcessFinished");
        msg+=QObject::tr("\nError getting exists no null values in raster file:\n%1\nError:\n%2")
                .arg(fileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    if(!existsNoNullValue)
    {
        addToRemove=true;
    }
    else
    {
        double minValue,maxValue;
        if(!ptrRaster->getMinimumValue(0,minValue,strError))
        {
            QString msg=QObject::tr("SceneSentinel2::onQuadkeyProcessFinished");
            msg+=QObject::tr("\nError getting min value in raster file:\n%1\nError:\n%2")
                    .arg(fileName).arg(strError);
            (*mStdOut)<<msg<<"\n";
            (*mStdOut).flush();
            QCoreApplication::exit();
        }
        if(!ptrRaster->getMaximumValue(0,maxValue,strError))
        {
            QString msg=QObject::tr("SceneSentinel2::onQuadkeyProcessFinished");
            msg+=QObject::tr("\nError getting min value in raster file:\n%1\nError:\n%2")
                    .arg(fileName).arg(strError);
            (*mStdOut)<<msg<<"\n";
            (*mStdOut).flush();
            QCoreApplication::exit();
        }
        if(fabs(minValue-maxValue)<0.1
                &&fabs(minValue-REMOTESENSING_SENTINEL2_NULL_VALUE)<0.1)
        {
            addToRemove=true;
        }
    }
    if(addToRemove)
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
        QString bandId=mBandCodeByBandEsaZipFormatFileCompleteBaseName[completeBaseName];
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

void SceneSentinel2::onReprojectionProcessFinished()
{
    QString fileName=mReprojectionProcessesOutputFileNames[0];
    QFileInfo fileInfo(fileName);
    QString completeBaseName=fileInfo.completeBaseName();
    QString bandId=mBandCodeByBandEsaZipFormatFileCompleteBaseName[completeBaseName];
//    int numberOfBand=mBandCodeByBandUsgsFormatFileCompleteBaseName[completeBaseName];
    QString outputFileName;
    QString outputFileExtension=mPtrNestedGridProject->getRasterFormat();
    if(outputFileExtension.compare(NESTED_GRID_RASTER_FORMAT_GTIFF,Qt::CaseInsensitive)!=0)
    {
        QString msg=QObject::tr("SceneSentinel2::onReprojectionProcessFinished");
        msg+=QObject::tr("\For scene: %1\nnot valid output format:%2")
                .arg(mId).arg(outputFileExtension);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    else
    {
        outputFileExtension=NESTED_GRID_RASTER_FORMAT_GTIFF_FILE_EXTENSION;
    }
//    outputFileName=fileInfo.absolutePath()+"/"+fileInfo.completeBaseName()+"_rpy."+fileInfo.suffix();
    outputFileName=fileInfo.absolutePath()+"/"+fileInfo.completeBaseName()+"_rpy."+outputFileExtension;
    if(!QFile::exists(outputFileName))
    {
        QString msg=QObject::tr("SceneSentinel2::onReprojectionProcessFinished\nOutput file not exists:\n%1")
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
        QString msg=QObject::tr("SceneSentinel2::onReprojectionProcessFinished");
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
        QString msg=QObject::tr("SceneSentinel2::onReprojectionProcessFinished");
        msg+=QObject::tr("\nError recovering bounding box from reprojected raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strError);
        (*mStdOut)<<msg<<"\n";
        (*mStdOut).flush();
        QCoreApplication::exit();
    }
    ptrReprojectedRasterBand->closeDataset();
    int lodStorage=mPtrNestedGridProject->getSentinel2LODStorage(bandId);
    int lodSpatialResolution=mPtrNestedGridProject->getSentinel2LODSpatialResolution(bandId);
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
        QString msg=QObject::tr("SceneSentinel2::onReprojectionProcessFinished");
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
        compressionMethod=REMOTESENSING_SENTINEL2_COMPRESSION_METHOD_JPEG_ALTERNATIVE;
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
                QString msg=QObject::tr("SceneSentinel2::onReprojectionProcessFinished");
                msg+=QObject::tr("\nError making tuplekey path:\n%1").arg(tuplekeyPath);
                (*mStdOut)<<msg<<"\n";
                (*mStdOut).flush();
                QCoreApplication::exit();
            }
        }
//        QString outputQuadkeyFileName=quadKeyPath+"/"+fileInfo.baseName()+"."+fileInfo.suffix();
        QString outputTuplekeyFileName=tuplekeyPath+"/"+fileInfo.baseName()+"."+outputFileExtension;
        if(QFile::exists(outputTuplekeyFileName))
        {
            if(!QFile::remove(outputTuplekeyFileName))
            {
                QString msg=QObject::tr("SceneSentinel2::onReprojectionProcessFinished");
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
            arguments<<QString::number(REMOTESENSING_SENTINEL2_NULL_VALUE);
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
