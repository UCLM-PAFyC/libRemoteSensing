#include "Scene.h"
#include "NestedGridProject.h"
#include "remotesensing_definitions.h"

using namespace RemoteSensing;

Scene::Scene(QString id,
             libCRS::CRSTools *ptrCrsTools):
    mPtrCrsTools(ptrCrsTools),
    mId(id)
{
    mPtrNestedGridProject=NULL;
    mStdOut = new QTextStream(stdout);
}

QString Scene::getType()
{
    QString sceneType;
    if(mSceneType==Landsat8)
    {
        sceneType=REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID;
    }
    else if(mSceneType==Sentinel2)
    {
        sceneType=REMOTESENSING_SENTINEL2_SPACECRAFT_ID;
    }
    return(sceneType);
}

bool Scene::isLandsat8()
{
    bool isLandast8=false;
    if(mSceneType==Landsat8)
        isLandast8=true;
    return(isLandast8);
}

bool Scene::isSentinel2()
{
    bool isSentinel2=false;
    if(mSceneType==Sentinel2)
        isSentinel2=true;
    return(isSentinel2);
}

Scene::~Scene()
{
    QMap<QString,IGDAL::Raster*>::iterator iterPtrRasterBands=mPtrRasterBands.begin();
    while(iterPtrRasterBands!=mPtrRasterBands.end())
    {
        QString id=iterPtrRasterBands.key();
        delete(iterPtrRasterBands.value());
        mPtrRasterBands[id]=NULL;
    }
}
