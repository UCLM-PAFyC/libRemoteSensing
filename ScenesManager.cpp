#include <ogr_geos.h>
#include <ogr_geometry.h>

#include "ScenesManager.h"
#include "SceneLandsat8.h"
#include "SceneSentinel2.h"
#include "CRSTools.h"

using namespace RemoteSensing;

ScenesManager::ScenesManager(libCRS::CRSTools *ptrCrsTools)
{
    mPtrCrsTools=ptrCrsTools;
}

ScenesManager::~ScenesManager()
{
    QMap<int,QMap<int,OGRGeometry*> >::iterator iter=mLandsat8PathRowGeometries.begin();
    while(iter!=mLandsat8PathRowGeometries.end())
    {
        int path=iter.key();
        QMap<int,OGRGeometry*>::iterator secondIter=iter.value().begin();
        while(secondIter!=iter.value().end())
        {
            int row=secondIter.key();
            OGRGeometry* ptrGeometry=secondIter.value();
            OGRGeometryFactory::destroyGeometry(ptrGeometry);
            mLandsat8PathRowGeometries[path][row]=NULL;
            secondIter++;
        }
        iter++;
    }
}

bool ScenesManager::insertSceneLandsat8UsgsFormat(QString &scenePath,
                                                  QString &sceneBaseName,
                                                  NestedGrid::NestedGridProject *ptrNestedGridProject,
                                                  QString &strError)
{
    QString sceneId=sceneBaseName;
    if(mPtrScenes.contains(sceneId))
    {
        strError=QObject::tr("ScenesManager::insertSceneLandsat8UsgsFormat");
        strError+=QObject::tr("\nExists scene id: %1").arg(sceneId);
        return(false);
    }
    SceneLandsat8* ptrScene=new SceneLandsat8(sceneId,
                                              mPtrCrsTools);
    QString strAuxError;
    if(!ptrScene->setFromUsgsFormat(scenePath,
                                    sceneBaseName,
                                    ptrNestedGridProject,
                                    strAuxError))
    {
        strError=QObject::tr("ScenesManager::insertSceneLandsat8UsgsFormat");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    mPtrScenes[sceneId]=ptrScene;
    return(true);
}

bool ScenesManager::insertSceneSentinel2EsaZipFormat(QString sceneId,
                                                     QString &scenePath,
                                                     QString &sceneMetadataFile,
                                                     NestedGrid::NestedGridProject *ptrNestedGridProject,
                                                     QString &strError)
{
    if(mPtrScenes.contains(sceneId))
    {
        strError=QObject::tr("ScenesManager::insertSceneSentinel2EsaZipFormat");
        strError+=QObject::tr("\nExists scene id: %1").arg(sceneId);
        return(false);
    }
    SceneSentinel2* ptrScene=new SceneSentinel2(sceneId,
                                                mPtrCrsTools);
    QString strAuxError;
    if(!ptrScene->setFromEsaZipFormat(scenePath,
                                      sceneMetadataFile,
                                      ptrNestedGridProject,
                                      strAuxError))
    {
        strError=QObject::tr("ScenesManager::insertSceneSentinel2EsaZipFormat");
        strError+=QObject::tr("\nError:\n%1").arg(strError);
        return(false);
    }
    mPtrScenes[sceneId]=ptrScene;
    return(true);
}

bool ScenesManager::createNestedGrid(QString &sceneId,
                                     NestedGrid::NestedGridProject *ptrNestedGridProject,
                                     ProcessTools::MultiProcess *ptrMultiProcess,
                                     bool reproject,
                                     QString &strError)
{
    if(!mPtrScenes.contains(sceneId))
    {
        strError=QObject::tr("ScenesManager::reprojectionToNestedGrid");
        strError+=QObject::tr("\nNot exists scene id: %1").arg(sceneId);
        return(false);
    }
    //    CameraModel* imageCameraModel=imageCamera->getCameraModel();
    //    PW::FraserModel *imageDistortionModel = dynamic_cast<PW::FraserModel*>(((PW::PhotogrammetricModel*)imageCameraModel)->getDistortionModel());
    //    if(!imageDistortionModel->isBasic())
    //    {
    //        strError=QObject::tr("OrientationProcessMonitor::writeSourceOrientation, orientation input is not Fraser basic:\n%1").arg(mSourceCalibration);
    //        return(false);
    //    }
    Scene* ptrScene=mPtrScenes[sceneId];
//    if( SceneLandsat8* ptrSceneLandsat8 = dynamic_cast< SceneLandsat8* >((Scene*)ptrScene) )
//    {
    QString auxStrError;
    if(ptrScene->isLandsat8())
    {
        if(!((SceneLandsat8*)ptrScene)->createNestedGrid(ptrNestedGridProject,
                                                         ptrMultiProcess,
                                                         mLandSat8PathRowCrsEpsgCode,
                                                         mLandsat8PathRowGeometries,
                                                         mLandsat8BandsToUse,
                                                         reproject,
                                                         auxStrError))
        {
            strError=QObject::tr("ScenesManager::reprojectionToNestedGrid");
            strError+=QObject::tr("\nError reproyecting to Nested Grid scene id: %1").arg(sceneId);
            strError+=QObject::tr("\nError:\n%1").arg(auxStrError);
            return(false);
        }
    }
    else if(ptrScene->isSentinel2())
    {
        if(!((SceneSentinel2*)ptrScene)->createNestedGrid(ptrNestedGridProject,
                                                          ptrMultiProcess,
                                                          mSentinel2BandsToUse,
                                                          reproject,
                                                          auxStrError))
        {
            strError=QObject::tr("ScenesManager::reprojectionToNestedGrid");
            strError+=QObject::tr("\nError reproyecting to Nested Grid scene id: %1").arg(sceneId);
            strError+=QObject::tr("\nError:\n%1").arg(auxStrError);
            return(false);
        }
    }
    else
    {
        strError=QObject::tr("ScenesManager::createNestedGrid");
        strError+=QObject::tr("\nInvalid scene type id: %1").arg(sceneId);
        return(false);
    }

    return(true);
}

bool ScenesManager::getTupleKeyBySceneId(QString sceneId,
                                        QMap<QString, QVector<QString> > &quadkeysByBand,
                                        QString &strError)
{
    if(!mPtrScenes.contains(sceneId))
    {
        strError=QObject::tr("ScenesManager::getQuadKeyBySceneId");
        strError+=QObject::tr("\nThere is not a scene with id: %1").arg(sceneId);
        return(false);
    }
    mPtrScenes[sceneId]->getQuadkeys(quadkeysByBand);
    return(true);
}

QString ScenesManager::getSceneType(QString sceneId,
                                    QString &strError)
{
    QString sceneType;
    if(!mPtrScenes.contains(sceneId))
    {
        strError=QObject::tr("ScenesManager::getSceneType");
        strError+=QObject::tr("\nThere is not a scene with id: %1").arg(sceneId);
        return(sceneType);
    }
    sceneType=mPtrScenes[sceneId]->getType();
    return(sceneType);
}

bool ScenesManager::getScenesPersistenceData(QMap<QString, QMap<QString, QString> > &scenesPersistenceData,
                                             QString &strError)
{
    QMap<QString,Scene*>::const_iterator iterScenes=mPtrScenes.begin();
    while(iterScenes!=mPtrScenes.end())
    {
        QString sceneId=iterScenes.key();
        Scene* ptrScene=iterScenes.value();
        QString auxStrError;
        QMap<QString, QString> persistenceData;
        if(ptrScene->isLandsat8())
        {
            if(!((SceneLandsat8*)ptrScene)->getPersistenceData(persistenceData,
                                                               auxStrError))
            {
                strError=QObject::tr("ScenesManager::getScenesPersistenceData");
                strError+=QObject::tr("\nError recovering persistence for scene id: %1").arg(sceneId);
                strError+=QObject::tr("\nError:\n%1").arg(auxStrError);
                return(false);
            }
        }
        else if(ptrScene->isSentinel2())
        {
            int yo=1; // de momento no hay
        }
        scenesPersistenceData[sceneId]=persistenceData;
        iterScenes++;
    }
    return(true);
}

bool ScenesManager::setLandsat8Parameters(QString &landSat8PathRowShpFileName,
                                          QString &landSat8PathRowShpPathField,
                                          QString &landSat8PathRowShpRowField,
                                          int &landSat8PathRowCrsEpsgCode,
                                          QMap<int, QMap<int, OGRGeometry *> > &landsat8PathRowGeometries,
                                          QVector<QString> &landsat8BandsToUse,
                                          QString &strError)
{
    mLandSat8PathRowShpFileName=landSat8PathRowShpFileName;
    mLandSat8PathRowShpPathField=landSat8PathRowShpPathField;
    mLandSat8PathRowShpRowField=landSat8PathRowShpRowField;
    mLandSat8PathRowCrsEpsgCode=landSat8PathRowCrsEpsgCode;
    mLandsat8PathRowGeometries=landsat8PathRowGeometries;
    mLandsat8BandsToUse=landsat8BandsToUse;
    return(true);
}

bool ScenesManager::setSentinel2Parameters(QVector<QString> &sentinel2BandsToUse,
                                           QString &strError)
{
    mSentinel2BandsToUse=sentinel2BandsToUse;
    return(true);
}
