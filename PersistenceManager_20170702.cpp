#include <QFile>
#include <QDir>
#include <QDate>
#include <QDateTime>

#include "Algorithms.h"
#include "PersistenceManager.h"
#include "NestedGridTools.h"
#include "CRSTools.h"
#include "remotesensing_definitions.h"
#include "SceneLandsat8_definitions.h"
#include "SceneSentinel2_definitions.h"
#include "algorithms_definitions.h"

//#include "SpatiaLite.h"

using namespace RemoteSensing;

PersistenceManager::PersistenceManager(QObject *parent) :
    QObject(parent)
{
    mPtrDb=NULL;
    mSRID=-1;
    mInitialJd=-1;
    mFinalJd=-1;
    mCrsDescription="";
    mCrsPrecision=PERSISTENCEMANAGER_CRS_PRECISION;
    mPtrAlgoritmhs=NULL;
    mZonesCodes.clear();
    mResultsPathByZone.clear();
    mInitialDateByZone.clear();
    mFinalDateByZone.clear();
    mOutputSridByZone.clear();
    mIdByZone.clear();
}

bool PersistenceManager::createDatabase(QString templateDb,
                                        QString fileName,
                                        QString proj4Text,
                                        QString geoCrsBaseProj4Text,
                                        QString nestedGridLocalParameters,
                                        int& srid,
                                        QString &strError,
                                        QString sqlCreateFileName)
{
    if(!QFile::exists(templateDb))
    {
        strError=QObject::tr("PersistenceManager::createDatabase");
        strError+=QObject::tr("\nNot exists template database:\n%1").arg(templateDb);
        return(false);
    }
    if(!QFile::copy(templateDb,fileName))
    {
        QString title=QObject::tr("PersistenceManager::createDatabase");
        QString strError=QObject::tr("Error copying template database in new database:\n%1\nCheck permissions and try again")
                .arg(fileName);
        return(false);
    }
    if(!sqlCreateFileName.isEmpty())
    {
        if(!QFile::exists(sqlCreateFileName))
        {
            strError=QObject::tr("PersistenceManager::createDatabase");
            strError+=QObject::tr("\nNot exists sql create tables file name:\n%1").arg(sqlCreateFileName);
            return(false);
        }
    }
    if(mPtrDb!=NULL)
    {
        delete(mPtrDb);
    }
    mPtrDb=new IGDAL::SpatiaLite();
    QString strAuxError;
    if(!mPtrDb->create(fileName,proj4Text,srid,
                       strAuxError,sqlCreateFileName))
    {
        strError=QObject::tr("PersistenceManager::createDatabase");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    if(!loadRasterUnitConversions(strAuxError))
    {
        strError=QObject::tr("PersistenceManager::createDatabase");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    mSRID=srid;
    mCrsDescription=proj4Text;
    mGeographicCrsBaseProj4Text=geoCrsBaseProj4Text;
    mNestedGridLocalParameters=nestedGridLocalParameters;
    // Table: nested_grid
    {
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_TABLE_NAME;
//        QString fieldName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
//        QString fieldValue=sceneId;
//        bool existsRegister=false;
        QString strAuxError;
//        QVector<QString> auxFieldsNames;
//        QVector<QString> auxFieldsValues;
//        auxFieldsNames.push_back(fieldName);
//        auxFieldsValues.push_back(fieldValue);
//        if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
//        {
//            strError=QObject::tr("PersistenceManager::insertLandsat8Scene");
//            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
//            return(false);
//        }
//        if(existsRegister)
//        {
//            return(true);
//        }
        QVector<QString> fieldsNames;
        QVector<QString> fieldsValues;
        QVector<QString> fieldsTypes;
        QMap<QString,QString> foreignTablesByFieldName;
        QMap<QString,QString> foreignFieldsNamesByFieldName;
        QMap<QString,QString> foreignFieldsTypesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_GEOGRAPHIC_CRS_PROJ4);
        fieldsValues.push_back(geoCrsBaseProj4Text);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_GEOGRAPHIC_CRS_PROJ4_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_PROJECTED_CRS_PROJ4);
        fieldsValues.push_back(proj4Text);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_PROJECTED_CRS_PROJ4_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_LOCAL_PARAMETERS);
        fieldsValues.push_back(nestedGridLocalParameters);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_LOCAL_PARAMETERS_FIELD_TYPE);

        if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                                   foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                                   foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                                   foreignFieldsMatchingsTypesByFieldName,
                                   strAuxError))
        {
            strError=QObject::tr("PersistenceManager::createDatabase");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
    }
    return(true);
}

QString PersistenceManager::getDatabaseFileName()
{
    QString databaseFileName="";
    if(mPtrDb!=NULL)
    {
        databaseFileName=mPtrDb->getFileName();
    }
    return(databaseFileName);
}

bool PersistenceManager::getInitialDate(int &initialJd,
                                        QString &strError)
{
    if(mInitialJd!=-1)
    {
        initialJd=mInitialJd;
    }
    else
    {
        QString strAuxError;
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        QVector<QString> fieldsNames;
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
        QVector<QVector<QString> > fieldsValues;
        QVector<QString> whereFieldsNames;
        QVector<QString> whereFieldsValues;
        QVector<QString> whereFieldsTypes;
        if(!mPtrDb->select(tableName,fieldsNames,fieldsValues,
                           whereFieldsNames,whereFieldsValues,whereFieldsTypes,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::getInitialDate");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        initialJd=100000000;
        int finalJd=-100000000;
        for(int i=0;i<fieldsValues.size();i++)
        {
            QString strValue=fieldsValues[i][0];
            bool okToInt=false;
            int intValue=strValue.toInt(&okToInt);
            if(!okToInt)
            {
                strError=QObject::tr("PersistenceManager::getInitialDate");
                strError+=QObject::tr("\nError: value: %1 is not an integer").arg(strValue);
                return(false);
            }
            if(intValue<initialJd)
            {
                initialJd=intValue;
            }
            if(intValue>finalJd)
            {
                finalJd=intValue;
            }
        }
        mInitialJd=initialJd;
        mFinalJd=finalJd;
    }
    return(true);
}

bool PersistenceManager::getFinalDate(int &finalJd,
                                      QString &strError)
{
    if(mFinalJd!=-1)
    {
        finalJd=mFinalJd;
    }
    else
    {
        QString strAuxError;
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        QVector<QString> fieldsNames;
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
        QVector<QVector<QString> > fieldsValues;
        QVector<QString> whereFieldsNames;
        QVector<QString> whereFieldsValues;
        QVector<QString> whereFieldsTypes;
        if(!mPtrDb->select(tableName,fieldsNames,fieldsValues,
                           whereFieldsNames,whereFieldsValues,whereFieldsTypes,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::getInitialDate");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        int initialJd=100000000;
        finalJd=-100000000;
        for(int i=0;i<fieldsValues.size();i++)
        {
            QString strValue=fieldsValues[i][0];
            bool okToInt=false;
            int intValue=strValue.toInt(&okToInt);
            if(!okToInt)
            {
                strError=QObject::tr("PersistenceManager::getInitialDate");
                strError+=QObject::tr("\nError: value: %1 is not an integer").arg(strValue);
                return(false);
            }
            if(intValue<initialJd)
            {
                initialJd=intValue;
            }
            if(intValue>finalJd)
            {
                finalJd=intValue;
            }
        }
        mInitialJd=initialJd;
        mFinalJd=finalJd;
    }
    return(true);
}

bool PersistenceManager::getSRID(QString proj4Text,
                                 int &srid,
                                 QString &strError)
{
    if(mSRID!=-1)
    {
        srid=mSRID;
        return(true);
    }
    QString strAuxError;
    if(!mPtrDb->getSRIDFromProj4text(proj4Text,mSRID,strAuxError))
    {
        strError=QObject::tr("PersistenceManager::getSRID");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    srid=mSRID;
    mCrsDescription=proj4Text;
    return(true);
}

bool PersistenceManager::getIsDatabaseDefined()
{
    bool success=false;
    if(mPtrDb!=NULL)
    {
        success=true;
    }
    return(success);
}

bool PersistenceManager::getProjectCodes(QVector<QString> &projectCodes,
                                         QString &strError)
{
    if(mPtrDb==NULL)
    {
        strError=QObject::tr("PersistenceManager::getProjectCodes");
        strError+=QObject::tr("\nPointer to database is null");
        return(false);
    }
    QString strAuxError;
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_TABLE_NAME;
    QVector<QString> fieldsNames;
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE);
    QVector<QVector<QString> > fieldsValues;
    QVector<QString> whereFieldsNames;
    QVector<QString> whereFieldsValues;
    QVector<QString> whereFieldsTypes;
    if(!mPtrDb->select(tableName,fieldsNames,fieldsValues,
                       whereFieldsNames,whereFieldsValues,whereFieldsTypes,
                       strAuxError))
    {
        strError=QObject::tr("PersistenceManager::getProjectCodes");
        strError+=QObject::tr("\nError getting project codes from database:\n%1\nError:\n%2")
                .arg(mPtrDb->getFileName()).arg(strAuxError);
        return(false);
    }
    for(int i=0;i<fieldsValues.size();i++)
    {
        projectCodes.push_back(fieldsValues[i][0]);
    }
    return(true);
}

bool PersistenceManager::initializeAlgorithms(libCRS::CRSTools* ptrCrsTools,
                                              NestedGrid::NestedGridTools* ptrNestedGridTools,
                                              IGDAL::libIGDALProcessMonitor* ptrLibIGDALProcessMonitor,
                                              QString &libPath,
                                              QString &strError)
{
    QDir currentDir=QDir::current();
    if(!currentDir.exists(libPath))
    {
        strError=QObject::tr("PersistenceManager::initializeAlgorithms");
        strError+=QObject::tr("\nNot exists lib path:\n%1").arg(libPath);
        return(false);
    }
    if(mPtrAlgoritmhs!=NULL)
    {
        delete(mPtrAlgoritmhs);
        mPtrAlgoritmhs=NULL;
    }
    mPtrAlgoritmhs=new Algorithms(ptrCrsTools,
                                  ptrNestedGridTools,
                                  ptrLibIGDALProcessMonitor,
                                  this);
    QString strAuxError;
    if(!mPtrAlgoritmhs->initialize(libPath,strAuxError))
    {
        strError=QObject::tr("PersistenceManager::initializeAlgorithms");
        strError+=QObject::tr("\nError initializing Algorithms:\n%1").arg(strAuxError);
        delete(mPtrAlgoritmhs);
        mPtrAlgoritmhs=NULL;
        return(false);
    }
    return(true);
}

bool PersistenceManager::insertLandsat8Scene(QString sceneId,
                                             int jd,
                                             QString metadataFileName,
                                             QVector<QString> metadataTags,
                                             QVector<QString> metadataValues,
                                             QString &strError)
{
    // Table: raster_files
    {
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        QString fieldName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
        QString fieldValue=sceneId;
        bool existsRegister=false;
        QString strAuxError;
        QVector<QString> auxFieldsNames;
        QVector<QString> auxFieldsValues;
        auxFieldsNames.push_back(fieldName);
        auxFieldsValues.push_back(fieldValue);
        if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertLandsat8Scene");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(existsRegister)
        {
            return(true);
        }
        QVector<QString> fieldsNames;
        QVector<QString> fieldsValues;
        QVector<QString> fieldsTypes;
        QMap<QString,QString> foreignTablesByFieldName;
        QMap<QString,QString> foreignFieldsNamesByFieldName;
        QMap<QString,QString> foreignFieldsTypesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
        fieldsValues.push_back(sceneId);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
        fieldsValues.push_back(QString::number(jd));
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID);
        fieldsValues.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8);
        fieldsTypes.push_back(SPATIALITE_FIELD_TYPE_FOREIGN_KEY);
        foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_TABLE_NAME;
        foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID;
        foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID_FIELD_TYPE;
        foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE;
        foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE_FIELD_TYPE;
        if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                                   foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                                   foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                                   foreignFieldsMatchingsTypesByFieldName,
                                   strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertOrthoimage");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
    }

    // Table: landsat8_metadata
    {
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_TABLE_NAME;
        QString fieldName=PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_METADATA_FILE;
        QString fieldValue=metadataFileName;
        bool existsRegister=false;
        QString strAuxError;
        QVector<QString> auxFieldsNames;
        QVector<QString> auxFieldsValues;
        auxFieldsNames.push_back(fieldName);
        auxFieldsValues.push_back(fieldValue);
        if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertLandsat8Scene");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(existsRegister)
        {
            return(true);
        }
        QVector<QString> fieldsNames;
        QVector<QString> fieldsValues;
        QVector<QString> fieldsTypes;
        QMap<QString,QString> foreignTablesByFieldName;
        QMap<QString,QString> foreignFieldsNamesByFieldName;
        QMap<QString,QString> foreignFieldsTypesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_METADATA_FILE);
        fieldsValues.push_back(metadataFileName);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_METADATA_FILE_FIELD_TYPE);

        QVector<QString> tagsNames,tagsTypes;
        QVector<int> tagsPrecisions;
        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B1);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B1_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B1_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B1);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B1_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B1_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B2);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B2_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B2_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B2);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B2_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B2_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B3);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B3_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B3_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B3);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B3_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B3_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B4);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B4_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B4_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B4);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B4_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B4_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B5);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B5_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B5_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B5);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B5_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B5_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B6);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B6_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B6_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B6);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B6_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B6_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B7);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B7_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B7_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B7);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B7_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B7_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B8);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B8_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B8_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B8);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B8_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B8_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B9);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B9_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B9_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B9);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B9_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B9_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B10);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B10_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B10_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B10);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B10_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B10_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B11);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B11_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B11_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B11);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B11_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B11_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B1);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B1_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B1_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B1);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B1_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B1_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B2);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B2_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B2_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B2);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B2_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B2_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B3);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B3_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B3_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B3);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B3_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B3_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B4);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B4_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B4_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B4);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B4_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B4_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B5);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B5_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B5_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B5);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B5_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B5_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B6);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B6_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B6_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B6);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B6_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B6_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B7);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B7_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B7_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B7);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B7_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B7_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B8);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B8_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B8_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B8);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B8_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B8_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B9);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B9_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B9_FIELD_TYPE);

        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B9);
        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B9_FIELD_PRECISION);
        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B9_FIELD_TYPE);

        for(int nt=0;nt<tagsNames.size();nt++)
        {
            QString tagName=tagsNames[nt];
            int tagPrecision=tagsPrecisions[nt];
            QString tagType=tagsTypes[nt];
            int pos=metadataTags.indexOf(tagName);
            if(pos==-1)
            {
                pos=metadataTags.indexOf(tagName.toLower());
                if(pos==-1)
                {
                    pos=metadataTags.indexOf(tagName.toUpper());
                }
            }
            if(pos==-1)
            {
                strError=QObject::tr("PersistenceManager::insertLandsat8Scene");
                strError+=QObject::tr("\nFor Landsat8 scene: %1 not exists tag: %2").arg(sceneId).arg(tagName);
                return(false);
            }
            double dblValue=0.0;
            bool okToDobule=false;
            dblValue=metadataValues.at(pos).toDouble(&okToDobule);
            if(!okToDobule)
            {
                strError=QObject::tr("PersistenceManager::insertLandsat8Scene");
                strError+=QObject::tr("\nFor Landsat8 scene: %1 for tag: %2 value is not a double: %3").arg(sceneId).arg(tagName).arg(metadataValues.at(pos));
                return(false);
            }
            fieldsNames.push_back(tagName);
            fieldsValues.push_back(QString::number(dblValue,'f',tagPrecision));
            fieldsTypes.push_back(tagType);
        }

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID);
        fieldsValues.push_back(sceneId);
        fieldsTypes.push_back(SPATIALITE_FIELD_TYPE_FOREIGN_KEY);
        foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID;
        foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID_FIELD_TYPE;
        foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
        foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE;
        if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                                   foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                                   foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                                   foreignFieldsMatchingsTypesByFieldName,
                                   strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertLandsat8Scene");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
    }
    return(true);
}

bool PersistenceManager::insertNdviTuplekeyFile(QString tuplekey,
                                               QString rasterFile,
                                               QString computationMethod,
                                               QString rasterUnitConversion,
                                               QString ndviFileName,
                                               QString &strError)
{
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES;
    QString strAuxError;
    // Para ver si existe el registro tengo que ver si hay un registro con el mismo: quadkey,filename,computationMethod
    // Obtengo el id para el quadkey en la tabla quadkeys
    int quadkeyId;
    {
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(tuplekey);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(SPATIALITE_FIELD_TYPE_TEXT);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertNdviQuadkeyFile");
            strError+=QObject::tr("\nError recovering quadkey id for ndvi file:\n%1").arg(ndviFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::insertNdviQuadkeyFile");
            strError+=QObject::tr("\nError recovering quadkey id for ndvi file:\n%1").arg(ndviFileName);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        quadkeyId=auxFieldsValues[0][0].toInt();
    }
    int computationMethodId;
    {
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(computationMethod);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(SPATIALITE_FIELD_TYPE_TEXT);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertNdviQuadkeyFile");
            strError+=QObject::tr("\nError recovering computation method id for ndvi file:\n%1").arg(ndviFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::insertNdviQuadkeyFile");
            strError+=QObject::tr("\nError recovering computation method id for ndvi file:\n%1").arg(ndviFileName);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        computationMethodId=auxFieldsValues[0][0].toInt();
    }
    // Compruebo si existe el registro
    {
        bool existsRegister=false;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID);
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_FILE_NAME);
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID);
        QVector<QString> auxFieldsValues;
        auxFieldsValues.push_back(QString::number(quadkeyId));
        auxFieldsValues.push_back(ndviFileName);
        auxFieldsValues.push_back(QString::number(computationMethodId));
        if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertNdviQuadkeyFile");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(existsRegister)
        {
            return(true);
        }
    }
    QVector<QString> fieldsNames;
    QVector<QString> fieldsValues;
    QVector<QString> fieldsTypes;
    QMap<QString,QString> foreignTablesByFieldName;
    QMap<QString,QString> foreignFieldsNamesByFieldName;
    QMap<QString,QString> foreignFieldsTypesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_FILE_NAME);
    fieldsValues.push_back(ndviFileName);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_FILE_NAME_FIELD_TYPE);

    // foreign_key quadkey_id
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID);
    fieldsValues.push_back(tuplekey);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID_FIELD_TYPE);
    foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
    foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
    foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID_FIELD_TYPE;
    foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY;
    foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY_FIELD_TYPE;

    // foreign_key raster_file_id
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID);
    fieldsValues.push_back(rasterFile);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID_FIELD_TYPE);
    foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
    foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID;
    foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID_FIELD_TYPE;
    foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
    foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE;

    // foreign_key ruc_id
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID);
    fieldsValues.push_back(rasterUnitConversion);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID_FIELD_TYPE);
    foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS;
    foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_ID;
    foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_ID_FIELD_TYPE;
    foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_TYPE;
    foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_TYPE_FIELD_TYPE;

    // foreign_key cm_id
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID);
    fieldsValues.push_back(computationMethod);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID_FIELD_TYPE);
    foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS;
    foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID;
    foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID_FIELD_TYPE;
    foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE;
    foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE_FIELD_TYPE;

    if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                               foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                               foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                               foreignFieldsMatchingsTypesByFieldName,
                               strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertNdviQuadkeyFile");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    return(true);
}

bool PersistenceManager::insertSentinel2Scene(QString sceneId,
                                              int jd,
                                              QString metadataFileName,
                                              QVector<QString> metadataTags,
                                              QVector<QString> metadataValues,
                                              QString &strError)
{
    // Table: raster_files
    {
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        QString fieldName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
        QString fieldValue=sceneId;
        bool existsRegister=false;
        QString strAuxError;
        QVector<QString> auxFieldsNames;
        QVector<QString> auxFieldsValues;
        auxFieldsNames.push_back(fieldName);
        auxFieldsValues.push_back(fieldValue);
        if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertSentinel2Scene");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(existsRegister)
        {
            return(true);
        }
        QVector<QString> fieldsNames;
        QVector<QString> fieldsValues;
        QVector<QString> fieldsTypes;
        QMap<QString,QString> foreignTablesByFieldName;
        QMap<QString,QString> foreignFieldsNamesByFieldName;
        QMap<QString,QString> foreignFieldsTypesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
        fieldsValues.push_back(sceneId);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
        fieldsValues.push_back(QString::number(jd));
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID);
        fieldsValues.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2);
        fieldsTypes.push_back(SPATIALITE_FIELD_TYPE_FOREIGN_KEY);
        foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_TABLE_NAME;
        foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID;
        foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID_FIELD_TYPE;
        foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE;
        foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE_FIELD_TYPE;
        if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                                   foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                                   foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                                   foreignFieldsMatchingsTypesByFieldName,
                                   strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertSentinel2Scene");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
    }

    // Table: sentinel2_metadata
    {
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_TABLE_NAME;
        QString fieldName=PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_METADATA_FILE;
        QString fieldValue=metadataFileName;
        bool existsRegister=false;
        QString strAuxError;
        QVector<QString> auxFieldsNames;
        QVector<QString> auxFieldsValues;
        auxFieldsNames.push_back(fieldName);
        auxFieldsValues.push_back(fieldValue);
        if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertSentinel2Scene");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(existsRegister)
        {
            return(true);
        }
        QVector<QString> fieldsNames;
        QVector<QString> fieldsValues;
        QVector<QString> fieldsTypes;
        QMap<QString,QString> foreignTablesByFieldName;
        QMap<QString,QString> foreignFieldsNamesByFieldName;
        QMap<QString,QString> foreignFieldsTypesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_METADATA_FILE);
        fieldsValues.push_back(metadataFileName);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_METADATA_FILE_FIELD_TYPE);

        QVector<QString> tagsNames,tagsTypes;
        QVector<int> tagsPrecisions;
//        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION);
//        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION_FIELD_PRECISION);
//        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION_FIELD_TYPE);

//        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH);
//        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH_FIELD_PRECISION);
//        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH_FIELD_TYPE);

//        tagsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE);
//        tagsPrecisions.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE_FIELD_PRECISION);
//        tagsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE_FIELD_TYPE);

        for(int nt=0;nt<tagsNames.size();nt++)
        {
            QString tagName=tagsNames[nt];
            int tagPrecision=tagsPrecisions[nt];
            QString tagType=tagsTypes[nt];
            int pos=metadataTags.indexOf(tagName);
            if(pos==-1)
            {
                pos=metadataTags.indexOf(tagName.toLower());
                if(pos==-1)
                {
                    pos=metadataTags.indexOf(tagName.toUpper());
                }
            }
            if(pos==-1)
            {
                strError=QObject::tr("PersistenceManager::insertSentinel2Scene");
                strError+=QObject::tr("\nFor Sentinel2 scene: %1 not exists tag: %2").arg(sceneId).arg(tagName);
                return(false);
            }
            double dblValue=0.0;
            bool okToDobule=false;
            dblValue=metadataValues.at(pos).toDouble(&okToDobule);
            if(!okToDobule)
            {
                strError=QObject::tr("PersistenceManager::insertSentinel2Scene");
                strError+=QObject::tr("\nFor Sentinel2 scene: %1 for tag: %2 value is not a double: %3").arg(sceneId).arg(tagName).arg(metadataValues.at(pos));
                return(false);
            }
            fieldsNames.push_back(tagName);
            fieldsValues.push_back(QString::number(dblValue,'f',tagPrecision));
            fieldsTypes.push_back(tagType);
        }

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_ID);
        fieldsValues.push_back(sceneId);
        fieldsTypes.push_back(SPATIALITE_FIELD_TYPE_FOREIGN_KEY);
        foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID;
        foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID_FIELD_TYPE;
        foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
        foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE;
        if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                                   foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                                   foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                                   foreignFieldsMatchingsTypesByFieldName,
                                   strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertSentinel2Scene");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
    }
    return(true);
}

bool PersistenceManager::loadRasterUnitConversions(QString &strError)
{
    if(mPtrDb==NULL)
    {
        strError=QObject::tr("PersistenceManager::loadRasterUnitConversions");
        strError+=QObject::tr("\nPointer to database is null");
        return(false);
    }
    mRUCGains.clear();
    mRUCOffsets.clear();
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS;
    QVector<QString> fieldsNames;
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_TYPE);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET);
    QVector<QVector<QString> > fieldsValues;
    QVector<QString> whereFieldsNames;
    QVector<QString> whereFieldsValues;
    QVector<QString> whereFieldsTypes;
    QString strAuxError;
    if(!mPtrDb->select(tableName,fieldsNames,fieldsValues,
                       whereFieldsNames,whereFieldsValues,whereFieldsTypes,
                       strAuxError))
    {
        strError=QObject::tr("PersistenceManager::loadRasterUnitConversions");
        strError+=QObject::tr("\nError recovering raster unit conversions from database:\n%1").arg(mPtrDb->getFileName());
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    bool validResult=true;
    if(fieldsValues.size()<1)
    {
        validResult=false;
    }
    if(validResult)
    {
        for(int nr=0;nr<fieldsValues.size();nr++)
        {
            if(fieldsValues[0].size()<3)
            {
                validResult=false;
                break;
            }
            QString conversion=fieldsValues.at(nr)[0];
            double gain=fieldsValues.at(nr)[1].toDouble();
            double offset=fieldsValues.at(nr)[2].toDouble();
            mRUCGains[conversion]=gain;
            mRUCOffsets[conversion]=offset;
        }
    }
    if(!validResult)
    {
        strError=QObject::tr("PersistenceManager::loadRasterUnitConversions");
        strError+=QObject::tr("\nError recovering raster unit conversions from database:\n%1").arg(mPtrDb->getFileName());
        strError+=QObject::tr("\nInvalid result");
        return(false);
    }
    return(true);
}

bool PersistenceManager::insertOrthoimage(QString orthoimageId,
                                          int jd,
                                          QString &strError)
{
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
    QString fieldName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
    QString fieldValue=orthoimageId;
    bool existsRegister=false;
    QString strAuxError;
    QVector<QString> auxFieldsNames;
    QVector<QString> auxFieldsValues;
    auxFieldsNames.push_back(fieldName);
    auxFieldsValues.push_back(fieldValue);
    if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertOrthoimage");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    if(existsRegister)
    {
        return(true);
    }
    QVector<QString> fieldsNames;
    QVector<QString> fieldsValues;
    QVector<QString> fieldsTypes;
    QMap<QString,QString> foreignTablesByFieldName;
    QMap<QString,QString> foreignFieldsNamesByFieldName;
    QMap<QString,QString> foreignFieldsTypesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
    fieldsValues.push_back(orthoimageId);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
    fieldsValues.push_back(QString::number(jd));
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID);
    fieldsValues.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_ORTHOIMAGE);
    fieldsTypes.push_back(SPATIALITE_FIELD_TYPE_FOREIGN_KEY);
    foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_TABLE_NAME;
    foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID;
    foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID_FIELD_TYPE;
    foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE;
    foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE_FIELD_TYPE;
    if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                               foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                               foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                               foreignFieldsMatchingsTypesByFieldName,
                               strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertOrthoimage");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    return(true);
}

bool PersistenceManager::deletePiasFile(int piasFileId,
                                        QString &strError)
{
    QString strAuxError;
    // Elimino las entradas en raster_files_by_pias_files
    {
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES;
        QVector<QString> whereFieldsNames;
        whereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID);
        QVector<QString> whereFieldsValues;
        whereFieldsValues.push_back(QString::number(piasFileId));
        QVector<QString> whereFieldsTypes;
        whereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID_FIELD_TYPE);
        if(!mPtrDb->deleteRegisters(tableName,whereFieldsNames,whereFieldsValues,whereFieldsTypes,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::deletePiasFile");
            strError+=QObject::tr("\nError deleting pias file id: %1\n in table: %2")
                    .arg(QString::number(piasFileId)).arg(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
    }
    // Elimino el registro en pias_files
    {
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES;
        QVector<QString> whereFieldsNames;
        whereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID);
        QVector<QString> whereFieldsValues;
        whereFieldsValues.push_back(QString::number(piasFileId));
        QVector<QString> whereFieldsTypes;
        whereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID_FIELD_TYPE);
        if(!mPtrDb->deleteRegisters(tableName,whereFieldsNames,whereFieldsValues,whereFieldsTypes,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::deletePiasFile");
            strError+=QObject::tr("\nError deleting pias file id: %1\n in table: %2")
                    .arg(QString::number(piasFileId)).arg(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
    }
    return(true);
}

bool PersistenceManager::getExistsPiasTuplekeyFile(QString tuplekey,
                                                   QVector<QString> rasterFiles,
                                                   QString computationMethod,
                                                   QString &previousPiasFileName,
                                                   int &previousPiasFileId,
                                                   QString &strError)
{
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES;
    previousPiasFileName="";
    previousPiasFileId=-1;
    QString strAuxError;
    // Para ver si existe el registro tengo que ver si hay un registro con el mismo: quadkey y los mismos ficheros
    // Obtengo el id para el quadkey en la tabla quadkeys
    int tuplekeyId;
    {
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(tuplekey);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(SPATIALITE_FIELD_TYPE_TEXT);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
            strError+=QObject::tr("\nError recovering quadkey id for pias file:\n%1").arg(previousPiasFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
            strError+=QObject::tr("\nError recovering quadkey id for pias file:\n%1").arg(previousPiasFileName);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        tuplekeyId=auxFieldsValues[0][0].toInt();
    }
    int computationMethodId;
    {
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(computationMethod);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(SPATIALITE_FIELD_TYPE_TEXT);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
            strError+=QObject::tr("\nError recovering computation method id for pias file:\n%1").arg(previousPiasFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
            strError+=QObject::tr("\nError recovering computation method id for pias file:\n%1").arg(previousPiasFileName);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        computationMethodId=auxFieldsValues[0][0].toInt();
    }
    // Recupero los id para los raster_files
    QMap<QString,int> rasterFileIdByRasterFile;
    for(int nrf=0;nrf<rasterFiles.size();nrf++)
    {
        QString rasterFile=rasterFiles.at(nrf);
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(rasterFile);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
            strError+=QObject::tr("\nError recovering raster file id for raster file:\n%1").arg(rasterFile);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
            strError+=QObject::tr("\nError recovering raster file id for raster file:\n%1").arg(rasterFile);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        rasterFileIdByRasterFile[rasterFile]=auxFieldsValues[0][0].toInt();
    }
    // Compruebo si existe el registro
    QVector<int> previousPiasFileIds;
    QVector<QString> previousPiasFileNames;
    bool existsRegister=false;
    QVector<QString> auxFieldsNames;
    auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
    QVector<QString> auxFieldsValues;
    auxFieldsValues.push_back(QString::number(tuplekeyId));
    if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
    {
        strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    if(existsRegister) // además tiene que estar con el mismo método y con los mismos ficheros
    {
        // recupero el id del pias_file
        //            int piasFileId;
        {
            QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES;
            QVector<QString> auxFieldsNames;
            auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID);
            auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME);
            QVector<QVector<QString> > auxFieldsValues;
            QVector<QString> auxWhereFieldsNames;
            auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
            auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID);
            QVector<QString> auxWhereFieldsValues;
            auxWhereFieldsValues.push_back(QString::number(tuplekeyId));
            auxWhereFieldsValues.push_back(QString::number(computationMethodId));
            QVector<QString> auxWhereFieldsTypes;
            auxWhereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID_FIELD_TYPE);
            auxWhereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID_FIELD_TYPE);
            if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                               auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                               strAuxError))
            {
                strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
                strError+=QObject::tr("\nError recovering pias file id for quadkey id:\n%1").arg(QString::number(tuplekeyId));
                strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                return(false);
            }
            if(auxFieldsValues.size()>0)
            {
                for(int pp=0;pp<auxFieldsValues.size();pp++)
                {
                    previousPiasFileIds.push_back(auxFieldsValues[pp][0].toInt());
                    previousPiasFileNames.push_back(auxFieldsValues[pp][1]);
                }
            }
        }
        for(int pp=0;pp<previousPiasFileIds.size();pp++)
        {
            QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES;
            QVector<QString> auxFieldsNames;
            auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID);
            QVector<QVector<QString> > auxFieldsValues;
            QVector<QString> auxWhereFieldsNames;
            auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID);
            QVector<QString> auxWhereFieldsValues;
            auxWhereFieldsValues.push_back(QString::number(previousPiasFileIds[pp]));
            QVector<QString> auxWhereFieldsTypes;
            auxWhereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID_FIELD_TYPE);
            if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                               auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                               strAuxError))
            {
                strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
                strError+=QObject::tr("\nError recovering raster files id for pias file:\n%1").arg(previousPiasFileName);
                strError+=QObject::tr("\nand previous pias file id:\n%1").arg(QString::number(previousPiasFileIds[pp]));
                strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                return(false);
            }
            bool validResult=true;
            if(auxFieldsValues.size()<1)
            {
                validResult=false;
            }
            if(validResult)
            {
                for(int nr=0;nr<auxFieldsValues.size();nr++)
                {
                    if(auxFieldsValues[nr].size()!=1)
                    {
                        validResult=false;
                    }
                }
            }
            if(!validResult)
            {
                strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
                strError+=QObject::tr("\nError recovering raster files id for pias file:\n%1").arg(previousPiasFileName);
                strError+=QObject::tr("\nInvalid result");
                return(false);
            }
            QVector<int> rasterFilesIdInDatabase;
            for(int nr=0;nr<auxFieldsValues.size();nr++)
            {
                rasterFilesIdInDatabase.push_back(auxFieldsValues.at(nr)[0].toInt());
            }
            bool equalRasterFilesIds=true;
            QMap<QString,int>::const_iterator iterAux=rasterFileIdByRasterFile.begin();
            while(iterAux!=rasterFileIdByRasterFile.end())
            {
                int idValue=iterAux.value();
                if(rasterFilesIdInDatabase.indexOf(idValue)!=-1)
                {
                    iterAux++;
                }
                else
                {
                    equalRasterFilesIds=false;
                    break;
                }
            }
            if(equalRasterFilesIds)
            {
                if(!previousPiasFileName.isEmpty())
                {
                    strError=QObject::tr("PersistenceManager::getExistsPiasTuplekeyFile");
                    strError+=QObject::tr("\nThere are more than one pias file\nFirst file:\n\t%1\nFirst file:\n\t%2")
                            .arg(previousPiasFileName).arg(previousPiasFileNames[pp]);
                    strError+=QObject::tr("\nInvalid result");
                    return(false);
                }
                else
                {
                    previousPiasFileName=previousPiasFileNames[pp];
                    previousPiasFileId=previousPiasFileIds[pp];
                }
            }
        }
    }
    return(true);
}

bool PersistenceManager::insertPiasTuplekeyFile(QString tuplekey,
                                                QVector<QString> rasterFiles,
                                                QMap<QString, int> jdByRasterFiles,
                                                QString computationMethod,
                                                int piaValue,
                                                QString piasFileName,
                                                bool reprocessFiles,
                                                QString &strError)
{
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES;
    QString strAuxError;
    QString previousPiasFileName;
    int previousPiasFileId;
    if(!getExistsPiasTuplekeyFile(tuplekey,
                                  rasterFiles,
                                  computationMethod,
                                  previousPiasFileName,
                                  previousPiasFileId,
                                  strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertPiasTuplekeyFile");
        strError+=QObject::tr("\nError storing pias file in database:\nPias file:%1\nError:\n%2")
                .arg(piasFileName).arg(strAuxError);
        return(false);
    }
    if(!previousPiasFileName.isEmpty())
    {
        if(!reprocessFiles)
        {
            return(true);
        }
        else
        {
            QFile::remove(previousPiasFileName);
            // Eliminar en la base de datos
            if(!deletePiasFile(previousPiasFileId,
                               strAuxError))
            {
                strError=QObject::tr("PersistenceManager::insertPiasTuplekeyFile");
                strError+=QObject::tr("\nError deleting pias file in database:\nPias file:%1\nError:\n%2")
                        .arg(previousPiasFileName).arg(strAuxError);
                return(false);
            }
        }
    }
    // Para ver si existe el registro tengo que ver si hay un registro con el mismo: quadkey y los mismos ficheros
    // Obtengo el id para el quadkey en la tabla quadkeys
    int quadkeyId;
    {
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(tuplekey);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(SPATIALITE_FIELD_TYPE_TEXT);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError recovering quadkey id for pias file:\n%1").arg(piasFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError recovering quadkey id for pias file:\n%1").arg(piasFileName);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        quadkeyId=auxFieldsValues[0][0].toInt();
    }
    // Recupero los id para los raster_files
    QMap<QString,int> rasterFileIdByRasterFile;
    int initialJd=100000000;
    int finalJd=0;
    for(int nrf=0;nrf<rasterFiles.size();nrf++)
    {
        QString rasterFile=rasterFiles.at(nrf);
        int jd=jdByRasterFiles[rasterFile];
        if(jd<=initialJd)
        {
            initialJd=jd;
        }
        if(jd>=finalJd)
        {
            finalJd=jd;
        }
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(rasterFile);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError recovering raster file id for raster file:\n%1").arg(rasterFile);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError recovering raster file id for raster file:\n%1").arg(rasterFile);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        rasterFileIdByRasterFile[rasterFile]=auxFieldsValues[0][0].toInt();
    }
/*
    // Compruebo si existe el registro
    QVector<int> previousPiasFileIds;
    int piasFileId=-1; // porque me puede servir para luego
    {
        bool existsRegister=false;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
        QVector<QString> auxFieldsValues;
        auxFieldsValues.push_back(QString::number(quadkeyId));
        if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(existsRegister) // además tiene que estar con los mismos ficheros
        {
            // recupero el id del pias_file
//            int piasFileId;
            {
                QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES;
                QVector<QString> auxFieldsNames;
                auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID);
                QVector<QVector<QString> > auxFieldsValues;
                QVector<QString> auxWhereFieldsNames;
                auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
                QVector<QString> auxWhereFieldsValues;
                auxWhereFieldsValues.push_back(QString::number(quadkeyId));
                QVector<QString> auxWhereFieldsTypes;
                auxWhereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID_FIELD_TYPE);
                if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                                   auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                                   strAuxError))
                {
                    strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
                    strError+=QObject::tr("\nError recovering pias file id for quadkey id:\n%1").arg(QString::number(quadkeyId));
                    strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                    return(false);
                }
                bool validResult=true;
                if(auxFieldsValues.size()!=1)
                {
                    validResult=false;
                }
                // Pueden ser varios si usan diferentes ficheros de ndvi
//                if(validResult)
//                {
//                    if(auxFieldsValues[0].size()!=1)
//                    {
//                        validResult=false;
//                    }
//                }
                if(!validResult)
                {
                    strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
                    strError+=QObject::tr("\nError recovering pias file id for quadkey id:\n%1").arg(QString::number(quadkeyId));
                    strError+=QObject::tr("\nInvalid result");
                    return(false);
                }
                for(int pp=0;pp<auxFieldsValues[0].size();pp++)
                {
                    previousPiasFileIds.push_back(auxFieldsValues[0][pp].toInt());
                }
                if(auxFieldsValues[0].size()==1)
                {
                    piasFileId=auxFieldsValues[0][0].toInt();
                }
            }
            for(int pp=0;pp<previousPiasFileIds.size();pp++)
            {
                QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES;
                QVector<QString> auxFieldsNames;
                auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID);
                QVector<QVector<QString> > auxFieldsValues;
                QVector<QString> auxWhereFieldsNames;
                auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID);
                QVector<QString> auxWhereFieldsValues;
                auxWhereFieldsValues.push_back(QString::number(previousPiasFileIds[pp]));
                QVector<QString> auxWhereFieldsTypes;
                auxWhereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID_FIELD_TYPE);
                if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                                   auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                                   strAuxError))
                {
                    strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
                    strError+=QObject::tr("\nError recovering raster files id for pias file:\n%1").arg(piasFileName);
                    strError+=QObject::tr("\nand previous pias file id:\n%1").arg(QString::number(previousPiasFileIds[pp]));
                    strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                    return(false);
                }
                bool validResult=true;
                if(auxFieldsValues.size()<1)
                {
                    validResult=false;
                }
                if(validResult)
                {
                    for(int nr=0;nr<auxFieldsValues.size();nr++)
                    {
                        if(auxFieldsValues[nr].size()!=1)
                        {
                            validResult=false;
                        }
                    }
                }
                if(!validResult)
                {
                    strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
                    strError+=QObject::tr("\nError recovering raster files id for pias file:\n%1").arg(piasFileName);
                    strError+=QObject::tr("\nInvalid result");
                    return(false);
                }
                QVector<int> rasterFilesIdInDatabase;
                for(int nr=0;nr<auxFieldsValues.size();nr++)
                {
                    rasterFilesIdInDatabase.push_back(auxFieldsValues.at(nr)[0].toInt());
                }
                bool equalRasterFilesIds=true;
                QMap<QString,int>::const_iterator iterAux=rasterFileIdByRasterFile.begin();
                while(iterAux!=rasterFileIdByRasterFile.end())
                {
                    int idValue=iterAux.value();
                    if(rasterFilesIdInDatabase.indexOf(idValue)!=-1)
                    {
                        iterAux++;
                    }
                    else
                    {
                        equalRasterFilesIds=false;
                        break;
                    }
                }
                if(equalRasterFilesIds)
                {
                    if(!reprocessFiles)
                        return(true);
                    else // hay que eliminar el registro
                    {

                    }
                }
            }
        }
    }
*/
    int computationMethodId;
    {
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(computationMethod);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(SPATIALITE_FIELD_TYPE_TEXT);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError recovering computation method id for pias file:\n%1").arg(piasFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError recovering computation method id for pias file:\n%1").arg(piasFileName);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        computationMethodId=auxFieldsValues[0][0].toInt();
    }
    {
        QVector<QString> fieldsNames;
        QVector<QString> fieldsValues;
        QVector<QString> fieldsTypes;
        QMap<QString,QString> foreignTablesByFieldName;
        QMap<QString,QString> foreignFieldsNamesByFieldName;
        QMap<QString,QString> foreignFieldsTypesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
        QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME);
        fieldsValues.push_back(piasFileName);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_INITIAL_DATE);
        fieldsValues.push_back(QString::number(initialJd));
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_INITIAL_DATE_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FINAL_DATE);
        fieldsValues.push_back(QString::number(finalJd));
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FINAL_DATE_FIELD_TYPE);

        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_PIA_VALUE);
        fieldsValues.push_back(QString::number(piaValue));
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_PIA_VALUE_FIELD_TYPE);

        // foreign_key quadkey_id
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
        fieldsValues.push_back(tuplekey);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID_FIELD_TYPE);
        foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
        foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID_FIELD_TYPE;
        foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY;
        foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY_FIELD_TYPE;

        // foreign_key cm_id
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID);
        fieldsValues.push_back(computationMethod);
        fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID_FIELD_TYPE);
        foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS;
        foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID;
        foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID_FIELD_TYPE;
        foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE;
        foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE_FIELD_TYPE;

        if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                                   foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                                   foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                                   foreignFieldsMatchingsTypesByFieldName,
                                   strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
    }
    int piasFileId=-1;
    {
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(piasFileName);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME_FIELD_TYPE);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError recovering pias file id for pias file name:\n%1").arg(piasFileName);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
            strError+=QObject::tr("\nError recovering pias file id for pias file name:\n%1").arg(piasFileName);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        piasFileId=auxFieldsValues[0][0].toInt();
    }
    if(piasFileId==-1)
    {
        strError=QObject::tr("PersistenceManager::insertPiasTuplekeyFile");
        strError+=QObject::tr("\nError recovering stored pias file in database:\nPias file:%1\nError:\n%2")
                .arg(piasFileName).arg(strAuxError);
        return(false);
    }
    // inserto los raster_files_by_pias_file
    {
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES;
        for(int nrf=0;nrf<rasterFiles.size();nrf++)
        {
            QVector<QString> fieldsNames;
            QVector<QString> fieldsValues;
            QVector<QString> fieldsTypes;
            QMap<QString,QString> foreignTablesByFieldName;
            QMap<QString,QString> foreignFieldsNamesByFieldName;
            QMap<QString,QString> foreignFieldsTypesByFieldName;
            QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
            QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

            QString rasterFile=rasterFiles.at(nrf);
            int rasterFileId=rasterFileIdByRasterFile[rasterFile];

            // foreign_key quadkey_id
            fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID);
            fieldsValues.push_back(QString::number(rasterFileId));
            fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID_FIELD_TYPE);
            foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
            foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID;
            foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID_FIELD_TYPE;
            foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID;
            foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID_FIELD_TYPE;

            // foreign_key pias_file_id
            fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID);
            fieldsValues.push_back(QString::number(piasFileId));
            fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID_FIELD_TYPE);
            foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES;
            foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID;
            foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID_FIELD_TYPE;
            foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID;
            foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID_FIELD_TYPE;

            if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                                       foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                                       foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                                       foreignFieldsMatchingsTypesByFieldName,
                                       strAuxError))
            {
                strError=QObject::tr("PersistenceManager::insertPiasQuadkeyFile");
                strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                return(false);
            }
        }
    }
    return(true);
}

bool PersistenceManager::insertProject(QString code,
                                       QString resultsPath,
                                       int initialJd,
                                       int finalJd,
                                       int srid,
                                       QString strWktGeometry,
                                       QString &strError)
{
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_TABLE_NAME;
    QVector<QString> fieldsNames;
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_RESULTS_PATH);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_INITIAL_DATE);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_FINAL_DATE);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_OUTPUT_SRID);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_THE_GEOM);
    QVector<QString> fieldsValues;
    fieldsValues.push_back(code);
    fieldsValues.push_back(resultsPath);
    fieldsValues.push_back(QString::number(initialJd));
    fieldsValues.push_back(QString::number(finalJd));
    fieldsValues.push_back(QString::number(srid));
    fieldsValues.push_back(strWktGeometry);
    QVector<QString> fieldsTypes;
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE_FIELD_TYPE);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_RESULTS_PATH_FIELD_TYPE);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_INITIAL_DATE_FIELD_TYPE);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_FINAL_DATE_FIELD_TYPE);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_OUTPUT_SRID_FIELD_TYPE);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_THE_GEOM_FIELD_TYPE);
    QMap<QString,QString> foreignTablesByFieldName;
    QMap<QString,QString> foreignFieldsNamesByFieldName;
    QMap<QString,QString> foreignFieldsTypesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;
    QString strAuxError;
    if(!mPtrDb->insertRegister(tableName,
                               fieldsNames,fieldsValues,fieldsTypes,
                               foreignTablesByFieldName,
                               foreignFieldsNamesByFieldName,
                               foreignFieldsTypesByFieldName,
                               foreignFieldsMatchingsNamesByFieldName,
                               foreignFieldsMatchingsTypesByFieldName,
                               strAuxError,
                               srid,
                               mSRID))
    {
        strError=QObject::tr("PersistenceManager::insertProject");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    return(true);
}

bool PersistenceManager::insertTuplekey(QString tuplekey,
                                        NestedGrid::NestedGridTools *ptrNestedGridTools,
                                        QString &strError)
{
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
    QString fieldName=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY;
    QString fieldValue=tuplekey;
    bool existsRegister=false;
    QString strAuxError;
    QVector<QString> auxFieldsNames;
    QVector<QString> auxFieldsValues;
    auxFieldsNames.push_back(fieldName);
    auxFieldsValues.push_back(fieldValue);
    if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertOrthoimage");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    if(existsRegister)
    {
        return(true);
    }
    int lod,tileX,tileY;
    if(!ptrNestedGridTools->conversionTuplekeyToTileCoordinates(tuplekey,lod,tileX,tileY,strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertQuadkey");
        strError+=QObject::tr("\nError getting tile coordinates from quadkey\n");
        strError+=QObject::tr("\nError for quadkey: %1\n%2").arg(strAuxError);
        return(false);
    }
    double nwFc,nwSc,seFc,seSc;
    if(!ptrNestedGridTools->getBoundingBoxFromTile(lod,tileX,tileY,mCrsDescription,
                                                   nwFc,nwSc,seFc,seSc,
                                                   strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertQuadkey");
        strError+=QObject::tr("\nError getting bounding box from tile\n");
        strError+=QObject::tr("\nError for quadkey: %1\n%2").arg(strAuxError);
        return(false);
    }
    if(mCrsPrecision==PERSISTENCEMANAGER_CRS_PRECISION)
    {
        libCRS::CRSTools* ptrCrsTools=ptrNestedGridTools->getCrsTools();
        int crs2dPrecision,crsHPrecision;
        if(!ptrCrsTools->getCrsPrecision(mCrsDescription,crs2dPrecision,crsHPrecision,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertQuadkey");
            strError+=QObject::tr("\nError getting crs precision\n");
            strError+=QObject::tr("\nError for quadkey: %1\n%2").arg(strAuxError);
            return(false);
        }
        if(crs2dPrecision==4)
        {
            crs2dPrecision=PERSISTENCEMANAGER_CRS_PROJECTED_PRECISION;
        }
        mCrsPrecision=crs2dPrecision;
    }
    QString wktGeometry="POLYGON((";
    wktGeometry+=QString::number(nwFc,'f',mCrsPrecision);
    wktGeometry+=" ";
    wktGeometry+=QString::number(nwSc,'f',mCrsPrecision);
    wktGeometry+=",";
    wktGeometry+=QString::number(seFc,'f',mCrsPrecision);
    wktGeometry+=" ";
    wktGeometry+=QString::number(nwSc,'f',mCrsPrecision);
    wktGeometry+=",";
    wktGeometry+=QString::number(seFc,'f',mCrsPrecision);
    wktGeometry+=" ";
    wktGeometry+=QString::number(seSc,'f',mCrsPrecision);
    wktGeometry+=",";
    wktGeometry+=QString::number(nwFc,'f',mCrsPrecision);
    wktGeometry+=" ";
    wktGeometry+=QString::number(seSc,'f',mCrsPrecision);
    wktGeometry+=",";
    wktGeometry+=QString::number(nwFc,'f',mCrsPrecision);
    wktGeometry+=" ";
    wktGeometry+=QString::number(nwSc,'f',mCrsPrecision);
    wktGeometry+="))";

    QVector<QString> fieldsNames;
    QVector<QString> fieldsValues;
    QVector<QString> fieldsTypes;
    QMap<QString,QString> foreignTablesByFieldName;
    QMap<QString,QString> foreignFieldsNamesByFieldName;
    QMap<QString,QString> foreignFieldsTypesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY);
    fieldsValues.push_back(tuplekey);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_LOD);
    fieldsValues.push_back(QString::number(lod));
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_LOD_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TILE_X);
    fieldsValues.push_back(QString::number(tileX));
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TILE_X_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TILE_Y);
    fieldsValues.push_back(QString::number(tileY));
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TILE_Y_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM);
    fieldsValues.push_back(wktGeometry);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM_FIELD_TYPE);

    if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                               foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                               foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                               foreignFieldsMatchingsTypesByFieldName,
                               strAuxError,mSRID))
    {
        strError=QObject::tr("PersistenceManager::insertQuadkey");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    return(true);
}

bool PersistenceManager::insertTuplekeyRasterFile(QString id,
                                                 QString tuplekey,
                                                 QString tuplekeyRasterFile,
                                                 QString bandId,
                                                 QString &strError)
{
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_TABLE_NAME;
    QString strAuxError;
    // Obtengo el id para el quadkey en la tabla quadkeys
    int quadkeyId;
    {
        QString auxTableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID);
        QVector<QVector<QString> > auxFieldsValues;
        QVector<QString> auxWhereFieldsNames;
        auxWhereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY);
        QVector<QString> auxWhereFieldsValues;
        auxWhereFieldsValues.push_back(tuplekey);
        QVector<QString> auxWhereFieldsTypes;
        auxWhereFieldsTypes.push_back(SPATIALITE_FIELD_TYPE_TEXT);
        if(!mPtrDb->select(auxTableName,auxFieldsNames,auxFieldsValues,
                           auxWhereFieldsNames,auxWhereFieldsValues,auxWhereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertQuadkeyRasterFile");
            strError+=QObject::tr("\nError recovering id for quadkey:\n%1").arg(tuplekey);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        bool validResult=true;
        if(auxFieldsValues.size()!=1)
        {
            validResult=false;
        }
        if(validResult)
        {
            if(auxFieldsValues[0].size()!=1)
            {
                validResult=false;
            }
        }
        if(!validResult)
        {
            strError=QObject::tr("PersistenceManager::insertQuadkeyRasterFile");
            strError+=QObject::tr("\nError recovering id for quadkey:\n%1").arg(tuplekey);
            strError+=QObject::tr("\nInvalid result");
            return(false);
        }
        quadkeyId=auxFieldsValues[0][0].toInt();
    }
    // Compruebo si existe el registro
    {
        bool existsRegister=false;
        QVector<QString> auxFieldsNames;
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID);
        auxFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME);
        QVector<QString> auxFieldsValues;
        auxFieldsValues.push_back(QString::number(quadkeyId));
        auxFieldsValues.push_back(tuplekeyRasterFile);
        if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
        {
            strError=QObject::tr("PersistenceManager::insertQuadkeyRasterFile");
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        if(existsRegister)
        {
            return(true);
        }
    }
    QVector<QString> fieldsNames;
    QVector<QString> fieldsValues;
    QVector<QString> fieldsTypes;
    QMap<QString,QString> foreignTablesByFieldName;
    QMap<QString,QString> foreignFieldsNamesByFieldName;
    QMap<QString,QString> foreignFieldsTypesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME);
    fieldsValues.push_back(tuplekeyRasterFile);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID);
    fieldsValues.push_back(bandId);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID_FIELD_TYPE);

    // foreign_key quadkey_id
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID);
    fieldsValues.push_back(tuplekey);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID_FIELD_TYPE);
    foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
    foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID;
    foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID_FIELD_TYPE;
    foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY;
    foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY_FIELD_TYPE;

    // foreign_key raster_file_id
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID);
    fieldsValues.push_back(id);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID_FIELD_TYPE);
    foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
    foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID;
    foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID_FIELD_TYPE;
    foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
    foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE;

    if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                               foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                               foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                               foreignFieldsMatchingsTypesByFieldName,
                               strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertQuadkeyRasterFile");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    return(true);
}

bool PersistenceManager::insertRasterUnitConversion(QString conversion,
                                                    double gain,
                                                    double offset,
                                                    QString &strError)
{
    if(mRUCGains.contains(conversion))
    {
        if(fabs(mRUCGains[conversion]-gain)>PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN_DIFFERENCE_TOLERANCE
                ||fabs(mRUCOffsets[conversion]-offset)>PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET_DIFFERENCE_TOLERANCE)
        {
            strError=QObject::tr("PersistenceManager::insertRasterUnitConversion");
            strError+=QObject::tr("\nIn database:\n%1\nExists a conversion: %2 with different values for gain and offset")
                    .arg(mPtrDb->getFileName()).arg(conversion);
            return(false);
        }
        return(true);
    }
    else
    {
        QMap<QString,double>::const_iterator iter=mRUCGains.begin();
        while(iter!=mRUCGains.end())
        {
            QString conversionInDb=iter.key();
            double gainInDb=iter.value();
            double offsetInDb=mRUCOffsets[conversionInDb];
            if(fabs(gainInDb-gain)<PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN_DIFFERENCE_TOLERANCE
                    &&fabs(offsetInDb-offset)<PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET_DIFFERENCE_TOLERANCE)
            {
                strError=QObject::tr("PersistenceManager::insertRasterUnitConversion");
                strError+=QObject::tr("\nIn database:\n%1\nExists a conversion with equal values for gain and offset: %2")
                        .arg(mPtrDb->getFileName()).arg(conversionInDb);
                return(false);
            }
            iter++;
        }
    }
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS;
    QVector<QString> fieldsNames;
    QVector<QString> fieldsValues;
    QVector<QString> fieldsTypes;
    QMap<QString,QString> foreignTablesByFieldName;
    QMap<QString,QString> foreignFieldsNamesByFieldName;
    QMap<QString,QString> foreignFieldsTypesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_TYPE);
    fieldsValues.push_back(conversion);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_TYPE_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN);
    fieldsValues.push_back(QString::number(gain,'f',PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN_FIELD_PRECISION));
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET);
    fieldsValues.push_back(QString::number(offset,'f',PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET_FIELD_PRECISION));
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET_FIELD_TYPE);

    QString strAuxError;
    if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                               foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                               foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                               foreignFieldsMatchingsTypesByFieldName,
                               strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertRasterUnitConversion");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    mRUCGains[conversion]=gain;
    mRUCOffsets[conversion]=offset;
    return(true);
}

bool PersistenceManager::insertRSProduct(QString rsProductId,
                                         int jd,
                                         QString rsProductDataType,
                                         QString &strError)
{
    if(rsProductDataType.compare(REMOTESENSING_PRODUCT_DATA_TYPE_NDVI,Qt::CaseInsensitive)!=0)
    {
        strError=QObject::tr("PersistenceManager::insertRSProduct");
        strError+=QObject::tr("\nError: invalid data type, no %1").arg(REMOTESENSING_PRODUCT_DATA_TYPE_NDVI);
        return(false);
    }
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
    QString fieldName=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID;
    QString fieldValue=rsProductId;
    bool existsRegister=false;
    QString strAuxError;
    QVector<QString> auxFieldsNames;
    QVector<QString> auxFieldsValues;
    auxFieldsNames.push_back(fieldName);
    auxFieldsValues.push_back(fieldValue);
    if(!mPtrDb->getExistsRegister(tableName,auxFieldsNames,auxFieldsValues,existsRegister,strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertRSProduct");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    if(existsRegister)
    {
        return(true);
    }
    QVector<QString> fieldsNames;
    QVector<QString> fieldsValues;
    QVector<QString> fieldsTypes;
    QMap<QString,QString> foreignTablesByFieldName;
    QMap<QString,QString> foreignFieldsNamesByFieldName;
    QMap<QString,QString> foreignFieldsTypesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsNamesByFieldName;
    QMap<QString,QString> foreignFieldsMatchingsTypesByFieldName;

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
    fieldsValues.push_back(rsProductId);
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
    fieldsValues.push_back(QString::number(jd));
    fieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD_FIELD_TYPE);

    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID);
    fieldsValues.push_back(REMOTESENSING_PRODUCT_DATA_TYPE_NDVI);
    fieldsTypes.push_back(SPATIALITE_FIELD_TYPE_FOREIGN_KEY);
    foreignTablesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_TABLE_NAME;
    foreignFieldsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID;
    foreignFieldsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID_FIELD_TYPE;
    foreignFieldsMatchingsNamesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE;
    foreignFieldsMatchingsTypesByFieldName[PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID]=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE_FIELD_TYPE;
    if(!mPtrDb->insertRegister(tableName,fieldsNames,fieldsValues,fieldsTypes,
                               foreignTablesByFieldName,foreignFieldsNamesByFieldName,
                               foreignFieldsTypesByFieldName,foreignFieldsMatchingsNamesByFieldName,
                               foreignFieldsMatchingsTypesByFieldName,
                               strAuxError))
    {
        strError=QObject::tr("PersistenceManager::insertRSProduct");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    return(true);
}

bool PersistenceManager::openDatabase(QString fileName,
                                      QString &strError)
{
    if(mPtrDb!=NULL)
    {
        if(mPtrDb->getFileName().compare(fileName,Qt::CaseInsensitive)!=0)
        {
            delete(mPtrDb);
            mPtrDb=new IGDAL::SpatiaLite();
        }
    }
    else
    {
        mPtrDb=new IGDAL::SpatiaLite();
    }
    QString strAuxError;
    if(!mPtrDb->open(fileName,strAuxError))
    {
        strError=QObject::tr("PersistenceManager::openDatabase");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    if(!mPtrDb->getSRIDFromTableName(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_TABLE_NAME,
                                     mSRID,
                                     strAuxError))
    {
        strError=QObject::tr("PersistenceManager::openDatabase");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_TABLE_NAME;
    QVector<QString> fieldsNames;
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_GEOGRAPHIC_CRS_PROJ4);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_PROJECTED_CRS_PROJ4);
    fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_LOCAL_PARAMETERS);
    QVector<QVector<QString> > fieldsValues;
    QVector<QString> whereFieldsNames;
//    whereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE);
    QVector<QString> whereFieldsValues;
//    whereFieldsValues.push_back(zoneCode);
    QVector<QString> whereFieldsTypes;
//    whereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE_FIELD_TYPE);
    if(!mPtrDb->select(tableName,fieldsNames,fieldsValues,
                       whereFieldsNames,whereFieldsValues,whereFieldsTypes,
                       strAuxError))
    {
        strError=QObject::tr("PersistenceManager::openDatabase");
        strError+=QObject::tr("\nError recovering CRS information");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    mCrsDescription=fieldsValues[0][1];
    mGeographicCrsBaseProj4Text=fieldsValues[0][0];
    mNestedGridLocalParameters=fieldsValues[0][2];
    if(!loadRasterUnitConversions(strAuxError))
    {
        strError=QObject::tr("PersistenceManager::openDatabase");
        strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
        return(false);
    }
    return(true);
}

bool PersistenceManager::processAlgorithm(QString algorithmCode,
                                          QString zoneCode,
                                          bool mergeFiles,
                                          bool reprocessFiles,
                                          bool reprojectFiles,
                                          QString &strError)
{
    if(mPtrDb==NULL)
    {
        strError=QObject::tr("PersistenceManager::processAlgorithm");
        strError+=QObject::tr("\nObject Database is NULL");
        return(false);
    }
    if(mPtrAlgoritmhs==NULL)
    {
        strError=QObject::tr("PersistenceManager::processAlgorithm");
        strError+=QObject::tr("\nObject Algorithms is NULL");
        return(false);
    }
    QString strAuxError;
    if(!mZonesCodes.contains(zoneCode))
    {
        //select id,results_path,initial_date,final_date,output_srid from projects where code="zone1"
        QString tableName=PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_TABLE_NAME;
        QVector<QString> fieldsNames;
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_ID);
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_RESULTS_PATH);
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_INITIAL_DATE);
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_FINAL_DATE);
        fieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_OUTPUT_SRID);
        QVector<QVector<QString> > fieldsValues;
        QVector<QString> whereFieldsNames;
        whereFieldsNames.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE);
        QVector<QString> whereFieldsValues;
        whereFieldsValues.push_back(zoneCode);
        QVector<QString> whereFieldsTypes;
        whereFieldsTypes.push_back(PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE_FIELD_TYPE);
        if(!mPtrDb->select(tableName,fieldsNames,fieldsValues,
                           whereFieldsNames,whereFieldsValues,whereFieldsTypes,
                           strAuxError))
        {
            strError=QObject::tr("PersistenceManager::processAlgorithm");
            strError+=QObject::tr("\nError recovering zone information:\n%1").arg(zoneCode);
            strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
            return(false);
        }
        mZonesCodes.push_back(zoneCode);
        mIdByZone[zoneCode]=fieldsValues[0][0].toInt();
        mResultsPathByZone[zoneCode]=fieldsValues[0][1];
        mInitialDateByZone[zoneCode]=fieldsValues[0][2].toInt();
        mFinalDateByZone[zoneCode]=fieldsValues[0][3].toInt();
        mOutputSridByZone[zoneCode]=fieldsValues[0][4].toInt();
    }
    QString landsat8IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8;
    QString sentinel2IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2;
    QString orthoimageIdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_ORTHOIMAGE;
    QMap<QString,QVector<QString> > bandsInAlgorithmBySpaceCraft;
    if(algorithmCode.compare(ALGORITHMS_PIAS_CODE,Qt::CaseInsensitive)==0)
    {
        // Landsat8
        QVector<QString> bandsInAlgorithmLandsat8;
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_PIAS_LANDSAT8_BAND_CLOUD_MASKS);
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_PIAS_LANDSAT8_BAND_RED);
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_PIAS_LANDSAT8_BAND_NIR);
        bandsInAlgorithmBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID]=bandsInAlgorithmLandsat8;
        // Sentinel2
        QVector<QString> bandsInAlgorithmSentinel;
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_PIAS_SENTINEL2_BAND_CLOUD_MASKS);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_PIAS_SENTINEL2_BAND_RED);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_PIAS_SENTINEL2_BAND_NIR);
        bandsInAlgorithmBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID]=bandsInAlgorithmSentinel;
    }
    else if(algorithmCode.compare(ALGORITHMS_INTC_CODE,Qt::CaseInsensitive)==0)
    {
        // Landsat8
        QVector<QString> bandsInAlgorithmLandsat8;
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_1);
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_2);
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_3);
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_4);
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_5);
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_6);
        bandsInAlgorithmLandsat8.push_back(ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_7);
        bandsInAlgorithmBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID]=bandsInAlgorithmLandsat8;
        // Sentinel2
        QVector<QString> bandsInAlgorithmSentinel;
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_1);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_2);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_3);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_4);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_5);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_6);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_7);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_8);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_9);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_10);
        bandsInAlgorithmSentinel.push_back(ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_11);
        bandsInAlgorithmBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID]=bandsInAlgorithmSentinel;
    }
    QMap<QString,QString> orthoimagesSuffixBySpaceCraft;
    {
        QString orthoimagesSuffix="_";
        orthoimagesSuffix+=ALGORITHMS_PIAS_LANDSAT8_BAND_CLOUD_MASKS;
        orthoimagesSuffixBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID]=orthoimagesSuffix;
        orthoimagesSuffix="_";
        orthoimagesSuffix+=ALGORITHMS_PIAS_SENTINEL2_BAND_CLOUD_MASKS;
        orthoimagesSuffixBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID]=orthoimagesSuffix;
    }
    if(algorithmCode.compare(ALGORITHMS_INTC_CODE,Qt::CaseInsensitive)==0)
    {
        QString tuplekeys_raster_files_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_TABLE_NAME;
        QString raster_files_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        QString raster_types_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_TABLE_NAME;
        QString tuplekeys_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        QString projects_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_TABLE_NAME;
        QString pias_files_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES;
        QString raster_files_by_pias_files_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES;
        // Primero obtengo la información de los pias_files
        //SELECT pias_files.id,pias_files.file_name,pias_files.pia_value,pias_files.tuplekey_id
        //FROM pias_files
        //WHERE pias_files.tuplekey_id IN (
        //  SELECT tuplekeys.id FROM pias_files,tuplekeys,projects
        //  WHERE pias_files.tuplekey_id = tuplekeys.id
        //  AND intersects(tuplekeys.the_geom, projects.the_geom) AND projects.id = 2)
        QString sqlSentence,sqlSentence1,sqlSentence2,sqlSentence3;
        sqlSentence1="SELECT ";
        sqlSentence1+=(pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID);
        sqlSentence1+=(","+pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME);
        sqlSentence1+=(","+pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_PIA_VALUE);
        sqlSentence1+=(","+pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
        sqlSentence2=(" FROM "+pias_files_table_name);
        sqlSentence2+=(" WHERE "+pias_files_table_name+"."PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
        sqlSentence2+=(" IN (SELECT "+tuplekeys_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID);
        sqlSentence2+=(" FROM "+pias_files_table_name+","+tuplekeys_table_name+","+projects_table_name);
        sqlSentence3=(" WHERE "+pias_files_table_name+"."PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
        sqlSentence3+=("="+tuplekeys_table_name+"."PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID);
        sqlSentence3+=(" AND intersects("+tuplekeys_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM);
        sqlSentence3+=(","+projects_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_THE_GEOM+")");
        sqlSentence3+=(" AND "+projects_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_ID+"="+QString::number(mIdByZone[zoneCode])+")");
        QVector<int> piasFilesIds;
        QMap<int,QString> piasFileNameByPiasFilesId;
        QMap<int,int> piasValueByPiasFilesId;
        QMap<int,int> piasTuplekeyIdByPiasFilesId;
        sqlSentence=sqlSentence1+sqlSentence2+sqlSentence3;
        {
            QVector<QString> fieldsNamesToRetrieve;
            fieldsNamesToRetrieve.push_back(pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID);
            fieldsNamesToRetrieve.push_back(pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME);
            fieldsNamesToRetrieve.push_back(pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_PIA_VALUE);
            fieldsNamesToRetrieve.push_back(pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID);
            QVector<QMap<QString,QString> > fieldsValuesToRetrieve;
            if(!mPtrDb->executeSqlQuery(sqlSentence,
                                        fieldsNamesToRetrieve,
                                        fieldsValuesToRetrieve,
                                        strError))
            {
                strError=QObject::tr("PersistenceManager::processAlgorithm");
                strError+=QObject::tr("\nError executing sql sentence:\n%1\n%Error:\n%2")
                    .arg(sqlSentence).arg(zoneCode);
                strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                return(false);
            }
            for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
            {
                int piasFileId=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[0]].toInt();
                QString piasFileName=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[1]];
                int piasValue=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[2]].toInt();
                int tuplekeyId=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[3]].toInt();
                piasFilesIds.push_back(piasFileId);
                piasFileNameByPiasFilesId[piasFileId]=piasFileName;
                piasValueByPiasFilesId[piasFileId]=piasValue;
                piasTuplekeyIdByPiasFilesId[piasFileId]=tuplekeyId;
            }
        }
        QMap<int,QVector<int> > rasterFilesIdsByPiasFileId; // el nombre sin la extensión ni ruta
        QMap<int,QString> rasterFilesRasterIdByRasterFileId;
        QMap<int,int> rasterFilesJdByRasterFileId;
        QMap<int,QString> rasterFilesTypeByRasterFileId;
        QMap<int,QMap<int,QMap<QString,QString> > > tuplekeyRasterFileNameByBandByRasterFileIdByPiasFileId;
        // Despues itero para cada pias_files.id y obtengo el resto de información
        for(int i=0;i<piasFilesIds.size();i++)
        {
            int piasFileId=piasFilesIds[i];
            //SELECT raster_files.id,tuplekeys_raster_files.file_name,tuplekeys_raster_files.band_id,raster_types.type
            //FROM raster_files_by_pias_files,tuplekeys_raster_files,raster_files,raster_types
            //WHERE raster_files_by_pias_files.pias_file_id = 1124
            //AND raster_files.id = raster_files_by_pias_files.raster_file_id
            //AND tuplekeys_raster_files.raster_file_id = raster_files.id
            //AND raster_files.type_id = raster_types.id
            //AND tuplekeys_raster_files.band_id IN ('B2','B3')
            //AND raster_types.type in ('sentinel2','landsat8')
            sqlSentence1="SELECT ";
            sqlSentence1+=(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
            sqlSentence1+=(","+tuplekeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME);
            sqlSentence1+=(","+tuplekeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID);
            sqlSentence1+=(","+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
            sqlSentence1+=(","+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
            sqlSentence1+=(","+raster_types_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE);
            sqlSentence2=(" FROM "+raster_files_by_pias_files_table_name+","+tuplekeys_raster_files_table_name);
            sqlSentence2+=(","+raster_files_table_name+","+raster_types_table_name);
            sqlSentence2+=(" WHERE "+raster_files_by_pias_files_table_name+"."PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID+"="+QString::number(piasFileId));
            sqlSentence2+=(" AND "+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
            sqlSentence2+=("="+raster_files_by_pias_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID);
            sqlSentence2+=(" AND "+tuplekeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID);
            sqlSentence2+=("="+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
            sqlSentence2+=(" AND "+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID);
            sqlSentence2+=("="+raster_types_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID);
            sqlSentence2+=(" AND "+raster_types_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE);
            sqlSentence2+=(" IN ('"+landsat8IdDb+"','"+sentinel2IdDb+"')");
            sqlSentence3=(" AND "+tuplekeys_raster_files_table_name+"."PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID);
            sqlSentence3+=(" IN ('");
            for(int i=0;i<bandsInAlgorithmBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID].size();i++)
            {
                QString bandId=bandsInAlgorithmBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID][i];
                sqlSentence3+=bandId;
                sqlSentence3+="','";
    //            if(i<(bandsInAlgorithmBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID].size()-1))
    //            {
    //                sqlSentence2+="','";
    //            }
    //            else
    //            {
    //                sqlSentence2+="')";
    //            }
            }
            for(int i=0;i<bandsInAlgorithmBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID].size();i++)
            {
                QString bandId=bandsInAlgorithmBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID][i];
                sqlSentence3+=bandId;
                if(i<(bandsInAlgorithmBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID].size()-1))
                {
                    sqlSentence3+="','";
                }
                else
                {
                    sqlSentence3+="')";
                }
            }
            sqlSentence=sqlSentence1+sqlSentence2+sqlSentence3;
            {
                QVector<QString> fieldsNamesToRetrieve;
                fieldsNamesToRetrieve.push_back(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
                fieldsNamesToRetrieve.push_back(tuplekeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME);
                fieldsNamesToRetrieve.push_back(tuplekeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID);
                fieldsNamesToRetrieve.push_back(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
                fieldsNamesToRetrieve.push_back(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
                fieldsNamesToRetrieve.push_back(raster_types_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE);
                QVector<QMap<QString,QString> > fieldsValuesToRetrieve;
                if(!mPtrDb->executeSqlQuery(sqlSentence,
                                            fieldsNamesToRetrieve,
                                            fieldsValuesToRetrieve,
                                            strError))
                {
                    strError=QObject::tr("PersistenceManager::processAlgorithm");
                    strError+=QObject::tr("\nError executing sql sentence:\n%1\n%Error:\n%2")
                        .arg(sqlSentence).arg(zoneCode);
                    strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                    return(false);
                }
                QVector<int> aux;
                rasterFilesIdsByPiasFileId[piasFileId]=aux;
                for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
                {
                    int rasterFileId=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[0]].toInt();
                    QString tuplekeysRasterFilesFileName=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[1]];
                    QString bandId=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[2]];
                    QString rasterFileRasterId=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[3]];
                    int jd=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[4]].toInt();
                    QString rasterType=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[5]];
                    if(rasterFilesIdsByPiasFileId[piasFileId].indexOf(rasterFileId)==-1)
                    {
                        rasterFilesIdsByPiasFileId[piasFileId].push_back(rasterFileId);
                    }
                    if(!rasterFilesRasterIdByRasterFileId.contains(rasterFileId))
                    {
                        rasterFilesRasterIdByRasterFileId[rasterFileId]=rasterFileRasterId;
                    }
                    if(!rasterFilesJdByRasterFileId.contains(rasterFileId))
                    {
                        rasterFilesJdByRasterFileId[rasterFileId]=jd;
                    }
                    if(!rasterFilesTypeByRasterFileId.contains(rasterFileId))
                    {
                        rasterFilesTypeByRasterFileId[rasterFileId]=rasterType;
                    }
                    if(!tuplekeyRasterFileNameByBandByRasterFileIdByPiasFileId.contains(piasFileId))
                    {
                        QMap<QString,QString> aux1;
                        aux1[bandId]=tuplekeysRasterFilesFileName;
                        QMap<int,QMap<QString,QString> > aux2;
                        aux2[rasterFileId]=aux1;
                        tuplekeyRasterFileNameByBandByRasterFileIdByPiasFileId[piasFileId]=aux2;
                    }
                    else
                    {
                        if(!tuplekeyRasterFileNameByBandByRasterFileIdByPiasFileId[piasFileId].contains(rasterFileId))
                        {
                            QMap<QString,QString> aux1;
                            aux1[bandId]=tuplekeysRasterFilesFileName;
                            tuplekeyRasterFileNameByBandByRasterFileIdByPiasFileId[piasFileId][rasterFileId]=aux1;
                        }
                        else
                        {
                            tuplekeyRasterFileNameByBandByRasterFileIdByPiasFileId[piasFileId][rasterFileId][bandId]=tuplekeysRasterFilesFileName;
                        }
                    }

                }
            }
        }
        int yo=1;


    }
    if(algorithmCode.compare(ALGORITHMS_PIAS_CODE,Qt::CaseInsensitive)==0)
    {
        QString quadkeys_raster_files_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_TABLE_NAME;
        QString raster_files_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME;
        QString raster_types_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_TABLE_NAME;
        QString quadkeys_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME;
        QString projects_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_TABLE_NAME;
        QString landsat8_metadata_table_name=PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_TABLE_NAME;
        // Obtener quadkeys, ....
//        select qrf.file_name,rf.id,rf.raster_id,rt.type,qrf.band_id,q.quadkey,rf.jd
//        from quadkeys_raster_files as qrf,raster_files as rf,raster_types as rt,quadkeys as q, projects as p
//        where qrf.band_id in ('B0','B4','B5')
//        and rf.id=qrf.raster_file_id
//        and rt.id=rf.type_id
//        and rt.type in ('landsat8','orthoimage')
//        and qrf.quadkey_id=q.id
//        and rf.jd>=p.initial_date
//        and rf.jd<=p.final_date
//        and qrf.quadkey_id in (select q.id from quadkeys as q,projects as p where intersects(q.the_geom,p.the_geom) and p.id=3)
//        order by rf.jd
        QString sqlSentence,sqlSentence1,sqlSentence2,sqlSentence3;
        sqlSentence1="SELECT ";
        sqlSentence1+=(quadkeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME);
        sqlSentence1+=(","+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
        sqlSentence1+=(","+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
        sqlSentence1+=(","+raster_types_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE);
        sqlSentence1+=(","+quadkeys_raster_files_table_name+"."PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID);
        sqlSentence1+=(","+quadkeys_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY);
        sqlSentence1+=(","+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
        sqlSentence1+=(" FROM "+quadkeys_raster_files_table_name+","+raster_files_table_name+","+raster_types_table_name);
        sqlSentence1+=(","+quadkeys_table_name+","+projects_table_name);
        sqlSentence2=(" WHERE "+quadkeys_raster_files_table_name+"."PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID);
        sqlSentence2+=(" IN ('");
        for(int i=0;i<bandsInAlgorithmBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID].size();i++)
        {
            QString bandId=bandsInAlgorithmBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID][i];
            sqlSentence2+=bandId;
            sqlSentence2+="','";
//            if(i<(bandsInAlgorithmBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID].size()-1))
//            {
//                sqlSentence2+="','";
//            }
//            else
//            {
//                sqlSentence2+="')";
//            }
        }
        for(int i=0;i<bandsInAlgorithmBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID].size();i++)
        {
            QString bandId=bandsInAlgorithmBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID][i];
            sqlSentence2+=bandId;
            if(i<(bandsInAlgorithmBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID].size()-1))
            {
                sqlSentence2+="','";
            }
            else
            {
                sqlSentence2+="')";
            }
        }
        sqlSentence2+=(" AND "+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
        sqlSentence2+=("="+quadkeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID);
        sqlSentence2+=(" AND "+raster_types_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID);
        sqlSentence2+=("="+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID);
        sqlSentence2+=(" AND "+raster_types_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE);
        sqlSentence2+=(" IN ('"+landsat8IdDb+"','"+orthoimageIdDb+"','"+sentinel2IdDb+"')");
        sqlSentence2+=(" AND "+quadkeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID);
        sqlSentence2+=("="+quadkeys_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID);
        sqlSentence3=(" AND "+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
        sqlSentence3+=(">="+projects_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_INITIAL_DATE);
        sqlSentence3+=(" AND "+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
        sqlSentence3+=("<="+projects_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_FINAL_DATE);
        sqlSentence3+=(" AND "+quadkeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID);
        sqlSentence3+=(" IN (SELECT "+quadkeys_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID);
        sqlSentence3+=(" FROM "+quadkeys_table_name+","+projects_table_name);
        sqlSentence3+=(" WHERE intersects("+quadkeys_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM);
        sqlSentence3+=(","+projects_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_THE_GEOM+")");
        sqlSentence3+=(" AND "+projects_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_ID+"="+QString::number(mIdByZone[zoneCode])+")");
        sqlSentence3+=(" ORDER BY "+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
//        select qrf.file_name,rf.id,rf.raster_id,rt.type,qrf.band_id,q.quadkey,rf.jd
        QVector<QString> quadkeys;
        QVector<QString> rasterFiles;
        QMap<QString,QString> rasterTypesByRasterFile;
        QMap<QString,QVector<QString> > rasterFilesByQuadkey; // no quadkey_raster_files
        QMap<QString,QMap<QString,QMap<QString,QString> > > quadkeysRasterFilesByQuadkeyByRasterFileAndByBand; // B0,B4,B5
        QMap<QString,int> jdByRasterFile;
        QMap<QString,int> idByRasterFile;
        QVector<int> rasterFilesIds;
        QVector<int> rasterFilesLandsat8Ids;
        QMap<QString,double> sunAzimuthByRasterFile;
        QMap<QString,double> sunElevationByRasterFile;
        QMap<QString,QMap<QString,double> > reflectanceAddValueByRasterFileAndByBand;
        QMap<QString,QMap<QString,double> > reflectanceMultValueByRasterFileAndByBand;
        sqlSentence=sqlSentence1+sqlSentence2+sqlSentence3;
        {
            QVector<QString> fieldsNamesToRetrieve;
            fieldsNamesToRetrieve.push_back(quadkeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME);
            fieldsNamesToRetrieve.push_back(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
            fieldsNamesToRetrieve.push_back(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
            fieldsNamesToRetrieve.push_back(raster_types_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE);
            fieldsNamesToRetrieve.push_back(quadkeys_raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID);
            fieldsNamesToRetrieve.push_back(quadkeys_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY);
            fieldsNamesToRetrieve.push_back(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD);
            QVector<QMap<QString,QString> > fieldsValuesToRetrieve;
            if(!mPtrDb->executeSqlQuery(sqlSentence,
                                        fieldsNamesToRetrieve,
                                        fieldsValuesToRetrieve,
                                        strError))
            {
                strError=QObject::tr("PersistenceManager::processAlgorithm");
                strError+=QObject::tr("\nError executing sql sentence:\n%1\n%Error:\n%2")
                    .arg(sqlSentence).arg(zoneCode);
                strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                return(false);
            }
            for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
            {
                QString quadkeyRasterFile=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[0]];
                int rasterFileId=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[1]].toInt();
                QString rasterFile=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[2]]; // el id, sin extension ni ruta
                QString rasterType=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[3]];
                rasterTypesByRasterFile[rasterFile]=rasterType;
                if(rasterType.compare(landsat8IdDb)==0)
                {
                    QString orthoimagesSuffix=orthoimagesSuffixBySpaceCraft[REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID];
                    rasterFile=rasterFile.remove(orthoimagesSuffix);
                    if(!rasterFilesLandsat8Ids.contains(rasterFileId))
                    {
                        rasterFilesLandsat8Ids.push_back(rasterFileId);
                    }
                }
                if(rasterType.compare(sentinel2IdDb)==0)
                {
                    QString orthoimagesSuffix=orthoimagesSuffixBySpaceCraft[REMOTESENSING_SENTINEL2_SPACECRAFT_ID];
                    rasterFile=rasterFile.remove(orthoimagesSuffix);
                }
                QString bandId=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[4]];
                QString quadkey=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[5]];
                int jd=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[6]].toInt();
                if(!rasterFiles.contains(rasterFile))
                {
                    if(rasterType.compare(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8)==0
                            ||rasterType.compare(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2)==0)
                    {
                        if(!rasterFilesIds.contains(rasterFileId))
                        {
                            rasterFilesIds.push_back(rasterFileId);
                        }
                        jdByRasterFile[rasterFile]=jd;
                        idByRasterFile[rasterFile]=rasterFileId;
                        rasterFiles.push_back(rasterFile);
                    }
                }
                if(!quadkeys.contains(quadkey))
                {
                    quadkeys.push_back(quadkey);
                }
                if(!rasterFilesByQuadkey.contains(quadkey))
                {
                    QVector<QString> rasterFilesInQuadkey;
                    rasterFilesByQuadkey[quadkey]=rasterFilesInQuadkey;
                }
                if(!rasterFilesByQuadkey[quadkey].contains(rasterFile))
                {
                    rasterFilesByQuadkey[quadkey].push_back(rasterFile);
                }
                quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][bandId]=quadkeyRasterFile;
            }
        }

        // Obtener metadatos de las escenas Landsat8 involucradas
        if(rasterFilesLandsat8Ids.size()>0)
        {
    //        select rf.raster_id,rf.id,l8m.metadata_file,l8m.sun_azimuth,l8m.sun_elevation,
    //        l8m.reflectance_add_band_4,l8m.reflectance_mult_band_4,
    //        l8m.reflectance_add_band_5,l8m.reflectance_mult_band_5
    //        from raster_files as rf,landsat8_metadata as l8m
    //        where rf.id=l8m.id and rf.id in(115,116,117,118,119,120)";
            sqlSentence1="SELECT ";
            sqlSentence1+=(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
            sqlSentence1+=(","+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
            sqlSentence1+=(","+landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_METADATA_FILE);
            sqlSentence1+=(","+landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH);
            sqlSentence1+=(","+landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION);
            sqlSentence1+=(","+landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B4);
            sqlSentence1+=(","+landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B4);
            sqlSentence1+=(","+landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B5);
            sqlSentence1+=(","+landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B5);
            sqlSentence1+=(" FROM "+raster_files_table_name+","+landsat8_metadata_table_name);
            sqlSentence1+=(" WHERE "+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
            sqlSentence1+=("="+landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID);
            sqlSentence2=(" AND "+raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID+" IN (");
            for(int nf=0;nf<rasterFilesLandsat8Ids.size();nf++)
            {
                sqlSentence2+=QString::number(rasterFilesLandsat8Ids[nf]);
                if(nf<(rasterFilesLandsat8Ids.size()-1))
                {
                    sqlSentence2+=",";
                }
            }
            sqlSentence2+=(")");
            sqlSentence=sqlSentence1+sqlSentence2;
            {
                QVector<QString> fieldsNamesToRetrieve;
                fieldsNamesToRetrieve.push_back(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID);
                fieldsNamesToRetrieve.push_back(raster_files_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID);
                fieldsNamesToRetrieve.push_back(landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_METADATA_FILE);
                fieldsNamesToRetrieve.push_back(landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH);
                fieldsNamesToRetrieve.push_back(landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION);
                fieldsNamesToRetrieve.push_back(landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B4);
                fieldsNamesToRetrieve.push_back(landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B4);
                fieldsNamesToRetrieve.push_back(landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B5);
                fieldsNamesToRetrieve.push_back(landsat8_metadata_table_name+"."+PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B5);
                QVector<QMap<QString,QString> > fieldsValuesToRetrieve;
                if(!mPtrDb->executeSqlQuery(sqlSentence,
                                            fieldsNamesToRetrieve,
                                            fieldsValuesToRetrieve,
                                            strError))
                {
                    strError=QObject::tr("PersistenceManager::processAlgorithm");
                    strError+=QObject::tr("\nError executing sql sentence:\n%1\n%Error:\n%2")
                        .arg(sqlSentence).arg(zoneCode);
                    strError+=QObject::tr("\nError:\n%1").arg(strAuxError);
                    return(false);
                }
                for(int nr=0;nr<fieldsValuesToRetrieve.size();nr++)
                {
                    QString rasterFile=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[0]];
                    double sunAzimuth=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[3]].toDouble();
                    double sunElevation=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[4]].toDouble();
                    double reflectanceAddB4=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[5]].toDouble();
                    double reflectanceMultB4=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[6]].toDouble();
                    double reflectanceAddB5=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[7]].toDouble();
                    double reflectanceMultB5=fieldsValuesToRetrieve[nr][fieldsNamesToRetrieve[8]].toDouble();
                    sunAzimuthByRasterFile[rasterFile]=sunAzimuth;
                    sunElevationByRasterFile[rasterFile]=sunElevation;
                    reflectanceAddValueByRasterFileAndByBand[rasterFile][ALGORITHMS_PIAS_LANDSAT8_BAND_RED]=reflectanceAddB4;
                    reflectanceMultValueByRasterFileAndByBand[rasterFile][ALGORITHMS_PIAS_LANDSAT8_BAND_RED]=reflectanceMultB4;
                    reflectanceAddValueByRasterFileAndByBand[rasterFile][ALGORITHMS_PIAS_LANDSAT8_BAND_NIR]=reflectanceAddB5;
                    reflectanceMultValueByRasterFileAndByBand[rasterFile][ALGORITHMS_PIAS_LANDSAT8_BAND_NIR]=reflectanceMultB5;
                }
            }
        }
//        QVector<QString> quadkeys;
//        QVector<QString> rasterFiles;
//        QMap<QString,QVector<QString> > rasterFilesByQuadkey; // no quadkey_raster_files
//        QMap<QString,QMap<QString,QMap<QString,QString> > > quadkeysRasterFilesByQuadkeyByRasterFileAndByBand; // B0,B4,B5
//        QMap<QString,int> jdByRasterFile;
//        QMap<QString,int> idByRasterFile;
//        QVector<int> rasterFilesIds;
//        QMap<QString,double> sunAzimuthByRasterFile;
//        QMap<QString,double> sunElevationByRasterFile;
//        QMap<QString,QMap<QString,double> > reflectanceAddValueByRasterFileAndByBand;
//        QMap<QString,QMap<QString,double> > reflectanceMultValueByRasterFileAndByBand;
//        mZonesCodes.push_back(zoneCode);
//        mIdByZone[zoneCode]=fieldsValues[0][0].toInt();
//        mResultsPathByZone[zoneCode]=fieldsValues[0][1];
//        mInitialDateByZone[zoneCode]=fieldsValues[0][2].toInt();
//        mFinalDateByZone[zoneCode]=fieldsValues[0][3].toInt();
//        mOutputSridByZone[zoneCode]=fieldsValues[0][4].toInt();
        QString algorithmCode=ALGORITHMS_PIAS_CODE;
        QString algorithmResultsFileExtension=ALGORITHMS_RESULTS_FILE_EXTENSION;
        QString strDate=QDateTime::currentDateTime().toString(ALGORITHMS_DATE_TIME_FILE_NAME_STRING_FORMAT);
        QString algorithmResultsFileName=mResultsPathByZone[zoneCode]+"/"+zoneCode;
        algorithmResultsFileName+="_"+algorithmCode+"_"+strDate+"."+algorithmResultsFileExtension;
        QFile file(algorithmResultsFileName);
        if (!file.open(QFile::WriteOnly |QFile::Text))
        {
            strError=QObject::tr("PersistenceManager::createPersintenceFileManager");
            strError+=QObject::tr("\nError opening results file: \n %1").arg(algorithmResultsFileName);
            return(false);
        }
        QTextStream out(&file);
        out<<"FICHERO DE RESULTADOS DEL PROCESO...........: "<<ALGORITHMS_PIAS_GUI_TAG<<"\n";
        out<<"- Zona de actuacion ........................: "<<zoneCode<<"\n";
        out<<"- Fecha inicial ............................: "<<QDate::fromJulianDay(mInitialDateByZone[zoneCode]).toString(ALGORITHMS_DATE_STRING_FORMAT)<<"\n";
        out<<"- Fecha final ..............................: "<<QDate::fromJulianDay(mFinalDateByZone[zoneCode]).toString(ALGORITHMS_DATE_STRING_FORMAT)<<"\n";
        out<<"- Identificador del CRS de salida ..........: "<<QString::number(mOutputSridByZone[zoneCode])<<"\n";
        file.close();
        if(!mPtrAlgoritmhs->piasComputation(rasterFiles,
                                            rasterTypesByRasterFile,
                                            rasterFilesByQuadkey,
                                            quadkeysRasterFilesByQuadkeyByRasterFileAndByBand,
                                            jdByRasterFile,
                                            sunAzimuthByRasterFile,
                                            sunElevationByRasterFile,
                                            reflectanceAddValueByRasterFileAndByBand,
                                            reflectanceMultValueByRasterFileAndByBand,
                                            mergeFiles,
                                            reprocessFiles,
                                            reprojectFiles,
                                            file,
                                            strAuxError))
        {
            strError=QObject::tr("PersistenceManager::processAlgorithm");
            strError+=QObject::tr("\nError computing PIAS for zone: %1\nError:\n%2")
                    .arg(zoneCode).arg(strAuxError);
            return(false);
        }
    }
    return(true);
}

bool PersistenceManager::updateDatabase(QString sqlFileName,
                                        QString &strError)
{
    if(mPtrDb==NULL)
    {
        strError=QObject::tr("PersistenceManager::updateDatabase");
        strError+=QObject::tr("\nPointer to database is null");
        return(false);
    }
    QString auxStrError;
    if(!mPtrDb->update(sqlFileName,
                       auxStrError))
    {
        strError=QObject::tr("PersistenceManager::updateDatabase");
        strError+=QObject::tr("\nError updatting database:\nError:\n%1").arg(auxStrError);
        return(false);
    }
    return(true);
}
