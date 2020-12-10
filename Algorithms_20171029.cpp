#include <QFile>
#include <QDir>
#include <QProgressDialog>
#include <QApplication>
#include <QTextStream>
#include <QDateTime>
#include <QMessageBox>
#include <Eigen/Dense>

#include "ParametersManager.h"
#include "Parameter.h"
#include "algorithms_definitions.h"
#include "Algorithms.h"
#include "Raster.h"
#include "PersistenceManager.h"
#include "persistencemanager_definitions.h"
#include <Splines.hh>
#include "remotesensing_definitions.h"
//#include "SceneLandsat8_definitions.h"
//#include "SceneSentinel2_definitions.h"


using namespace RemoteSensing;

Algorithms::Algorithms(libCRS::CRSTools *ptrCrsTools,
                       NestedGrid::NestedGridTools *ptrNestedGridTools,
                       IGDAL::libIGDALProcessMonitor *ptrLibIGDALProcessMonitor,
                       PersistenceManager *ptrPersistenceManager,
                       QWidget *ptrWidgetParent):
    mPtrCrsTools(ptrCrsTools),
    mPtrNestedGridTools(ptrNestedGridTools),
    mPtrLibIGDALProcessMonitor(ptrLibIGDALProcessMonitor),
    mPtrPersistenceManager(ptrPersistenceManager)
{
    mPtrParametersManager=NULL;
    mIsInitialized=false;
    mPtrWidgetParent=ptrWidgetParent;
    if(mPtrWidgetParent==NULL)
    {
        mPtrWidgetParent=new QWidget();
    }
}

bool Algorithms::applyIntercalibration(QString inputFileName,
                                       QString outputFileName,
                                       bool outputTo8bits,
                                       double gain,
                                       double offset,
                                       bool parametersIn8Bits,
                                       bool parametersInReflectance,
                                       double toReflectanceMultValue,
                                       double toReflectanceAddValue,
                                       QString &strError)
{
    if(parametersIn8Bits&&parametersInReflectance)
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nInvalid option: parameters in 8 bits and in reflectance for input image file: \n %1").arg(inputFileName);
        return(false);
    }
    // Casos:
    // - Tipo byte y outputTo8Bits a falso. Se genera en 8 bits
    if(!QFile::exists(inputFileName))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nNot exists input image file: \n %1").arg(inputFileName);
        return(false);
    }
    IGDAL::ImageTypes imageType=IGDAL::TIFF;
    QFileInfo inputFileInfo(inputFileName);
    QString inputFileExtension=inputFileInfo.suffix();
    bool validExtension=false;
    if(inputFileExtension.compare(RASTER_TIFF_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_ECW_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::ECW;
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_JPEG_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::JPEG;
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_BMP_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::BMP;
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_PNG_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::PNG;
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_ERDAS_IMAGINE_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::HFA;
        validExtension=true;
    }
    if(!validExtension)
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nNot valid type for input image file: \n %1").arg(inputFileName);
        return(false);
    }
    if(QFile::exists(outputFileName))
    {
        if(!QFile::remove(outputFileName))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError removing existing intercalibrated image file: \n %1").arg(outputFileName);
            return(false);
        }
    }
    IGDAL::Raster* ptrInputRasterFile=new IGDAL::Raster(mPtrCrsTools);
    bool inputUpdate=false;
    QString strAuxError;
    if(!ptrInputRasterFile->setFromFile(inputFileName,strAuxError,inputUpdate))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
    GDALDataType inputGdalDataType;//GDT_Float32;
    if(!ptrInputRasterFile->getDataType(inputGdalDataType,strAuxError))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nError recovering data type from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
//    if(gdalDataType!=GDT_Float32
//            &&gdalDataType!=GDT_Byte
//            &&gdalDataType!=GDT_UInt16
//            &&gdalDataType!=GDT_Int16
//            &&gdalDataType!=GDT_UInt32
//            &&gdalDataType!=GDT_Int32)
    if(inputGdalDataType!=GDT_Float32
            &&inputGdalDataType!=GDT_Byte
            &&inputGdalDataType!=GDT_UInt16
            &&inputGdalDataType!=GDT_Int16)
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nInvalid type of data: not float32, byte, uint16, int16, uint32 or int32\nin file:\n%1")
                .arg(inputFileName);
        delete(ptrInputRasterFile);
        return(false);
    }
    GDALDataType outputGdalDataType=inputGdalDataType;
    if(outputTo8bits
            &&outputGdalDataType!=GDT_Byte)
    {
        outputGdalDataType=GDT_Byte;
    }
    double inputFactorTo8Bits=1.0;
    if(outputTo8bits)
    {
        if(!ptrInputRasterFile->getFactorTo8Bits(inputFactorTo8Bits,strAuxError))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError recovering factor to 8 bits from raster file:\n%1\nError:\n%2")
                    .arg(inputFileName).arg(strAuxError);
            return(false);
        }
    }
    int numberOfBands=1;
    if(!ptrInputRasterFile->getNumberOfBands(numberOfBands,strAuxError))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nError recovering number of bands from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        return(false);
    }
    if(numberOfBands!=1)
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nThere is not a unique band in raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
    int columns,rows;
    if(!ptrInputRasterFile->getSize(columns,rows,strAuxError))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
//    bool internalGeoRef=true;
//    bool externalGeoRef=false;
//    QString crsDescription=ptrDlRasterFile->getCrsDescription();
//    double nwFc,nwSc,seFc,seSc;
//    if(!ptrDlRasterFile->getBoundingBox(nwFc,nwSc,seFc,seSc,
//                                        strAuxError))
//    {
//        strError=QObject::tr("Algorithms::applyIntercalibration");
//        strError+=QObject::tr("\nError getting bounding box from raster file:\n%1\nError:\n%2")
//                .arg(outputFileName).arg(strAuxError);
//        return(false);
//    }
    int numberOfBand=0;
    double inputDlNoDataValue;
    if(!ptrInputRasterFile->getNoDataValue(numberOfBand,inputDlNoDataValue,strAuxError))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
    if(outputTo8bits)
    {
        if(inputDlNoDataValue<0||inputDlNoDataValue>255)
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nInvalid no data value with to8bits option: %1\nfor input file:\n%2")
                    .arg(QString::number(inputDlNoDataValue,'f',2)).arg(inputFileName);
            delete(ptrInputRasterFile);
            return(false);
        }
    }

    IGDAL::Raster* ptrOutputRasterFile=new IGDAL::Raster(mPtrCrsTools);
    if(outputTo8bits
            &&inputGdalDataType!=GDT_Byte)
    {
        QVector<double> georef;
        if(!ptrInputRasterFile->getGeoRef(georef,strAuxError))
        {
            strError=QObject::tr("Algorithms::reflectanceComputation");
            strError+=QObject::tr("\nError recovering GeoRef from input raster file:\n%1\nError:\n%2")
                    .arg(inputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        bool closeAfterCreate=false;
        double nwFc,nwSc,seFc,seSc;
        if(!ptrInputRasterFile->getBoundingBox(nwFc,nwSc,seFc,seSc,strAuxError))
        {
            strError=QObject::tr("Algorithms::reflectanceComputation");
            strError+=QObject::tr("\nError recovering bounding box from input raster file:\n%1\nError:\n%2")
                    .arg(inputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        QString crsDescription=ptrInputRasterFile->getCrsDescription();
        bool internalGeoRef=true;
        bool externalGeoRef=false;
        QMap<QString, QString> refImageOptions; // por implementar
        if(!ptrOutputRasterFile->createRaster(outputFileName, // Se le añade la extension
                                              imageType,
                                              outputGdalDataType,
                                              numberOfBands,
                                              columns,rows,
                                              internalGeoRef,externalGeoRef,crsDescription,
                                              nwFc,nwSc,seFc,seSc,
                                              georef, // vacío si se georeferencia con las esquinas
                                              inputDlNoDataValue,
                                              closeAfterCreate,
                                              refImageOptions,
                                              // buildOverviews,
                                              strAuxError))
        {
            strError=QObject::tr("Algorithms::reflectanceComputation");
            strError+=QObject::tr("\nError creating raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
    }
    else
    {
        if(!QFile::copy(inputFileName,outputFileName))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError copying file:\n%1\nto intercalibrated image file:\n%2")
                    .arg(inputFileName).arg(outputFileName);
            return(false);
        }
        bool update=true;
        QString strAuxError;
        if(!ptrOutputRasterFile->setFromFile(outputFileName,strAuxError,update))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
    }
    GDALRasterBand* ptrOutputRasterBand=NULL;
    if(!ptrOutputRasterFile->getRasterBand(numberOfBand,ptrOutputRasterBand,strAuxError))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nError recovering raster band from raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        delete(ptrOutputRasterFile);
        return(false);
    }

    int initialColumn=0;
    int initialRow=0;
    int columnsToRead=columns;
    int rowsToRead=rows;
    int numberOfPixels=columnsToRead*rowsToRead;

    GDALRasterBand* ptrInputRasterBand;
    if(!ptrInputRasterFile->getRasterBand(numberOfBand,ptrInputRasterBand,strAuxError))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nError recovering raster band from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        delete(ptrOutputRasterFile);
        return(false);
    }
    if(inputGdalDataType==GDT_Byte)
    {
        GByte*  pData=NULL;
        pData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
        if(CE_None!=ptrInputRasterBand->RasterIO( GF_Read, initialColumn, initialRow, columnsToRead, rowsToRead,
                                                 pData, columnsToRead, rowsToRead, GDT_Byte, 0, 0 ))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(inputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        for(int i=0;i<numberOfPixels;i++)
        {
            int initialValue=(int)pData[i];
            if(fabs((double)initialValue-inputDlNoDataValue)<0.01)
            {
                pData[i]=(int)inputDlNoDataValue;
            }
            else
            {
                int intValue=initialValue;
                if(parametersInReflectance)
                {
                    double dblValue=((double)intValue)*toReflectanceMultValue+toReflectanceAddValue;
                    dblValue=dblValue*gain+offset;
//                    dblValue=dblValue*255.0;
                    dblValue=(dblValue-toReflectanceAddValue)/toReflectanceMultValue;
                    intValue=qRound(dblValue);
                }
                else
                {
                    intValue=qRound(((double)initialValue)*gain+offset);
                }
                if(intValue<0)
                {
                    intValue=0;
                }
                else if(intValue>255)
                {
                    intValue=255;
                }
                pData[i]=intValue;
            }
        }
        if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                  rowsToRead,pData,columnsToRead,rowsToRead,GDT_Byte,0,0))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        CPLFree(pData);
    }
    else if(inputGdalDataType==GDT_UInt16)
    {
        GUInt16*  pData=NULL;
        pData=(GUInt16 *) CPLMalloc(numberOfPixels*sizeof(GUInt16));
        if(CE_None!=ptrInputRasterBand->RasterIO( GF_Read, initialColumn, initialRow, columnsToRead, rowsToRead,
                                                  pData, columnsToRead, rowsToRead, GDT_UInt16, 0, 0 ))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(inputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        GByte*  pByteData=NULL;
        if(outputTo8bits)
        {
            pByteData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
        }
        for(int i=0;i<numberOfPixels;i++)
        {
            int initialValue=(int)pData[i];
            if(fabs((double)initialValue-inputDlNoDataValue)<0.01)
            {
                pData[i]=(int)inputDlNoDataValue;
            }
            else
            {
                int intValue=initialValue;
                if(parametersInReflectance)
                {
                    double dblValue=((double)intValue)*toReflectanceMultValue+toReflectanceAddValue;
                    dblValue=dblValue*gain+offset;
//                    dblValue=dblValue*255.0*255.0;
                    dblValue=(dblValue-toReflectanceAddValue)/toReflectanceMultValue;
                    intValue=qRound(dblValue);
                }
                else if(parametersIn8Bits)
                {
                    double dblValue=initialValue/255.0;
                    dblValue=dblValue*gain+offset;
                    intValue=qRound(dblValue*255.0);
                }
                else
                {
                    intValue=qRound(((double)initialValue)*gain+offset);
                }
                if(intValue<0)
                {
                    intValue=0;
                }
                else if(intValue>65535)
                {
                    intValue=65535;
                }
                if(!outputTo8bits)
                {
                    pData[i]=intValue;
                }
                else
                {
                    intValue=qRound(((double)intValue)/255.0);
                    if(intValue<0)
                    {
                        intValue=0;
                    }
                    else if(intValue>255)
                    {
                        intValue=255;
                    }
                    pByteData[i]=intValue;
                }
            }
        }
        if(!outputTo8bits)
        {
            if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                      rowsToRead,pData,columnsToRead,rowsToRead,GDT_UInt16,0,0))
            {
                strError=QObject::tr("Algorithms::applyIntercalibration");
                strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                        .arg(outputFileName).arg(strAuxError);
                delete(ptrInputRasterFile);
                delete(ptrOutputRasterFile);
                return(false);
            }
        }
        else
        {
            if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                      rowsToRead,pByteData,columnsToRead,rowsToRead,GDT_Byte,0,0))
            {
                strError=QObject::tr("Algorithms::applyIntercalibration");
                strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                        .arg(outputFileName).arg(strAuxError);
                delete(ptrInputRasterFile);
                delete(ptrOutputRasterFile);
                return(false);
            }
            CPLFree(pByteData);
        }
        CPLFree(pData);
    }
    else if(inputGdalDataType==GDT_Int16)
    {
        GInt16*  pData=NULL;
        pData=(GInt16 *) CPLMalloc(numberOfPixels*sizeof(GInt16));
        if(CE_None!=ptrInputRasterBand->RasterIO( GF_Read, initialColumn, initialRow, columnsToRead, rowsToRead,
                                                 pData, columnsToRead, rowsToRead, GDT_Int16, 0, 0 ))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        GByte*  pByteData=NULL;
        if(outputTo8bits)
        {
            pByteData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
        }
        for(int i=0;i<numberOfPixels;i++)
        {
            int initialValue=(int)pData[i];
            if(fabs((double)initialValue-inputDlNoDataValue)<0.01)
            {
                pData[i]=(int)inputDlNoDataValue;
            }
            else
            {
                int intValue=initialValue;
                if(parametersInReflectance)
                {
                    double dblValue=((double)intValue)*toReflectanceMultValue+toReflectanceAddValue;
                    dblValue=dblValue*gain+offset;
//                    dblValue=dblValue*255.0*255.0;
                    dblValue=(dblValue-toReflectanceAddValue)/toReflectanceMultValue;
                    intValue=qRound(dblValue);
                }
                else if(parametersIn8Bits)
                {
                    double dblValue=initialValue/255.0;
                    dblValue=dblValue*gain+offset;
                    intValue=qRound(dblValue*255.0);
                }
                else
                {
                    intValue=qRound(((double)initialValue)*gain+offset);
                }
                if(intValue<-32768)
                {
                    intValue=-32768;
                }
                else if(intValue>32767)
                {
                    intValue=32767;
                }
                if(!outputTo8bits)
                {
                    pData[i]=intValue;
                }
                else
                {
                    intValue=qRound(((double)intValue)/255.0);
                    if(intValue<0)
                    {
                        intValue=0;
                    }
                    else if(intValue>255)
                    {
                        intValue=255;
                    }
                    pByteData[i]=intValue;
                }
            }
        }
        if(!outputTo8bits)
        {
            if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                      rowsToRead,pData,columnsToRead,rowsToRead,GDT_Int16,0,0))
            {
                strError=QObject::tr("Algorithms::applyIntercalibration");
                strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                        .arg(outputFileName).arg(strAuxError);
                delete(ptrInputRasterFile);
                delete(ptrOutputRasterFile);
                return(false);
            }
        }
        else
        {
            if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                      rowsToRead,pByteData,columnsToRead,rowsToRead,GDT_Byte,0,0))
            {
                strError=QObject::tr("Algorithms::applyIntercalibration");
                strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                        .arg(outputFileName).arg(strAuxError);
                delete(ptrInputRasterFile);
                delete(ptrOutputRasterFile);
                return(false);
            }
            CPLFree(pByteData);
        }
        CPLFree(pData);
    }
    else if(inputGdalDataType==GDT_Float32)
    {
        float* pData;
        pData=(float*)malloc(numberOfPixels*sizeof(GDT_Float32));
        if(CE_None!=ptrInputRasterBand->RasterIO( GF_Read, initialColumn, initialRow, columnsToRead, rowsToRead,
                                                 pData, columnsToRead, rowsToRead, GDT_Float32, 0, 0 ))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        GByte*  pByteData=NULL;
        if(outputTo8bits)
        {
            pByteData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
        }
        for(int i=0;i<numberOfPixels;i++)
        {
            double initialValue=(double)pData[i];
            if(fabs(initialValue-inputDlNoDataValue)<0.01)
            {
                pData[i]=inputDlNoDataValue;
            }
            else
            {
                double dblValue=initialValue;
                if(parametersIn8Bits)
                {
                    dblValue=dblValue/255.0;
                    dblValue=dblValue*gain+offset;
                    dblValue=dblValue*255.0;
                }
                else
                {
                    dblValue=dblValue*gain+offset;
                }
//                if(parametersInReflectance)
//                {
//                    double dblValue=dblValue*toReflectanceMultValue+toReflectanceAddValue;
//                    dblValue=dblValue*255.0*255.0;
//                    intValue=qRound(dblValue);
//                }
                if(!outputTo8bits)
                {
                    pData[i]=(float)dblValue;
                }
                else
                {
                    int intValue=qRound(dblValue/255.0); // se supone que es una reflectividad
                    if(intValue<0)
                    {
                        intValue=0;
                    }
                    else if(intValue>255)
                    {
                        intValue=255;
                    }
                    pByteData[i]=intValue;
                }
            }
        }
        if(!outputTo8bits)
        {
            if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                      rowsToRead,pData,columnsToRead,rowsToRead,GDT_Float32,0,0))
            {
                strError=QObject::tr("Algorithms::applyIntercalibration");
                strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                        .arg(outputFileName).arg(strAuxError);
                delete(ptrInputRasterFile);
                delete(ptrOutputRasterFile);
                return(false);
            }
        }
        else
        {
            if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                      rowsToRead,pByteData,columnsToRead,rowsToRead,GDT_Byte,0,0))
            {
                strError=QObject::tr("Algorithms::applyIntercalibration");
                strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                        .arg(outputFileName).arg(strAuxError);
                delete(ptrInputRasterFile);
                delete(ptrOutputRasterFile);
                return(false);
            }
            CPLFree(pByteData);
        }
        CPLFree(pData);
    }
    /*
    else if(gdalDataType==GDT_UInt32)
    {
        GUInt32*  pData=NULL;
        pData=(GUInt32 *) CPLMalloc(numberOfPixels*sizeof(GUInt32));
        if(CE_None!=ptrRasterBand->RasterIO( GF_Read, initialColumn, initialRow, columnsToRead, rowsToRead,
                                                 pData, columnsToRead, rowsToRead, GDT_UInt32, 0, 0 ))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrRasterFile);
            return(false);
        }
        for(int i=0;i<numberOfPixels;i++)
        {
            int initialValue=(int)pData[i];
            if(fabs((double)initialValue-dlNoDataValue)<0.01)
            {
                pData[i]=(int)dlNoDataValue;
            }
            else
            {
                int intValue=qRound(((double)initialValue)*gain+offset);
                if(intValue<0)
                {
                    intValue=0;
                }
                else if(intValue>4294967295)
                {
                    intValue=4294967295;
                }
                pData[i]=intValue;
            }
        }
        if(CE_None!=ptrRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                            rowsToRead,pData,columnsToRead,rowsToRead,GDT_UInt32,0,0))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrRasterFile);
            return(false);
        }
        CPLFree(pData);
    }
    else if(gdalDataType==GDT_Int32)
    {
        GInt32*  pData=NULL;
        pData=(GInt32 *) CPLMalloc(numberOfPixels*sizeof(GInt32));
        if(CE_None!=ptrRasterBand->RasterIO( GF_Read, initialColumn, initialRow, columnsToRead, rowsToRead,
                                                 pData, columnsToRead, rowsToRead, GDT_Int32, 0, 0 ))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrRasterFile);
            return(false);
        }
        for(int i=0;i<numberOfPixels;i++)
        {
            int initialValue=(int)pData[i];
            if(fabs((double)initialValue-dlNoDataValue)<0.01)
            {
                pData[i]=(int)dlNoDataValue;
            }
            else
            {
                int intValue=qRound(((double)initialValue)*gain+offset);
                if(intValue<-2147483648)
                {
                    intValue=-2147483648;
                }
                else if(intValue>2147483647)
                {
                    intValue=2147483647;
                }
                pData[i]=intValue;
            }
        }
        if(CE_None!=ptrRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                            rowsToRead,pData,columnsToRead,rowsToRead,GDT_Int32,0,0))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrRasterFile);
            return(false);
        }
        CPLFree(pData);
    }
    */
    delete(ptrOutputRasterFile);
    delete(ptrInputRasterFile);
    return(true);
}

bool Algorithms::cloudRemoval(QVector<QString> &rasterFiles,
                              QMap<QString, QString> &rasterTypesByRasterFile,
                              QMap<QString, QVector<QString> > &rasterFilesByTuplekey,
                              QMap<QString, QMap<QString, QMap<QString, QString> > > &tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand,
                              QMap<QString, int> &jdByRasterFile,
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
                              QMap<QString, QVector<QString> > &bandsInAlgorithmBySpaceCraft,
                              QString &strError)
{
    QDir auxDir(QDir::currentPath());
    QDateTime initialDateTime=QDateTime::currentDateTime();
    bool debugMode=true;
    bool printDetail=true; // para el pixel primero y ultimo
    QString landsat8IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8;
    QString sentinel2IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2;
    QString orthoimageIdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_ORTHOIMAGE;
    QString strAuxError;
    // Obtener la ruta del workspace
    QMap<QString, QVector<QString> >::const_iterator iterRasterFilesByQuadkey=rasterFilesByTuplekey.begin();
    QString workspaceBasePath;
    // Ojo, esto funciona si están las bandas de ndvi, que se supone estarán siempre
    while(iterRasterFilesByQuadkey!=rasterFilesByTuplekey.end())
    {
        QString quadkey=iterRasterFilesByQuadkey.key();
        QVector<QString> rasterFilesInQuadkey=iterRasterFilesByQuadkey.value();
        int numberOfRasterFilesInQuadkey=rasterFilesInQuadkey.size();
        for(int nrf=0;nrf<numberOfRasterFilesInQuadkey;nrf++)
        {
            QString rasterFile=rasterFilesInQuadkey[nrf];
            QString rasterType=rasterTypesByRasterFile[rasterFile];
            if(rasterType.compare(orthoimageIdDb)==0)
            {
                continue;
            }
            if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.contains(quadkey))
            {
                continue;
            }
            if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey].contains(rasterFile))
            {
                continue;
            }
            QString redBandRasterFile,nirBandRasterFile;
            bool validRasterType=false;
            if(rasterType.compare(landsat8IdDb)==0)
            {
                if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_LANDSAT8_BAND_B4_CODE)
                        ||!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_LANDSAT8_BAND_B5_CODE))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                redBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_LANDSAT8_BAND_B4_CODE];
                nirBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_LANDSAT8_BAND_B5_CODE];
                validRasterType=true;
            }
            if(rasterType.compare(sentinel2IdDb)==0)
            {
                if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_SENTINEL2_BAND_B4_CODE)
                        ||!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_SENTINEL2_BAND_B8_CODE))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                redBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_SENTINEL2_BAND_B4_CODE];
                nirBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_SENTINEL2_BAND_B8_CODE];
                validRasterType=true;
            }
            if(!validRasterType)
            {
                strError=QObject::tr("Algorithms::intercalibrationComputation");
                strError+=QObject::tr("\nInvalid raste type: %1\nfor raster file: %2")
                        .arg(rasterType).arg(rasterFile);
                return(false);
            }
            if(QFile::exists(redBandRasterFile))
            {
                QFileInfo redBandRasterFileInfo(redBandRasterFile);
                QDir redBandDir=redBandRasterFileInfo.absoluteDir();
                redBandDir.cdUp();
                workspaceBasePath=redBandDir.absolutePath();
                break;
            }
            else if(QFile::exists(nirBandRasterFile))
            {
                QFileInfo nirBandRasterFileInfo(nirBandRasterFile);
                QDir nirBandDir=nirBandRasterFileInfo.absoluteDir();
                nirBandDir.cdUp();
                workspaceBasePath=nirBandDir.absolutePath();
                break;
            }
        }
        if(!workspaceBasePath.isEmpty())
        {
            iterRasterFilesByQuadkey=rasterFilesByTuplekey.end();
        }
        else
        {
            iterRasterFilesByQuadkey++;
        }
    }

    // Obtengo los parametros de procesamiento
    int numberOfDates;
    int cloudValue;
    double maxPercentagePartiallyCloudy;
    QString interpolationMethod,strRemoveFullCloudy;
    bool removeFullCloudy=false;
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_NUMBER_OF_DATES)->getValue(numberOfDates);
    if(numberOfDates!=0)
    {
        strError=QObject::tr("Algorithms::cloudRemoval");
        strError+=QObject::tr("\nNot use all dates option is not implemented yet");
        return(false);
    }
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_MAX_PER_PART_CLOUDY)->getValue(maxPercentagePartiallyCloudy);
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_INTERPOLATION_METHOD)->getValue(interpolationMethod);
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_PIA_CLOUD_VALUE)->getValue(cloudValue);
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_REMOVE_FULL_CLOUDY)->getValue(strRemoveFullCloudy);
    if(mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_REMOVE_FULL_CLOUDY)->isEnabled())
    {
        if(strRemoveFullCloudy.compare("Yes",Qt::CaseInsensitive)==0)
        {
            removeFullCloudy=true;
        }
    }
    QMap<QString,QMap<QString,QVector<QString> > > bandsCombinationsByRasterType;
    if(mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS)->isEnabled())
    {
        QString strBandsCombinations;
        mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS)->getValue(strBandsCombinations);
        QStringList bandsCombinationsList=strBandsCombinations.split(ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS_STRING_SEPARATOR);
        for(int i=0;i<bandsCombinationsList.size();i++)
        {
            QStringList bands=bandsCombinationsList.at(i).split(ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS_BANDS_STRING_SEPARATOR);
            if(bands.size()!=4)
            {
                strError=QObject::tr("Algorithms::cloudRemoval");
                strError+=QObject::tr("\nThere are not four values in combination: %1").arg(bandsCombinationsList.at(i));
                strError+=QObject::tr("\nFirst parameter is the spacecraft id and the other parameters are the three bands");
                return(false);
            }
            QString spacecraftId=bands.at(0).trimmed().toUpper();
            if(spacecraftId.compare(REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID,Qt::CaseInsensitive)!=0
                &&spacecraftId.compare(REMOTESENSING_SENTINEL2_SPACECRAFT_ID,Qt::CaseInsensitive)!=0)
            {
                strError=QObject::tr("Algorithms::cloudRemoval");
                strError+=QObject::tr("\nInvalid spacecraft id %1 in combination: %2").arg(spacecraftId).arg(bandsCombinationsList.at(i));
                return(false);
            }
            if(!bandsInAlgorithmBySpaceCraft.contains(spacecraftId))
            {
                strError=QObject::tr("Algorithms::cloudRemoval");
                strError+=QObject::tr("\nNot implemented spacecraft id %1 in combination: %2").arg(spacecraftId).arg(bandsCombinationsList.at(i));
                return(false);
            }
            QVector<QString> bandsCombination;
            for(int j=1;j<4;j++)
            {
                QString bandId=bands.at(j).trimmed().toUpper();
                if(!bandsInAlgorithmBySpaceCraft[spacecraftId].contains(bandId))
                {
                    strError=QObject::tr("Algorithms::cloudRemoval");
                    strError+=QObject::tr("\nInvalid band id %1 in combination: %2").arg(bandId).arg(bandsCombinationsList.at(i));
                    return(false);
                }
                if(bandsCombination.indexOf(bandId)>-1)
                {
                    strError=QObject::tr("Algorithms::cloudRemoval");
                    strError+=QObject::tr("\nRepeated band id %1 in combination: %2").arg(bandId).arg(bandsCombinationsList.at(i));
                    return(false);
                }
                bandsCombination.push_back(bandId);
            }
            if(spacecraftId.compare(REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID,Qt::CaseInsensitive)==0)
            {
                spacecraftId=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8;
            }
            if(spacecraftId.compare(REMOTESENSING_SENTINEL2_SPACECRAFT_ID,Qt::CaseInsensitive)==0)
            {
                spacecraftId=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2;
            }
            QString bandsCombinationId=spacecraftId+"_";
            for(int nb=0;nb<bandsCombination.size();nb++)
            {
                bandsCombinationId+=bandsCombination.at(nb);
            }
            if(bandsCombinationsByRasterType.contains(spacecraftId))
            {
                if(bandsCombinationsByRasterType[spacecraftId].contains(bandsCombinationId))
                {
                    strError=QObject::tr("Algorithms::cloudRemoval");
                    strError+=QObject::tr("\nRepeat bands combinations: %1").arg(bandsCombinationsList.at(i));
                    return(false);
                }
            }
            bandsCombinationsByRasterType[spacecraftId][bandsCombinationId]=bandsCombination;
        }
    }

    // Fichero de resultados: cabecera
    if (!resultsFile.open(QFile::Append |QFile::Text))
    {
        strError=QObject::tr("Algorithms::cloudRemoval");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(resultsFile.fileName());
        return(false);
    }
    QTextStream out(&resultsFile);
    out<<"- Parametros de procesamiento:\n";
    out<<"  - Escena de referencia de intercalibracion: "<<intercalibrationReferenceImageRasterId<<"\n";
    out<<"  - Maximo porcentaje para parcialmente nuboso : "<<QString::number(maxPercentagePartiallyCloudy,'f',1)<<"\n";
    out<<"  - Numero de fechas para interpolacion ....: "<<QString::number(numberOfDates);
    if(numberOfDates==0)
    {
        out<<", todas";
    }
    out<<"\n";
    out<<"  - Metodo de interpolacion ................: "<<interpolationMethod<<"\n";
    out<<"  - Numero de tuplekeys a procesar .........: "<<rasterFilesByTuplekey.size()<<"\n";

//    QString title=QObject::tr("Removing clouds");
//    QString msgGlobal=QObject::tr("Processing %1 tuplekeys ...").arg(QString::number(rasterFilesByTuplekey.size()));
//    QProgressDialog progress(title, QObject::tr("Abort"),0,rasterFilesByTuplekey.size(), mPtrWidgetParent);
//    progress.setWindowModality(Qt::WindowModal);
//    progress.setLabelText(msgGlobal);
//    progress.show();
//    qApp->processEvents();

//    int yo=3/2; // 1
//    yo=1/2; // 0
//    yo=4/2; // 2

    QMap<QString, QMap<QString, QMap<QString, QString> > >::const_iterator iterTuplekey =tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.begin();
    int numberOfProcessedTuplekeys=0;
    while(iterTuplekey!=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.end())
    {
        numberOfProcessedTuplekeys++;
        QString tuplekey=iterTuplekey.key();
        QMap<QString, QMap<QString, QMap<QString, QString> > > tuplekeysRasterFilesByRasterTypeByBandByRasterFile;
//        QMap<QString,QVector<QVector<bool> > > maskDataByRasterFile;
        QMap<QString,QMap<int,QVector<int> > > maskDataByRasterFile;
        QMap<QString,QVector<double> > maskGeorefByRasterFile;
        QMap<QString,bool> partiallyCloudyByRasterFile; // si no está no tiene nubes
        QMap<QString,float> cloudyPercentageByRasterFile; // si no está no tiene nubes
        QMap<QString, QMap<QString, QString> >::const_iterator iterRasterFile=iterTuplekey.value().begin();
//        QMap<QString,QMap<QString,QVector<QString> > > bandsCombinationsByRasterType;
        QMap<QString,QMap<QString,QMap<QString,QString> > > bandsCombinationsFileBaseNameByRasterTypeByRasterFile;
        QString tuplekeyPath;
        while(iterRasterFile!=iterTuplekey.value().end())
        {
            QString rasterFile=iterRasterFile.key();
            QString rasterType=rasterTypesByRasterFile[rasterFile];
            if(bandsCombinationsByRasterType.contains(rasterType))
            {
                QMap<QString,QVector<QString> >::const_iterator iterBandsCombinations=bandsCombinationsByRasterType[rasterType].begin();
                while(iterBandsCombinations!=bandsCombinationsByRasterType[rasterType].end())
                {
                    QString subFolder;
                    QString fileBaseName=rasterFile+"_";
                    for(int nb=0;nb<iterBandsCombinations.value().size();nb++)
                    {
                        fileBaseName+=iterBandsCombinations.value().at(nb);
                        subFolder+=iterBandsCombinations.value().at(nb);
                    }
                    fileBaseName+=".";
                    fileBaseName+=RASTER_JPEG_FILE_EXTENSION;
                    QString fileBaseNameAndSubFolder=subFolder+"/"+fileBaseName;
//                    fileBaseName+=RASTER_PNG_FILE_EXTENSION;
                    bandsCombinationsFileBaseNameByRasterTypeByRasterFile[rasterType][rasterFile][iterBandsCombinations.key()]=fileBaseNameAndSubFolder;
                    iterBandsCombinations++;
                }
            }
            QString maskBandCode;
            if(rasterType.compare(landsat8IdDb)==0)
            {
                maskBandCode=REMOTESENSING_LANDSAT8_BAND_B0_CODE;
            }
            else if(rasterType.compare(sentinel2IdDb)==0)
            {
                maskBandCode=REMOTESENSING_SENTINEL2_BAND_B0_CODE;
            }
            QMap<QString, QString>::const_iterator iterBand=iterRasterFile.value().begin();
            while(iterBand!=iterRasterFile.value().end())
            {
                QString bandId=iterBand.key();
                QString tuplekyeRasterFile=iterBand.value();
                if(!cloudyPercentageByRasterFile.contains(rasterFile))
                {
                    QFileInfo tuplekeyRasterFileInfo(tuplekyeRasterFile);
                    QString maskBandFileName=tuplekeyRasterFileInfo.absolutePath()+"//"+rasterFile+"_"+maskBandCode+".tif";
                    if(QFile::exists(maskBandFileName))
                    {
                        QVector<double> georef;
                        QMap<int,QVector<int> > values;
                        float cloudValuesPercentage;
                        if(!readMaskRasterFile(maskBandFileName,cloudValue,georef,values,cloudValuesPercentage,strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError reading mask raster file:\n%1\nError:\n%2")
                                    .arg(tuplekyeRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
//                        if(rasterFile.compare("LC82000322015101LGN00",Qt::CaseInsensitive)==0)
//                        {
//                            QMap<int,QVector<int> >::const_iterator kk1=values.begin();
//                            while(kk1!=values.end())
//                            {
//                                int rowKk=kk1.key();
//                                for(int kk2=0;kk2<kk1.value().size();kk2++)
//                                {
//                                    int columnKk=kk1.value()[kk2];
//                                    int yo=1;
//                                }
//                                kk1++;
//                            }
//                        }
                        if(cloudValuesPercentage>0)
                        {
                            maskDataByRasterFile[rasterFile]=values;
                            maskGeorefByRasterFile[rasterFile]=georef;
                            partiallyCloudyByRasterFile[rasterFile]=true;
                            if(cloudValuesPercentage>=maxPercentagePartiallyCloudy)
                            {
                                partiallyCloudyByRasterFile[rasterFile]=false;
                            }
                        }
                        cloudyPercentageByRasterFile[rasterFile]=cloudValuesPercentage;
                    }
                }
                tuplekeysRasterFilesByRasterTypeByBandByRasterFile[rasterType][bandId][rasterFile]=tuplekyeRasterFile;
                iterBand++;
            }
            iterRasterFile++;
        }
        out<<"- Procesamiento de tuplekey ................: "<<tuplekey<<"\n";
        out<<"  - Numero de tipos de escenas a procesar ..: "<<tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()<<"\n";
        QMap<QString, QMap<QString, QMap<QString, QString> > >::const_iterator iterRasterType=tuplekeysRasterFilesByRasterTypeByBandByRasterFile.begin();
        int numberOfProcessedRasterTypes=0;
        while(iterRasterType!=tuplekeysRasterFilesByRasterTypeByBandByRasterFile.end())
        {
            numberOfProcessedRasterTypes++;
            QString rasterType=iterRasterType.key();
            out<<"  - Tipo de escenas ........................: "<<rasterType<<"\n";
            QMap<QString, QMap<QString, QString> > tuplekeysRasterFilesByBandByRasterFile=iterRasterType.value();
            out<<"    - Numero de bandas a procesar ..........: "<<tuplekeysRasterFilesByBandByRasterFile.size()<<"\n";
            QMap<QString, QMap<QString, QString> >::const_iterator iterBand=tuplekeysRasterFilesByBandByRasterFile.begin();
            int numberOfProcessedBands=0;
            QMap<QString,QMap<QString,QVector<QVector<qint8> > > > combinationsDataByBandByRasterFile;
//            QMap<QString,QMap<QString,GByte* > > combinationsDataByBandByRasterFile;
            QMap<QString,QVector<QString> > bandsCombinations;
            if(bandsCombinationsByRasterType.contains(rasterType))
            {
                bandsCombinations=bandsCombinationsByRasterType[rasterType];
            }
            int numberOfScenes=0;
            while(iterBand!=tuplekeysRasterFilesByBandByRasterFile.end())
            {
                numberOfProcessedBands++;
                QString bandId=iterBand.key();
                QVector<QString> bandCombinationsIds;
                if(bandsCombinations.size()>0)
                {
                    QMap<QString,QVector<QString> >::const_iterator iterBandCombinations=bandsCombinations.begin();
                    while(iterBandCombinations!=bandsCombinations.end())
                    {
                        for(int nbc=0;nbc<iterBandCombinations.value().size();nbc++)
                        {
                            QString bandIdInCombination=iterBandCombinations.value().at(nbc);
                            if(bandId.compare(bandIdInCombination,Qt::CaseInsensitive)==0)
                            {
                                bandCombinationsIds.push_back(iterBandCombinations.key());
                                break;
                            }
                        }
                        iterBandCombinations++;
                    }
                }
                QMap<QString,QVector<QVector<double> > > bandDataByRasterFile;
                QMap<QString,QMap<int,QVector<int> > > bandMaskDataByRasterFile;
                QMap<QString,QVector<double> > bandGeorefByRasterFile;
                QMap<QString,double> bandNoDataValueByRasterFile;
                QMap<int,QVector<int> > pixelsToCompute;
                QVector<QString> computedRasterFiles; // solo hay que salvar estos
                QMap<QString,double> factorTo8BitsByRasterFile;
                QMap<QString, QString> tuplekeysRasterFilesByRasterFile=iterBand.value();
                out<<"    - Banda ................................: "<<bandId<<"\n";
                out<<"      - Numero de escenas ..................: "<<tuplekeysRasterFilesByRasterFile.size()<<"\n";
                numberOfScenes=tuplekeysRasterFilesByRasterFile.size();
                QMap<QString, QString>::const_iterator iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                int numberOfPixelsToCompute=0;
                int rowMaxColumn=0;
                int maxColumn=0;
                QString title=QObject::tr("Removing clouds - Reading files");
                QString msgGlobal;
                msgGlobal=title+"\n\n";
                msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
                msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                        .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
                msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                        .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
                msgGlobal+=QObject::tr("\n  Number of bands to process .......: %1").arg(QString::number(tuplekeysRasterFilesByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing band number .......: %1, %2")
                        .arg(QString::number(numberOfProcessedBands)).arg(bandId);
                msgGlobal+=QObject::tr("\n  Number of files to read ..........: %1").arg(QString::number(tuplekeysRasterFilesByRasterFile.size()));
                QProgressDialog readingFilesProgress(title, QObject::tr("Abort"),0,tuplekeysRasterFilesByRasterFile.size(), mPtrWidgetParent);
                readingFilesProgress.setWindowModality(Qt::WindowModal);
                readingFilesProgress.setLabelText(msgGlobal);
                readingFilesProgress.setWindowTitle(title);
                readingFilesProgress.show();
                readingFilesProgress.adjustSize();
                qApp->processEvents();
                int numberOfReadedFiles=0;
                while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                {
                    numberOfReadedFiles++;
                    readingFilesProgress.setValue(numberOfReadedFiles);
                    if(readingFilesProgress.wasCanceled())
                    {
                        QMessageBox msgBox(mPtrWidgetParent);
                        QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Reading files");
                        msgBox.setText(msg);
                        QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                        msgBox.setInformativeText(question);
                        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                        msgBox.setDefaultButton(QMessageBox::Yes);
                        int ret = msgBox.exec();
                        bool abort=false;
                        switch (ret)
                        {
                          case QMessageBox::Yes:
                            abort=true;
                              // Save was clicked
                              break;
                          case QMessageBox::No:
                              break;
                          case QMessageBox::Cancel:
                              break;
                          default:
                              // should never be reached
                              break;
                        }
                        if(abort)
                        {
                            strError=QObject::tr("Process was canceled");
                            resultsFile.close();
                            return(false);
                        }
                    }
                    QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                    if(tuplekeyPath.isEmpty())
                    {
                        QFileInfo tuplekeyRasterFileFileInfo(tuplekeysRasterFile);
                        tuplekeyPath=tuplekeyRasterFileFileInfo.absolutePath();
                    }
                    QString rasterFile=iterTuplekeyRasterFile.key();
                    factorTo8BitsByRasterFile[rasterFile]=1.0;
                    float cloudyPercentage=0.0;
                    if(cloudyPercentageByRasterFile.contains(rasterFile))
                    {
                        cloudyPercentage=cloudyPercentageByRasterFile[rasterFile];
                    }
                    out<<"      - Escena a procesar ..................: "<<rasterFile;
                    out<<" ("<<QString::number(cloudyPercentage,'f',2).rightJustified(6)<<" % nubes -> ";
                    if(cloudyPercentage==0)
                    {
                        out<<"sin nubes";
                    }
                    else if(partiallyCloudyByRasterFile[rasterFile])
                    {
                        out<<"parcialmente nuboso";
                    }
                    else
                    {
                        out<<"totalmente nuboso";
                    }
                    out<<")\n";
                    IGDAL::Raster* ptrRasterFile=new IGDAL::Raster(mPtrCrsTools);
                    if(!ptrRasterFile->setFromFile(tuplekeysRasterFile,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    int columns,rows;
                    if(!ptrRasterFile->getSize(columns,rows,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    double dlNoDataValue;
                    int numberOfBand=0;
                    if(!ptrRasterFile->getNoDataValue(numberOfBand,dlNoDataValue,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    QVector<double> georef;
                    if(!ptrRasterFile->getGeoRef(georef,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError reading georef in raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    QVector<QVector<double> > values;
                    int initialColumn=0;
                    int initialRow=0;
                    if(!ptrRasterFile->readValues(numberOfBand, // desde 0
                                                  initialColumn,
                                                  initialRow,
                                                  columns,
                                                  rows,
                                                  values,
                                                  strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    // Conversión de los valores
                    double gain=intercalibrationGainByRasterFileAndByBand[rasterFile][bandId];
                    double offset=intercalibrationOffsetByRasterFileAndByBand[rasterFile][bandId];
                    bool intercalibrationTo8Bits=intercalibrationTo8BitsByRasterFileAndByBand[rasterFile][bandId];
                    bool intercalibrationToReflectance=intercalibrationToReflectanceByRasterFileAndByBand[rasterFile][bandId];
                    double reflectanceAddValue,reflectanceMultValue;
                    if(intercalibrationToReflectance)
                    {
                        reflectanceAddValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId];
                        reflectanceMultValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][bandId];
                    }
                    bool intercalibrationByInterpolation=intercalibrationInterpolatedByRasterFileAndByBand[rasterFile][bandId];
                    double factorTo8Bits=1.0;
                    if(intercalibrationTo8Bits
                            ||bandsCombinations.size()>0)
                    {
                        if(!ptrRasterFile->getFactorTo8Bits(factorTo8Bits,strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError getting factor to 8 bits for raster file:\n%1\nError:\n%2")
                                    .arg(tuplekeysRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
                        factorTo8BitsByRasterFile[rasterFile]=factorTo8Bits;
                    }
                    delete(ptrRasterFile);
                    for(int row=0;row<rows;row++)
                    {
                        for(int column=0;column<columns;column++)
                        {
                            double value=values[row][column];
                            if(intercalibrationToReflectance)
                            {
                                value=value*reflectanceMultValue+reflectanceAddValue;
                            }
                            if(intercalibrationTo8Bits)
                            {
                                value=value*factorTo8Bits;
                            }
                            value=value*gain+offset;
                            values[row][column]=value;
                        }
                    }
                    bandDataByRasterFile[rasterFile]=values;
                    bandGeorefByRasterFile[rasterFile]=georef;
                    bandNoDataValueByRasterFile[rasterFile]=dlNoDataValue;
                    QMap<int,QVector<int> > maskData;
                    if(cloudyPercentageByRasterFile[rasterFile]>0.0)
                    {
                        QMap<int,QVector<int> > maskDataRasterFile=maskDataByRasterFile[rasterFile];
//                        if(rasterFile.compare("LC82000322015101LGN00",Qt::CaseInsensitive)==0)
//                        {
//                            QMap<int,QVector<int> >::const_iterator kk1=maskDataRasterFile.begin();
//                            while(kk1!=maskDataRasterFile.end())
//                            {
//                                int rowKk=kk1.key();
//                                for(int kk2=0;kk2<kk1.value().size();kk2++)
//                                {
//                                    int columnKk=kk1.value()[kk2];
//                                    int yo=1;
//                                }
//                                kk1++;
//                            }
//                        }
                        QVector<double> maskGeorefRasterFile=maskGeorefByRasterFile[rasterFile];
                        double maskColumnGsdRasterFile=maskGeorefRasterFile[1];
                        double maskColumnGsd=georef[1];
                        if(fabs(maskColumnGsdRasterFile-maskColumnGsd)<0.01)
                        {
                            maskData=maskDataRasterFile;
                        }
                        else
                        {
                            if(maskColumnGsdRasterFile>maskColumnGsd)
                            {
                                int factorGsd=qRound(maskColumnGsdRasterFile/maskColumnGsd);
                                if(fabs(factorGsd-maskColumnGsdRasterFile/maskColumnGsd)>0.01)
                                {
                                    strError=QObject::tr("Algorithms::cloudRemoval");
                                    strError+=QObject::tr("\nError getting GSD factor for raster file:\n%1\nError:\n%2")
                                            .arg(tuplekeysRasterFile).arg(strAuxError);
                                    resultsFile.close();
                                    return(false);
                                }
                                QMap<int,QVector<int> >::const_iterator iterMaskDataRasterFile=maskDataRasterFile.begin();
                                while(iterMaskDataRasterFile!=maskDataRasterFile.end())
                                {
                                    int rowMaskRasterFile=iterMaskDataRasterFile.key();
                                    int firstRowMask=rowMaskRasterFile*factorGsd;
                                    for(int i=0;i<iterMaskDataRasterFile.value().size();i++)
                                    {
                                        int columnMaskRasterFile=iterMaskDataRasterFile.value()[i];
                                        int firstColumnMask=columnMaskRasterFile*factorGsd;
                                        for(int r=0;r<factorGsd;r++)
                                        {
                                            int rowMask=firstRowMask+r;
                                            if(!maskData.contains(rowMask))
                                            {
                                                QVector<int> aux;
                                                maskData[rowMask]=aux;
                                            }
                                            for(int c=0;c<factorGsd;c++)
                                            {
                                                int columnMask=firstColumnMask+c;
                                                // Si hay un noDataValue no lo añado como pixel de nube ya que no hay que obtenerlo
                                                if(fabs(bandDataByRasterFile[rasterFile][rowMask][columnMask]-bandNoDataValueByRasterFile[rasterFile])>0.01)
                                                {
                                                    maskData[rowMask].push_back(columnMask);
                                                }
                                            }
                                        }
                                    }
                                    iterMaskDataRasterFile++;
                                }
                            }
                            else
                            {
                                int factorGsd=qRound(maskColumnGsd/maskColumnGsdRasterFile);
                                if(fabs(factorGsd-maskColumnGsd/maskColumnGsdRasterFile)>0.01)
                                {
                                    strError=QObject::tr("Algorithms::cloudRemoval");
                                    strError+=QObject::tr("\nError getting GSD factor for raster file:\n%1\nError:\n%2")
                                            .arg(tuplekeysRasterFile).arg(strAuxError);
                                    resultsFile.close();
                                    return(false);
                                }
                                QMap<int,QVector<int> >::const_iterator iterMaskDataRasterFile=maskDataRasterFile.begin();
                                while(iterMaskDataRasterFile!=maskDataRasterFile.end())
                                {
                                    int rowMaskRasterFile=iterMaskDataRasterFile.key();
                                    int rowMask=rowMaskRasterFile/factorGsd; // 3/2 = 1
                                    for(int i=0;i<iterMaskDataRasterFile.value().size();i++)
                                    {
                                        int columnMaskRasterFile=iterMaskDataRasterFile.value()[i];
                                        int columnMask=columnMaskRasterFile/factorGsd;
                                        if(!maskData.contains(rowMask))
                                        {
                                            QVector<int> aux;
                                            maskData[rowMask]=aux;
                                        }
                                        // Si hay un noDataValue no lo añado como pixel de nube ya que no hay que obtenerlo
                                        if(fabs(bandDataByRasterFile[rasterFile][rowMask][columnMask]-bandNoDataValueByRasterFile[rasterFile])>0.01)
                                        {
                                            if(!maskData[rowMask].contains(columnMask))
                                            {
                                                maskData[rowMask].push_back(columnMask);
                                            }
                                        }
                                    }
                                    iterMaskDataRasterFile++;
                                }
                            }
                        }
                        if(partiallyCloudyByRasterFile[rasterFile]
                                ||removeFullCloudy) // para trabajar con las completamente cubiertas
                        {
                            QMap<int,QVector<int> >::const_iterator iterMaskDataRasterFile=maskDataRasterFile.begin();
                            while(iterMaskDataRasterFile!=maskDataRasterFile.end())
                            {
                                int row=iterMaskDataRasterFile.key();
                                if(!pixelsToCompute.contains(row))
                                {
                                    QVector<int> aux;
                                    pixelsToCompute[row]=aux;
                                }
                                for(int i=0;i<iterMaskDataRasterFile.value().size();i++)
                                {
                                    int column=iterMaskDataRasterFile.value()[i];
                                    if(!pixelsToCompute[row].contains(column))
                                    {
                                        pixelsToCompute[row].push_back(column);
                                        if(column>maxColumn)
                                        {
                                            maxColumn=column;
                                            rowMaxColumn=row;
                                        }
                                        numberOfPixelsToCompute++;
                                    }
                                }
                                iterMaskDataRasterFile++;
                            }
                        }
                    }
                    bandMaskDataByRasterFile[rasterFile]=maskData;
                    iterTuplekeyRasterFile++;
                }
                readingFilesProgress.setValue(tuplekeysRasterFilesByRasterFile.size());
                readingFilesProgress.close();

                out<<"      - Numero de pixeles a procesar .......: "<<QString::number(numberOfPixelsToCompute);
                out<<", columna mayor "<<QString::number(maxColumn)<<" para la fila "<<QString::number(rowMaxColumn);
                out<<"\n";
                title=QObject::tr("Removing clouds - Processing pixels");
                msgGlobal=title+"\n\n";
                msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
                msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                        .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
                msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                        .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
                msgGlobal+=QObject::tr("\n  Number of bands to process .......: %1").arg(QString::number(tuplekeysRasterFilesByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing band number .......: %1, %2")
                        .arg(QString::number(numberOfProcessedBands)).arg(bandId);
                msgGlobal+=QObject::tr("\n  Number of pixels to process ......: %1").arg(QString::number(numberOfPixelsToCompute));
                int numberOfProcessedPixelsSteps=1000;
                int numberOfPixelsByStep=numberOfPixelsToCompute/numberOfProcessedPixelsSteps;
                QProgressDialog processingPixelsProgress(title, QObject::tr("Abort"),0,numberOfProcessedPixelsSteps, mPtrWidgetParent);
                processingPixelsProgress.setWindowModality(Qt::WindowModal);
                processingPixelsProgress.setLabelText(msgGlobal);
//                processingPixelsProgress.setWindowTitle(title);
                processingPixelsProgress.show();
//                processingPixelsProgress.adjustSize();
                qApp->processEvents();
                if(printDetail)
                {
                    out<<"      - Resultados para el primer y ultimo pixel:\n";
                    out<<"     Row  Column                        Escena        ND    Nube   %Nube        Jd   ND_Intc  Dato  Calc  Interpol Sintetico     Error\n";
                }
                QMap<int,QVector<int> >::const_iterator iterPixelsToCompute=pixelsToCompute.begin();
                int computedPixels=-1;
                int computedPixelsInStep=0;
                int numberOfStep=0;
                while(iterPixelsToCompute!=pixelsToCompute.end())
                {
                    int row=iterPixelsToCompute.key();
                    for(int i=0;i<iterPixelsToCompute.value().size();i++)
//                    for(int i=0;i<1;i++)
                    {
                        int column=iterPixelsToCompute.value()[i];
                        computedPixels++;
                        computedPixelsInStep++;
                        if(computedPixelsInStep==numberOfPixelsByStep)
                        {
                            numberOfStep++;
                            processingPixelsProgress.setValue(numberOfStep);
                            computedPixelsInStep=0;
                            if (processingPixelsProgress.wasCanceled())
                            {
                                QMessageBox msgBox(mPtrWidgetParent);
                                QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Processing pixels");
                                msgBox.setText(msg);
                                QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                                msgBox.setInformativeText(question);
                                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                                msgBox.setDefaultButton(QMessageBox::Yes);
                                int ret = msgBox.exec();
                                bool abort=false;
                                switch (ret)
                                {
                                  case QMessageBox::Yes:
                                    abort=true;
                                      // Save was clicked
                                      break;
                                  case QMessageBox::No:
                                      break;
                                  case QMessageBox::Cancel:
                                      break;
                                  default:
                                      // should never be reached
                                      break;
                                }
                                if(abort)
                                {
                                    strError=QObject::tr("Process was canceled");
                                    resultsFile.close();
                                    return(false);
                                }
                            }
                        }
                        int pixelDataMinJd=100000000;
                        int pixelDataMaxJd=-100000000;
                        int pixelToInterpolateMinJd=100000000;
                        int pixelToInterpolateMaxJd=-100000000;
                        QMap<int,double> pixelDataByJd;
                        QMap<int,QString> rasterFileDataByJd;
                        QVector<int> jdToInterpolate;
                        QMap<int,QString> rasterFileToInterpolateByJd;
                        QMap<QString, QString>::const_iterator iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                        // para imprimir
                        QMap<int,double> toPrintPixelDataByJd;
                        QMap<int,bool> toPrintPixelValidData;
                        while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                        {
//                            QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                            QString rasterFile=iterTuplekeyRasterFile.key();
                            int jd=jdByRasterFile[rasterFile];
                            double pixelData=bandDataByRasterFile[rasterFile][row][column];
                            bool pixelValidData=false;
                            if(fabs(bandDataByRasterFile[rasterFile][row][column]-bandNoDataValueByRasterFile[rasterFile])>0.01)
                            {
                                if(!bandMaskDataByRasterFile[rasterFile].contains(row))
                                {
                                    pixelValidData=true;
                                }
                                else
                                {
                                    if(!bandMaskDataByRasterFile[rasterFile][row].contains(column))
                                    {
                                        pixelValidData=true;
                                    }
                                }
                                if(pixelValidData)
                                {
                                    pixelDataByJd[jd]=pixelData;
                                    rasterFileDataByJd[jd]=rasterFile;
                                    if(jd<pixelDataMinJd)
                                    {
                                        pixelDataMinJd=jdByRasterFile[rasterFile];
                                    }
                                    if(jd>pixelDataMaxJd)
                                    {
                                        pixelDataMaxJd=jdByRasterFile[rasterFile];
                                    }
                                }
                                else
                                {
                                    if(partiallyCloudyByRasterFile[rasterFile]
                                            ||removeFullCloudy)
                                    {
                                        jdToInterpolate.push_back(jd);
                                        rasterFileToInterpolateByJd[jd]=rasterFile;
                                        if(jd<pixelToInterpolateMinJd)
                                        {
                                            pixelToInterpolateMinJd=jd;
                                        }
                                        if(jd>pixelToInterpolateMaxJd)
                                        {
                                            pixelToInterpolateMaxJd=jd;
                                        }
                                    }
                                }
                            }
                            if(printDetail&&(computedPixels==0||computedPixels==(numberOfPixelsToCompute-1)))
                            {
                                toPrintPixelDataByJd[jd]=pixelData;
                                toPrintPixelValidData[jd]=pixelValidData;
                            }
                            iterTuplekeyRasterFile++;
                        }
                        if(jdToInterpolate.size()==0
                                ||pixelDataByJd.size()==0)
                        {
                            continue;
                        }
                        int firstDataJd=pixelDataMinJd;
                        int lastDataJd=pixelDataMaxJd;
                        if(numberOfDates>0)
                        {
                            firstDataJd=pixelToInterpolateMinJd-numberOfDates;
                            if(firstDataJd<pixelDataMinJd)
                            {
                                firstDataJd=pixelDataMinJd;
                            }
                            lastDataJd=pixelToInterpolateMaxJd+numberOfDates;
                            if(lastDataJd>pixelDataMaxJd)
                            {
                                lastDataJd=pixelDataMaxJd;
                            }
                        }
                        int contData=0;
                        QMap<int,double>::const_iterator iterPixelDataByJd=pixelDataByJd.begin();
                        QVector<int> pixelDataJds; // va a estar ordenado porque lo creo recorriendo un map que lo esta
                        while(iterPixelDataByJd!=pixelDataByJd.end())
                        {
                            int jd=iterPixelDataByJd.key();
                            if(jd<firstDataJd||jd>lastDataJd)
                            {
                                iterPixelDataByJd++;
                                continue;
                            }
                            contData++;
                            pixelDataJds.push_back(jd);
                            iterPixelDataByJd++;
                        }
                        if(contData==0)
                        {
                            continue;
                        }
                        QMap<int,double> toPrintInterpolatedValues;
                        QMap<int,double> sinteticValues;
                        if(contData>1)
                        {
                            Splines::AkimaSpline* ptrAkimaSpline=NULL;
                            Splines::CubicSpline* ptrCubicSpline=NULL;
                            Splines::BesselSpline* ptrBesselSpline=NULL;
                            double* x;
                            double* y;
                            x=new double[contData];
                            y=new double[contData];
                            contData=0;
                            iterPixelDataByJd=pixelDataByJd.begin();
                            while(iterPixelDataByJd!=pixelDataByJd.end())
                            {
                                int jd=iterPixelDataByJd.key();
                                if(jd<firstDataJd||jd>lastDataJd)
                                {
                                    iterPixelDataByJd++;
                                    continue;
                                }
                                x[contData]=jd-firstDataJd;
                                y[contData]=iterPixelDataByJd.value();
                                contData++;
                                iterPixelDataByJd++;
                            }
                            if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                ptrAkimaSpline=new Splines::AkimaSpline();
                                ptrAkimaSpline->build(x,y,contData) ;
                            }
                            else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                ptrCubicSpline=new Splines::CubicSpline();
                                ptrCubicSpline->build(x,y,contData) ;
                            }
                            else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                ptrBesselSpline=new Splines::BesselSpline();
                                ptrBesselSpline->build(x,y,contData) ;
                            }
                            delete(x);
                            delete(y);
                            // Calculo de datos sintéticos
                            if(contData==2)
                            {
                                sinteticValues[pixelDataJds[0]]=pixelDataByJd[pixelDataJds[1]];
                                sinteticValues[pixelDataJds[1]]=pixelDataByJd[pixelDataJds[0]];
                            }
                            else
                            {
                                iterPixelDataByJd=pixelDataByJd.begin();
                                while(iterPixelDataByJd!=pixelDataByJd.end())
                                {
                                    int jd=iterPixelDataByJd.key();
                                    int posJdInPixelDataJds=pixelDataJds.indexOf(jd);
                                    if(jd<firstDataJd||jd>lastDataJd)
                                    {
                                        if(jd<firstDataJd)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[firstDataJd];
                                        }
                                        if(jd>lastDataJd)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[lastDataJd];
                                        }
                                        iterPixelDataByJd++;
                                        continue;
                                    }
                                    else if(jd==firstDataJd)
                                    {
                                        if((posJdInPixelDataJds+1)<pixelDataJds.size())
                                        {
                                            sinteticValues[jd]=pixelDataByJd[pixelDataJds[posJdInPixelDataJds+1]];
                                        }
                                        else
                                        {
                                            sinteticValues[jd]=pixelDataByJd[jd];
                                        }
                                    }
                                    else if(jd==lastDataJd)
                                    {
                                        if(posJdInPixelDataJds>0)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[pixelDataJds[posJdInPixelDataJds-1]];
                                        }
                                        else
                                        {
                                            sinteticValues[jd]=pixelDataByJd[jd];
                                        }
                                    }
                                    else // en este caso hay que interpolar
                                    {
                                        double* xSintetic;
                                        double* ySintetic;
                                        int contDataSintetic=contData-1;
                                        xSintetic=new double[contDataSintetic];
                                        ySintetic=new double[contDataSintetic];
                                        contDataSintetic=0;
                                        QMap<int,double>::const_iterator iterPixelDataByJdSintetic=pixelDataByJd.begin();
                                        while(iterPixelDataByJdSintetic!=pixelDataByJd.end())
                                        {
                                            int jdAux=iterPixelDataByJdSintetic.key();
                                            if(jdAux==jd)
                                            {
                                                iterPixelDataByJdSintetic++;
                                                continue;
                                            }
                                            if(jd<firstDataJd||jd>lastDataJd)
                                            {
                                                iterPixelDataByJdSintetic++;
                                                continue;
                                            }
                                            xSintetic[contDataSintetic]=jdAux-firstDataJd;
                                            ySintetic[contDataSintetic]=iterPixelDataByJdSintetic.value();
                                            contDataSintetic++;
                                            iterPixelDataByJdSintetic++;
                                        }
                                        double xJdSintetic=jd-firstDataJd;
                                        if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::AkimaSpline* ptrAkimaSplineSintetic=NULL;
                                            ptrAkimaSplineSintetic=new Splines::AkimaSpline();
                                            ptrAkimaSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrAkimaSplineSintetic)(xJdSintetic);
                                            delete(ptrAkimaSplineSintetic);
                                        }
                                        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::CubicSpline* ptrCubicSplineSintetic=NULL;
                                            ptrCubicSplineSintetic=new Splines::CubicSpline();
                                            ptrCubicSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrCubicSplineSintetic)(xJdSintetic);
                                            delete(ptrCubicSplineSintetic);
                                        }
                                        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::BesselSpline* ptrBesselSplineSintetic=NULL;
                                            ptrBesselSplineSintetic=new Splines::BesselSpline();
                                            ptrBesselSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrBesselSplineSintetic)(xJdSintetic);
                                            delete(ptrBesselSplineSintetic);
                                        }
                                        delete(xSintetic);
                                        delete(ySintetic);
                                    }
                                    iterPixelDataByJd++;
                                }
                            }
                            for(int k=0;k<jdToInterpolate.size();k++)
                            {
                                double interpolatedValue=0.0;
                                int jd=jdToInterpolate[k];
                                double xJd=jd-firstDataJd;
                                if(jd<=firstDataJd)
                                {
                                    interpolatedValue=pixelDataByJd[firstDataJd];
                                }
                                else if(jd>=lastDataJd)
                                {
                                    interpolatedValue=pixelDataByJd[lastDataJd];
                                }
                                else
                                {
                                    if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                                    {
                                        interpolatedValue=(*ptrAkimaSpline)(xJd);
                                    }
                                    else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                                    {
                                        interpolatedValue=(*ptrCubicSpline)(xJd);
                                    }
                                    else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                                    {
                                        interpolatedValue=(*ptrBesselSpline)(xJd);
                                    }
                                }
                                // No hay que deshacer porque generaremos los ficheros intercalibrados
                                QString rasterFile=rasterFileToInterpolateByJd[jd];
                                bandDataByRasterFile[rasterFile][row][column]=interpolatedValue;
                                if(!computedRasterFiles.contains(rasterFile))
                                {
                                    computedRasterFiles.push_back(rasterFile);
                                }
                                if(printDetail&&(computedPixels==0||computedPixels==(numberOfPixelsToCompute-1)))
                                {
                                    toPrintInterpolatedValues[jd]=interpolatedValue;
                                }
                            }
                            if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                delete(ptrAkimaSpline);
                            }
                            else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                delete(ptrCubicSpline);
                            }
                            else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                delete(ptrBesselSpline);
                            }
                        }
                        else // solo hay un dato, que se asigna a todos los valores a interpolar
                        {
                            sinteticValues[pixelDataJds[0]]=pixelDataByJd[pixelDataJds[0]];
                            for(int k=0;k<jdToInterpolate.size();k++)
                            {
                                double interpolatedValue=pixelDataByJd[pixelDataJds[0]];
                                int jd=jdToInterpolate[k];
                                // No hay que deshacer porque generaremos los ficheros intercalibrados
                                QString rasterFile=rasterFileToInterpolateByJd[jd];
                                bandDataByRasterFile[rasterFile][row][column]=interpolatedValue;
                                if(!computedRasterFiles.contains(rasterFile))
                                {
                                    computedRasterFiles.push_back(rasterFile);
                                }
                                if(printDetail&&(computedPixels==0||computedPixels==(numberOfPixelsToCompute-1)))
                                {
                                    toPrintInterpolatedValues[jd]=interpolatedValue;
                                }
                            }
                        }
                        if(printDetail&&(computedPixels==0||computedPixels==(numberOfPixelsToCompute-1)))
                        {
                            iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                            while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                            {
    //                            QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                                QString rasterFile=iterTuplekeyRasterFile.key();
                                int jd=jdByRasterFile[rasterFile];
                                double intercalibratedValue=toPrintPixelDataByJd[jd];
                                double gain=intercalibrationGainByRasterFileAndByBand[rasterFile][bandId];
                                double offset=intercalibrationOffsetByRasterFileAndByBand[rasterFile][bandId];
                                bool intercalibrationTo8Bits=intercalibrationTo8BitsByRasterFileAndByBand[rasterFile][bandId];
                                bool intercalibrationToReflectance=intercalibrationToReflectanceByRasterFileAndByBand[rasterFile][bandId];
                                bool intercalibrationByInterpolation=intercalibrationInterpolatedByRasterFileAndByBand[rasterFile][bandId];
                                double factorTo8Bits=1.0;
                                if(intercalibrationTo8Bits)
                                {
                                    factorTo8Bits=factorTo8BitsByRasterFile[rasterFile];
                                }
                                double value=(intercalibratedValue-offset)/gain;
                                if(intercalibrationTo8Bits)
                                {
                                    value=value/factorTo8Bits;
                                }
                                if(intercalibrationToReflectance)
                                {
                                    double reflectanceAddValue,reflectanceMultValue;
                                    reflectanceAddValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId];
                                    reflectanceMultValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][bandId];
                                    value=(value-reflectanceAddValue)/reflectanceMultValue;
                                }
                                out<<QString::number(row).rightJustified(8);
                                out<<QString::number(column).rightJustified(8);
                                out<<rasterFile.rightJustified(30);
                                out<<QString::number(value,'f',1).rightJustified(10);
                                if(toPrintPixelValidData[jd])
                                {
                                    out<<QString::number(0.0,'f',1).rightJustified(8);
                                }
                                else
                                {
                                    out<<QString::number(255.0,'f',1).rightJustified(8);
                                }
                                out<<QString::number(cloudyPercentageByRasterFile[rasterFile],'f',2).rightJustified(8);
                                out<<QString::number(jd).rightJustified(10);
                                out<<QString::number(intercalibratedValue,'f',1).rightJustified(10);
                                QString strToUse="No";
                                if(toPrintPixelValidData[jd])
                                {
                                    strToUse="Si";
                                }
                                out<<strToUse.rightJustified(6);
                                QString strToCalc;
                                if(!toPrintPixelValidData[jd])
                                {
                                    if(partiallyCloudyByRasterFile[rasterFile]
                                            ||removeFullCloudy)
                                    {
                                        strToCalc="Si";
                                    }
                                    else
                                    {
                                        strToCalc="No";
                                    }
                                }
                                out<<strToCalc.rightJustified(6);
                                QString strInterpolatedValue;
                                if(strToCalc.compare("Si",Qt::CaseInsensitive)==0)
                                {
                                    strInterpolatedValue=QString::number(toPrintInterpolatedValues[jd],'f',1);
                                }
                                out<<strInterpolatedValue.rightJustified(10);
                                QString strSinteticValue;
                                QString strErrorValue;
                                if(sinteticValues.contains(jd))
                                {
                                    strSinteticValue=QString::number(sinteticValues[jd],'f',1);
                                    double errorValue=intercalibratedValue-sinteticValues[jd];
                                    strErrorValue=QString::number(errorValue,'f',1);
                                }
                                out<<strSinteticValue.rightJustified(10);
                                out<<strErrorValue.rightJustified(10);
                                out<<"\n";
                                iterTuplekeyRasterFile++;
                            }
                        }
                    }
                    iterPixelsToCompute++;
                }
                processingPixelsProgress.setValue(numberOfProcessedPixelsSteps);
                processingPixelsProgress.close();

                // Proceso de escritura
                title=QObject::tr("Removing clouds - Writting files");
                msgGlobal=title+"\n\n";
                msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
                msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                        .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
                msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                        .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
                msgGlobal+=QObject::tr("\n  Number of bands to process .......: %1").arg(QString::number(tuplekeysRasterFilesByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing band number .......: %1, %2")
                        .arg(QString::number(numberOfProcessedBands)).arg(bandId);
                msgGlobal+=QObject::tr("\n  Number of files to write .........: %1").arg(QString::number(tuplekeysRasterFilesByRasterFile.size()));
                QProgressDialog writtingFilesProgress(title, QObject::tr("Abort"),0,tuplekeysRasterFilesByRasterFile.size(), mPtrWidgetParent);
                writtingFilesProgress.setWindowModality(Qt::WindowModal);
                writtingFilesProgress.setLabelText(msgGlobal);
//                writtingFilesProgress.setWindowTitle(title);
                writtingFilesProgress.show();
//                writtingFilesProgress.adjustSize();
                qApp->processEvents();
                int numberOfWrittenFiles=0;
                iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                {
                    numberOfWrittenFiles++;
                    writtingFilesProgress.setValue(numberOfWrittenFiles);
                    if(writtingFilesProgress.wasCanceled())
                    {
                        QMessageBox msgBox(mPtrWidgetParent);
                        QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Reading files");
                        msgBox.setText(msg);
                        QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                        msgBox.setInformativeText(question);
                        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                        msgBox.setDefaultButton(QMessageBox::Yes);
                        int ret = msgBox.exec();
                        bool abort=false;
                        switch (ret)
                        {
                          case QMessageBox::Yes:
                            abort=true;
                              // Save was clicked
                              break;
                          case QMessageBox::No:
                              break;
                          case QMessageBox::Cancel:
                              break;
                          default:
                              // should never be reached
                              break;
                        }
                        if(abort)
                        {
                            strError=QObject::tr("Process was canceled");
                            resultsFile.close();
                            return(false);
                        }
                    }
                    QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                    QString rasterFile=iterTuplekeyRasterFile.key();
                    QVector<QVector<double> > bandData=bandDataByRasterFile[rasterFile];
                    QFileInfo rasterFileInfo(tuplekeysRasterFile);
                    QString removedCloudsRasterFileName=rasterFileInfo.absolutePath()+"/";
                    removedCloudsRasterFileName+=ALGORITHMS_CLOUDREMOVAL_PARAMETER_WRITEIMAGEFILES_FOLDER;
                    removedCloudsRasterFileName+="/";
                    removedCloudsRasterFileName+=bandId;
                    QString removedCloudsRasterFilePath=removedCloudsRasterFileName;
                    QDir rasterFileDir(rasterFileInfo.absolutePath());
                    if(!rasterFileDir.exists(removedCloudsRasterFileName))
                    {
                        if(!rasterFileDir.mkpath(removedCloudsRasterFilePath))
                        {
                            strError=QObject::tr("Algorithms::intercalibrationComputation");
                            strError+=QObject::tr("\nError making path for removed clouds file:\n%1")
                                    .arg(removedCloudsRasterFilePath);
                            resultsFile.close();
                            return(false);
                        }
                    }
                    removedCloudsRasterFileName+=("/"+rasterFileInfo.fileName());
                    if(!writeCloudRemovedFile(tuplekeysRasterFile,
                                              removedCloudsRasterFileName,
                                              bandData,
                                              strAuxError))
                    {
                        strError=QObject::tr("Algorithms::intercalibrationComputation");
                        strError+=QObject::tr("\nError writting removed clouds file:\n%1\nError:\%2")
                                .arg(removedCloudsRasterFileName).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
//                    QMap<QString,QMap<QString,QVector<QString> > > bandsCombinationsByRasterType;
//                    QMap<QString,QMap<QString,QString> > bandsCombinationsFileBaseNameByRasterType;
//                    QMap<QString,QMap<QString,QVector<GByte*> > > bandsCombinationsBandsDataByRasterType;
                    iterTuplekeyRasterFile++;
                }
                writtingFilesProgress.setValue(tuplekeysRasterFilesByRasterFile.size());
                writtingFilesProgress.close();

                // Informacion para combinaciones
                if(bandCombinationsIds.size()>0)
                {
                    iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                    while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                    {
                        QString rasterFile=iterTuplekeyRasterFile.key();
                        QVector<QVector<double> > bandData=bandDataByRasterFile[rasterFile];
                        int rows=bandData.size();
                        int columns=bandData[0].size();
//                        int numberOfPixels=rows*columns;
                        QVector<QVector<qint8> > data(rows);
                        for(int row=0;row<rows;row++)
                        {
                            data[row].resize(columns);
                            for(int column=0;column<columns;column++)
                            {
                                data[row][column]=(int)(bandData[row][column]*factorTo8BitsByRasterFile[rasterFile]);
                            }
                        }
//                        GByte*  pData=NULL;
//                        pData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
//                        int pos=0;
//                        for(int row=0;row<rows;row++)
//                        {
//                            for(int column=0;column<columns;column++)
//                            {
//                                pData[pos]=(int)((int)bandData[row][column])*((int)factorTo8BitsByRasterFile[rasterFile]);
//                                pos++;
//                            }
//                        }
//                        combinationsDataByBandByRasterFile[bandId][rasterFile]=pData;
                        combinationsDataByBandByRasterFile[bandId][rasterFile]=data;

                        iterTuplekeyRasterFile++;
                    }
                }
                iterBand++;
//                break;
            }
            // Creacion de las combinaciones de bandas
            int numberOfBandCombinationsFiles=0;
            for(int nrf=0;nrf<rasterFiles.size();nrf++)
            {
                QString rasterFile=rasterFiles.at(nrf);
                if(rasterTypesByRasterFile[rasterFile].compare(rasterType,Qt::CaseInsensitive)!=0)
                {
                    continue;
                }
                QMap<QString,QVector<QString> >::const_iterator iterBandsCombinations=bandsCombinations.begin();
                while(iterBandsCombinations!=bandsCombinations.end())
                {
                    QString bandsCombination=iterBandsCombinations.key();
                    bool existsBandsForCombination=true;
                    for(int nb=0;nb<iterBandsCombinations.value().size();nb++)
                    {
                        QString bandId=iterBandsCombinations.value().at(nb);
                        if(!combinationsDataByBandByRasterFile.contains(bandId))
                        {
                            existsBandsForCombination=false;
                            break;
                        }
                        else
                        {
                            if(!combinationsDataByBandByRasterFile[bandId].contains(rasterFile))
                            {
                                existsBandsForCombination=false;
                                break;
                            }
                        }
                    }
                    if(!existsBandsForCombination)
                    {
                        iterBandsCombinations++;
                        continue;
                    }
                    numberOfBandCombinationsFiles++;
                    iterBandsCombinations++;
                }
            }
            out<<"  - Ficheros de combinaciones de bandas ....: "<<QString::number(numberOfBandCombinationsFiles)<<"\n";
            QString title=QObject::tr("Removing clouds - Reading files");
            QString msgGlobal;
            msgGlobal=title+"\n\n";
            msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
            msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                    .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
            msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
            msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                    .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
            msgGlobal+=QObject::tr("\n  Number of bands combinations files to write .......: %1").arg(QString::number(numberOfBandCombinationsFiles));
            QProgressDialog writtingBandsCombinationsFilesProgress(title, QObject::tr("Abort"),0,numberOfBandCombinationsFiles, mPtrWidgetParent);
            writtingBandsCombinationsFilesProgress.setWindowModality(Qt::WindowModal);
            writtingBandsCombinationsFilesProgress.setLabelText(msgGlobal);
            writtingBandsCombinationsFilesProgress.setWindowTitle(title);
            writtingBandsCombinationsFilesProgress.show();
            writtingBandsCombinationsFilesProgress.adjustSize();
            qApp->processEvents();
            int numberOfBandsCombinatiosnFilesWritted=0;
            for(int nrf=0;nrf<rasterFiles.size();nrf++)
            {
                QString rasterFile=rasterFiles.at(nrf);
                out<<"    - Raster file ..........................: "<<rasterFile<<"\n";
                if(rasterTypesByRasterFile[rasterFile].compare(rasterType,Qt::CaseInsensitive)!=0)
                {
                    continue;
                }
                QMap<QString,QVector<QString> >::const_iterator iterBandsCombinations=bandsCombinations.begin();
                while(iterBandsCombinations!=bandsCombinations.end())
                {
                    QString bandsCombination=iterBandsCombinations.key();
                    out<<"      - Combinacion ........................: "<<bandsCombination<<"\n";
                    bool existsBandsForCombination=true;
                    QVector<QVector<QVector<qint8> > > bandsData;
                    int columns=0;
                    int rows=0;
                    for(int nb=0;nb<iterBandsCombinations.value().size();nb++)
                    {
                        QString bandId=iterBandsCombinations.value().at(nb);
                        if(!combinationsDataByBandByRasterFile.contains(bandId))
                        {
                            existsBandsForCombination=false;
                            break;
                        }
                        else
                        {
                            if(!combinationsDataByBandByRasterFile[bandId].contains(rasterFile))
                            {
                                existsBandsForCombination=false;
                                break;
                            }
                        }
                        if(columns==0)
                        {
                            rows=combinationsDataByBandByRasterFile[bandId][rasterFile].size();
                            columns=combinationsDataByBandByRasterFile[bandId][rasterFile][0].size();
                        }
                        bandsData.push_back(combinationsDataByBandByRasterFile[bandId][rasterFile]);
                    }
                    if(!existsBandsForCombination)
                    {
                        iterBandsCombinations++;
                        continue;
                    }
                    numberOfBandsCombinatiosnFilesWritted++;
                    writtingBandsCombinationsFilesProgress.setValue(numberOfBandsCombinatiosnFilesWritted);
                    if(writtingBandsCombinationsFilesProgress.wasCanceled())
                    {
                        QMessageBox msgBox(mPtrWidgetParent);
                        QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Reading files");
                        msgBox.setText(msg);
                        QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                        msgBox.setInformativeText(question);
                        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                        msgBox.setDefaultButton(QMessageBox::Yes);
                        int ret = msgBox.exec();
                        bool abort=false;
                        switch (ret)
                        {
                          case QMessageBox::Yes:
                            abort=true;
                              // Save was clicked
                              break;
                          case QMessageBox::No:
                              break;
                          case QMessageBox::Cancel:
                              break;
                          default:
                              // should never be reached
                              break;
                        }
                        if(abort)
                        {
                            strError=QObject::tr("Process was canceled");
                            resultsFile.close();
                            return(false);
                        }
                    }
                    QString bandsCombinationFileName=tuplekeyPath+"/";
                    bandsCombinationFileName+=ALGORITHMS_CLOUDREMOVAL_PARAMETER_WRITEIMAGEFILES_FOLDER;
                    bandsCombinationFileName+="/";
                    bandsCombinationFileName+=bandsCombinationsFileBaseNameByRasterTypeByRasterFile[rasterType][rasterFile][bandsCombination];
                    QFileInfo bandsCombinationFileInfo(bandsCombinationFileName);
                    QString bandsCombinationFilePath=bandsCombinationFileInfo.absolutePath();
                    if(!auxDir.exists(bandsCombinationFilePath))
                    {
                        if(!auxDir.mkpath(bandsCombinationFilePath))
                        {
                            strError=QObject::tr("\nError making path for removed clouds file:\n%1")
                                    .arg(bandsCombinationFilePath);
                            resultsFile.close();
                            return(false);
                        }
                    }
                    out<<"      - Fichero de salida ..................: "<<bandsCombinationFileName<<"\n";
//                    QImage bandCombinationImage(columns,rows,QImage::Format_RGB32);//QImage::Format_RGB888 );
                    QImage bandCombinationImage(columns,rows,QImage::Format_RGB888 );
                    for(int column=0;column<columns;column++)
                    {
                        for(int row=0;row<rows;row++)
                        {
                            int red=bandsData[0][row][column];
                            int green=bandsData[1][row][column];
                            int blue=bandsData[2][row][column];
                            bandCombinationImage.setPixel(column,row,qRgb(red,green,blue));
                        }
                    }
//                    QString removedCloudsRasterFileName=rasterFileInfo.absolutePath()+"/";
//                    removedCloudsRasterFileName+=ALGORITHMS_CLOUDREMOVAL_PARAMETER_WRITEIMAGEFILES_FOLDER;
//                    removedCloudsRasterFileName+="/";
//                    removedCloudsRasterFileName+=bandId;
//                    QString removedCloudsRasterFilePath=removedCloudsRasterFileName;
                    if(!bandCombinationImage.save(bandsCombinationFileName,"JPG", 100))
//                    if(!bandCombinationImage.save(bandsCombinationFileName))
                    {
                        strError+=QObject::tr("\nError writting band combination image:\n%1").arg(bandsCombinationFileName);
                        resultsFile.close();
                        return(false);
                    }
                    iterBandsCombinations++;
                }
            }
            writtingBandsCombinationsFilesProgress.setValue(numberOfBandCombinationsFiles);
            writtingBandsCombinationsFilesProgress.close();
            iterRasterType++;
        }
        iterTuplekey++;
    }
//    progress.close();
    QDateTime finalDateTime=QDateTime::currentDateTime();
    int initialSeconds=(int)initialDateTime.toTime_t();
    int finalSeconds=(int)finalDateTime.toTime_t();
    int totalDurationSeconds=finalSeconds-initialSeconds;
    double dblTotalDurationSeconds=(double)totalDurationSeconds;
    int durationDays=(int)floor(dblTotalDurationSeconds/60.0/60.0/24.0);
    int durationHours=(int)floor((dblTotalDurationSeconds-durationDays*60.0*60.0*24.0)/60.0/60.0);
    int durationMinutes=(int)floor((dblTotalDurationSeconds-durationDays*60.0*60.0*24.0-durationHours*60.0*60.0)/60.0);
    int durationSeconds=dblTotalDurationSeconds-durationDays*60.0*60.0*24.0-durationHours*60.0*60.0-durationMinutes*60.0;
    {
        QString msgTtime="\n- Process time:\n";
        msgTtime+="  - Start time of the process ......................: ";
        msgTtime+=initialDateTime.toString("yyyy/MM/dd - hh/mm/ss.zzz");
        msgTtime+="\n";
        msgTtime+="  - End time of the process ........................: ";
        msgTtime+=finalDateTime.toString("yyyy/MM/dd - hh/mm/ss.zzz");
        msgTtime+="\n";
        msgTtime+="  - Number of total seconds ........................: ";
        msgTtime+=QString::number(dblTotalDurationSeconds,'f',3);
        msgTtime+="\n";
        msgTtime+="    - Number of days ...............................: ";
        msgTtime+=QString::number(durationDays);
        msgTtime+="\n";
        msgTtime+="    - Number of hours ..............................: ";
        msgTtime+=QString::number(durationHours);
        msgTtime+="\n";
        msgTtime+="    - Number of minutes ............................: ";
        msgTtime+=QString::number(durationMinutes);
        msgTtime+="\n";
        msgTtime+="    - Number of seconds ............................: ";
        msgTtime+=QString::number(durationSeconds,'f',3);
        msgTtime+="\n";
        out<<msgTtime;
    }
    resultsFile.close();
    return(true);
}

bool Algorithms::cloudRemovalByCloudFreeImprovement(QVector<QString> &rasterFiles,
                                                    QMap<QString, QString> &rasterTypesByRasterFile,
                                                    QMap<QString, QVector<QString> > &rasterFilesByTuplekey,
                                                    QMap<QString, QMap<QString, QMap<QString, QString> > > &tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand,
                                                    QMap<QString, int> &jdByRasterFile,
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
                                                    QMap<QString, QVector<QString> > &bandsInAlgorithmBySpaceCraft,
                                                    QString &strError)
{
    QDir auxDir(QDir::currentPath());
    QDateTime initialDateTime=QDateTime::currentDateTime();
    bool debugMode=true;
    bool printDetail=true; // para el pixel primero y ultimo
    QString landsat8IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8;
    QString sentinel2IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2;
    QString orthoimageIdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_ORTHOIMAGE;
    QString strAuxError;
    // Obtener la ruta del workspace
    QMap<QString, QVector<QString> >::const_iterator iterRasterFilesByQuadkey=rasterFilesByTuplekey.begin();
    QString workspaceBasePath;
    // Ojo, esto funciona si están las bandas de ndvi, que se supone estarán siempre
    while(iterRasterFilesByQuadkey!=rasterFilesByTuplekey.end())
    {
        QString quadkey=iterRasterFilesByQuadkey.key();
        QVector<QString> rasterFilesInQuadkey=iterRasterFilesByQuadkey.value();
        int numberOfRasterFilesInQuadkey=rasterFilesInQuadkey.size();
        for(int nrf=0;nrf<numberOfRasterFilesInQuadkey;nrf++)
        {
            QString rasterFile=rasterFilesInQuadkey[nrf];
            QString rasterType=rasterTypesByRasterFile[rasterFile];
            if(rasterType.compare(orthoimageIdDb)==0)
            {
                continue;
            }
            if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.contains(quadkey))
            {
                continue;
            }
            if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey].contains(rasterFile))
            {
                continue;
            }
            QString redBandRasterFile,nirBandRasterFile;
            bool validRasterType=false;
            if(rasterType.compare(landsat8IdDb)==0)
            {
                if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_LANDSAT8_BAND_B4_CODE)
                        ||!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_LANDSAT8_BAND_B5_CODE))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                redBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_LANDSAT8_BAND_B4_CODE];
                nirBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_LANDSAT8_BAND_B5_CODE];
                validRasterType=true;
            }
            if(rasterType.compare(sentinel2IdDb)==0)
            {
                if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_SENTINEL2_BAND_B4_CODE)
                        ||!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_SENTINEL2_BAND_B8_CODE))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                redBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_SENTINEL2_BAND_B4_CODE];
                nirBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_SENTINEL2_BAND_B8_CODE];
                validRasterType=true;
            }
            if(!validRasterType)
            {
                strError=QObject::tr("Algorithms::intercalibrationComputation");
                strError+=QObject::tr("\nInvalid raste type: %1\nfor raster file: %2")
                        .arg(rasterType).arg(rasterFile);
                return(false);
            }
            if(QFile::exists(redBandRasterFile))
            {
                QFileInfo redBandRasterFileInfo(redBandRasterFile);
                QDir redBandDir=redBandRasterFileInfo.absoluteDir();
                redBandDir.cdUp();
                workspaceBasePath=redBandDir.absolutePath();
                break;
            }
            else if(QFile::exists(nirBandRasterFile))
            {
                QFileInfo nirBandRasterFileInfo(nirBandRasterFile);
                QDir nirBandDir=nirBandRasterFileInfo.absoluteDir();
                nirBandDir.cdUp();
                workspaceBasePath=nirBandDir.absolutePath();
                break;
            }
        }
        if(!workspaceBasePath.isEmpty())
        {
            iterRasterFilesByQuadkey=rasterFilesByTuplekey.end();
        }
        else
        {
            iterRasterFilesByQuadkey++;
        }
    }

    // Obtengo los parametros de procesamiento
    int numberOfDates;
    int cloudValue;
    double maxPercentagePartiallyCloudy;
    QString interpolationMethod,strRemoveFullCloudy,strApplyCloudFreeImprovement;
    bool removeFullCloudy=false;
    bool applyCloudFreeImprovement=false;
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_NUMBER_OF_DATES)->getValue(numberOfDates);
    if(numberOfDates!=0)
    {
        strError=QObject::tr("Algorithms::cloudRemoval");
        strError+=QObject::tr("\nNot use all dates option is not implemented yet");
        return(false);
    }
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_MAX_PER_PART_CLOUDY)->getValue(maxPercentagePartiallyCloudy);
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_INTERPOLATION_METHOD)->getValue(interpolationMethod);
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_PIA_CLOUD_VALUE)->getValue(cloudValue);
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_REMOVE_FULL_CLOUDY)->getValue(strRemoveFullCloudy);
    if(mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_REMOVE_FULL_CLOUDY)->isEnabled())
    {
        if(strRemoveFullCloudy.compare("Yes",Qt::CaseInsensitive)==0)
        {
            removeFullCloudy=true;
        }
    }
    mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_APPLY_CLOUD_FREE_IMPROVEMENT)->getValue(strApplyCloudFreeImprovement);
    if(mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_APPLY_CLOUD_FREE_IMPROVEMENT)->isEnabled())
    {
        if(strApplyCloudFreeImprovement.compare("Yes",Qt::CaseInsensitive)==0)
        {
            applyCloudFreeImprovement=true;
        }
    }
    QMap<QString,QMap<QString,QVector<QString> > > bandsCombinationsByRasterType;
    if(mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS)->isEnabled())
    {
        QString strBandsCombinations;
        mPtrParametersManager->getParameter(ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS)->getValue(strBandsCombinations);
        QStringList bandsCombinationsList=strBandsCombinations.split(ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS_STRING_SEPARATOR);
        for(int i=0;i<bandsCombinationsList.size();i++)
        {
            QStringList bands=bandsCombinationsList.at(i).split(ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS_BANDS_STRING_SEPARATOR);
            if(bands.size()!=4)
            {
                strError=QObject::tr("Algorithms::cloudRemoval");
                strError+=QObject::tr("\nThere are not four values in combination: %1").arg(bandsCombinationsList.at(i));
                strError+=QObject::tr("\nFirst parameter is the spacecraft id and the other parameters are the three bands");
                return(false);
            }
            QString spacecraftId=bands.at(0).trimmed().toUpper();
            if(spacecraftId.compare(REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID,Qt::CaseInsensitive)!=0
                &&spacecraftId.compare(REMOTESENSING_SENTINEL2_SPACECRAFT_ID,Qt::CaseInsensitive)!=0)
            {
                strError=QObject::tr("Algorithms::cloudRemoval");
                strError+=QObject::tr("\nInvalid spacecraft id %1 in combination: %2").arg(spacecraftId).arg(bandsCombinationsList.at(i));
                return(false);
            }
            if(!bandsInAlgorithmBySpaceCraft.contains(spacecraftId))
            {
                strError=QObject::tr("Algorithms::cloudRemoval");
                strError+=QObject::tr("\nNot implemented spacecraft id %1 in combination: %2").arg(spacecraftId).arg(bandsCombinationsList.at(i));
                return(false);
            }
            QVector<QString> bandsCombination;
            for(int j=1;j<4;j++)
            {
                QString bandId=bands.at(j).trimmed().toUpper();
                if(!bandsInAlgorithmBySpaceCraft[spacecraftId].contains(bandId))
                {
                    strError=QObject::tr("Algorithms::cloudRemoval");
                    strError+=QObject::tr("\nInvalid band id %1 in combination: %2").arg(bandId).arg(bandsCombinationsList.at(i));
                    return(false);
                }
                if(bandsCombination.indexOf(bandId)>-1)
                {
                    strError=QObject::tr("Algorithms::cloudRemoval");
                    strError+=QObject::tr("\nRepeated band id %1 in combination: %2").arg(bandId).arg(bandsCombinationsList.at(i));
                    return(false);
                }
                bandsCombination.push_back(bandId);
            }
            if(spacecraftId.compare(REMOTESENSING_LANDSAT8_USGS_SPACECRAFT_ID,Qt::CaseInsensitive)==0)
            {
                spacecraftId=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8;
            }
            if(spacecraftId.compare(REMOTESENSING_SENTINEL2_SPACECRAFT_ID,Qt::CaseInsensitive)==0)
            {
                spacecraftId=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2;
            }
            QString bandsCombinationId=spacecraftId+"_";
            for(int nb=0;nb<bandsCombination.size();nb++)
            {
                bandsCombinationId+=bandsCombination.at(nb);
            }
            if(bandsCombinationsByRasterType.contains(spacecraftId))
            {
                if(bandsCombinationsByRasterType[spacecraftId].contains(bandsCombinationId))
                {
                    strError=QObject::tr("Algorithms::cloudRemoval");
                    strError+=QObject::tr("\nRepeat bands combinations: %1").arg(bandsCombinationsList.at(i));
                    return(false);
                }
            }
            bandsCombinationsByRasterType[spacecraftId][bandsCombinationId]=bandsCombination;
        }
    }

    // Fichero de resultados: cabecera
    if (!resultsFile.open(QFile::Append |QFile::Text))
    {
        strError=QObject::tr("Algorithms::cloudRemoval");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(resultsFile.fileName());
        return(false);
    }
    QTextStream out(&resultsFile);
    out<<"- Parametros de procesamiento:\n";
    out<<"  - Escena de referencia de intercalibracion: "<<intercalibrationReferenceImageRasterId<<"\n";
    out<<"  - Maximo % para parcialmente nuboso ......: "<<QString::number(maxPercentagePartiallyCloudy,'f',1)<<"\n";
    out<<"  - Numero de fechas para interpolacion ....: "<<QString::number(numberOfDates);
    if(numberOfDates==0)
    {
        out<<", todas";
    }
    out<<"\n";
    out<<"  - Metodo de interpolacion ................: "<<interpolationMethod<<"\n";
    out<<"  - Numero de tuplekeys a procesar .........: "<<rasterFilesByTuplekey.size()<<"\n";
    out<<"  - Eliminar en escenas totalmente nubosas .: ";
    if(removeFullCloudy)
    {
        out<<"Si";
    }
    else
    {
        out<<"No";
    }
    out<<"\n";
    out<<"  - Aplicar mejora por valores sin nubes ...: ";
    if(applyCloudFreeImprovement)
    {
        out<<"Si";
    }
    else
    {
        out<<"No";
    }
    out<<"\n";

//    QString title=QObject::tr("Removing clouds");
//    QString msgGlobal=QObject::tr("Processing %1 tuplekeys ...").arg(QString::number(rasterFilesByTuplekey.size()));
//    QProgressDialog progress(title, QObject::tr("Abort"),0,rasterFilesByTuplekey.size(), mPtrWidgetParent);
//    progress.setWindowModality(Qt::WindowModal);
//    progress.setLabelText(msgGlobal);
//    progress.show();
//    qApp->processEvents();

//    int yo=3/2; // 1
//    yo=1/2; // 0
//    yo=4/2; // 2

    QMap<QString, QMap<QString, QMap<QString, QString> > >::const_iterator iterTuplekey =tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.begin();
    int numberOfProcessedTuplekeys=0;

    QMap<QString,QMap<QString,QMap<QString,double> > > gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand;
    QMap<QString,QMap<QString,QMap<QString,double> > > offsetIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand;
    if(applyCloudFreeImprovement)
    {
        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > syntheticMeanValueByRasterTypeByRasterFileByBandByTuplekey;
        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > syntheticStdValueByRasterTypeByRasterFileByBandByTuplekey;
        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > meanValueByRasterTypeByRasterFileByBandByTuplekey;
        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > stdValueByRasterTypeByRasterFileByBandByTuplekey;
        QMap<QString,QMap<QString,QMap<QString,QMap<QString,int> > > > numberOfValueByRasterTypeByRasterFileByBandByTuplekey;
        out<<"- Intercalibracion para mejora de eliminacion de nubes a partir de valores libres de nubes:"<<"\n";
        while(iterTuplekey!=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.end())
        {
            numberOfProcessedTuplekeys++;
            QString tuplekey=iterTuplekey.key();
            QMap<QString, QMap<QString, QMap<QString, QString> > > tuplekeysRasterFilesByRasterTypeByBandByRasterFile;
    //        QMap<QString,QVector<QVector<bool> > > maskDataByRasterFile;
            QMap<QString,QMap<int,QVector<int> > > maskDataByRasterFile;
            QMap<QString,QVector<double> > maskGeorefByRasterFile;
            QMap<QString,bool> partiallyCloudyByRasterFile; // si no está no tiene nubes
            QMap<QString,float> cloudyPercentageByRasterFile; // si no está no tiene nubes
            QMap<QString, QMap<QString, QString> >::const_iterator iterRasterFile=iterTuplekey.value().begin();
    //        QMap<QString,QMap<QString,QVector<QString> > > bandsCombinationsByRasterType;
            QMap<QString,QMap<QString,QMap<QString,QString> > > bandsCombinationsFileBaseNameByRasterTypeByRasterFile;
            QString tuplekeyPath;
            while(iterRasterFile!=iterTuplekey.value().end())
            {
                QString rasterFile=iterRasterFile.key();
                QString rasterType=rasterTypesByRasterFile[rasterFile];
                if(bandsCombinationsByRasterType.contains(rasterType))
                {
                    QMap<QString,QVector<QString> >::const_iterator iterBandsCombinations=bandsCombinationsByRasterType[rasterType].begin();
                    while(iterBandsCombinations!=bandsCombinationsByRasterType[rasterType].end())
                    {
                        QString subFolder;
                        QString fileBaseName=rasterFile+"_";
                        for(int nb=0;nb<iterBandsCombinations.value().size();nb++)
                        {
                            fileBaseName+=iterBandsCombinations.value().at(nb);
                            subFolder+=iterBandsCombinations.value().at(nb);
                        }
                        fileBaseName+=".";
                        fileBaseName+=RASTER_JPEG_FILE_EXTENSION;
                        QString fileBaseNameAndSubFolder=subFolder+"/"+fileBaseName;
    //                    fileBaseName+=RASTER_PNG_FILE_EXTENSION;
                        bandsCombinationsFileBaseNameByRasterTypeByRasterFile[rasterType][rasterFile][iterBandsCombinations.key()]=fileBaseNameAndSubFolder;
                        iterBandsCombinations++;
                    }
                }
                QString maskBandCode;
                if(rasterType.compare(landsat8IdDb)==0)
                {
                    maskBandCode=REMOTESENSING_LANDSAT8_BAND_B0_CODE;
                }
                else if(rasterType.compare(sentinel2IdDb)==0)
                {
                    maskBandCode=REMOTESENSING_SENTINEL2_BAND_B0_CODE;
                }
                QMap<QString, QString>::const_iterator iterBand=iterRasterFile.value().begin();
                while(iterBand!=iterRasterFile.value().end())
                {
                    QString bandId=iterBand.key();
                    QString tuplekyeRasterFile=iterBand.value();
                    if(!cloudyPercentageByRasterFile.contains(rasterFile))
                    {
                        QFileInfo tuplekeyRasterFileInfo(tuplekyeRasterFile);
                        QString maskBandFileName=tuplekeyRasterFileInfo.absolutePath()+"//"+rasterFile+"_"+maskBandCode+".tif";
                        if(QFile::exists(maskBandFileName))
                        {
                            QVector<double> georef;
                            QMap<int,QVector<int> > values;
                            float cloudValuesPercentage;
                            if(!readMaskRasterFile(maskBandFileName,cloudValue,georef,values,cloudValuesPercentage,strAuxError))
                            {
                                strError=QObject::tr("Algorithms::cloudRemoval");
                                strError+=QObject::tr("\nError reading mask raster file:\n%1\nError:\n%2")
                                        .arg(tuplekyeRasterFile).arg(strAuxError);
                                resultsFile.close();
                                return(false);
                            }
    //                        if(rasterFile.compare("LC82000322015101LGN00",Qt::CaseInsensitive)==0)
    //                        {
    //                            QMap<int,QVector<int> >::const_iterator kk1=values.begin();
    //                            while(kk1!=values.end())
    //                            {
    //                                int rowKk=kk1.key();
    //                                for(int kk2=0;kk2<kk1.value().size();kk2++)
    //                                {
    //                                    int columnKk=kk1.value()[kk2];
    //                                    int yo=1;
    //                                }
    //                                kk1++;
    //                            }
    //                        }
                            if(cloudValuesPercentage>0)
                            {
                                maskDataByRasterFile[rasterFile]=values;
                                maskGeorefByRasterFile[rasterFile]=georef;
                                partiallyCloudyByRasterFile[rasterFile]=true;
                                if(cloudValuesPercentage>=maxPercentagePartiallyCloudy)
                                {
                                    partiallyCloudyByRasterFile[rasterFile]=false;
                                }
                            }
                            cloudyPercentageByRasterFile[rasterFile]=cloudValuesPercentage;
                        }
                    }
                    tuplekeysRasterFilesByRasterTypeByBandByRasterFile[rasterType][bandId][rasterFile]=tuplekyeRasterFile;
                    iterBand++;
                }
                iterRasterFile++;
            }
//            out<<"- Procesamiento de tuplekey ................: "<<tuplekey<<"\n";
//            out<<"  - Numero de tipos de escenas a procesar ..: "<<tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()<<"\n";
            QMap<QString, QMap<QString, QMap<QString, QString> > >::const_iterator iterRasterType=tuplekeysRasterFilesByRasterTypeByBandByRasterFile.begin();
            int numberOfProcessedRasterTypes=0;
            while(iterRasterType!=tuplekeysRasterFilesByRasterTypeByBandByRasterFile.end())
            {
                numberOfProcessedRasterTypes++;
                QString rasterType=iterRasterType.key();
//                out<<"  - Tipo de escenas ........................: "<<rasterType<<"\n";
                QMap<QString, QMap<QString, QString> > tuplekeysRasterFilesByBandByRasterFile=iterRasterType.value();
//                out<<"    - Numero de bandas a procesar ..........: "<<tuplekeysRasterFilesByBandByRasterFile.size()<<"\n";
                QMap<QString, QMap<QString, QString> >::const_iterator iterBand=tuplekeysRasterFilesByBandByRasterFile.begin();
                int numberOfProcessedBands=0;
                int numberOfScenes=0;
                while(iterBand!=tuplekeysRasterFilesByBandByRasterFile.end())
                {
                    numberOfProcessedBands++;
                    QString bandId=iterBand.key();
                    QMap<QString,QVector<QVector<double> > > bandDataByRasterFile;
                    QMap<QString,QVector<QVector<double> > > bandSyntheticDataByRasterFile;
                    QMap<QString,QMap<int,QVector<int> > > bandMaskDataByRasterFile;
                    QMap<QString,QVector<double> > bandGeorefByRasterFile;
                    QMap<QString,double> bandNoDataValueByRasterFile;
                    QMap<int,QVector<int> > pixelsToCompute;
                    QVector<QString> computedRasterFiles; // solo hay que salvar estos
                    QMap<QString,double> factorTo8BitsByRasterFile;
                    QMap<QString, QString> tuplekeysRasterFilesByRasterFile=iterBand.value();
//                    out<<"    - Banda ................................: "<<bandId<<"\n";
//                    out<<"      - Numero de escenas ..................: "<<tuplekeysRasterFilesByRasterFile.size()<<"\n";
                    numberOfScenes=tuplekeysRasterFilesByRasterFile.size();
                    QMap<QString, QString>::const_iterator iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                    QString title=QObject::tr("Computing cloud free improvement - Reading files");
                    QString msgGlobal;
                    msgGlobal=title+"\n\n";
                    msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
                    msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                            .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
                    msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
                    msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                            .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
                    msgGlobal+=QObject::tr("\n  Number of bands to process .......: %1").arg(QString::number(tuplekeysRasterFilesByBandByRasterFile.size()));
                    msgGlobal+=QObject::tr("\n  ... Processing band number .......: %1, %2")
                            .arg(QString::number(numberOfProcessedBands)).arg(bandId);
                    msgGlobal+=QObject::tr("\n  Number of files to read ..........: %1").arg(QString::number(tuplekeysRasterFilesByRasterFile.size()));
                    QProgressDialog readingFilesProgress(title, QObject::tr("Abort"),0,tuplekeysRasterFilesByRasterFile.size(), mPtrWidgetParent);
                    readingFilesProgress.setWindowModality(Qt::WindowModal);
                    readingFilesProgress.setLabelText(msgGlobal);
                    readingFilesProgress.setWindowTitle(title);
                    readingFilesProgress.show();
                    readingFilesProgress.adjustSize();
                    qApp->processEvents();
                    int numberOfReadedFiles=0;
                    int numberOfColumns=-1;
                    int numberOfRows=-1;
                    while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                    {
                        numberOfReadedFiles++;
                        readingFilesProgress.setValue(numberOfReadedFiles);
                        if(readingFilesProgress.wasCanceled())
                        {
                            QMessageBox msgBox(mPtrWidgetParent);
                            QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Reading files");
                            msgBox.setText(msg);
                            QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                            msgBox.setInformativeText(question);
                            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                            msgBox.setDefaultButton(QMessageBox::Yes);
                            int ret = msgBox.exec();
                            bool abort=false;
                            switch (ret)
                            {
                              case QMessageBox::Yes:
                                abort=true;
                                  // Save was clicked
                                  break;
                              case QMessageBox::No:
                                  break;
                              case QMessageBox::Cancel:
                                  break;
                              default:
                                  // should never be reached
                                  break;
                            }
                            if(abort)
                            {
                                strError=QObject::tr("Process was canceled");
                                resultsFile.close();
                                return(false);
                            }
                        }
                        QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                        if(tuplekeyPath.isEmpty())
                        {
                            QFileInfo tuplekeyRasterFileFileInfo(tuplekeysRasterFile);
                            tuplekeyPath=tuplekeyRasterFileFileInfo.absolutePath();
                        }
                        QString rasterFile=iterTuplekeyRasterFile.key();
                        factorTo8BitsByRasterFile[rasterFile]=1.0;
                        float cloudyPercentage=0.0;
                        if(cloudyPercentageByRasterFile.contains(rasterFile))
                        {
                            cloudyPercentage=cloudyPercentageByRasterFile[rasterFile];
                        }
//                        out<<"      - Escena a procesar ..................: "<<rasterFile;
//                        out<<" ("<<QString::number(cloudyPercentage,'f',2).rightJustified(6)<<" % nubes -> ";
//                        if(cloudyPercentage==0)
//                        {
//                            out<<"sin nubes";
//                        }
//                        else if(partiallyCloudyByRasterFile[rasterFile])
//                        {
//                            out<<"parcialmente nuboso";
//                        }
//                        else
//                        {
//                            out<<"totalmente nuboso";
//                        }
//                        out<<")\n";
                        IGDAL::Raster* ptrRasterFile=new IGDAL::Raster(mPtrCrsTools);
                        if(!ptrRasterFile->setFromFile(tuplekeysRasterFile,strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                                    .arg(tuplekeysRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
                        int columns,rows;
                        if(!ptrRasterFile->getSize(columns,rows,strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                                    .arg(tuplekeysRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
                        if(numberOfColumns==-1)
                        {
                            numberOfColumns=columns;
                        }
                        if(numberOfRows==-1)
                        {
                            numberOfRows=rows;
                        }
                        double dlNoDataValue;
                        int numberOfBand=0;
                        if(!ptrRasterFile->getNoDataValue(numberOfBand,dlNoDataValue,strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                                    .arg(tuplekeysRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
                        QVector<double> georef;
                        if(!ptrRasterFile->getGeoRef(georef,strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError reading georef in raster file:\n%1\nError:\n%2")
                                    .arg(tuplekeysRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
                        QVector<QVector<double> > values;
                        QVector<QVector<double> > syntheticValues(rows);
                        int initialColumn=0;
                        int initialRow=0;
                        if(!ptrRasterFile->readValues(numberOfBand, // desde 0
                                                      initialColumn,
                                                      initialRow,
                                                      columns,
                                                      rows,
                                                      values,
                                                      strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                                    .arg(tuplekeysRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
                        // Conversión de los valores
                        double gain=intercalibrationGainByRasterFileAndByBand[rasterFile][bandId];
                        double offset=intercalibrationOffsetByRasterFileAndByBand[rasterFile][bandId];
                        bool intercalibrationTo8Bits=intercalibrationTo8BitsByRasterFileAndByBand[rasterFile][bandId];
                        bool intercalibrationToReflectance=intercalibrationToReflectanceByRasterFileAndByBand[rasterFile][bandId];
                        double reflectanceAddValue,reflectanceMultValue;
                        if(intercalibrationToReflectance)
                        {
                            reflectanceAddValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId];
                            reflectanceMultValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][bandId];
                        }
                        bool intercalibrationByInterpolation=intercalibrationInterpolatedByRasterFileAndByBand[rasterFile][bandId];
                        double factorTo8Bits=1.0;
                        if(intercalibrationTo8Bits)
                        {
                            if(!ptrRasterFile->getFactorTo8Bits(factorTo8Bits,strAuxError))
                            {
                                strError=QObject::tr("Algorithms::cloudRemoval");
                                strError+=QObject::tr("\nError getting factor to 8 bits for raster file:\n%1\nError:\n%2")
                                        .arg(tuplekeysRasterFile).arg(strAuxError);
                                resultsFile.close();
                                return(false);
                            }
                            factorTo8BitsByRasterFile[rasterFile]=factorTo8Bits;
                        }
                        delete(ptrRasterFile);
                        for(int row=0;row<rows;row++)
                        {
                            syntheticValues[row].resize(columns);
                            for(int column=0;column<columns;column++)
                            {
                                double value=values[row][column];
                                if(fabs(value-dlNoDataValue)>0.1)
                                {
                                    if(intercalibrationToReflectance)
                                    {
                                        value=value*reflectanceMultValue+reflectanceAddValue;
                                    }
                                    if(intercalibrationTo8Bits)
                                    {
                                        value=value*factorTo8Bits;
                                    }
                                    value=value*gain+offset;
                                }
                                values[row][column]=value;
                                syntheticValues[row][column]=dlNoDataValue;
                            }
                        }
                        bandDataByRasterFile[rasterFile]=values;
                        bandSyntheticDataByRasterFile[rasterFile]=syntheticValues;
                        bandGeorefByRasterFile[rasterFile]=georef;
                        bandNoDataValueByRasterFile[rasterFile]=dlNoDataValue;
                        QMap<int,QVector<int> > maskData;
                        if(cloudyPercentageByRasterFile[rasterFile]>0.0)
                        {
                            QMap<int,QVector<int> > maskDataRasterFile=maskDataByRasterFile[rasterFile];
    //                        if(rasterFile.compare("LC82000322015101LGN00",Qt::CaseInsensitive)==0)
    //                        {
    //                            QMap<int,QVector<int> >::const_iterator kk1=maskDataRasterFile.begin();
    //                            while(kk1!=maskDataRasterFile.end())
    //                            {
    //                                int rowKk=kk1.key();
    //                                for(int kk2=0;kk2<kk1.value().size();kk2++)
    //                                {
    //                                    int columnKk=kk1.value()[kk2];
    //                                    int yo=1;
    //                                }
    //                                kk1++;
    //                            }
    //                        }
                            QVector<double> maskGeorefRasterFile=maskGeorefByRasterFile[rasterFile];
                            double maskColumnGsdRasterFile=maskGeorefRasterFile[1];
                            double maskColumnGsd=georef[1];
                            if(fabs(maskColumnGsdRasterFile-maskColumnGsd)<0.01)
                            {
                                maskData=maskDataRasterFile;
                            }
                            else
                            {
                                if(maskColumnGsdRasterFile>maskColumnGsd)
                                {
                                    int factorGsd=qRound(maskColumnGsdRasterFile/maskColumnGsd);
                                    if(fabs(factorGsd-maskColumnGsdRasterFile/maskColumnGsd)>0.01)
                                    {
                                        strError=QObject::tr("Algorithms::cloudRemoval");
                                        strError+=QObject::tr("\nError getting GSD factor for raster file:\n%1\nError:\n%2")
                                                .arg(tuplekeysRasterFile).arg(strAuxError);
                                        resultsFile.close();
                                        return(false);
                                    }
                                    QMap<int,QVector<int> >::const_iterator iterMaskDataRasterFile=maskDataRasterFile.begin();
                                    while(iterMaskDataRasterFile!=maskDataRasterFile.end())
                                    {
                                        int rowMaskRasterFile=iterMaskDataRasterFile.key();
                                        int firstRowMask=rowMaskRasterFile*factorGsd;
                                        for(int i=0;i<iterMaskDataRasterFile.value().size();i++)
                                        {
                                            int columnMaskRasterFile=iterMaskDataRasterFile.value()[i];
                                            int firstColumnMask=columnMaskRasterFile*factorGsd;
                                            for(int r=0;r<factorGsd;r++)
                                            {
                                                int rowMask=firstRowMask+r;
                                                if(!maskData.contains(rowMask))
                                                {
                                                    QVector<int> aux;
                                                    maskData[rowMask]=aux;
                                                }
                                                for(int c=0;c<factorGsd;c++)
                                                {
                                                    int columnMask=firstColumnMask+c;
                                                    // Si hay un noDataValue no lo añado como pixel de nube ya que no hay que obtenerlo
                                                    if(fabs(bandDataByRasterFile[rasterFile][rowMask][columnMask]-bandNoDataValueByRasterFile[rasterFile])>0.01)
                                                    {
                                                        maskData[rowMask].push_back(columnMask);
                                                    }
                                                }
                                            }
                                        }
                                        iterMaskDataRasterFile++;
                                    }
                                }
                                else
                                {
                                    int factorGsd=qRound(maskColumnGsd/maskColumnGsdRasterFile);
                                    if(fabs(factorGsd-maskColumnGsd/maskColumnGsdRasterFile)>0.01)
                                    {
                                        strError=QObject::tr("Algorithms::cloudRemoval");
                                        strError+=QObject::tr("\nError getting GSD factor for raster file:\n%1\nError:\n%2")
                                                .arg(tuplekeysRasterFile).arg(strAuxError);
                                        resultsFile.close();
                                        return(false);
                                    }
                                    QMap<int,QVector<int> >::const_iterator iterMaskDataRasterFile=maskDataRasterFile.begin();
                                    while(iterMaskDataRasterFile!=maskDataRasterFile.end())
                                    {
                                        int rowMaskRasterFile=iterMaskDataRasterFile.key();
                                        int rowMask=rowMaskRasterFile/factorGsd; // 3/2 = 1
                                        for(int i=0;i<iterMaskDataRasterFile.value().size();i++)
                                        {
                                            int columnMaskRasterFile=iterMaskDataRasterFile.value()[i];
                                            int columnMask=columnMaskRasterFile/factorGsd;
                                            if(!maskData.contains(rowMask))
                                            {
                                                QVector<int> aux;
                                                maskData[rowMask]=aux;
                                            }
                                            // Si hay un noDataValue no lo añado como pixel de nube ya que no hay que obtenerlo
                                            if(fabs(bandDataByRasterFile[rasterFile][rowMask][columnMask]-bandNoDataValueByRasterFile[rasterFile])>0.01)
                                            {
                                                if(!maskData[rowMask].contains(columnMask))
                                                {
                                                    maskData[rowMask].push_back(columnMask);
                                                }
                                            }
                                        }
                                        iterMaskDataRasterFile++;
                                    }
                                }
                            }
                        }
                        bandMaskDataByRasterFile[rasterFile]=maskData;
                        iterTuplekeyRasterFile++;
                    }
                    readingFilesProgress.setValue(tuplekeysRasterFilesByRasterFile.size());
                    readingFilesProgress.close();

//                    out<<"      - Numero de pixeles a procesar .......: "<<QString::number(numberOfPixelsToCompute);
//                    out<<", columna mayor "<<QString::number(maxColumn)<<" para la fila "<<QString::number(rowMaxColumn);
//                    out<<"\n";
                    int numberOfPixelsToCompute=numberOfRows*numberOfColumns;
                    title=QObject::tr("Computing cloud free improvement - Processing pixels");
                    msgGlobal=title+"\n\n";
                    msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
                    msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                            .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
                    msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
                    msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                            .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
                    msgGlobal+=QObject::tr("\n  Number of bands to process .......: %1").arg(QString::number(tuplekeysRasterFilesByBandByRasterFile.size()));
                    msgGlobal+=QObject::tr("\n  ... Processing band number .......: %1, %2")
                            .arg(QString::number(numberOfProcessedBands)).arg(bandId);
                    msgGlobal+=QObject::tr("\n  Number of pixels to process ......: %1").arg(QString::number(numberOfPixelsToCompute));
                    int numberOfProcessedPixelsSteps=1000;
                    int numberOfPixelsByStep=numberOfPixelsToCompute/numberOfProcessedPixelsSteps;
                    QProgressDialog processingPixelsProgress(title, QObject::tr("Abort"),0,numberOfProcessedPixelsSteps, mPtrWidgetParent);
                    processingPixelsProgress.setWindowModality(Qt::WindowModal);
                    processingPixelsProgress.setLabelText(msgGlobal);
    //                processingPixelsProgress.setWindowTitle(title);
                    processingPixelsProgress.show();
    //                processingPixelsProgress.adjustSize();
                    qApp->processEvents();
//                    if(printDetail)
//                    {
//                        out<<"      - Resultados para el primer y ultimo pixel:\n";
//                        out<<"     Row  Column                        Escena        ND    Nube   %Nube        Jd   ND_Intc  Dato  Calc  Interpol Sintetico     Error\n";
//                    }
                    int computedPixels=-1;
                    int computedPixelsInStep=0;
                    int numberOfStep=0;
                    for(int row=0;row<numberOfRows;row++)
                    {
                        for(int column=0;column<numberOfColumns;column++)
                        {
//                            out<<"(row,column)=("<<QString::number(row)<<","<<QString::number(column)<<")"<<"\n";
//                            out.flush();
                            computedPixels++;
                            computedPixelsInStep++;
                            if(computedPixelsInStep==numberOfPixelsByStep)
                            {
                                numberOfStep++;
                                processingPixelsProgress.setValue(numberOfStep);
                                computedPixelsInStep=0;
                                if (processingPixelsProgress.wasCanceled())
                                {
                                    QMessageBox msgBox(mPtrWidgetParent);
                                    QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Processing pixels");
                                    msgBox.setText(msg);
                                    QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                                    msgBox.setInformativeText(question);
                                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                                    msgBox.setDefaultButton(QMessageBox::Yes);
                                    int ret = msgBox.exec();
                                    bool abort=false;
                                    switch (ret)
                                    {
                                      case QMessageBox::Yes:
                                        abort=true;
                                          // Save was clicked
                                          break;
                                      case QMessageBox::No:
                                          break;
                                      case QMessageBox::Cancel:
                                          break;
                                      default:
                                          // should never be reached
                                          break;
                                    }
                                    if(abort)
                                    {
                                        strError=QObject::tr("Process was canceled");
                                        resultsFile.close();
                                        return(false);
                                    }
                                }
                            }
                            int pixelDataMinJd=100000000;
                            int pixelDataMaxJd=-100000000;
                            QMap<int,double> pixelDataByJd; // ahora todos los píxeles que no tengan nube serán dato y serán interpolados
                            QMap<int,QString> rasterFileDataByJd;
                            QMap<QString, QString>::const_iterator iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                            while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                            {
    //                            QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                                QString rasterFile=iterTuplekeyRasterFile.key();
                                int jd=jdByRasterFile[rasterFile];
                                double pixelData=bandDataByRasterFile[rasterFile][row][column];
                                bool pixelValidData=false;
                                if(fabs(bandDataByRasterFile[rasterFile][row][column]-bandNoDataValueByRasterFile[rasterFile])<0.01)
                                {
                                    iterTuplekeyRasterFile++;
                                    continue;
                                }
                                if(bandMaskDataByRasterFile[rasterFile].contains(row))
                                {
                                    if(bandMaskDataByRasterFile[rasterFile][row].contains(column))
                                    {
                                        iterTuplekeyRasterFile++;
                                        continue;
                                    }
                                }
                                if(!partiallyCloudyByRasterFile[rasterFile])
                                {
                                    iterTuplekeyRasterFile++;
                                    continue;
                                }
                                // ni tiene nodata, ni hay nube, ni pertenece a una escena libre de nubes
                                // esto último no lo tengo claro porque es a nivel de tuplekey y no de escena completa
                                pixelDataByJd[jd]=pixelData;
                                rasterFileDataByJd[jd]=rasterFile;
                                if(jd<pixelDataMinJd)
                                {
                                    pixelDataMinJd=jdByRasterFile[rasterFile];
                                }
                                if(jd>pixelDataMaxJd)
                                {
                                    pixelDataMaxJd=jdByRasterFile[rasterFile];
                                }
                                iterTuplekeyRasterFile++;
                            }
                            if(pixelDataByJd.size()==0)
                            {
                                continue;
                            }
                            int firstDataJd=pixelDataMinJd;
                            int lastDataJd=pixelDataMaxJd;
//                            if(numberOfDates>0)
//                            {
//                                firstDataJd=pixelToInterpolateMinJd-numberOfDates;
//                                if(firstDataJd<pixelDataMinJd)
//                                {
//                                    firstDataJd=pixelDataMinJd;
//                                }
//                                lastDataJd=pixelToInterpolateMaxJd+numberOfDates;
//                                if(lastDataJd>pixelDataMaxJd)
//                                {
//                                    lastDataJd=pixelDataMaxJd;
//                                }
//                            }
                            int contData=0;
                            QMap<int,double>::const_iterator iterPixelDataByJd=pixelDataByJd.begin();
                            QVector<int> pixelDataJds; // va a estar ordenado porque lo creo recorriendo un map que lo esta
                            while(iterPixelDataByJd!=pixelDataByJd.end())
                            {
                                int jd=iterPixelDataByJd.key();
                                if(jd<firstDataJd||jd>lastDataJd)
                                {
                                    iterPixelDataByJd++;
                                    continue;
                                }
                                contData++;
                                pixelDataJds.push_back(jd);
                                iterPixelDataByJd++;
                            }
//                            QMap<int,double> toPrintInterpolatedValues;
                            QMap<int,double> sinteticValues;
                            if(contData==0)
                            {
                                continue;
                            }
                            else if(contData==1)
                            {
                                sinteticValues[pixelDataJds[0]]=pixelDataByJd[pixelDataJds[0]];
                                QString rasterFile=rasterFileDataByJd[pixelDataJds[0]];
                                bandSyntheticDataByRasterFile[rasterFile][row][column]=sinteticValues[pixelDataJds[0]];
                            }
                            else if(contData==2)
                            {
                                sinteticValues[pixelDataJds[0]]=pixelDataByJd[pixelDataJds[1]];
                                QString rasterFile=rasterFileDataByJd[pixelDataJds[0]];
                                bandSyntheticDataByRasterFile[rasterFile][row][column]=sinteticValues[pixelDataJds[0]];
                                sinteticValues[pixelDataJds[1]]=pixelDataByJd[pixelDataJds[0]];
                                rasterFile=rasterFileDataByJd[pixelDataJds[1]];
                                bandSyntheticDataByRasterFile[rasterFile][row][column]=sinteticValues[pixelDataJds[1]];
                            }
                            else
                            {
                                // Calculo de datos sintéticos
                                iterPixelDataByJd=pixelDataByJd.begin();
                                while(iterPixelDataByJd!=pixelDataByJd.end())
                                {
                                    int jd=iterPixelDataByJd.key();
                                    int posJdInPixelDataJds=pixelDataJds.indexOf(jd);
                                    if(jd<firstDataJd||jd>lastDataJd)
                                    {
                                        if(jd<firstDataJd)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[firstDataJd];
                                        }
                                        if(jd>lastDataJd)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[lastDataJd];
                                        }
                                        iterPixelDataByJd++;
                                        continue;
                                    }
                                    else if(jd==firstDataJd)
                                    {
                                        if((posJdInPixelDataJds+1)<pixelDataJds.size())
                                        {
                                            sinteticValues[jd]=pixelDataByJd[pixelDataJds[posJdInPixelDataJds+1]];
                                        }
                                        else
                                        {
                                            sinteticValues[jd]=pixelDataByJd[jd];
                                        }
                                    }
                                    else if(jd==lastDataJd)
                                    {
                                        if(posJdInPixelDataJds>0)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[pixelDataJds[posJdInPixelDataJds-1]];
                                        }
                                        else
                                        {
                                            sinteticValues[jd]=pixelDataByJd[jd];
                                        }
                                    }
                                    else // en este caso hay que interpolar
                                    {
                                        double* xSintetic;
                                        double* ySintetic;
                                        int contDataSintetic=contData-1;
                                        xSintetic=new double[contDataSintetic];
                                        ySintetic=new double[contDataSintetic];
                                        contDataSintetic=0;
                                        QMap<int,double>::const_iterator iterPixelDataByJdSintetic=pixelDataByJd.begin();
                                        while(iterPixelDataByJdSintetic!=pixelDataByJd.end())
                                        {
                                            int jdAux=iterPixelDataByJdSintetic.key();
                                            if(jdAux==jd)
                                            {
                                                iterPixelDataByJdSintetic++;
                                                continue;
                                            }
                                            if(jd<firstDataJd||jd>lastDataJd)
                                            {
                                                iterPixelDataByJdSintetic++;
                                                continue;
                                            }
                                            xSintetic[contDataSintetic]=jdAux-firstDataJd;
                                            ySintetic[contDataSintetic]=iterPixelDataByJdSintetic.value();
                                            contDataSintetic++;
                                            iterPixelDataByJdSintetic++;
                                        }
                                        double xJdSintetic=jd-firstDataJd;
                                        if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::AkimaSpline* ptrAkimaSplineSintetic=NULL;
                                            ptrAkimaSplineSintetic=new Splines::AkimaSpline();
                                            ptrAkimaSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrAkimaSplineSintetic)(xJdSintetic);
                                            delete(ptrAkimaSplineSintetic);
                                        }
                                        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::CubicSpline* ptrCubicSplineSintetic=NULL;
                                            ptrCubicSplineSintetic=new Splines::CubicSpline();
                                            ptrCubicSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrCubicSplineSintetic)(xJdSintetic);
                                            delete(ptrCubicSplineSintetic);
                                        }
                                        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::BesselSpline* ptrBesselSplineSintetic=NULL;
                                            ptrBesselSplineSintetic=new Splines::BesselSpline();
                                            ptrBesselSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrBesselSplineSintetic)(xJdSintetic);
                                            delete(ptrBesselSplineSintetic);
                                        }
                                        delete(xSintetic);
                                        delete(ySintetic);
                                    }
                                    QString rasterFile=rasterFileDataByJd[jd];
                                    bandSyntheticDataByRasterFile[rasterFile][row][column]=sinteticValues[jd];
                                    iterPixelDataByJd++;
                                }
                            }
                        }
                    }
                    processingPixelsProgress.setValue(numberOfProcessedPixelsSteps);
                    processingPixelsProgress.close();
//                    QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > syntheticMeanValueByRasterTypeByRasterFileByBandByTuplekey;
//                    QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > syntheticStdValueByRasterTypeByRasterFileByBandByTuplekey;
//                    QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > meanValueByRasterTypeByRasterFileByBandByTuplekey;
//                    QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > stdValueByRasterTypeByRasterFileByBandByTuplekey;
//                    QMap<QString,QMap<QString,QMap<QString,QMap<QString,int> > > > numberOfValueByRasterTypeByRasterFileByBandByTuplekey;
                    iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                    while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                    {
                        QString rasterFile=iterTuplekeyRasterFile.key();
                        double syntheticMeanValue=0.0;
                        double meanValue=0.0;
                        double syntheticStdValue=0.0;
                        double stdValue=0.0;
                        int numberOfValues=0;

                        QVector<QVector<double> > values=bandDataByRasterFile[rasterFile];
                        QVector<QVector<double> > syntheticValues=bandSyntheticDataByRasterFile[rasterFile];
                        double dblNoDataValue=bandNoDataValueByRasterFile[rasterFile];
                        for(int row=0;row<numberOfRows;row++)
                        {
                            for(int column=0;column<numberOfColumns;column++)
                            {
                                if(fabs(syntheticValues[row][column]-dblNoDataValue)>0.1)
                                {
                                    numberOfValues++;
                                    meanValue+=values[row][column];
                                    syntheticMeanValue+=syntheticValues[row][column];
                                }
                            }
                        }
                        if(numberOfValues==0)
                        {
                            iterTuplekeyRasterFile++;
                            continue;
                        }
                        meanValue=meanValue/numberOfValues;
                        syntheticMeanValue=syntheticMeanValue/numberOfValues;
                        if(numberOfValues>1)
                        {
                            for(int row=0;row<numberOfRows;row++)
                            {
                                for(int column=0;column<numberOfColumns;column++)
                                {
                                    if(fabs(syntheticValues[row][column]-dblNoDataValue)>0.1)
                                    {
                                        stdValue+=pow(meanValue-values[row][column],2.0);
                                        syntheticStdValue+=pow(syntheticValues[row][column]-syntheticMeanValue,2.0);
                                    }
                                }
                            }
                            stdValue=sqrt(stdValue/(numberOfValues-1));
                            syntheticStdValue=sqrt(syntheticStdValue/(numberOfValues-1));
                        }
                        else
                        {
                            syntheticStdValue=syntheticMeanValue; // muy poco preciso
                            stdValue=meanValue;
                        }

                        syntheticMeanValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][bandId][tuplekey]=syntheticMeanValue;
                        syntheticStdValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][bandId][tuplekey]=syntheticStdValue;
                        meanValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][bandId][tuplekey]=meanValue;
                        stdValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][bandId][tuplekey]=stdValue;
                        numberOfValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][bandId][tuplekey]=numberOfValues;
                        iterTuplekeyRasterFile++;
                    }
                    iterBand++;
    //                break;
                }
                iterRasterType++;
            }
            iterTuplekey++;
        }
//        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > syntheticMeanValueByRasterTypeByRasterFileByBandByTuplekey;
//        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > syntheticStdValueByRasterTypeByRasterFileByBandByTuplekey;
//        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > meanValueByRasterTypeByRasterFileByBandByTuplekey;
//        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > > stdValueByRasterTypeByRasterFileByBandByTuplekey;
//        QMap<QString,QMap<QString,QMap<QString,QMap<QString,int> > > > numberOfValueByRasterTypeByRasterFileByBandByTuplekey;
//        QMap<QString,QMap<QString,QMap<QString,double> > > gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand;
//        QMap<QString,QMap<QString,QMap<QString,double> > > offsetIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand;
        QMap<QString,QMap<QString,QMap<QString,QMap<QString,double> > > >::const_iterator iterRasterType=syntheticMeanValueByRasterTypeByRasterFileByBandByTuplekey.begin();
        out<<"        Type                         Scene      Band        gain    offset synthMean  synthStd      Mean       Std           Values by tuplekey"<<"\n";
        while(iterRasterType!=syntheticMeanValueByRasterTypeByRasterFileByBandByTuplekey.end())
        {
            QString rasterType=iterRasterType.key();
            QMap<QString,QMap<QString,QMap<QString,double> > >::const_iterator iterRasterFile=iterRasterType.value().begin();
            while(iterRasterFile!=iterRasterType.value().end())
            {
                QString rasterFile=iterRasterFile.key();
                QMap<QString,QMap<QString,double> >::const_iterator iterRasterBand=iterRasterFile.value().begin();
                while(iterRasterBand!=iterRasterFile.value().end())
                {
                    QString rasterBand=iterRasterBand.key();
                    double syntheticMeanValueGlobalNumerator=0.0;
                    double syntheticMeanValueGlobalDenominator=0.0;
                    double meanValueGlobalNumerator=0.0;
                    double meanValueGlobalDenominator=0.0;
                    // https://en.wikipedia.org/wiki/Weighted_arithmetic_mean
                    QMap<QString,double>::const_iterator iterTuplekey=iterRasterBand.value().begin();
                    QString strTuplekeyDataReport;
                    while(iterTuplekey!=iterRasterBand.value().end())
                    {
                        QString tuplekey=iterTuplekey.key();
                        double syntheticMeanValue=syntheticMeanValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][rasterBand][tuplekey];
                        double syntheticStdValue=syntheticStdValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][rasterBand][tuplekey];
                        double meanValue=meanValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][rasterBand][tuplekey];
                        double stdValue=stdValueByRasterTypeByRasterFileByBandByTuplekey[rasterType][rasterFile][rasterBand][tuplekey];
                        syntheticMeanValueGlobalNumerator+=(syntheticMeanValue*1.0/pow(syntheticStdValue,2.0));
                        syntheticMeanValueGlobalDenominator+=(1.0/pow(syntheticStdValue,2.0));
                        meanValueGlobalNumerator+=(meanValue*1.0/pow(stdValue,2.0));
                        meanValueGlobalDenominator+=(1.0/pow(stdValue,2.0));
                        strTuplekeyDataReport+=tuplekey;
                        strTuplekeyDataReport+="[";
                        strTuplekeyDataReport+=QString::number(syntheticMeanValue,'f',1);
                        strTuplekeyDataReport+=",";
                        strTuplekeyDataReport+=QString::number(syntheticStdValue,'f',2);
                        strTuplekeyDataReport+=",";
                        strTuplekeyDataReport+=QString::number(meanValue,'f',1);
                        strTuplekeyDataReport+=",";
                        strTuplekeyDataReport+=QString::number(stdValue,'f',2);
                        strTuplekeyDataReport+="]";
                        iterTuplekey++;
                    }
                    double syntheticMeanValueGlobal=syntheticMeanValueGlobalNumerator/syntheticMeanValueGlobalDenominator;
                    double syntheticStdValueGlobal=sqrt(1.0/syntheticMeanValueGlobalDenominator);
                    double meanValueGlobal=meanValueGlobalNumerator/meanValueGlobalDenominator;
                    double stdValueGlobal=sqrt(1.0/meanValueGlobalDenominator);
                    double gain=stdValueGlobal/syntheticStdValueGlobal;
                    double offset=meanValueGlobal-gain*syntheticMeanValueGlobal;
                    QString strReport=rasterType.rightJustified(12);
                    strReport+=rasterFile.rightJustified(30);
                    strReport+=rasterBand.rightJustified(10);
                    strReport+=QString::number(gain,'f',6).rightJustified(12);
                    strReport+=QString::number(offset,'f',2).rightJustified(10);
                    strReport+=QString::number(syntheticMeanValueGlobal,'f',1).rightJustified(10);
                    strReport+=QString::number(syntheticStdValueGlobal,'f',2).rightJustified(10);
                    strReport+=QString::number(meanValueGlobal,'f',1).rightJustified(10);
                    strReport+=QString::number(stdValueGlobal,'f',2).rightJustified(10);
                    strReport+=" ";
                    strReport+=strTuplekeyDataReport;
                    out<<strReport<<"\n";
                    gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType][rasterFile][rasterBand]=gain;
                    offsetIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType][rasterFile][rasterBand]=offset;
                    iterRasterBand++;
                }
                iterRasterFile++;
            }
            iterRasterType++;
        }
        /*
        QDateTime finalDateTime=QDateTime::currentDateTime();
        int initialSeconds=(int)initialDateTime.toTime_t();
        int finalSeconds=(int)finalDateTime.toTime_t();
        int totalDurationSeconds=finalSeconds-initialSeconds;
        double dblTotalDurationSeconds=(double)totalDurationSeconds;
        int durationDays=(int)floor(dblTotalDurationSeconds/60.0/60.0/24.0);
        int durationHours=(int)floor((dblTotalDurationSeconds-durationDays*60.0*60.0*24.0)/60.0/60.0);
        int durationMinutes=(int)floor((dblTotalDurationSeconds-durationDays*60.0*60.0*24.0-durationHours*60.0*60.0)/60.0);
        int durationSeconds=dblTotalDurationSeconds-durationDays*60.0*60.0*24.0-durationHours*60.0*60.0-durationMinutes*60.0;
        {
            QString msgTtime="\n- Process time:\n";
            msgTtime+="  - Start time of the process ......................: ";
            msgTtime+=initialDateTime.toString("yyyy/MM/dd - hh/mm/ss.zzz");
            msgTtime+="\n";
            msgTtime+="  - End time of the process ........................: ";
            msgTtime+=finalDateTime.toString("yyyy/MM/dd - hh/mm/ss.zzz");
            msgTtime+="\n";
            msgTtime+="  - Number of total seconds ........................: ";
            msgTtime+=QString::number(dblTotalDurationSeconds,'f',3);
            msgTtime+="\n";
            msgTtime+="    - Number of days ...............................: ";
            msgTtime+=QString::number(durationDays);
            msgTtime+="\n";
            msgTtime+="    - Number of hours ..............................: ";
            msgTtime+=QString::number(durationHours);
            msgTtime+="\n";
            msgTtime+="    - Number of minutes ............................: ";
            msgTtime+=QString::number(durationMinutes);
            msgTtime+="\n";
            msgTtime+="    - Number of seconds ............................: ";
            msgTtime+=QString::number(durationSeconds,'f',3);
            msgTtime+="\n";
            out<<msgTtime;
        }
        resultsFile.close();
        return(false);
        */
    }

    // Eliminacion de nubes
//    QMap<QString, QMap<QString, QMap<QString, QString> > >::const_iterator iterTuplekey =tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.begin();
    iterTuplekey =tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.begin();
    numberOfProcessedTuplekeys=0;
    while(iterTuplekey!=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.end())
    {
        numberOfProcessedTuplekeys++;
        QString tuplekey=iterTuplekey.key();
        QMap<QString, QMap<QString, QMap<QString, QString> > > tuplekeysRasterFilesByRasterTypeByBandByRasterFile;
//        QMap<QString,QVector<QVector<bool> > > maskDataByRasterFile;
        QMap<QString,QMap<int,QVector<int> > > maskDataByRasterFile;
        QMap<QString,QVector<double> > maskGeorefByRasterFile;
        QMap<QString,bool> partiallyCloudyByRasterFile; // si no está no tiene nubes
        QMap<QString,float> cloudyPercentageByRasterFile; // si no está no tiene nubes
        QMap<QString, QMap<QString, QString> >::const_iterator iterRasterFile=iterTuplekey.value().begin();
//        QMap<QString,QMap<QString,QVector<QString> > > bandsCombinationsByRasterType;
        QMap<QString,QMap<QString,QMap<QString,QString> > > bandsCombinationsFileBaseNameByRasterTypeByRasterFile;
        QString tuplekeyPath;
        while(iterRasterFile!=iterTuplekey.value().end())
        {
            QString rasterFile=iterRasterFile.key();
            QString rasterType=rasterTypesByRasterFile[rasterFile];
            if(bandsCombinationsByRasterType.contains(rasterType))
            {
                QMap<QString,QVector<QString> >::const_iterator iterBandsCombinations=bandsCombinationsByRasterType[rasterType].begin();
                while(iterBandsCombinations!=bandsCombinationsByRasterType[rasterType].end())
                {
                    QString subFolder;
                    QString fileBaseName=rasterFile+"_";
                    for(int nb=0;nb<iterBandsCombinations.value().size();nb++)
                    {
                        fileBaseName+=iterBandsCombinations.value().at(nb);
                        subFolder+=iterBandsCombinations.value().at(nb);
                    }
                    fileBaseName+=".";
                    fileBaseName+=RASTER_JPEG_FILE_EXTENSION;
                    QString fileBaseNameAndSubFolder=subFolder+"/"+fileBaseName;
//                    fileBaseName+=RASTER_PNG_FILE_EXTENSION;
                    bandsCombinationsFileBaseNameByRasterTypeByRasterFile[rasterType][rasterFile][iterBandsCombinations.key()]=fileBaseNameAndSubFolder;
                    iterBandsCombinations++;
                }
            }
            QString maskBandCode;
            if(rasterType.compare(landsat8IdDb)==0)
            {
                maskBandCode=REMOTESENSING_LANDSAT8_BAND_B0_CODE;
            }
            else if(rasterType.compare(sentinel2IdDb)==0)
            {
                maskBandCode=REMOTESENSING_SENTINEL2_BAND_B0_CODE;
            }
            QMap<QString, QString>::const_iterator iterBand=iterRasterFile.value().begin();
            while(iterBand!=iterRasterFile.value().end())
            {
                QString bandId=iterBand.key();
                QString tuplekyeRasterFile=iterBand.value();
                if(!cloudyPercentageByRasterFile.contains(rasterFile))
                {
                    QFileInfo tuplekeyRasterFileInfo(tuplekyeRasterFile);
                    QString maskBandFileName=tuplekeyRasterFileInfo.absolutePath()+"//"+rasterFile+"_"+maskBandCode+".tif";
                    if(QFile::exists(maskBandFileName))
                    {
                        QVector<double> georef;
                        QMap<int,QVector<int> > values;
                        float cloudValuesPercentage;
                        if(!readMaskRasterFile(maskBandFileName,cloudValue,georef,values,cloudValuesPercentage,strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError reading mask raster file:\n%1\nError:\n%2")
                                    .arg(tuplekyeRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
//                        if(rasterFile.compare("LC82000322015101LGN00",Qt::CaseInsensitive)==0)
//                        {
//                            QMap<int,QVector<int> >::const_iterator kk1=values.begin();
//                            while(kk1!=values.end())
//                            {
//                                int rowKk=kk1.key();
//                                for(int kk2=0;kk2<kk1.value().size();kk2++)
//                                {
//                                    int columnKk=kk1.value()[kk2];
//                                    int yo=1;
//                                }
//                                kk1++;
//                            }
//                        }
                        if(cloudValuesPercentage>0)
                        {
                            maskDataByRasterFile[rasterFile]=values;
                            maskGeorefByRasterFile[rasterFile]=georef;
                            partiallyCloudyByRasterFile[rasterFile]=true;
                            if(cloudValuesPercentage>=maxPercentagePartiallyCloudy)
                            {
                                partiallyCloudyByRasterFile[rasterFile]=false;
                            }
                        }
                        cloudyPercentageByRasterFile[rasterFile]=cloudValuesPercentage;
                    }
                }
                tuplekeysRasterFilesByRasterTypeByBandByRasterFile[rasterType][bandId][rasterFile]=tuplekyeRasterFile;
                iterBand++;
            }
            iterRasterFile++;
        }
        out<<"- Procesamiento de tuplekey ................: "<<tuplekey<<"\n";
        out<<"  - Numero de tipos de escenas a procesar ..: "<<tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()<<"\n";
        QMap<QString, QMap<QString, QMap<QString, QString> > >::const_iterator iterRasterType=tuplekeysRasterFilesByRasterTypeByBandByRasterFile.begin();
        int numberOfProcessedRasterTypes=0;
        while(iterRasterType!=tuplekeysRasterFilesByRasterTypeByBandByRasterFile.end())
        {
            numberOfProcessedRasterTypes++;
            QString rasterType=iterRasterType.key();
            out<<"  - Tipo de escenas ........................: "<<rasterType<<"\n";
            QMap<QString, QMap<QString, QString> > tuplekeysRasterFilesByBandByRasterFile=iterRasterType.value();
            out<<"    - Numero de bandas a procesar ..........: "<<tuplekeysRasterFilesByBandByRasterFile.size()<<"\n";
            QMap<QString, QMap<QString, QString> >::const_iterator iterBand=tuplekeysRasterFilesByBandByRasterFile.begin();
            int numberOfProcessedBands=0;
            QMap<QString,QMap<QString,QVector<QVector<qint8> > > > combinationsDataByBandByRasterFile;
//            QMap<QString,QMap<QString,GByte* > > combinationsDataByBandByRasterFile;
            QMap<QString,QVector<QString> > bandsCombinations;
            if(bandsCombinationsByRasterType.contains(rasterType))
            {
                bandsCombinations=bandsCombinationsByRasterType[rasterType];
            }
            int numberOfScenes=0;
            while(iterBand!=tuplekeysRasterFilesByBandByRasterFile.end())
            {
                numberOfProcessedBands++;
                QString bandId=iterBand.key();
                QVector<QString> bandCombinationsIds;
                if(bandsCombinations.size()>0)
                {
                    QMap<QString,QVector<QString> >::const_iterator iterBandCombinations=bandsCombinations.begin();
                    while(iterBandCombinations!=bandsCombinations.end())
                    {
                        for(int nbc=0;nbc<iterBandCombinations.value().size();nbc++)
                        {
                            QString bandIdInCombination=iterBandCombinations.value().at(nbc);
                            if(bandId.compare(bandIdInCombination,Qt::CaseInsensitive)==0)
                            {
                                bandCombinationsIds.push_back(iterBandCombinations.key());
                                break;
                            }
                        }
                        iterBandCombinations++;
                    }
                }
                QMap<QString,QVector<QVector<double> > > bandDataByRasterFile;
                QMap<QString,QMap<int,QVector<int> > > bandMaskDataByRasterFile;
                QMap<QString,QVector<double> > bandGeorefByRasterFile;
                QMap<QString,double> bandNoDataValueByRasterFile;
                QMap<int,QVector<int> > pixelsToCompute;
                QVector<QString> computedRasterFiles; // solo hay que salvar estos
                QMap<QString,double> factorTo8BitsByRasterFile;
                QMap<QString, QString> tuplekeysRasterFilesByRasterFile=iterBand.value();
                out<<"    - Banda ................................: "<<bandId<<"\n";
                out<<"      - Numero de escenas ..................: "<<tuplekeysRasterFilesByRasterFile.size()<<"\n";
                numberOfScenes=tuplekeysRasterFilesByRasterFile.size();
                QMap<QString, QString>::const_iterator iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                int numberOfPixelsToCompute=0;
                int rowMaxColumn=0;
                int maxColumn=0;
                QString title=QObject::tr("Removing clouds - Reading files");
                QString msgGlobal;
                msgGlobal=title+"\n\n";
                msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
                msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                        .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
                msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                        .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
                msgGlobal+=QObject::tr("\n  Number of bands to process .......: %1").arg(QString::number(tuplekeysRasterFilesByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing band number .......: %1, %2")
                        .arg(QString::number(numberOfProcessedBands)).arg(bandId);
                msgGlobal+=QObject::tr("\n  Number of files to read ..........: %1").arg(QString::number(tuplekeysRasterFilesByRasterFile.size()));
                QProgressDialog readingFilesProgress(title, QObject::tr("Abort"),0,tuplekeysRasterFilesByRasterFile.size(), mPtrWidgetParent);
                readingFilesProgress.setWindowModality(Qt::WindowModal);
                readingFilesProgress.setLabelText(msgGlobal);
                readingFilesProgress.setWindowTitle(title);
                readingFilesProgress.show();
                readingFilesProgress.adjustSize();
                qApp->processEvents();
                int numberOfReadedFiles=0;
                while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                {
                    numberOfReadedFiles++;
                    readingFilesProgress.setValue(numberOfReadedFiles);
                    if(readingFilesProgress.wasCanceled())
                    {
                        QMessageBox msgBox(mPtrWidgetParent);
                        QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Reading files");
                        msgBox.setText(msg);
                        QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                        msgBox.setInformativeText(question);
                        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                        msgBox.setDefaultButton(QMessageBox::Yes);
                        int ret = msgBox.exec();
                        bool abort=false;
                        switch (ret)
                        {
                          case QMessageBox::Yes:
                            abort=true;
                              // Save was clicked
                              break;
                          case QMessageBox::No:
                              break;
                          case QMessageBox::Cancel:
                              break;
                          default:
                              // should never be reached
                              break;
                        }
                        if(abort)
                        {
                            strError=QObject::tr("Process was canceled");
                            resultsFile.close();
                            return(false);
                        }
                    }
                    QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                    if(tuplekeyPath.isEmpty())
                    {
                        QFileInfo tuplekeyRasterFileFileInfo(tuplekeysRasterFile);
                        tuplekeyPath=tuplekeyRasterFileFileInfo.absolutePath();
                    }
                    QString rasterFile=iterTuplekeyRasterFile.key();
                    factorTo8BitsByRasterFile[rasterFile]=1.0;
                    float cloudyPercentage=0.0;
                    if(cloudyPercentageByRasterFile.contains(rasterFile))
                    {
                        cloudyPercentage=cloudyPercentageByRasterFile[rasterFile];
                    }
                    out<<"      - Escena a procesar ..................: "<<rasterFile;
                    out<<" ("<<QString::number(cloudyPercentage,'f',2).rightJustified(6)<<" % nubes -> ";
                    if(cloudyPercentage==0)
                    {
                        out<<"sin nubes";
                    }
                    else if(partiallyCloudyByRasterFile[rasterFile])
                    {
                        out<<"parcialmente nuboso";
                    }
                    else
                    {
                        out<<"totalmente nuboso";
                    }
                    out<<")\n";
                    IGDAL::Raster* ptrRasterFile=new IGDAL::Raster(mPtrCrsTools);
                    if(!ptrRasterFile->setFromFile(tuplekeysRasterFile,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    int columns,rows;
                    if(!ptrRasterFile->getSize(columns,rows,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    double dlNoDataValue;
                    int numberOfBand=0;
                    if(!ptrRasterFile->getNoDataValue(numberOfBand,dlNoDataValue,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    QVector<double> georef;
                    if(!ptrRasterFile->getGeoRef(georef,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError reading georef in raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    QVector<QVector<double> > values;
                    int initialColumn=0;
                    int initialRow=0;
                    if(!ptrRasterFile->readValues(numberOfBand, // desde 0
                                                  initialColumn,
                                                  initialRow,
                                                  columns,
                                                  rows,
                                                  values,
                                                  strAuxError))
                    {
                        strError=QObject::tr("Algorithms::cloudRemoval");
                        strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                                .arg(tuplekeysRasterFile).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    // Conversión de los valores
                    double gain=intercalibrationGainByRasterFileAndByBand[rasterFile][bandId];
                    double offset=intercalibrationOffsetByRasterFileAndByBand[rasterFile][bandId];
                    bool intercalibrationTo8Bits=intercalibrationTo8BitsByRasterFileAndByBand[rasterFile][bandId];
                    bool intercalibrationToReflectance=intercalibrationToReflectanceByRasterFileAndByBand[rasterFile][bandId];
                    double reflectanceAddValue,reflectanceMultValue;
                    if(intercalibrationToReflectance)
                    {
                        reflectanceAddValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId];
                        reflectanceMultValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][bandId];
                    }
                    bool intercalibrationByInterpolation=intercalibrationInterpolatedByRasterFileAndByBand[rasterFile][bandId];
                    double factorTo8Bits=1.0;
                    if(intercalibrationTo8Bits
                            ||bandsCombinations.size()>0)
                    {
                        if(!ptrRasterFile->getFactorTo8Bits(factorTo8Bits,strAuxError))
                        {
                            strError=QObject::tr("Algorithms::cloudRemoval");
                            strError+=QObject::tr("\nError getting factor to 8 bits for raster file:\n%1\nError:\n%2")
                                    .arg(tuplekeysRasterFile).arg(strAuxError);
                            resultsFile.close();
                            return(false);
                        }
                        factorTo8BitsByRasterFile[rasterFile]=factorTo8Bits;
                    }
                    delete(ptrRasterFile);
                    for(int row=0;row<rows;row++)
                    {
                        for(int column=0;column<columns;column++)
                        {
                            double value=values[row][column];
                            if(fabs(value-dlNoDataValue)>0.1)
                            {
                                if(intercalibrationToReflectance)
                                {
                                    value=value*reflectanceMultValue+reflectanceAddValue;
                                }
                                if(intercalibrationTo8Bits)
                                {
                                    value=value*factorTo8Bits;
                                }
                                value=value*gain+offset;
                            }
                            values[row][column]=value;
                        }
                    }
                    bandDataByRasterFile[rasterFile]=values;
                    bandGeorefByRasterFile[rasterFile]=georef;
                    bandNoDataValueByRasterFile[rasterFile]=dlNoDataValue;
                    QMap<int,QVector<int> > maskData;
                    if(cloudyPercentageByRasterFile[rasterFile]>0.0)
                    {
                        QMap<int,QVector<int> > maskDataRasterFile=maskDataByRasterFile[rasterFile];
//                        if(rasterFile.compare("LC82000322015101LGN00",Qt::CaseInsensitive)==0)
//                        {
//                            QMap<int,QVector<int> >::const_iterator kk1=maskDataRasterFile.begin();
//                            while(kk1!=maskDataRasterFile.end())
//                            {
//                                int rowKk=kk1.key();
//                                for(int kk2=0;kk2<kk1.value().size();kk2++)
//                                {
//                                    int columnKk=kk1.value()[kk2];
//                                    int yo=1;
//                                }
//                                kk1++;
//                            }
//                        }
                        QVector<double> maskGeorefRasterFile=maskGeorefByRasterFile[rasterFile];
                        double maskColumnGsdRasterFile=maskGeorefRasterFile[1];
                        double maskColumnGsd=georef[1];
                        if(fabs(maskColumnGsdRasterFile-maskColumnGsd)<0.01)
                        {
                            maskData=maskDataRasterFile;
                        }
                        else
                        {
                            if(maskColumnGsdRasterFile>maskColumnGsd)
                            {
                                int factorGsd=qRound(maskColumnGsdRasterFile/maskColumnGsd);
                                if(fabs(factorGsd-maskColumnGsdRasterFile/maskColumnGsd)>0.01)
                                {
                                    strError=QObject::tr("Algorithms::cloudRemoval");
                                    strError+=QObject::tr("\nError getting GSD factor for raster file:\n%1\nError:\n%2")
                                            .arg(tuplekeysRasterFile).arg(strAuxError);
                                    resultsFile.close();
                                    return(false);
                                }
                                QMap<int,QVector<int> >::const_iterator iterMaskDataRasterFile=maskDataRasterFile.begin();
                                while(iterMaskDataRasterFile!=maskDataRasterFile.end())
                                {
                                    int rowMaskRasterFile=iterMaskDataRasterFile.key();
                                    int firstRowMask=rowMaskRasterFile*factorGsd;
                                    for(int i=0;i<iterMaskDataRasterFile.value().size();i++)
                                    {
                                        int columnMaskRasterFile=iterMaskDataRasterFile.value()[i];
                                        int firstColumnMask=columnMaskRasterFile*factorGsd;
                                        for(int r=0;r<factorGsd;r++)
                                        {
                                            int rowMask=firstRowMask+r;
                                            if(!maskData.contains(rowMask))
                                            {
                                                QVector<int> aux;
                                                maskData[rowMask]=aux;
                                            }
                                            for(int c=0;c<factorGsd;c++)
                                            {
                                                int columnMask=firstColumnMask+c;
                                                // Si hay un noDataValue no lo añado como pixel de nube ya que no hay que obtenerlo
                                                if(fabs(bandDataByRasterFile[rasterFile][rowMask][columnMask]-bandNoDataValueByRasterFile[rasterFile])>0.01)
                                                {
                                                    maskData[rowMask].push_back(columnMask);
                                                }
                                            }
                                        }
                                    }
                                    iterMaskDataRasterFile++;
                                }
                            }
                            else
                            {
                                int factorGsd=qRound(maskColumnGsd/maskColumnGsdRasterFile);
                                if(fabs(factorGsd-maskColumnGsd/maskColumnGsdRasterFile)>0.01)
                                {
                                    strError=QObject::tr("Algorithms::cloudRemoval");
                                    strError+=QObject::tr("\nError getting GSD factor for raster file:\n%1\nError:\n%2")
                                            .arg(tuplekeysRasterFile).arg(strAuxError);
                                    resultsFile.close();
                                    return(false);
                                }
                                QMap<int,QVector<int> >::const_iterator iterMaskDataRasterFile=maskDataRasterFile.begin();
                                while(iterMaskDataRasterFile!=maskDataRasterFile.end())
                                {
                                    int rowMaskRasterFile=iterMaskDataRasterFile.key();
                                    int rowMask=rowMaskRasterFile/factorGsd; // 3/2 = 1
                                    for(int i=0;i<iterMaskDataRasterFile.value().size();i++)
                                    {
                                        int columnMaskRasterFile=iterMaskDataRasterFile.value()[i];
                                        int columnMask=columnMaskRasterFile/factorGsd;
                                        if(!maskData.contains(rowMask))
                                        {
                                            QVector<int> aux;
                                            maskData[rowMask]=aux;
                                        }
                                        // Si hay un noDataValue no lo añado como pixel de nube ya que no hay que obtenerlo
                                        if(fabs(bandDataByRasterFile[rasterFile][rowMask][columnMask]-bandNoDataValueByRasterFile[rasterFile])>0.01)
                                        {
                                            if(!maskData[rowMask].contains(columnMask))
                                            {
                                                maskData[rowMask].push_back(columnMask);
                                            }
                                        }
                                    }
                                    iterMaskDataRasterFile++;
                                }
                            }
                        }
                        if(partiallyCloudyByRasterFile[rasterFile]
                                ||removeFullCloudy) // para trabajar con las completamente cubiertas
                        {
                            QMap<int,QVector<int> >::const_iterator iterMaskDataRasterFile=maskDataRasterFile.begin();
                            while(iterMaskDataRasterFile!=maskDataRasterFile.end())
                            {
                                int row=iterMaskDataRasterFile.key();
                                if(!pixelsToCompute.contains(row))
                                {
                                    QVector<int> aux;
                                    pixelsToCompute[row]=aux;
                                }
                                for(int i=0;i<iterMaskDataRasterFile.value().size();i++)
                                {
                                    int column=iterMaskDataRasterFile.value()[i];
                                    if(!pixelsToCompute[row].contains(column))
                                    {
                                        pixelsToCompute[row].push_back(column);
                                        if(column>maxColumn)
                                        {
                                            maxColumn=column;
                                            rowMaxColumn=row;
                                        }
                                        numberOfPixelsToCompute++;
                                    }
                                }
                                iterMaskDataRasterFile++;
                            }
                        }
                    }
                    bandMaskDataByRasterFile[rasterFile]=maskData;
                    iterTuplekeyRasterFile++;
                }
                readingFilesProgress.setValue(tuplekeysRasterFilesByRasterFile.size());
                readingFilesProgress.close();

                out<<"      - Numero de pixeles a procesar .......: "<<QString::number(numberOfPixelsToCompute);
                out<<", columna mayor "<<QString::number(maxColumn)<<" para la fila "<<QString::number(rowMaxColumn);
                out<<"\n";
                title=QObject::tr("Removing clouds - Processing pixels");
                msgGlobal=title+"\n\n";
                msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
                msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                        .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
                msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                        .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
                msgGlobal+=QObject::tr("\n  Number of bands to process .......: %1").arg(QString::number(tuplekeysRasterFilesByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing band number .......: %1, %2")
                        .arg(QString::number(numberOfProcessedBands)).arg(bandId);
                msgGlobal+=QObject::tr("\n  Number of pixels to process ......: %1").arg(QString::number(numberOfPixelsToCompute));
                int numberOfProcessedPixelsSteps=1000;
                int numberOfPixelsByStep=numberOfPixelsToCompute/numberOfProcessedPixelsSteps;
                QProgressDialog processingPixelsProgress(title, QObject::tr("Abort"),0,numberOfProcessedPixelsSteps, mPtrWidgetParent);
                processingPixelsProgress.setWindowModality(Qt::WindowModal);
                processingPixelsProgress.setLabelText(msgGlobal);
//                processingPixelsProgress.setWindowTitle(title);
                processingPixelsProgress.show();
//                processingPixelsProgress.adjustSize();
                qApp->processEvents();
                if(printDetail)
                {
                    out<<"      - Resultados para el primer y ultimo pixel:\n";
                    out<<"     Row  Column                        Escena        ND    Nube   %Nube        Jd   ND_Intc  Dato  Calc  Interpol Sintetico     Error\n";
                }
                QMap<int,QVector<int> >::const_iterator iterPixelsToCompute=pixelsToCompute.begin();
                int computedPixels=-1;
                int computedPixelsInStep=0;
                int numberOfStep=0;
                while(iterPixelsToCompute!=pixelsToCompute.end())
                {
                    int row=iterPixelsToCompute.key();
                    for(int i=0;i<iterPixelsToCompute.value().size();i++)
//                    for(int i=0;i<1;i++)
                    {
                        int column=iterPixelsToCompute.value()[i];
                        computedPixels++;
                        computedPixelsInStep++;
                        if(computedPixelsInStep==numberOfPixelsByStep)
                        {
                            numberOfStep++;
                            processingPixelsProgress.setValue(numberOfStep);
                            computedPixelsInStep=0;
                            if (processingPixelsProgress.wasCanceled())
                            {
                                QMessageBox msgBox(mPtrWidgetParent);
                                QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Processing pixels");
                                msgBox.setText(msg);
                                QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                                msgBox.setInformativeText(question);
                                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                                msgBox.setDefaultButton(QMessageBox::Yes);
                                int ret = msgBox.exec();
                                bool abort=false;
                                switch (ret)
                                {
                                  case QMessageBox::Yes:
                                    abort=true;
                                      // Save was clicked
                                      break;
                                  case QMessageBox::No:
                                      break;
                                  case QMessageBox::Cancel:
                                      break;
                                  default:
                                      // should never be reached
                                      break;
                                }
                                if(abort)
                                {
                                    strError=QObject::tr("Process was canceled");
                                    resultsFile.close();
                                    return(false);
                                }
                            }
                        }
                        int pixelDataMinJd=100000000;
                        int pixelDataMaxJd=-100000000;
                        int pixelToInterpolateMinJd=100000000;
                        int pixelToInterpolateMaxJd=-100000000;
                        QMap<int,double> pixelDataByJd;
                        QMap<int,QString> rasterFileDataByJd;
                        QVector<int> jdToInterpolate;
                        QMap<int,QString> rasterFileToInterpolateByJd;
                        QMap<QString, QString>::const_iterator iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                        // para imprimir
                        QMap<int,double> toPrintPixelDataByJd;
                        QMap<int,bool> toPrintPixelValidData;
                        while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                        {
//                            QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                            QString rasterFile=iterTuplekeyRasterFile.key();
                            int jd=jdByRasterFile[rasterFile];
                            double pixelData=bandDataByRasterFile[rasterFile][row][column];
                            bool pixelValidData=false;
                            if(fabs(bandDataByRasterFile[rasterFile][row][column]-bandNoDataValueByRasterFile[rasterFile])>0.01)
                            {
                                if(!bandMaskDataByRasterFile[rasterFile].contains(row))
                                {
                                    pixelValidData=true;
                                }
                                else
                                {
                                    if(!bandMaskDataByRasterFile[rasterFile][row].contains(column))
                                    {
                                        pixelValidData=true;
                                    }
                                }
                                if(pixelValidData)
                                {
                                    pixelDataByJd[jd]=pixelData;
                                    rasterFileDataByJd[jd]=rasterFile;
                                    if(jd<pixelDataMinJd)
                                    {
                                        pixelDataMinJd=jdByRasterFile[rasterFile];
                                    }
                                    if(jd>pixelDataMaxJd)
                                    {
                                        pixelDataMaxJd=jdByRasterFile[rasterFile];
                                    }
                                }
                                else
                                {
                                    if(partiallyCloudyByRasterFile[rasterFile]
                                            ||removeFullCloudy)
                                    {
                                        jdToInterpolate.push_back(jd);
                                        rasterFileToInterpolateByJd[jd]=rasterFile;
                                        if(jd<pixelToInterpolateMinJd)
                                        {
                                            pixelToInterpolateMinJd=jd;
                                        }
                                        if(jd>pixelToInterpolateMaxJd)
                                        {
                                            pixelToInterpolateMaxJd=jd;
                                        }
                                    }
                                }
                            }
                            if(printDetail&&(computedPixels==0||computedPixels==(numberOfPixelsToCompute-1)))
                            {
                                toPrintPixelDataByJd[jd]=pixelData;
                                toPrintPixelValidData[jd]=pixelValidData;
                            }
                            iterTuplekeyRasterFile++;
                        }
                        if(jdToInterpolate.size()==0
                                ||pixelDataByJd.size()==0)
                        {
                            continue;
                        }
                        int firstDataJd=pixelDataMinJd;
                        int lastDataJd=pixelDataMaxJd;
                        if(numberOfDates>0)
                        {
                            firstDataJd=pixelToInterpolateMinJd-numberOfDates;
                            if(firstDataJd<pixelDataMinJd)
                            {
                                firstDataJd=pixelDataMinJd;
                            }
                            lastDataJd=pixelToInterpolateMaxJd+numberOfDates;
                            if(lastDataJd>pixelDataMaxJd)
                            {
                                lastDataJd=pixelDataMaxJd;
                            }
                        }
                        int contData=0;
                        QMap<int,double>::const_iterator iterPixelDataByJd=pixelDataByJd.begin();
                        QVector<int> pixelDataJds; // va a estar ordenado porque lo creo recorriendo un map que lo esta
                        while(iterPixelDataByJd!=pixelDataByJd.end())
                        {
                            int jd=iterPixelDataByJd.key();
                            if(jd<firstDataJd||jd>lastDataJd)
                            {
                                iterPixelDataByJd++;
                                continue;
                            }
                            contData++;
                            pixelDataJds.push_back(jd);
                            iterPixelDataByJd++;
                        }
                        if(contData==0)
                        {
                            continue;
                        }
                        QMap<int,double> toPrintInterpolatedValues;
                        QMap<int,double> sinteticValues;
                        if(contData>1)
                        {
                            Splines::AkimaSpline* ptrAkimaSpline=NULL;
                            Splines::CubicSpline* ptrCubicSpline=NULL;
                            Splines::BesselSpline* ptrBesselSpline=NULL;
                            double* x;
                            double* y;
                            x=new double[contData];
                            y=new double[contData];
                            contData=0;
                            iterPixelDataByJd=pixelDataByJd.begin();
                            while(iterPixelDataByJd!=pixelDataByJd.end())
                            {
                                int jd=iterPixelDataByJd.key();
                                if(jd<firstDataJd||jd>lastDataJd)
                                {
                                    iterPixelDataByJd++;
                                    continue;
                                }
                                x[contData]=jd-firstDataJd;
                                y[contData]=iterPixelDataByJd.value();
                                contData++;
                                iterPixelDataByJd++;
                            }
                            if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                ptrAkimaSpline=new Splines::AkimaSpline();
                                ptrAkimaSpline->build(x,y,contData) ;
                            }
                            else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                ptrCubicSpline=new Splines::CubicSpline();
                                ptrCubicSpline->build(x,y,contData) ;
                            }
                            else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                ptrBesselSpline=new Splines::BesselSpline();
                                ptrBesselSpline->build(x,y,contData) ;
                            }
                            delete(x);
                            delete(y);
                            // Calculo de datos sintéticos
                            if(contData==2)
                            {
                                sinteticValues[pixelDataJds[0]]=pixelDataByJd[pixelDataJds[1]];
                                sinteticValues[pixelDataJds[1]]=pixelDataByJd[pixelDataJds[0]];
                            }
                            else
                            {
                                iterPixelDataByJd=pixelDataByJd.begin();
                                while(iterPixelDataByJd!=pixelDataByJd.end())
                                {
                                    int jd=iterPixelDataByJd.key();
                                    int posJdInPixelDataJds=pixelDataJds.indexOf(jd);
                                    if(jd<firstDataJd||jd>lastDataJd)
                                    {
                                        if(jd<firstDataJd)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[firstDataJd];
                                        }
                                        if(jd>lastDataJd)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[lastDataJd];
                                        }
                                        iterPixelDataByJd++;
                                        continue;
                                    }
                                    else if(jd==firstDataJd)
                                    {
                                        if((posJdInPixelDataJds+1)<pixelDataJds.size())
                                        {
                                            sinteticValues[jd]=pixelDataByJd[pixelDataJds[posJdInPixelDataJds+1]];
                                        }
                                        else
                                        {
                                            sinteticValues[jd]=pixelDataByJd[jd];
                                        }
                                    }
                                    else if(jd==lastDataJd)
                                    {
                                        if(posJdInPixelDataJds>0)
                                        {
                                            sinteticValues[jd]=pixelDataByJd[pixelDataJds[posJdInPixelDataJds-1]];
                                        }
                                        else
                                        {
                                            sinteticValues[jd]=pixelDataByJd[jd];
                                        }
                                    }
                                    else // en este caso hay que interpolar
                                    {
                                        double* xSintetic;
                                        double* ySintetic;
                                        int contDataSintetic=contData-1;
                                        xSintetic=new double[contDataSintetic];
                                        ySintetic=new double[contDataSintetic];
                                        contDataSintetic=0;
                                        QMap<int,double>::const_iterator iterPixelDataByJdSintetic=pixelDataByJd.begin();
                                        while(iterPixelDataByJdSintetic!=pixelDataByJd.end())
                                        {
                                            int jdAux=iterPixelDataByJdSintetic.key();
                                            if(jdAux==jd)
                                            {
                                                iterPixelDataByJdSintetic++;
                                                continue;
                                            }
                                            if(jd<firstDataJd||jd>lastDataJd)
                                            {
                                                iterPixelDataByJdSintetic++;
                                                continue;
                                            }
                                            xSintetic[contDataSintetic]=jdAux-firstDataJd;
                                            ySintetic[contDataSintetic]=iterPixelDataByJdSintetic.value();
                                            contDataSintetic++;
                                            iterPixelDataByJdSintetic++;
                                        }
                                        double xJdSintetic=jd-firstDataJd;
                                        if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::AkimaSpline* ptrAkimaSplineSintetic=NULL;
                                            ptrAkimaSplineSintetic=new Splines::AkimaSpline();
                                            ptrAkimaSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrAkimaSplineSintetic)(xJdSintetic);
                                            delete(ptrAkimaSplineSintetic);
                                        }
                                        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::CubicSpline* ptrCubicSplineSintetic=NULL;
                                            ptrCubicSplineSintetic=new Splines::CubicSpline();
                                            ptrCubicSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrCubicSplineSintetic)(xJdSintetic);
                                            delete(ptrCubicSplineSintetic);
                                        }
                                        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                                        {
                                            Splines::BesselSpline* ptrBesselSplineSintetic=NULL;
                                            ptrBesselSplineSintetic=new Splines::BesselSpline();
                                            ptrBesselSplineSintetic->build(xSintetic,ySintetic,contDataSintetic);
                                            sinteticValues[jd]=(*ptrBesselSplineSintetic)(xJdSintetic);
                                            delete(ptrBesselSplineSintetic);
                                        }
                                        delete(xSintetic);
                                        delete(ySintetic);
                                    }
                                    iterPixelDataByJd++;
                                }
                            }
                            for(int k=0;k<jdToInterpolate.size();k++)
                            {
                                double interpolatedValue=0.0;
                                int jd=jdToInterpolate[k];
                                double xJd=jd-firstDataJd;
                                if(jd<=firstDataJd)
                                {
                                    interpolatedValue=pixelDataByJd[firstDataJd];
                                }
                                else if(jd>=lastDataJd)
                                {
                                    interpolatedValue=pixelDataByJd[lastDataJd];
                                }
                                else
                                {
                                    if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                                    {
                                        interpolatedValue=(*ptrAkimaSpline)(xJd);
                                    }
                                    else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                                    {
                                        interpolatedValue=(*ptrCubicSpline)(xJd);
                                    }
                                    else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                                    {
                                        interpolatedValue=(*ptrBesselSpline)(xJd);
                                    }
                                }
                                // No hay que deshacer porque generaremos los ficheros intercalibrados
                                QString rasterFile=rasterFileToInterpolateByJd[jd];
                                if(applyCloudFreeImprovement)
                                {
                                    if(gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand.contains(rasterType))
                                    {
                                        if(gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType].contains(rasterFile))
                                        {
                                            if(gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType][rasterFile].contains(bandId))
                                            {
                                                double gainIntercalibrationCloudFreeImprovement=gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType][rasterFile][bandId];
                                                double offsetIntercalibrationCloudFreeImprovement=offsetIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType][rasterFile][bandId];
                                                interpolatedValue=interpolatedValue*gainIntercalibrationCloudFreeImprovement+offsetIntercalibrationCloudFreeImprovement;
                                            }
                                        }
                                    }
                                }
                                bandDataByRasterFile[rasterFile][row][column]=interpolatedValue;
                                if(!computedRasterFiles.contains(rasterFile))
                                {
                                    computedRasterFiles.push_back(rasterFile);
                                }
                                if(printDetail&&(computedPixels==0||computedPixels==(numberOfPixelsToCompute-1)))
                                {
                                    toPrintInterpolatedValues[jd]=interpolatedValue;
                                }
                            }
                            if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                delete(ptrAkimaSpline);
                            }
                            else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                delete(ptrCubicSpline);
                            }
                            else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                            {
                                delete(ptrBesselSpline);
                            }
                        }
                        else // solo hay un dato, que se asigna a todos los valores a interpolar
                        {
                            sinteticValues[pixelDataJds[0]]=pixelDataByJd[pixelDataJds[0]];
                            for(int k=0;k<jdToInterpolate.size();k++)
                            {
                                double interpolatedValue=pixelDataByJd[pixelDataJds[0]];
                                int jd=jdToInterpolate[k];
                                // No hay que deshacer porque generaremos los ficheros intercalibrados
                                QString rasterFile=rasterFileToInterpolateByJd[jd];
                                if(applyCloudFreeImprovement)
                                {
                                    if(gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand.contains(rasterType))
                                    {
                                        if(gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType].contains(rasterFile))
                                        {
                                            if(gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType][rasterFile].contains(bandId))
                                            {
                                                double gainIntercalibrationCloudFreeImprovement=gainIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType][rasterFile][bandId];
                                                double offsetIntercalibrationCloudFreeImprovement=offsetIntercalibrationCloudFreeImprovementByRasterTypeByRasterFileByBand[rasterType][rasterFile][bandId];
                                                interpolatedValue=interpolatedValue*gainIntercalibrationCloudFreeImprovement+offsetIntercalibrationCloudFreeImprovement;
                                            }
                                        }
                                    }
                                }
                                bandDataByRasterFile[rasterFile][row][column]=interpolatedValue;
                                if(!computedRasterFiles.contains(rasterFile))
                                {
                                    computedRasterFiles.push_back(rasterFile);
                                }
                                if(printDetail&&(computedPixels==0||computedPixels==(numberOfPixelsToCompute-1)))
                                {
                                    toPrintInterpolatedValues[jd]=interpolatedValue;
                                }
                            }
                        }
                        if(printDetail&&(computedPixels==0||computedPixels==(numberOfPixelsToCompute-1)))
                        {
                            iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                            while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                            {
    //                            QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                                QString rasterFile=iterTuplekeyRasterFile.key();
                                int jd=jdByRasterFile[rasterFile];
                                double intercalibratedValue=toPrintPixelDataByJd[jd];
                                double gain=intercalibrationGainByRasterFileAndByBand[rasterFile][bandId];
                                double offset=intercalibrationOffsetByRasterFileAndByBand[rasterFile][bandId];
                                bool intercalibrationTo8Bits=intercalibrationTo8BitsByRasterFileAndByBand[rasterFile][bandId];
                                bool intercalibrationToReflectance=intercalibrationToReflectanceByRasterFileAndByBand[rasterFile][bandId];
                                bool intercalibrationByInterpolation=intercalibrationInterpolatedByRasterFileAndByBand[rasterFile][bandId];
                                double factorTo8Bits=1.0;
                                if(intercalibrationTo8Bits)
                                {
                                    factorTo8Bits=factorTo8BitsByRasterFile[rasterFile];
                                }
                                double value=(intercalibratedValue-offset)/gain;
                                if(intercalibrationTo8Bits)
                                {
                                    value=value/factorTo8Bits;
                                }
                                if(intercalibrationToReflectance)
                                {
                                    double reflectanceAddValue,reflectanceMultValue;
                                    reflectanceAddValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId];
                                    reflectanceMultValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][bandId];
                                    value=(value-reflectanceAddValue)/reflectanceMultValue;
                                }
                                out<<QString::number(row).rightJustified(8);
                                out<<QString::number(column).rightJustified(8);
                                out<<rasterFile.rightJustified(30);
                                out<<QString::number(value,'f',1).rightJustified(10);
                                if(toPrintPixelValidData[jd])
                                {
                                    out<<QString::number(0.0,'f',1).rightJustified(8);
                                }
                                else
                                {
                                    out<<QString::number(255.0,'f',1).rightJustified(8);
                                }
                                out<<QString::number(cloudyPercentageByRasterFile[rasterFile],'f',2).rightJustified(8);
                                out<<QString::number(jd).rightJustified(10);
                                out<<QString::number(intercalibratedValue,'f',1).rightJustified(10);
                                QString strToUse="No";
                                if(toPrintPixelValidData[jd])
                                {
                                    strToUse="Si";
                                }
                                out<<strToUse.rightJustified(6);
                                QString strToCalc;
                                if(!toPrintPixelValidData[jd])
                                {
                                    if(partiallyCloudyByRasterFile[rasterFile]
                                            ||removeFullCloudy)
                                    {
                                        strToCalc="Si";
                                    }
                                    else
                                    {
                                        strToCalc="No";
                                    }
                                }
                                out<<strToCalc.rightJustified(6);
                                QString strInterpolatedValue;
                                if(strToCalc.compare("Si",Qt::CaseInsensitive)==0)
                                {
                                    strInterpolatedValue=QString::number(toPrintInterpolatedValues[jd],'f',1);
                                }
                                out<<strInterpolatedValue.rightJustified(10);
                                QString strSinteticValue;
                                QString strErrorValue;
                                if(sinteticValues.contains(jd))
                                {
                                    strSinteticValue=QString::number(sinteticValues[jd],'f',1);
                                    double errorValue=intercalibratedValue-sinteticValues[jd];
                                    strErrorValue=QString::number(errorValue,'f',1);
                                }
                                out<<strSinteticValue.rightJustified(10);
                                out<<strErrorValue.rightJustified(10);
                                out<<"\n";
                                iterTuplekeyRasterFile++;
                            }
                        }
                    }
                    iterPixelsToCompute++;
                }
                processingPixelsProgress.setValue(numberOfProcessedPixelsSteps);
                processingPixelsProgress.close();

                // Proceso de escritura
                title=QObject::tr("Removing clouds - Writting files");
                msgGlobal=title+"\n\n";
                msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
                msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                        .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
                msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                        .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
                msgGlobal+=QObject::tr("\n  Number of bands to process .......: %1").arg(QString::number(tuplekeysRasterFilesByBandByRasterFile.size()));
                msgGlobal+=QObject::tr("\n  ... Processing band number .......: %1, %2")
                        .arg(QString::number(numberOfProcessedBands)).arg(bandId);
                msgGlobal+=QObject::tr("\n  Number of files to write .........: %1").arg(QString::number(tuplekeysRasterFilesByRasterFile.size()));
                QProgressDialog writtingFilesProgress(title, QObject::tr("Abort"),0,tuplekeysRasterFilesByRasterFile.size(), mPtrWidgetParent);
                writtingFilesProgress.setWindowModality(Qt::WindowModal);
                writtingFilesProgress.setLabelText(msgGlobal);
//                writtingFilesProgress.setWindowTitle(title);
                writtingFilesProgress.show();
//                writtingFilesProgress.adjustSize();
                qApp->processEvents();
                int numberOfWrittenFiles=0;
                iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                {
                    numberOfWrittenFiles++;
                    writtingFilesProgress.setValue(numberOfWrittenFiles);
                    if(writtingFilesProgress.wasCanceled())
                    {
                        QMessageBox msgBox(mPtrWidgetParent);
                        QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Reading files");
                        msgBox.setText(msg);
                        QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                        msgBox.setInformativeText(question);
                        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                        msgBox.setDefaultButton(QMessageBox::Yes);
                        int ret = msgBox.exec();
                        bool abort=false;
                        switch (ret)
                        {
                          case QMessageBox::Yes:
                            abort=true;
                              // Save was clicked
                              break;
                          case QMessageBox::No:
                              break;
                          case QMessageBox::Cancel:
                              break;
                          default:
                              // should never be reached
                              break;
                        }
                        if(abort)
                        {
                            strError=QObject::tr("Process was canceled");
                            resultsFile.close();
                            return(false);
                        }
                    }
                    QString tuplekeysRasterFile=iterTuplekeyRasterFile.value();
                    QString rasterFile=iterTuplekeyRasterFile.key();
                    QVector<QVector<double> > bandData=bandDataByRasterFile[rasterFile];
                    QFileInfo rasterFileInfo(tuplekeysRasterFile);
                    QString removedCloudsRasterFileName=rasterFileInfo.absolutePath()+"/";
                    removedCloudsRasterFileName+=ALGORITHMS_CLOUDREMOVAL_PARAMETER_WRITEIMAGEFILES_FOLDER;
                    removedCloudsRasterFileName+="/";
                    removedCloudsRasterFileName+=bandId;
                    QString removedCloudsRasterFilePath=removedCloudsRasterFileName;
                    QDir rasterFileDir(rasterFileInfo.absolutePath());
                    if(!rasterFileDir.exists(removedCloudsRasterFileName))
                    {
                        if(!rasterFileDir.mkpath(removedCloudsRasterFilePath))
                        {
                            strError=QObject::tr("Algorithms::intercalibrationComputation");
                            strError+=QObject::tr("\nError making path for removed clouds file:\n%1")
                                    .arg(removedCloudsRasterFilePath);
                            resultsFile.close();
                            return(false);
                        }
                    }
                    removedCloudsRasterFileName+=("/"+rasterFileInfo.fileName());
                    if(!writeCloudRemovedFile(tuplekeysRasterFile,
                                              removedCloudsRasterFileName,
                                              bandData,
                                              strAuxError))
                    {
                        strError=QObject::tr("Algorithms::intercalibrationComputation");
                        strError+=QObject::tr("\nError writting removed clouds file:\n%1\nError:\%2")
                                .arg(removedCloudsRasterFileName).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
//                    QMap<QString,QMap<QString,QVector<QString> > > bandsCombinationsByRasterType;
//                    QMap<QString,QMap<QString,QString> > bandsCombinationsFileBaseNameByRasterType;
//                    QMap<QString,QMap<QString,QVector<GByte*> > > bandsCombinationsBandsDataByRasterType;
                    iterTuplekeyRasterFile++;
                }
                writtingFilesProgress.setValue(tuplekeysRasterFilesByRasterFile.size());
                writtingFilesProgress.close();

                // Informacion para combinaciones
                if(bandCombinationsIds.size()>0)
                {
                    iterTuplekeyRasterFile=tuplekeysRasterFilesByRasterFile.begin();
                    while(iterTuplekeyRasterFile!=tuplekeysRasterFilesByRasterFile.end())
                    {
                        QString rasterFile=iterTuplekeyRasterFile.key();
                        QVector<QVector<double> > bandData=bandDataByRasterFile[rasterFile];
                        int rows=bandData.size();
                        int columns=bandData[0].size();
//                        int numberOfPixels=rows*columns;
                        QVector<QVector<qint8> > data(rows);
                        for(int row=0;row<rows;row++)
                        {
                            data[row].resize(columns);
                            for(int column=0;column<columns;column++)
                            {
                                data[row][column]=(int)(bandData[row][column]*factorTo8BitsByRasterFile[rasterFile]);
                            }
                        }
//                        GByte*  pData=NULL;
//                        pData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
//                        int pos=0;
//                        for(int row=0;row<rows;row++)
//                        {
//                            for(int column=0;column<columns;column++)
//                            {
//                                pData[pos]=(int)((int)bandData[row][column])*((int)factorTo8BitsByRasterFile[rasterFile]);
//                                pos++;
//                            }
//                        }
//                        combinationsDataByBandByRasterFile[bandId][rasterFile]=pData;
                        combinationsDataByBandByRasterFile[bandId][rasterFile]=data;

                        iterTuplekeyRasterFile++;
                    }
                }
                iterBand++;
//                break;
            }
            // Creacion de las combinaciones de bandas
            int numberOfBandCombinationsFiles=0;
            for(int nrf=0;nrf<rasterFiles.size();nrf++)
            {
                QString rasterFile=rasterFiles.at(nrf);
                if(rasterTypesByRasterFile[rasterFile].compare(rasterType,Qt::CaseInsensitive)!=0)
                {
                    continue;
                }
                QMap<QString,QVector<QString> >::const_iterator iterBandsCombinations=bandsCombinations.begin();
                while(iterBandsCombinations!=bandsCombinations.end())
                {
                    QString bandsCombination=iterBandsCombinations.key();
                    bool existsBandsForCombination=true;
                    for(int nb=0;nb<iterBandsCombinations.value().size();nb++)
                    {
                        QString bandId=iterBandsCombinations.value().at(nb);
                        if(!combinationsDataByBandByRasterFile.contains(bandId))
                        {
                            existsBandsForCombination=false;
                            break;
                        }
                        else
                        {
                            if(!combinationsDataByBandByRasterFile[bandId].contains(rasterFile))
                            {
                                existsBandsForCombination=false;
                                break;
                            }
                        }
                    }
                    if(!existsBandsForCombination)
                    {
                        iterBandsCombinations++;
                        continue;
                    }
                    numberOfBandCombinationsFiles++;
                    iterBandsCombinations++;
                }
            }
            out<<"  - Ficheros de combinaciones de bandas ....: "<<QString::number(numberOfBandCombinationsFiles)<<"\n";
            QString title=QObject::tr("Removing clouds - Reading files");
            QString msgGlobal;
            msgGlobal=title+"\n\n";
            msgGlobal+=QObject::tr("Number of tuplekeys to process ....: %1").arg(QString::number(rasterFilesByTuplekey.size()));
            msgGlobal+=QObject::tr("\n  ... Processing tuplekey number ...: %1, %2")
                    .arg(QString::number(numberOfProcessedTuplekeys)).arg(tuplekey);
            msgGlobal+=QObject::tr("\n  Number of sensors to process .....: %1").arg(QString::number(tuplekeysRasterFilesByRasterTypeByBandByRasterFile.size()));
            msgGlobal+=QObject::tr("\n  ... Processing sensor number .....: %1, %2")
                    .arg(QString::number(numberOfProcessedRasterTypes)).arg(rasterType);
            msgGlobal+=QObject::tr("\n  Number of bands combinations files to write .......: %1").arg(QString::number(numberOfBandCombinationsFiles));
            QProgressDialog writtingBandsCombinationsFilesProgress(title, QObject::tr("Abort"),0,numberOfBandCombinationsFiles, mPtrWidgetParent);
            writtingBandsCombinationsFilesProgress.setWindowModality(Qt::WindowModal);
            writtingBandsCombinationsFilesProgress.setLabelText(msgGlobal);
            writtingBandsCombinationsFilesProgress.setWindowTitle(title);
            writtingBandsCombinationsFilesProgress.show();
            writtingBandsCombinationsFilesProgress.adjustSize();
            qApp->processEvents();
            int numberOfBandsCombinatiosnFilesWritted=0;
            for(int nrf=0;nrf<rasterFiles.size();nrf++)
            {
                QString rasterFile=rasterFiles.at(nrf);
                out<<"    - Raster file ..........................: "<<rasterFile<<"\n";
                if(rasterTypesByRasterFile[rasterFile].compare(rasterType,Qt::CaseInsensitive)!=0)
                {
                    continue;
                }
                QMap<QString,QVector<QString> >::const_iterator iterBandsCombinations=bandsCombinations.begin();
                while(iterBandsCombinations!=bandsCombinations.end())
                {
                    QString bandsCombination=iterBandsCombinations.key();
                    out<<"      - Combinacion ........................: "<<bandsCombination<<"\n";
                    bool existsBandsForCombination=true;
                    QVector<QVector<QVector<qint8> > > bandsData;
                    int columns=0;
                    int rows=0;
                    for(int nb=0;nb<iterBandsCombinations.value().size();nb++)
                    {
                        QString bandId=iterBandsCombinations.value().at(nb);
                        if(!combinationsDataByBandByRasterFile.contains(bandId))
                        {
                            existsBandsForCombination=false;
                            break;
                        }
                        else
                        {
                            if(!combinationsDataByBandByRasterFile[bandId].contains(rasterFile))
                            {
                                existsBandsForCombination=false;
                                break;
                            }
                        }
                        if(columns==0)
                        {
                            rows=combinationsDataByBandByRasterFile[bandId][rasterFile].size();
                            columns=combinationsDataByBandByRasterFile[bandId][rasterFile][0].size();
                        }
                        bandsData.push_back(combinationsDataByBandByRasterFile[bandId][rasterFile]);
                    }
                    if(!existsBandsForCombination)
                    {
                        iterBandsCombinations++;
                        continue;
                    }
                    numberOfBandsCombinatiosnFilesWritted++;
                    writtingBandsCombinationsFilesProgress.setValue(numberOfBandsCombinatiosnFilesWritted);
                    if(writtingBandsCombinationsFilesProgress.wasCanceled())
                    {
                        QMessageBox msgBox(mPtrWidgetParent);
                        QString msg=QObject::tr("The Abort button was clicked in process: Removing clouds - Reading files");
                        msgBox.setText(msg);
                        QString question=QObject::tr("Do you want to abort the process and to loss the processed information?");
                        msgBox.setInformativeText(question);
                        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
                        msgBox.setDefaultButton(QMessageBox::Yes);
                        int ret = msgBox.exec();
                        bool abort=false;
                        switch (ret)
                        {
                          case QMessageBox::Yes:
                            abort=true;
                              // Save was clicked
                              break;
                          case QMessageBox::No:
                              break;
                          case QMessageBox::Cancel:
                              break;
                          default:
                              // should never be reached
                              break;
                        }
                        if(abort)
                        {
                            strError=QObject::tr("Process was canceled");
                            resultsFile.close();
                            return(false);
                        }
                    }
                    QString bandsCombinationFileName=tuplekeyPath+"/";
                    bandsCombinationFileName+=ALGORITHMS_CLOUDREMOVAL_PARAMETER_WRITEIMAGEFILES_FOLDER;
                    bandsCombinationFileName+="/";
                    bandsCombinationFileName+=bandsCombinationsFileBaseNameByRasterTypeByRasterFile[rasterType][rasterFile][bandsCombination];
                    QFileInfo bandsCombinationFileInfo(bandsCombinationFileName);
                    QString bandsCombinationFilePath=bandsCombinationFileInfo.absolutePath();
                    if(!auxDir.exists(bandsCombinationFilePath))
                    {
                        if(!auxDir.mkpath(bandsCombinationFilePath))
                        {
                            strError=QObject::tr("\nError making path for removed clouds file:\n%1")
                                    .arg(bandsCombinationFilePath);
                            resultsFile.close();
                            return(false);
                        }
                    }
                    out<<"      - Fichero de salida ..................: "<<bandsCombinationFileName<<"\n";
//                    QImage bandCombinationImage(columns,rows,QImage::Format_RGB32);//QImage::Format_RGB888 );
                    QImage bandCombinationImage(columns,rows,QImage::Format_RGB888 );
                    for(int column=0;column<columns;column++)
                    {
                        for(int row=0;row<rows;row++)
                        {
                            int red=bandsData[0][row][column];
                            int green=bandsData[1][row][column];
                            int blue=bandsData[2][row][column];
                            bandCombinationImage.setPixel(column,row,qRgb(red,green,blue));
                        }
                    }
//                    QString removedCloudsRasterFileName=rasterFileInfo.absolutePath()+"/";
//                    removedCloudsRasterFileName+=ALGORITHMS_CLOUDREMOVAL_PARAMETER_WRITEIMAGEFILES_FOLDER;
//                    removedCloudsRasterFileName+="/";
//                    removedCloudsRasterFileName+=bandId;
//                    QString removedCloudsRasterFilePath=removedCloudsRasterFileName;
                    if(!bandCombinationImage.save(bandsCombinationFileName,"JPG", 100))
//                    if(!bandCombinationImage.save(bandsCombinationFileName))
                    {
                        strError+=QObject::tr("\nError writting band combination image:\n%1").arg(bandsCombinationFileName);
                        resultsFile.close();
                        return(false);
                    }
                    iterBandsCombinations++;
                }
            }
            writtingBandsCombinationsFilesProgress.setValue(numberOfBandCombinationsFiles);
            writtingBandsCombinationsFilesProgress.close();
            iterRasterType++;
        }
        iterTuplekey++;
    }
//    progress.close();
    QDateTime finalDateTime=QDateTime::currentDateTime();
    int initialSeconds=(int)initialDateTime.toTime_t();
    int finalSeconds=(int)finalDateTime.toTime_t();
    int totalDurationSeconds=finalSeconds-initialSeconds;
    double dblTotalDurationSeconds=(double)totalDurationSeconds;
    int durationDays=(int)floor(dblTotalDurationSeconds/60.0/60.0/24.0);
    int durationHours=(int)floor((dblTotalDurationSeconds-durationDays*60.0*60.0*24.0)/60.0/60.0);
    int durationMinutes=(int)floor((dblTotalDurationSeconds-durationDays*60.0*60.0*24.0-durationHours*60.0*60.0)/60.0);
    int durationSeconds=dblTotalDurationSeconds-durationDays*60.0*60.0*24.0-durationHours*60.0*60.0-durationMinutes*60.0;
    {
        QString msgTtime="\n- Process time:\n";
        msgTtime+="  - Start time of the process ......................: ";
        msgTtime+=initialDateTime.toString("yyyy/MM/dd - hh/mm/ss.zzz");
        msgTtime+="\n";
        msgTtime+="  - End time of the process ........................: ";
        msgTtime+=finalDateTime.toString("yyyy/MM/dd - hh/mm/ss.zzz");
        msgTtime+="\n";
        msgTtime+="  - Number of total seconds ........................: ";
        msgTtime+=QString::number(dblTotalDurationSeconds,'f',3);
        msgTtime+="\n";
        msgTtime+="    - Number of days ...............................: ";
        msgTtime+=QString::number(durationDays);
        msgTtime+="\n";
        msgTtime+="    - Number of hours ..............................: ";
        msgTtime+=QString::number(durationHours);
        msgTtime+="\n";
        msgTtime+="    - Number of minutes ............................: ";
        msgTtime+=QString::number(durationMinutes);
        msgTtime+="\n";
        msgTtime+="    - Number of seconds ............................: ";
        msgTtime+=QString::number(durationSeconds,'f',3);
        msgTtime+="\n";
        out<<msgTtime;
    }
    resultsFile.close();
    return(true);
}

bool Algorithms::getAlgorithmsGuiTags(QVector<QString>& algorithmsCodes,
                                      QMap<QString, QString> &algorithmsGuiTags,
                                      QString &strError)
{
    if(!mIsInitialized)
    {
        strError=QObject::tr("Algorithms::getAlgorithmsGuiTags");
        strError+=QObject::tr("Object is not initialized");
        return(false);
    }
    algorithmsCodes=mAlgorithmsCodes;
    algorithmsGuiTags=mAlgorithmsGuiTagsByCode;
    return(true);
}

bool Algorithms::getParameterValue(QString algorithm,
                                   QString code,
                                   QString &value,
                                   QString &strError)
{
    if(mPtrParametersManager==NULL)
    {
        strError=QObject::tr("Algorithms::getParameterValue, parameters is NULL");
        return(false);
    }
    return(mPtrParametersManager->getParameterValue(algorithm,code,value,strError));
}

bool Algorithms::getParametersTagAndValues(QString command,
                                           QVector<QString> &codes,
                                           QVector<QString> &tags,
                                           QVector<QString> &values,
                                           QString &strError)
{
    if(mPtrParametersManager==NULL)
    {
        strError=QObject::tr("Algorithms::getParametersTagAndValues, parameters is NULL");
        return(false);
    }
    return(mPtrParametersManager->getParametersTagAndValues(command,codes,tags,values,strError));
}

bool Algorithms::initialize(QString libPath,
                            QString &strError)
{
    mIsInitialized=false;
    QString parametersFileName=libPath+"/"+LIB_REMOTE_SENSING_ALGORITHMS_PARAMETERS_FILE_NAME;
    QString strAuxError;
    if(!setParametersManager(parametersFileName,strAuxError))
    {
        strError=QObject::tr("Algorithms::initialize");
        strError+=QObject::tr("Error:\n%1").arg(strAuxError);
        return(false);
    }
    if((!setAlgorithms(strAuxError)))
    {
        strError=QObject::tr("Algorithms::initialize");
        strError+=QObject::tr("Error:\n%1").arg(strAuxError);
        return(false);
    }
    mIsInitialized=true;
    return(mIsInitialized);
}

bool Algorithms::intercalibrationComputation(QVector<QString> &rasterFiles,
                                             QMap<QString, QString> &rasterTypesByRasterFile,
                                             QMap<QString, QVector<QString> > &rasterFilesByTuplekey,
                                             QMap<QString, QMap<QString, QMap<QString, QString> > > &tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand,
                                             QMap<QString, int> &jdByRasterFile,
                                             QMap<QString, double> &sunAzimuthByRasterFile,
                                             QMap<QString, double> &sunElevationByRasterFile,
                                             QMap<QString, QMap<QString, double> > &reflectanceAddValueByRasterFileAndByBand,
                                             QMap<QString, QMap<QString, double> > &reflectanceMultValueByRasterFileAndByBand,
                                             QVector<int> &piasFilesIds,
                                             QMap<int, QString> &piasFileNameByPiasFilesId,
                                             QMap<int, int> &piasValueByPiasFilesId,
                                             QMap<int, QString> &piasTuplekeyByPiasFilesId,
                                             QMap<int, QVector<QString> > &rasterFilesByPiasFilesIds,
                                             bool mergeFiles,
                                             bool reprocessFiles,
                                             bool reprojectFiles,
                                             QFile &resultsFile,
                                             int mainScenceJd,
                                             QString &strError)
{
    bool debugMode=true;
    bool printDetail=false;
    bool printLsData=true;
    QString landsat8IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8;
    QString sentinel2IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2;
    QString orthoimageIdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_ORTHOIMAGE;
    QString ndviRasterUnitConversion=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_NONE;
    QString strAuxError;
    // Obtener la ruta del workspace
    QMap<QString, QVector<QString> >::const_iterator iterRasterFilesByQuadkey=rasterFilesByTuplekey.begin();
    QString workspaceBasePath;
    // Ojo, esto funciona si están las bandas de ndvi, que se supone estarán siempre
    while(iterRasterFilesByQuadkey!=rasterFilesByTuplekey.end())
    {
        QString quadkey=iterRasterFilesByQuadkey.key();
        QVector<QString> rasterFilesInQuadkey=iterRasterFilesByQuadkey.value();
        int numberOfRasterFilesInQuadkey=rasterFilesInQuadkey.size();
        for(int nrf=0;nrf<numberOfRasterFilesInQuadkey;nrf++)
        {
            QString rasterFile=rasterFilesInQuadkey[nrf];
            QString rasterType=rasterTypesByRasterFile[rasterFile];
            if(rasterType.compare(orthoimageIdDb)==0)
            {
                continue;
            }
            if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.contains(quadkey))
            {
                continue;
            }
            if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey].contains(rasterFile))
            {
                continue;
            }
            QString redBandRasterFile,nirBandRasterFile;
            bool validRasterType=false;
            if(rasterType.compare(landsat8IdDb)==0)
            {
                if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_LANDSAT8_BAND_B4_CODE)
                        ||!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_LANDSAT8_BAND_B5_CODE))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                redBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_LANDSAT8_BAND_B4_CODE];
                nirBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_LANDSAT8_BAND_B5_CODE];
                validRasterType=true;
            }
            if(rasterType.compare(sentinel2IdDb)==0)
            {
                if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_SENTINEL2_BAND_B4_CODE)
                        ||!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_SENTINEL2_BAND_B8_CODE))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                redBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_SENTINEL2_BAND_B4_CODE];
                nirBandRasterFile=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_SENTINEL2_BAND_B8_CODE];
                validRasterType=true;
            }
            if(!validRasterType)
            {
                strError=QObject::tr("Algorithms::intercalibrationComputation");
                strError+=QObject::tr("\nInvalid raste type: %1\nfor raster file: %2")
                        .arg(rasterType).arg(rasterFile);
                return(false);
            }
            if(QFile::exists(redBandRasterFile))
            {
                QFileInfo redBandRasterFileInfo(redBandRasterFile);
                QDir redBandDir=redBandRasterFileInfo.absoluteDir();
                redBandDir.cdUp();
                workspaceBasePath=redBandDir.absolutePath();
                break;
            }
            else if(QFile::exists(nirBandRasterFile))
            {
                QFileInfo nirBandRasterFileInfo(nirBandRasterFile);
                QDir nirBandDir=nirBandRasterFileInfo.absoluteDir();
                nirBandDir.cdUp();
                workspaceBasePath=nirBandDir.absolutePath();
                break;
            }
        }
        if(!workspaceBasePath.isEmpty())
        {
            iterRasterFilesByQuadkey=rasterFilesByTuplekey.end();
        }
        else
        {
            iterRasterFilesByQuadkey++;
        }
    }

//    QString computationMethod;
//    QString strMinPiasPixels;
    double gainMinValue,gainMaxValue,offsetMinValue,offsetMaxValue;
    int minPiasPixels,cloudValue,minBandsPixels;
    QString strTo8Bits,strToReflectance,strWriteImageFiles,strWriteImageFilesTo8Bits;
    QString strRemoveOutliers;
    bool to8Bits=false;
    bool writeImageFiles=false;
    bool writeImageFilesTo8Bits=false;
    bool removeOutliers=false;
    bool toReflectance=false;
    QString interpolationMethod;
    QString strWeightRefereceSceneEquations;
    double weightReferenceSceneEquations=1.0;
//    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_COMPUTATION_METHOD)->getValue(computationMethod);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_MIN_PIAS_PIXELS)->getValue(minPiasPixels);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_MIN_BANDS_PIXELS)->getValue(minBandsPixels);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_GAIN)->getMinValue(gainMinValue);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_GAIN)->getMaxValue(gainMaxValue);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_OFFSET)->getMinValue(offsetMinValue);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_OFFSET)->getMaxValue(offsetMaxValue);
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_PIA_CLOUD_VALUE)->getValue(cloudValue);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_TO8BITS)->getValue(strTo8Bits);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILES)->getValue(strWriteImageFiles);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILESTO8BITS)->getValue(strWriteImageFilesTo8Bits);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_REMOVE_OUTLIERS)->getValue(strRemoveOutliers);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_TOREFLECTANCE)->getValue(strToReflectance);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_INTERPOLATION_METHOD)->getValue(interpolationMethod);
    mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_WEIGHT_REF)->getValue(strWeightRefereceSceneEquations);
    if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)!=0
            &&interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)!=0
            &&interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)!=0)
    {
        strError=QObject::tr("Algorithms::intercalibrationComputation");
        strError+=QObject::tr("\nInvalid interpolation method: %1").arg(interpolationMethod);
        return(false);
    }
    if(mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_TO8BITS)->isEnabled())
    {
        if(strTo8Bits.compare("Yes",Qt::CaseInsensitive)==0)
        {
            to8Bits=true;
        }
    }
    if(mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILES)->isEnabled())
    {
        if(strWriteImageFiles.compare("Yes",Qt::CaseInsensitive)==0)
        {
            writeImageFiles=true;
        }
    }
    if(mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILESTO8BITS)->isEnabled())
    {
        if(strWriteImageFilesTo8Bits.compare("Yes",Qt::CaseInsensitive)==0)
        {
            writeImageFilesTo8Bits=true;
            if(!mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILES)->isEnabled())
            {
                strError=QObject::tr("Algorithms::intercalibrationComputation");
                strError+=QObject::tr("\nWrite images disabled but write to 8 bits enabled and true");
                return(false);
            }
        }
    }
    if(mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_TOREFLECTANCE)->isEnabled())
    {
        if(strToReflectance.compare("Yes",Qt::CaseInsensitive)==0)
        {
            toReflectance=true;
            if(to8Bits)
            {
                strError=QObject::tr("Algorithms::intercalibrationComputation");
                strError+=QObject::tr("\nInvalid option: to reflectance ant to 8 bits");
                return(false);
            }
        }
    }
    if( mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_REMOVE_OUTLIERS)->isEnabled())
    {
        if(strRemoveOutliers.compare("Yes",Qt::CaseInsensitive)==0)
        {
            removeOutliers=true;
        }
    }
//    if(piasComputationMethod.compare(ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD_1)!=0)
//    {
//        strError=QObject::tr("Algorithms::intercalibrationComputation");
//        strError+=QObject::tr("\nParameter value for pias computation method: %1\n is not it is not implemented yet")
//                .arg(piasComputationMethod);
//        return(false);
//    }
    bool okToDouble=true;
    if(mPtrParametersManager->getParameter(ALGORITHMS_INTC_PARAMETER_WEIGHT_REF)->isEnabled())
    {
        weightReferenceSceneEquations=strWeightRefereceSceneEquations.toDouble(&okToDouble);
        if(!okToDouble)
        {
            strError=QObject::tr("Algorithms::intercalibrationComputation");
            strError+=QObject::tr("\nInvalid weight for equations of reference scence: %1").arg(strWeightRefereceSceneEquations);
            return(false);
        }
    }
    QString title=QObject::tr("Computing Intercalibration");
    QString msgGlobal=QObject::tr("Processing %1 pias files ...").arg(QString::number(piasFilesIds.size()));
    QProgressDialog progress(title, QObject::tr("Abort"),0,piasFilesIds.size(), mPtrWidgetParent);
    progress.setWindowModality(Qt::WindowModal);
    progress.setLabelText(msgGlobal);
    progress.show();
    qApp->processEvents();
    if (!resultsFile.open(QFile::Append |QFile::Text))
    {
        strError=QObject::tr("Algorithms::intercalibrationComputation");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(resultsFile.fileName());
        return(false);
    }
    QTextStream out(&resultsFile);
    out<<"- Parametros de procesamiento:\n";
    out<<"  - Paso a 8 bits ..........................: ";
    if(to8Bits)
    {
        out<<"Si\n";
    }
    else
    {
        out<<"No\n";
    }
    out<<"  - Paso a reflectancia ....................: ";
    if(toReflectance)
    {
        out<<"Si\n";
    }
    else
    {
        out<<"No\n";
    }
    out<<"  - Eliminar errores groseros ..............: ";
    if(removeOutliers)
    {
        out<<"Si\n";
    }
    else
    {
        out<<"No\n";
    }
    out<<"  - Minimo numero de pixel en PIAS .........: ";
    out<<QString::number(minPiasPixels)<<"\n";
    out<<"  - Minimo numero de pixel en bandas .......: ";
    out<<QString::number(minBandsPixels)<<"\n";
    out<<"  - Fecha manual de referencia .............: ";
    if(mainScenceJd!=-1)
    {
        out<<QDate::fromJulianDay(mainScenceJd).toString(ALGORITHMS_DATE_STRING_FORMAT)<<"\n";
    }
    else
    {
        out<<"Se obtiene de forma automatica\n";
    }
    out<<"  - Metodo de interpolacion ................: "<<interpolationMethod<<"\n";
    out<<"  - Peso para ecuaciones de referencia .....: "<<QString::number(weightReferenceSceneEquations,'f',2)<<"\n";
    out<<"  - Escribir ficheros resultantes ..........: ";
    if(writeImageFiles)
    {
        out<<"Si\n";
        out<<"    - Escribir en 8 bits ...................: ";
        if(writeImageFilesTo8Bits)
        {
            out<<"Si\n";
        }
        else
        {
            out<<"No\n";
        }
    }
    else
    {
        out<<"No\n";
    }
    out<<"- Numero de ficheros de PIAS a procesar ....: "<<QString::number(piasFilesIds.size())<<"\n";
    IGDAL::ImageTypes piasImageType=IGDAL::GEOTIFF;
    GDALDataType piasGdalDataType=ALGORITHMS_PIAS_GDAL_DATA_TYPE;
    GDALDataType to8BitsGDalDataType=GDT_Byte;
    QVector<int> years;
    resultsFile.close();
    double pi=4.0*atan(1.0);

    // Para cada banda tengo un contenedor escenas (identificadas por su rasterFile),
    // que contiene tuplekeys, que contiene un vector
    // con los píxeles comunes donde: no hay nube, no hay no data value
    // para cada pixel almaceno dos valores, el valor en cada una de las dos escenas
    QMap<QString,QMap<QString,QMap<QString,QMap<int,QMap<int,float> > > > > valuesByColumnByRowByTuplekeyBySceneByBand;
    // Para cada banda tengo un contenedor de escenas donde almaceno el número de píxeles que intervienen en el contenedor anterior
    QMap<QString,QMap<QString,int> > iDataNumberOfPixelsBySceneByBand;
//    for(int np=0;np<9;np++)
    for(int np=0;np<piasFilesIds.size();np++)
    {
        progress.setValue(np);
        qApp->processEvents();
        if (progress.wasCanceled())
            break;
        if (!resultsFile.open(QFile::Append |QFile::Text))
        {
            strError=QObject::tr("Algorithms::intercalibrationComputation");
            strError+=QObject::tr("\nError opening results file: \n %1").arg(resultsFile.fileName());
            return(false);
        }
        QTextStream out(&resultsFile);
        int piasFileId=piasFilesIds[np];
        QString piasFileName=piasFileNameByPiasFilesId[piasFileId];
        int piaValue=piasValueByPiasFilesId[piasFileId];
        int piasColumns,piasRows;
        if(printDetail)
        {
            out<<"- Fichero de PIAS ..........................: "<<piasFileName<<"\n";
            out<<"  Numero de fichero ........................: "<<QString::number(np+1)<<"\n";
        }
        IGDAL::Raster* ptrPiasRasterFile=new IGDAL::Raster(mPtrCrsTools);
        if(!ptrPiasRasterFile->setFromFile(piasFileName,strAuxError))
        {
            strError=QObject::tr("Algorithms::intercalibrationComputation");
            strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                    .arg(piasFileName).arg(strAuxError);
            resultsFile.close();
            return(false);
        }
        QVector<double> piasGeoref;
        if(!ptrPiasRasterFile->getGeoRef(piasGeoref,
                                         strError))
        {
            strError=QObject::tr("Algorithms::intercalibrationComputation");
            strError+=QObject::tr("\nError getting georeferencing from raster:\n%1\nError:\n%2").arg(piasFileName).arg(strError);
            resultsFile.close();
            return(false);
        }
        double piasColumnGsd=piasGeoref[1];
        double piasRowGsd=fabs(piasGeoref[5]);
        if(!ptrPiasRasterFile->getSize(piasColumns,piasRows,strAuxError))
        {
            strError=QObject::tr("Algorithms::intercalibrationComputation");
            strError+=QObject::tr("\nError getting size fro raster file:\n%1\nError:\n%2")
                    .arg(piasFileName).arg(strAuxError);
            resultsFile.close();
            return(false);
        }
        int numberOfBand=0;
        int initialColumn=0;
        int initialRow=0;
        int columnsToRead=piasColumns;
        int rowsToRead=piasRows;
//        int numberOfPixels=columnsToRead*rowsToRead;
        int* pIntData;
        if(!ptrPiasRasterFile->readValues(numberOfBand, // desde 0
                                          initialColumn,
                                          initialRow,
                                          columnsToRead,
                                          rowsToRead,
                                          pIntData,
                                          strError))
        {
            strError=QObject::tr("Algorithms::intercalibrationComputation");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(piasFileName).arg(strAuxError);
            resultsFile.close();
            return(false);
        }
        QMap<int,QVector<int> > piasPixelsByRow;
        int numberOfPiasPixels=0;
        for(int row=0;row<piasRows;row++)
        {
            for(int column=0;column<piasColumns;column++)
            {
                int posInData=row*piasColumns+column;
                int intValue=pIntData[posInData];
                if(intValue==piaValue)
                {
                    if(!piasPixelsByRow.contains(row))
                    {
                        QVector<int> aux;
                        piasPixelsByRow[row]=aux;
                    }
                    piasPixelsByRow[row].push_back(column);
                    numberOfPiasPixels++;
                }
            }
        }
        free(pIntData);
        delete(ptrPiasRasterFile);
        QString tuplekey=piasTuplekeyByPiasFilesId[piasFileId];
        QVector<QString> rasterfilesInPias=rasterFilesByPiasFilesIds[piasFileId];
        if(printDetail)
        {
            out<<"  - Tuplekey ...............................: "<<tuplekey<<"\n";
            out<<"  - Valor de PIAS ..........................: "<<QString::number(piaValue)<<"\n";
            out<<"  - Numero de pixels con valor de PIAS .....: "<<QString::number(numberOfPiasPixels)<<"\n";
            out<<"  - Numero de ficheros raster ..............: "<<QString::number(rasterfilesInPias.size())<<"\n";
        }
        QMap<QString,bool> existsMaskBandByRasterFile;
        QMap<QString,QVector<double> > maskBandGeorefByRasterFile;
        QMap<QString,QVector<QVector<bool> > > maskBandValuesByRasterFile;
        QMap<QString,QMap<QString,QVector<double> > > rasterGeorefByBandByRasterFile;
        QMap<QString,QMap<QString,QVector<QVector<float> > > > rasterValuesByRowByBandByRasterFile;
        QMap<QString,QMap<QString,double> > noDataValueByBandByRasterFile;
        for(int nr=0;nr<rasterfilesInPias.size();nr++)
        {
            QString rasterFile=rasterfilesInPias[nr];
            if(!rasterTypesByRasterFile.contains(rasterFile))
                continue;
            double sunElevation;
            existsMaskBandByRasterFile[rasterFile]=false;
            QString rasterType=rasterTypesByRasterFile[rasterFile];
            QString maskBandCode;
            if(rasterType.compare(landsat8IdDb)==0)
            {
                maskBandCode=REMOTESENSING_LANDSAT8_BAND_B0_CODE;
                if(toReflectance)
                {
                    if(!sunElevationByRasterFile.contains(rasterFile))
                    {
                        strError=QObject::tr("Algorithms::intercalibrationComputation");
                        strError+=QObject::tr("\nThere is no Sun elevation for raster file:\n%1")
                                .arg(rasterFile);
                        resultsFile.close();
                        return(false);
                    }
                    sunElevation=sunElevationByRasterFile[rasterFile];
                }
            }
            if(rasterType.compare(sentinel2IdDb)==0)
            {
                maskBandCode=REMOTESENSING_SENTINEL2_BAND_B0_CODE;
            }
            if(!jdByRasterFile.contains(rasterFile))
                continue;
            int jd=jdByRasterFile[rasterFile];
            int year=QDate::fromJulianDay(jd).year();
            if(years.indexOf(year)==-1)
                years.push_back(year);
            if(printDetail)
            {
                out<<"    - Fichero raster .......................: "<<rasterFile<<"\n";
            }
            if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.contains(tuplekey))
                continue;
            if(!tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[tuplekey].contains(rasterFile))
                continue;
            QMap<QString, QString> rasterFileNamesByBand=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[tuplekey][rasterFile];
            if(printDetail)
            {
                out<<"      - Numero de bandas ...................: "<<QString::number(rasterFileNamesByBand.size())<<"\n";
            }
            QMap<QString, QString>::const_iterator iter1=rasterFileNamesByBand.begin();
            QVector<double> maskBandGeoref;
            QVector<QVector<bool> > maskBandValues;
            QMap<QString,QVector<double> > rasterGeorefByBand;
            QMap<QString,QVector<QVector<float> > > rasterValuesByRowByBand;
            QMap<QString,double> noDataValueByBand;
            while(iter1!=rasterFileNamesByBand.end())
            {
                QString bandId=iter1.key();
                if(bandId.compare(maskBandCode)==0)
                {
                    iter1++;
                    continue;
                }
                double addValue,multValue;
                if(rasterType.compare(landsat8IdDb)==0)
                {
                    if(toReflectance)
                    {
                        if(!reflectanceAddValueByRasterFileAndByBand.contains(rasterFile))
                        {
                            strError=QObject::tr("Algorithms::intercalibrationComputation");
                            strError+=QObject::tr("\nThere is no reflectance add value for raster file:\n%12")
                                    .arg(rasterFile);
                            resultsFile.close();
                            return(false);
                        }
                        if(!reflectanceAddValueByRasterFileAndByBand[rasterFile].contains(bandId))
                        {
                            strError=QObject::tr("Algorithms::intercalibrationComputation");
                            strError+=QObject::tr("\nThere is no reflectance add value for band: %1 in raster file:\n%2")
                                    .arg(bandId).arg(rasterFile);
                            resultsFile.close();
                            return(false);
                        }
                        addValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId];
                        if(!reflectanceMultValueByRasterFileAndByBand.contains(rasterFile))
                        {
                            strError=QObject::tr("Algorithms::intercalibrationComputation");
                            strError+=QObject::tr("\nThere is no reflectance mult value for raster file:\n%12")
                                    .arg(rasterFile);
                            resultsFile.close();
                            return(false);
                        }
                        if(!reflectanceMultValueByRasterFileAndByBand[rasterFile].contains(bandId))
                        {
                            strError=QObject::tr("Algorithms::intercalibrationComputation");
                            strError+=QObject::tr("\nThere is no reflectance mult value for band: %1 in raster file:\n%2")
                                    .arg(bandId).arg(rasterFile);
                            resultsFile.close();
                            return(false);
                        }
                        multValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][REMOTESENSING_LANDSAT8_BAND_B4_CODE];
                    }
                }
                QString rasterBandFileName=iter1.value();
                // Para testear la escritura
//                {
//                    QString rasterFileName=rasterBandFileName;
//                    QFileInfo rasterFileInfo(rasterFileName);
//                    QString intercalibratedRasterFileName=rasterFileInfo.absolutePath()+"/";
//                    intercalibratedRasterFileName+=ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILES_FOLDER;
//                    QString intercalibratedRasterFilePath=intercalibratedRasterFileName;
//                    intercalibratedRasterFileName+=("/"+rasterFileInfo.fileName());
//                    QDir rasterFileDir(rasterFileInfo.absolutePath());
//                    if(!rasterFileDir.exists(intercalibratedRasterFilePath))
//                    {
//                        if(!rasterFileDir.mkpath(intercalibratedRasterFilePath))
//                        {
//                            strError=QObject::tr("Algorithms::intercalibrationComputation");
//                            strError+=QObject::tr("\nError making path for intercalibrated file:\n%1")
//                                    .arg(intercalibratedRasterFilePath);
//                            return(false);
//                        }
//                    }
//                    QString auxStrError;
//                    double gain=1.0;
//                    double offset=0;
//                    if(!applyIntercalibration(rasterFileName,
//                                              intercalibratedRasterFileName,writeImageFilesTo8Bits,
//                                              gain,offset,to8Bits,
//                                              toReflectance,multValue,addValue,
//                                              auxStrError))
//                    {
//                        strError=QObject::tr("Algorithms::intercalibrationComputation");
//                        strError+=QObject::tr("\nError applying intercalibration to file:\n%1\nError:\n%2")
//                                .arg(rasterFileName).arg(auxStrError);
//                        return(false);
//                    }
//                }
                QFileInfo rasterBandFileInfo(rasterBandFileName);
                QString maskBandFileName=rasterBandFileInfo.absolutePath()+"//"+rasterFile+"_"+maskBandCode+".tif";
                if(QFile::exists(maskBandFileName)&&!existsMaskBandByRasterFile[rasterFile])
                {
                    float cloudValuesPercentage;
                    if(!readMaskRasterFile(maskBandFileName,
                                           cloudValue,
                                           maskBandGeoref,
                                           maskBandValues,
                                           cloudValuesPercentage,
                                           strAuxError))
                    {
                        strError=QObject::tr("Algorithms::intercalibrationComputation");
                        strError+=QObject::tr("\nError reading mask band raster file:\n%1\nError:\n%2")
                                .arg(maskBandFileName).arg(strAuxError);
                        resultsFile.close();
                        return(false);
                    }
                    existsMaskBandByRasterFile[rasterFile]=true;
                }
                IGDAL::Raster* ptrRasterBandFile=new IGDAL::Raster(mPtrCrsTools);
                if(!ptrRasterBandFile->setFromFile(rasterBandFileName,strAuxError))
                {
                    strError=QObject::tr("Algorithms::intercalibrationComputation");
                    strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                            .arg(rasterBandFileName).arg(strAuxError);
                    resultsFile.close();
                    return(false);
                }
                double rasterBandNoDataValue;
                if(!ptrRasterBandFile->getNoDataValue(0,rasterBandNoDataValue,strAuxError))
                {
                    strError=QObject::tr("Algorithms::intercalibrationComputation");
                    strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                            .arg(rasterBandFileName).arg(strAuxError);
                    resultsFile.close();
                    return(false);
                }
                QVector<double> georef;
                if(!ptrRasterBandFile->getGeoRef(georef,
                                             strError))
                {
                    strError=QObject::tr("Algorithms::intercalibrationComputation");
                    strError+=QObject::tr("\nError getting georeferencing from raster:\n%1\nError:\n%2").arg(rasterBandFileName).arg(strError);
                    resultsFile.close();
                    return(false);
                }
                int rasterBandColumns,rasterBandRows;
                if(!ptrRasterBandFile->getSize(rasterBandColumns,rasterBandRows,strAuxError))
                {
                    strError=QObject::tr("Algorithms::intercalibrationComputation");
                    strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                            .arg(rasterBandFileName).arg(strAuxError);
                    return(false);
                }
                int numberOfBand=0;
                int rasterBandInitialColumn=0;
                int rasterBandInitialRow=0;
                int rasterBandColumnsToRead=rasterBandColumns;
                int rasterBandRowsToRead=rasterBandRows;
                int numberOfPixels=rasterBandColumnsToRead*rasterBandRowsToRead;
                float* rasterBandData;//=(float*)malloc(numberOfPixels*sizeof(GDT_Float32));
            //    =(GDT_Float32 *) CPLMalloc(numberOfPixels*sizeof(GDT_Float32));
                if(!ptrRasterBandFile->readValues(numberOfBand, // desde 0
                                                 rasterBandInitialColumn,
                                                 rasterBandInitialRow,
                                                 rasterBandColumnsToRead,
                                                 rasterBandRowsToRead,
                                                 rasterBandData,
                                                 strAuxError))
                {
                    strError=QObject::tr("Algorithms::intercalibrationComputation");
                    strError+=QObject::tr("\nError reading values from raster file:\n%1\nError:\n%2")
                            .arg(rasterBandFileName).arg(strAuxError);
                    return(false);
                }
                double to8BitsFactor=1.0;
                if(to8Bits)
                {
                    if(!ptrRasterBandFile->getFactorTo8Bits(to8BitsFactor,strAuxError))
                    {
                        strError=QObject::tr("Algorithms::intercalibrationComputation");
                        strError+=QObject::tr("\nError getting factor to 8 bits from raster file:\n%1\nError:\n%2")
                                .arg(rasterBandFileName).arg(strAuxError);
                        return(false);
                    }
                }
                delete(ptrRasterBandFile);
                QVector<QVector<float> > rasterBandValues;
                rasterBandValues.resize(rasterBandRows);
                for(int row=0;row<rasterBandRows;row++)
                {
                    rasterBandValues[row].resize(rasterBandColumns);
                    for(int column=0;column<rasterBandColumns;column++)
                    {
                        int posInData=row*rasterBandColumns+column;
                        float rasterBandValue=rasterBandData[posInData];
                        if(fabs(rasterBandValue-rasterBandNoDataValue)<0.01)
                        {
                            rasterBandValue=rasterBandNoDataValue;
                        }
                        else
                        {
                            if(toReflectance
                                    &&rasterType.compare(landsat8IdDb)==0)
                            {
                                rasterBandValue=1.0/sin(sunElevation*pi/180.)*(rasterBandValue*multValue+addValue);
                            }
                            rasterBandValue*=to8BitsFactor;
                        }
                        rasterBandValues[row][column]=rasterBandValue;
                    }
                }
                free(rasterBandData);
                rasterValuesByRowByBand[bandId]=rasterBandValues;
                noDataValueByBand[bandId]=rasterBandNoDataValue;
                rasterGeorefByBand[bandId]=georef;
                iter1++;
            }
            maskBandGeorefByRasterFile[rasterFile]=maskBandGeoref;
            maskBandValuesByRasterFile[rasterFile]=maskBandValues;
            rasterGeorefByBandByRasterFile[rasterFile]=rasterGeorefByBand;
            rasterValuesByRowByBandByRasterFile[rasterFile]=rasterValuesByRowByBand;
            noDataValueByBandByRasterFile[rasterFile]=noDataValueByBand;
        }
        QMap<int,QVector<int> >::const_iterator iterPiasPixelsByRow=piasPixelsByRow.begin();
        while(iterPiasPixelsByRow!=piasPixelsByRow.end())
        {
            int piaRow=iterPiasPixelsByRow.key();
            for(int nc=0;nc<iterPiasPixelsByRow.value().size();nc++)
            {
                int piaColumn=iterPiasPixelsByRow.value()[nc];
                QMap<QString,QMap<QString,QVector<QVector<float> > > >::const_iterator iterRasterValuesByRowByBandByRasterFile=rasterValuesByRowByBandByRasterFile.begin();
                while(iterRasterValuesByRowByBandByRasterFile!=rasterValuesByRowByBandByRasterFile.end())
                {
                    QString rasterFile=iterRasterValuesByRowByBandByRasterFile.key();
                    // Si hay nube se ignoran todas las bandas
                    bool existsCloud=false;
                    if(existsMaskBandByRasterFile[rasterFile])
                    {
                        double maskColumnGsd=maskBandGeorefByRasterFile[rasterFile][1];
                        if(fabs(maskColumnGsd-piasColumnGsd)<0.01)
                        {
                            if(maskBandValuesByRasterFile[rasterFile][piaRow][piaColumn])
                            {
                                existsCloud=true;
                            }
                        }
                        else
                        {
                            double gsdMaskRatio=piasColumnGsd/maskColumnGsd;
                            if(piasColumnGsd>maskColumnGsd)
                            {
                                int firstMaskRow=qRound(piaRow*gsdMaskRatio);
                                int lastMaskRow=qRound((piaRow+1)*gsdMaskRatio-1);
                                int firstMaskColumn=qRound(piaColumn*gsdMaskRatio);
                                int lastMaskColumn=qRound((piaColumn+1)*gsdMaskRatio-1);
                                for(int rr=firstMaskRow;rr<lastMaskRow;rr++)
                                {
                                    for(int cc=firstMaskColumn;cc<lastMaskColumn;cc++)
                                    {
                                        if(maskBandValuesByRasterFile[rasterFile][rr][cc])
                                        {
                                            existsCloud=true;
                                            break;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                int maskRow=floor(piaRow*gsdMaskRatio);
                                int maskColumn=floor(piaColumn*gsdMaskRatio);
                                if(maskBandValuesByRasterFile[rasterFile][maskRow][maskColumn])
                                {
                                    existsCloud=true;
                                }
                            }
                        }
                    }
                    if(existsCloud)
                    {
                        iterRasterValuesByRowByBandByRasterFile++;
                        continue;
                    }
                    QMap<QString,QVector<QVector<float> > >::const_iterator iterRasterValuesByRowByBand=iterRasterValuesByRowByBandByRasterFile.value().begin();
                    while(iterRasterValuesByRowByBand!=iterRasterValuesByRowByBandByRasterFile.value().end())
                    {
                        QString bandId=iterRasterValuesByRowByBand.key();
                        double noDataValue=noDataValueByBandByRasterFile[rasterFile][bandId];
                        QVector<QVector<float> > rasterFileValues=iterRasterValuesByRowByBand.value();
                        double rasterFileColumnGsd=rasterGeorefByBandByRasterFile[rasterFile][bandId][1];
                        QVector<int> valuesColumns;
                        QVector<int> valuesRows;
                        QVector<float> values;
                        if(fabs(rasterFileColumnGsd-piasColumnGsd)<0.01)
                        {
                            if(fabs(rasterFileValues[piaRow][piaColumn]-noDataValue)>0.01)
                            {
                                valuesRows.push_back(piaRow);
                                valuesColumns.push_back(piaColumn);
                                values.push_back(rasterFileValues[piaRow][piaColumn]);
                            }
                        }
                        else
                        {
                            double gsdRatio=piasColumnGsd/rasterFileColumnGsd;
                            if(piasColumnGsd>rasterFileColumnGsd)
                            {
                                int firstRow=qRound(piaRow*gsdRatio);
                                int lastRow=qRound((piaRow+1)*gsdRatio-1);
                                int firstColumn=qRound(piaColumn*gsdRatio);
                                int lastColumn=qRound((piaColumn+1)*gsdRatio-1);
                                for(int rr=firstRow;rr<lastRow;rr++)
                                {
                                    for(int cc=firstColumn;cc<lastColumn;cc++)
                                    {
                                        if(fabs(rasterFileValues[rr][cc]-noDataValue)>0.01)
                                        {
                                            valuesRows.push_back(rr);
                                            valuesColumns.push_back(cc);
                                            values.push_back(rasterFileValues[rr][cc]);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                int rasterFileRow=floor(piaRow*gsdRatio);
                                int rasterFileColumn=floor(piaColumn*gsdRatio);
                                if(fabs(rasterFileValues[rasterFileRow][rasterFileColumn]-noDataValue)>0.01)
                                {
                                    valuesRows.push_back(rasterFileRow);
                                    valuesColumns.push_back(rasterFileColumn);
                                    values.push_back(rasterFileValues[rasterFileRow][rasterFileColumn]);
                                }
                            }
                        }
                        if(values.size()>0)
                        {
                            for(int nv=0;nv<values.size();nv++)
                            {
                                valuesByColumnByRowByTuplekeyBySceneByBand[bandId][rasterFile][tuplekey][valuesRows[nv]][valuesColumns[nv]]=values[nv];
                                if(!iDataNumberOfPixelsBySceneByBand.contains(bandId))
                                {
                                    iDataNumberOfPixelsBySceneByBand[bandId][rasterFile]=0;
                                }
                                iDataNumberOfPixelsBySceneByBand[bandId][rasterFile]=iDataNumberOfPixelsBySceneByBand[bandId][rasterFile]+1;
                            }
                        }
                        iterRasterValuesByRowByBand++;
                    }
                    iterRasterValuesByRowByBandByRasterFile++;
                }
            }
            iterPiasPixelsByRow++;
        }
        resultsFile.close();
    }
    progress.setValue(piasFilesIds.size());
    progress.close();
    qApp->processEvents();

    if (!resultsFile.open(QFile::Append |QFile::Text))
    {
        strError=QObject::tr("Algorithms::intercalibrationComputation");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(resultsFile.fileName());
        return(false);
    }
    QTextStream out2(&resultsFile);

    title=QObject::tr("Computing Intercalibration");
    msgGlobal=QObject::tr("Recovering values by band ...").arg(QString::number(iDataNumberOfPixelsBySceneByBand.size()));
    QProgressDialog progress2(title, QObject::tr("Abort"),0,piasFilesIds.size(), mPtrWidgetParent);
    progress2.setWindowModality(Qt::WindowModal);
    progress2.setLabelText(msgGlobal);
    progress2.show();
    qApp->processEvents();
    QMap<QString,QMap<QString,int> >::const_iterator iterBand=iDataNumberOfPixelsBySceneByBand.begin();
    out2<<"- Proceso para determinar la escena candidata como base para la intercalibracion\n";
    out2<<"    Band                    RasterFile    NoPixels   NoDays_SS\n";
    QMap<QString,int> numberOBandsBelowMinimumPixelsByScene;
    QMap<QString,int> jdsToSummerSolsticeByScene;
    int contBand=0;
    while(iterBand!=iDataNumberOfPixelsBySceneByBand.end())
    {
        contBand++;
        progress2.setValue(contBand);
        qApp->processEvents();
        if (progress2.wasCanceled())
            break;
        QString bandId=iterBand.key();
        QMap<QString,int> iDataNumberOfPixelsByScene=iterBand.value();
        QMap<QString,int>::const_iterator iterRasterFile=iDataNumberOfPixelsByScene.begin();
        while(iterRasterFile!=iDataNumberOfPixelsByScene.end())
        {
            QString rasterFile=iterRasterFile.key();
            int jd=jdByRasterFile[rasterFile];
            QDate sceneDate=QDate::fromJulianDay(jd);
            int jdSummerSolstice=QDate(sceneDate.year(),6,21).toJulianDay();
            int jdInterval=jd-jdSummerSolstice;
            out2<<bandId.rightJustified(8);
            out2<<rasterFile.rightJustified((30));
            out2<<QString::number(iterRasterFile.value()).rightJustified(12);
            out2<<QString::number(jdInterval).rightJustified(12);
            out2<<"\n";
            if(!jdsToSummerSolsticeByScene.contains(rasterFile))
            {
                jdsToSummerSolsticeByScene[rasterFile]=jdInterval;
            }
            if(iterRasterFile.value()<minPiasPixels)
            {
                if(!numberOBandsBelowMinimumPixelsByScene.contains(rasterFile))
                {
                    numberOBandsBelowMinimumPixelsByScene[rasterFile]=0;
                }
                numberOBandsBelowMinimumPixelsByScene[rasterFile]++;
            }
            iterRasterFile++;
        }
        iterBand++;
    }
    progress2.setValue(iDataNumberOfPixelsBySceneByBand.size());
    progress2.close();
    qApp->processEvents();
    int minimumJdsInterval=366;
    int minimumJdsIntervalWitoutPixelsCriterion=366;
    QString rasterFileCandidate;
    QString rasterFileCandidateWithoutPixelsCriterion;
    QMap<QString,int>::const_iterator iterJdsToSummerSolsticeByScene=jdsToSummerSolsticeByScene.begin();
    while(iterJdsToSummerSolsticeByScene!=jdsToSummerSolsticeByScene.end())
    {
        QString rasterFile=iterJdsToSummerSolsticeByScene.key();
        if(fabs((float)iterJdsToSummerSolsticeByScene.value())<fabs((float)minimumJdsIntervalWitoutPixelsCriterion))
        {
            minimumJdsIntervalWitoutPixelsCriterion=iterJdsToSummerSolsticeByScene.value();
            rasterFileCandidateWithoutPixelsCriterion=rasterFile;
        }
        if(!numberOBandsBelowMinimumPixelsByScene.contains(rasterFile))
        {
            if(fabs((float)iterJdsToSummerSolsticeByScene.value())<fabs((float)minimumJdsInterval))
            {
                minimumJdsInterval=iterJdsToSummerSolsticeByScene.value();
                rasterFileCandidate=rasterFile;
            }
        }
        iterJdsToSummerSolsticeByScene++;
    }
    out2<<"  - Candidata sin criterio de pixeles ......: "<<rasterFileCandidateWithoutPixelsCriterion<<" ,dias: "<<QString::number(minimumJdsIntervalWitoutPixelsCriterion)<<"\n";
    out2<<"  - Candidata con criterio de pixeles ......: "<<rasterFileCandidate<<" ,dias: "<<QString::number(minimumJdsInterval)<<", para un minimo de pixeles: "<<QString::number(minPiasPixels)<<"\n";
    QString rasterFileReference=rasterFileCandidate;
    if(minimumJdsInterval==366)
    {
        rasterFileReference=rasterFileCandidateWithoutPixelsCriterion;
    }
    out2<<"  - Candidata final ........................: "<<rasterFileReference<<"\n";
//    // Para cada banda tengo un contenedor escenas (identificadas por su rasterFile),
//    // que contiene tuplekeys, que contiene un vector
//    // con los píxeles comunes donde: no hay nube, no hay no data value
//    // para cada pixel almaceno dos valores, el valor en cada una de las dos escenas
//    QMap<QString,QMap<QString,QMap<QString,QMap<int,QMap<int,float> > > > > valuesByColumnByRowByTuplekeyBySceneByBand;
//    // Para cada banda tengo un contenedor de escenas donde almaceno el número de píxeles que intervienen en el contenedor anterior
//    QMap<QString,QMap<QString,int> > iDataNumberOfPixelsBySceneByBand;
    // Para cada banda tengo un contenedor de parejas de escenas (identificadas por su rasterFile), en un único sentido
    // cada uno de estos contiene un contenedor de tuplekeys que a su vez contiene
    // un vector con los píxeles comunes donde: no hay nube, no hay no data value
    // para cada pixel almaceno dos valores, el valor en cada una de las dos escenas
    QMap<QString,QMap<QString,QMap<QString,QVector<QVector<float> > > > > iDataPixelsValuesByScenesByBand;
    title=QObject::tr("Computing Intercalibration");
    msgGlobal=QObject::tr("Data reestructuring by band ...").arg(QString::number(iDataNumberOfPixelsBySceneByBand.size()));
    QProgressDialog progress3(title, QObject::tr("Abort"),0,piasFilesIds.size(), mPtrWidgetParent);
    progress3.setWindowModality(Qt::WindowModal);
    progress3.setLabelText(msgGlobal);
    progress3.show();
    qApp->processEvents();
    contBand=0;
    QMap<QString,QMap<QString,QMap<QString,QMap<int,QMap<int,float> > > > >::const_iterator iterValuesByBand=valuesByColumnByRowByTuplekeyBySceneByBand.begin();
//    out2<<"- Datos para solucion por banda y parejas de escenas:\n";
//    out2<<"    Band                    RasterFile                    RasterFile    N.Tuplekeys\n";
    while(iterValuesByBand!=valuesByColumnByRowByTuplekeyBySceneByBand.end())
    {
        contBand++;
        progress3.setValue(contBand);
        qApp->processEvents();
        if (progress3.wasCanceled())
            break;
        QString bandId=iterValuesByBand.key();
//        QString title1=QObject::tr("Algorithms::intercalibrationComputation");
//        QString msg1=QObject::tr("Band id:\n%1").arg(bandId);
//        QMessageBox::information(mPtrWidgetParent,title1,msg1);
        QMap<QString,QMap<QString,QMap<int,QMap<int,float> > > > valuesByColumnByRowByTuplekeyByScene=valuesByColumnByRowByTuplekeyBySceneByBand[bandId];
        QMap<QString,QMap<QString,QMap<int,QMap<int,float> > > >::const_iterator iterValuesByScene=valuesByColumnByRowByTuplekeyByScene.begin();
        QVector<QString> rasterFilesToProcess;
        while(iterValuesByScene!=valuesByColumnByRowByTuplekeyByScene.end())
        {
            rasterFilesToProcess.push_back(iterValuesByScene.key());
            iterValuesByScene++;
        }
        for(int nrf1=0;nrf1<(rasterFilesToProcess.size()-1);nrf1++)
        {
            QString firstRasterFile=rasterFilesToProcess[nrf1];
            QMap<QString,QMap<int,QMap<int,float> > > valuesByColumnByRowByTuplekey=valuesByColumnByRowByTuplekeyByScene[firstRasterFile];
            QVector<QString> tuplekeysToProcess;
            QMap<QString,QMap<int,QMap<int,float> > >::const_iterator iterValuesByTuplekey=valuesByColumnByRowByTuplekey.begin();
            while(iterValuesByTuplekey!=valuesByColumnByRowByTuplekey.end())
            {
                tuplekeysToProcess.push_back(iterValuesByTuplekey.key());
                iterValuesByTuplekey++;
            }
            for(int nrf2=nrf1+1;nrf2<rasterFilesToProcess.size();nrf2++)
            {
                QString secondRasterFile=rasterFilesToProcess[nrf2];
                QString minRasterFile=firstRasterFile;
                QString maxRasterFile=secondRasterFile;
                if(secondRasterFile<minRasterFile)
                {
                    minRasterFile=secondRasterFile;
                    maxRasterFile=firstRasterFile;
                }
                for(int nt=0;nt<tuplekeysToProcess.size();nt++)
                {
                    QString tuplekey=tuplekeysToProcess[nt];
                    if(valuesByColumnByRowByTuplekeyByScene[secondRasterFile].contains(tuplekey))
                    {
                        QVector<QVector<float> > values;
                        QMap<int,QMap<int,float> > minRasterFileIndexedValuesByRow=valuesByColumnByRowByTuplekeyByScene[minRasterFile][tuplekey];
                        QMap<int,QMap<int,float> > maxRasterFileIndexedValuesByRow=valuesByColumnByRowByTuplekeyByScene[maxRasterFile][tuplekey];
                        QMap<int,QMap<int,float> >::const_iterator iterValuesByRow=minRasterFileIndexedValuesByRow.begin();
                        while(iterValuesByRow!=minRasterFileIndexedValuesByRow.end())
                        {
                            int row=iterValuesByRow.key();
                            if(maxRasterFileIndexedValuesByRow.contains(row))
                            {
                                QMap<int,float> minRasterFileIndexedValuesByRowByColumn=iterValuesByRow.value();
                                QMap<int,float>::const_iterator iterValuesByColumn=minRasterFileIndexedValuesByRowByColumn.begin();
                                while(iterValuesByColumn!=minRasterFileIndexedValuesByRowByColumn.end())
                                {
                                    int column=iterValuesByColumn.key();
                                    if(maxRasterFileIndexedValuesByRow[row].contains(column))
                                    {
                                        QVector<float> pixelValues(2);
                                        pixelValues[0]=minRasterFileIndexedValuesByRow[row][column];
                                        pixelValues[1]=maxRasterFileIndexedValuesByRow[row][column];
                                        values.push_back(pixelValues);
                                    }
                                    iterValuesByColumn++;
                                }
                            }
                            iterValuesByRow++;
                        }
//                        out2<<bandId.rightJustified(10);
//                        out2<<minRasterFile.rightJustified(30);
//                        out2<<maxRasterFile.rightJustified(30);
//                        out2<<QString::number(tuplekeysToProcess.size()).rightJustified(15);
//                        out2<<QString::number(firstIndexedValuesByRow.size()).rightJustified(6);
//                        out2<<QString::number(secondIndexedValuesByRow.size()).rightJustified(6);
//                        out2<<QString::number(values.size()).rightJustified(6)<<"\n";
                        if(values.size()>0)
                        {
                            if(!iDataPixelsValuesByScenesByBand.contains(bandId))
                            {
                                iDataPixelsValuesByScenesByBand[bandId][minRasterFile][maxRasterFile]=values;
                            }
                            else if(!iDataPixelsValuesByScenesByBand[bandId].contains(minRasterFile))
                            {
                                iDataPixelsValuesByScenesByBand[bandId][minRasterFile][maxRasterFile]=values;
                            }
                            else if(!iDataPixelsValuesByScenesByBand[bandId][minRasterFile].contains(maxRasterFile))
                            {
                                iDataPixelsValuesByScenesByBand[bandId][minRasterFile][maxRasterFile]=values;
                            }
                            else if(iDataPixelsValuesByScenesByBand[bandId][minRasterFile].contains(maxRasterFile))
                            {
                                for(int nv=0;nv<values.size();nv++)
                                {
                                    QVector<float> pixelValues;
                                    pixelValues.push_back(values[nv][0]);
                                    pixelValues.push_back(values[nv][1]);
                                    iDataPixelsValuesByScenesByBand[bandId][minRasterFile][maxRasterFile].push_back(pixelValues);
                                }
                            }
                        }
                    }
                }
            }
        }
        iterValuesByBand++;
    }
    progress3.setValue(iDataNumberOfPixelsBySceneByBand.size());
    progress3.close();

//    // Para cada banda tengo un contenedor de parejas de escenas (identificadas por su rasterFile), en un único sentido
//    // cada uno de estos contiene un vector con los píxeles comunes donde: no hay nube, no hay no data value
//    // para cada pixel almaceno dos valores, el valor en cada una de las dos escenas
//    QMap<QString,QMap<QString,QMap<QString,QVector<QVector<float> > > > > iDataPixelsValuesByScenesByBand;
    //    // Para cada banda tengo un contenedor de parejas de escenas (identificadas por su rasterFile), en un único sentido
    //    // cada uno de estos contiene un vector con las estadisticas de los píxeles comunes donde: no hay nube, no hay no data value
    //    // El vector tiene cuatro valores: media1,sigma1,media2,sigma2
    QMap<QString,QMap<QString,QMap<QString,QVector<double> > > > statisticsValuesByScenesByBand;
    QMap<QString,QMap<QString,QMap<QString,QVector<QVector<float> > > > >::const_iterator iterIDataByBand=iDataPixelsValuesByScenesByBand.begin();
    out2<<"- Datos para solucion por banda y parejas de escenas:\n";
    out2<<"      Band                  RasterFile_1                  RasterFile_2   Num.Valores   Media_1   Sigma_1   Media_2   Sigma_2  MinVal_1  MaxVal_1  MinVal_2  MaxVal_2   Num.Err\n";
    QVector<QString> bandsToProcess; // incluye todas aquellas que incluyan la imagen de referencia
    QMap<QString,QVector<QString> > scenesByBand; // para cada banda incluye un vector con las escenas que intervienen, sin incluir la de referencia
    while(iterIDataByBand!=iDataPixelsValuesByScenesByBand.end())
    {
        QString bandId=iterIDataByBand.key();
        QVector<QString> scenes;
        QMap<QString,QMap<QString,QVector<QVector<float> > > > iDataPixelsValuesByScenes=iDataPixelsValuesByScenesByBand[bandId];
        QMap<QString,QMap<QString,QVector<QVector<float> > > >::const_iterator iterIDataByFirstScene=iDataPixelsValuesByScenes.begin();
        while(iterIDataByFirstScene!=iDataPixelsValuesByScenes.end())
        {
            QString firstScene=iterIDataByFirstScene.key();
            QMap<QString,QVector<QVector<float> > > iDataPixelsValuesByScene=iDataPixelsValuesByScenes[firstScene];
            QMap<QString,QVector<QVector<float> > >::const_iterator iterIDataBySecondScene=iDataPixelsValuesByScene.begin();
            while(iterIDataBySecondScene!=iDataPixelsValuesByScene.end())
            {
                QString secondScene=iterIDataBySecondScene.key();
                QVector<QVector<float> > values=iterIDataBySecondScene.value();
                int numberOfValues=values.size();
                out2<<bandId.rightJustified(10);
                out2<<firstScene.rightJustified(30);
                out2<<secondScene.rightJustified(30);
                if(numberOfValues<minBandsPixels)
                {
//                    QVector<double> statistics(4);
//                    statistics[0]=0.;
//                    statistics[1]=0.;
//                    statistics[2]=0.;
//                    statistics[3]=0.;
//                    statistics[4]=numberOfValues;
//                    statisticsValuesByScenesByBand[bandId][firstScene][secondScene]=statistics;
                    out2<<QString::number(numberOfValues).rightJustified(14);
                    out2<<"   There are not enough values\n";
                    iterIDataBySecondScene++;
                    continue;
                }
                double firstMean=0.0;
                double secondMean=0.0;
                QString strFirstValues="(";
                QString strSecondValues="(";
                for(int nv=0;nv<numberOfValues;nv++)
                {
                    firstMean+=(double)values[nv][0];
                    secondMean+=(double)values[nv][1];
                    strFirstValues+=QString::number(values[nv][0],'f',2);
                    strSecondValues+=QString::number(values[nv][1],'f',2);
                    if(nv<(numberOfValues-1))
                    {
                        strFirstValues+=",";
                        strSecondValues+=",";
                    }
                }
                strFirstValues+=")";
                strSecondValues+=")";
                firstMean/=(double)numberOfValues;
                secondMean/=(double)numberOfValues;
                double firstStd=0.0;
                double secondStd=0.0;
                double firstMinValue=1000000.0;
                double firstMaxValue=-1000000.0;
                double secondMinValue=1000000.0;
                double secondMaxValue=-1000000.0;
                int numberOfOutliers=0;
                for(int nv=0;nv<numberOfValues;nv++)
                {
                    firstStd+=pow(firstMean-values[nv][0],2.0);
                    secondStd+=pow(secondMean-values[nv][1],2.0);
                    if(values[nv][0]>firstMaxValue)
                    {
                        firstMaxValue=values[nv][0];
                    }
                    if(values[nv][0]<firstMinValue)
                    {
                        firstMinValue=values[nv][0];
                    }
                    if(values[nv][1]>secondMaxValue)
                    {
                        secondMaxValue=values[nv][1];
                    }
                    if(values[nv][1]<secondMinValue)
                    {
                        secondMinValue=values[nv][1];
                    }
                }
                firstStd=sqrt(firstStd/((double)(numberOfValues-1)));
                secondStd=sqrt(secondStd/((double)(numberOfValues-1)));
                for(int nv=0;nv<numberOfValues;nv++)
                {
                    double firstDifference=fabs(firstMean-values[nv][0]);
                    double secondDifference=fabs(secondMean-values[nv][1]);
                    if(firstDifference>(3.0*firstStd)
                            ||secondDifference>(3.0*secondStd))
                    {
                        numberOfOutliers++;
                    }
                }
                bool existsValues=true;
                if(removeOutliers)
                {
                    int outliersContCycles=0;
                    bool existsOutliers=false;
                    if(numberOfOutliers>0)
                        existsOutliers=true;
                    while(existsOutliers&&outliersContCycles<1) // solo elimino una vez
                    {
                        QVector<QVector<float> > valuesWithoutOutliers;
                        for(int nv=0;nv<numberOfValues;nv++)
                        {
                            double firstDifference=fabs(firstMean-values[nv][0]);
                            double secondDifference=fabs(secondMean-values[nv][1]);
                            if(firstDifference>(3.0*firstStd)
                                    ||secondDifference>(3.0*secondStd))
                            {
                                continue;
                            }
                            QVector<float> aux(2);
                            aux[0]=values[nv][0];
                            aux[1]=values[nv][1];
                            valuesWithoutOutliers.push_back(aux);
                        }
                        values=valuesWithoutOutliers;
                        numberOfValues=values.size();
                        if(numberOfValues<minBandsPixels)
                        {
                            out2<<"   After remove outliers there are not enough values\n";
                            existsValues=false;
                            break;
                        }
                        firstMean=0.0;
                        secondMean=0.0;
                        for(int nv=0;nv<numberOfValues;nv++)
                        {
                            firstMean+=(double)values[nv][0];
                            secondMean+=(double)values[nv][1];
                        }
                        firstMean/=(double)numberOfValues;
                        secondMean/=(double)numberOfValues;
                        firstStd=0.0;
                        secondStd=0.0;
                        firstMinValue=1000000.0;
                        firstMaxValue=-1000000.0;
                        secondMinValue=1000000.0;
                        secondMaxValue=-1000000.0;
                        numberOfOutliers=0;
                        for(int nv=0;nv<numberOfValues;nv++)
                        {
                            firstStd+=pow(firstMean-values[nv][0],2.0);
                            secondStd+=pow(secondMean-values[nv][1],2.0);
                            if(values[nv][0]>firstMaxValue)
                            {
                                firstMaxValue=values[nv][0];
                            }
                            if(values[nv][0]<firstMinValue)
                            {
                                firstMinValue=values[nv][0];
                            }
                            if(values[nv][1]>secondMaxValue)
                            {
                                secondMaxValue=values[nv][1];
                            }
                            if(values[nv][1]<secondMinValue)
                            {
                                secondMinValue=values[nv][1];
                            }
                        }
                        firstStd=sqrt(firstStd/((double)(numberOfValues-1)));
                        secondStd=sqrt(secondStd/((double)(numberOfValues-1)));
                        for(int nv=0;nv<numberOfValues;nv++)
                        {
                            double firstDifference=fabs(firstMean-values[nv][0]);
                            double secondDifference=fabs(secondMean-values[nv][1]);
                            if(firstDifference>(3.0*firstStd)
                                    ||secondDifference>(3.0*secondStd))
                            {
                                numberOfOutliers++;
                            }
                        }
                        if(numberOfOutliers==0)
                            existsOutliers=false;
                        outliersContCycles++;
                    }
                }
                if(!existsValues)
                {
                    iterIDataBySecondScene++;
                    continue;
                }
                out2<<QString::number(numberOfValues).rightJustified(14);
                out2<<QString::number(firstMean,'f',2).rightJustified(10);
                out2<<QString::number(firstStd,'f',4).rightJustified(10);
                out2<<QString::number(secondMean,'f',2).rightJustified(10);
                out2<<QString::number(secondStd,'f',4).rightJustified(10);
                out2<<QString::number(firstMinValue,'f',2).rightJustified(10);
                out2<<QString::number(firstMaxValue,'f',2).rightJustified(10);
                out2<<QString::number(secondMinValue,'f',2).rightJustified(10);
                out2<<QString::number(secondMaxValue,'f',2).rightJustified(10);
                out2<<QString::number(numberOfOutliers).rightJustified(10)<<"\n";
//                out2<<"                  "<<strFirstValues<<"\n";
//                out2<<"                  "<<strSecondValues<<"\n";
                QVector<double> statistics(5);
                statistics[0]=firstMean;
                statistics[1]=firstStd;
                statistics[2]=secondMean;
                statistics[3]=secondStd;
                statistics[4]=numberOfValues;
                statisticsValuesByScenesByBand[bandId][firstScene][secondScene]=statistics;
                if(firstScene.compare(rasterFileReference,Qt::CaseInsensitive)==0
                        ||secondScene.compare(rasterFileReference,Qt::CaseInsensitive)==0)
                {
                    if(bandsToProcess.indexOf(bandId)==-1)
                    {
                        bandsToProcess.push_back(bandId);
                    }
                }
                if(firstScene.compare(rasterFileReference,Qt::CaseInsensitive)!=0)
                {
                    if(scenes.indexOf(firstScene)==-1)
                    {
                        scenes.push_back(firstScene);
                    }
                }
                if(secondScene.compare(rasterFileReference,Qt::CaseInsensitive)!=0)
                {
                    if(scenes.indexOf(secondScene)==-1)
                    {
                        scenes.push_back(secondScene);
                    }
                }
                iterIDataBySecondScene++;
            }
            iterIDataByFirstScene++;
        }
        scenesByBand[bandId]=scenes;
        iterIDataByBand++;
    }
    out2<<"- Resultados de la intercalibracion:\n";
    out2<<"    Band                    RasterFile                gain              offset    Interpolated";
    if(toReflectance)
    {
        out2<<"       Sun.Elevation     Refl.Mult.Value      Refl.Add.Value";
    }
    out2<<"\n";
//    //    // Para cada banda tengo un contenedor de parejas de escenas (identificadas por su rasterFile), en un único sentido
//    //    // cada uno de estos contiene un vector con las estadisticas de los píxeles comunes donde: no hay nube, no hay no data value
//    //    // El vector tiene cuatro valores: media1,sigma1,media2,sigma2
//    QMap<QString,QMap<QString,QMap<QString,QVector<double> > > > iStatisticsValuesByScenesByBand;
    //    // Para cada banda tengo un contenedor de parejas de escenas (identificadas por su rasterFile), en un único sentido
    //    // cada uno de estos contiene un vector con los valores de la intercalibracion: gain y offset
    title=QObject::tr("Computing Intercalibration");
    msgGlobal=QObject::tr("Solving LS adjustment for bands ...").arg(QString::number(bandsToProcess.size()));
    QProgressDialog progress4(title, QObject::tr("Abort"),0,piasFilesIds.size(), mPtrWidgetParent);
    progress4.setWindowModality(Qt::WindowModal);
    progress4.setLabelText(msgGlobal);
    progress4.show();
    qApp->processEvents();
    QMap<QString,QMap<QString,QMap<QString,QVector<double> > > > intercalibrationByScenesByBand;
    for(int nb=0;nb<bandsToProcess.size();nb++)
    {
        progress4.setValue(nb);
        qApp->processEvents();
        if (progress4.wasCanceled())
            break;
        QString bandId=bandsToProcess[nb];
        QVector<QString> scenes=scenesByBand[bandId];
        QMap<QString,QMap<QString,QVector<double> > > statisticsByScenes=statisticsValuesByScenesByBand[bandId];
        QMap<QString,QMap<QString,QVector<double> > >::const_iterator iterStatisticsByScenes=statisticsByScenes.begin();
        int numberOfEquations=0;
        while(iterStatisticsByScenes!=statisticsByScenes.end())
        {
            QMap<QString,QVector<double> > statisticsByScene=iterStatisticsByScenes.value();
            QMap<QString,QVector<double> >::const_iterator iterStatisticsByScene=statisticsByScene.begin();
            while(iterStatisticsByScene!=statisticsByScene.end())
            {
                numberOfEquations=numberOfEquations+2;
                iterStatisticsByScene++;
            }
            iterStatisticsByScenes++;
        }
        int numberOfUnknowns=scenes.size()*2;
//        int numberOfEquations=2*statisticsByScenes.size();
        Eigen::MatrixXd A(numberOfEquations,numberOfUnknowns);
        A.setZero();
        Eigen::VectorXd b(numberOfEquations);
        b.setZero();
        int numberOfEquation=0;
        iterStatisticsByScenes=statisticsByScenes.begin();
        while(iterStatisticsByScenes!=statisticsByScenes.end())
        {
            QString firstScene=iterStatisticsByScenes.key();
            QMap<QString,QVector<double> > statisticsByScene=iterStatisticsByScenes.value();
            QMap<QString,QVector<double> >::const_iterator iterStatisticsByScene=statisticsByScene.begin();
            while(iterStatisticsByScene!=statisticsByScene.end())
            {
                QString secondScene=iterStatisticsByScene.key();
                int numberOfValues=iterStatisticsByScene.value()[4];
//                if(numberOfValues<minBandsPixels)
//                {
//                    iterStatisticsByScene++;
//                }
                double firstMean=iterStatisticsByScene.value()[0];
                double firstStd=iterStatisticsByScene.value()[1];
                double secondMean=iterStatisticsByScene.value()[2];
                double secondStd=iterStatisticsByScene.value()[3];
                int firstSceneFirstUnknownColumn=scenes.indexOf(firstScene)*2;
                int secondSceneFirstUnknownColumn=scenes.indexOf(secondScene)*2;
                b(numberOfEquation,0)=0.0;
                b(numberOfEquation+1,0)=0.0;
                double weight=1.0;
                if(firstSceneFirstUnknownColumn<0
                        ||secondSceneFirstUnknownColumn<0)
                {
                    weight=weightReferenceSceneEquations;
                }
                if(firstSceneFirstUnknownColumn>=0)
                {
                    A(numberOfEquation,firstSceneFirstUnknownColumn) = firstMean*weight;
                    A(numberOfEquation,firstSceneFirstUnknownColumn+1) = 1.0*weight;
                    A(numberOfEquation+1,firstSceneFirstUnknownColumn) = firstStd*weight;
                }
                else
                {
                    b(numberOfEquation,0)=-1.0*firstMean*weight;
                    b(numberOfEquation+1,0)=-1.0*firstStd*weight;
                }
                if(secondSceneFirstUnknownColumn>=0)
                {
                    A(numberOfEquation,secondSceneFirstUnknownColumn) = -1.0*secondMean*weight;
                    A(numberOfEquation,secondSceneFirstUnknownColumn+1) = -1.0*weight;
                    A(numberOfEquation+1,secondSceneFirstUnknownColumn) = -1.0*secondStd*weight;
                }
                else
                {
                    b(numberOfEquation,0)=secondMean*weight;
                    b(numberOfEquation+1,0)=secondStd*weight;
                }
                numberOfEquation=numberOfEquation+2;
                iterStatisticsByScene++;
            }
            iterStatisticsByScenes++;
        }
        Eigen::VectorXd sol=(A.transpose() * A).ldlt().solve(A.transpose() * b);
        Eigen::VectorXd residuals=A*sol-b;
        // Obtengo los rasterFiles que no han entrado en el ajuste para interpolar los valores
        QVector<QString> noComputedRasterFiles;
        for(int nrf=0;nrf<rasterFiles.size();nrf++)
        {
            QString rasterFile=rasterFiles.at(nrf);
            int posInScenes=scenes.indexOf(rasterFile);
            if(posInScenes==-1)
            {
                noComputedRasterFiles.push_back(rasterFile);
            }
        }
        QMap<int,QVector<QString> > totalRasterFilesByJd;
        for(int nrf=0;nrf<scenes.size();nrf++)
        {
            QString rasterFile=scenes.at(nrf);
            int rastefFileJd=jdByRasterFile[rasterFile];
            if(!totalRasterFilesByJd.contains(rastefFileJd))
            {
                QVector<QString> aux;
                totalRasterFilesByJd[rastefFileJd]=aux;
            }
            totalRasterFilesByJd[rastefFileJd].push_back(rasterFile);
        }
        for(int nrf=0;nrf<noComputedRasterFiles.size();nrf++)
        {
            QString rasterFile=noComputedRasterFiles.at(nrf);
            int rastefFileJd=jdByRasterFile[rasterFile];
            if(!totalRasterFilesByJd.contains(rastefFileJd))
            {
                QVector<QString> aux;
                totalRasterFilesByJd[rastefFileJd]=aux;
            }
            totalRasterFilesByJd[rastefFileJd].push_back(rasterFile);
        }
        QMap<QString,double> gainByRasterFile;
        QMap<QString,double> offsetByRasterFile;
        QMap<QString,bool> interpolatedByRasterFile;
        for(int ns=0;ns<scenes.size();ns++)
        {
            QString sceneId=scenes.at(ns);
            double gain=sol(ns*2);
            double offset=sol(ns*2+1);
            gainByRasterFile[sceneId]=gain;
            offsetByRasterFile[sceneId]=offset;
            interpolatedByRasterFile[sceneId]=false;
//            bool interpolated=false;
//            if(!mPtrPersistenceManager->insertIntercalibration(sceneId,
//                                                               rasterFileReference,
//                                                               bandId,
//                                                               gain,
//                                                               offset,
//                                                               to8Bits,
//                                                               toReflectance,
//                                                               interpolated,
//                                                               strAuxError))
//            {
//                strError=QObject::tr("Algorithms::insertIntercalibration");
//                strError+=QObject::tr("\nError storing intercalibration in database:\nBand: %1\nScene: %2\nError:\n%3")
//                        .arg(bandId).arg(sceneId).arg(strAuxError);
//                return(false);
//            }
        }
        // Proceso de interpolacion
        QMap<int,double> gainByJd; // si existe mas de una solucion por fecha uso la media
        QMap<int,double> offsetByJd;
        QMap<int,QVector<QString> >::const_iterator iterTotalRasterFilesByJd=totalRasterFilesByJd.begin();
        int dataMinJd=100000000;
        int dataMaxJd=-100000000;
        while(iterTotalRasterFilesByJd!=totalRasterFilesByJd.end())
        {
            int rasterFileJd=iterTotalRasterFilesByJd.key();
            double gain=0.0;
            double offset=0.0;
            bool existsValueForJd=false;
            int numberOfSolutionsByJd=0;
            QVector<QString> auxRasterFilesByJd=iterTotalRasterFilesByJd.value();
            for(int nrf=0;nrf<auxRasterFilesByJd.size();nrf++)
            {
                QString rasterFile=auxRasterFilesByJd.at(nrf);
                if(gainByRasterFile.contains(rasterFile))
                {
                    if(existsValueForJd)
                    {
                        gain+=gainByRasterFile[rasterFile];
                    }
                    else
                    {
                        gain=gainByRasterFile[rasterFile];
                        existsValueForJd=true;
                    }
                    numberOfSolutionsByJd++;
                }
                if(offsetByRasterFile.contains(rasterFile))
                {
                    if(existsValueForJd)
                    {
                        offset+=offsetByRasterFile[rasterFile];
                    }
                    else
                    {
                        offset=offsetByRasterFile[rasterFile];
                        existsValueForJd=true;
                    }
                }
            }
            if(existsValueForJd)
            {
                if(rasterFileJd<dataMinJd)
                {
                    dataMinJd=rasterFileJd;
                }
                if(rasterFileJd>dataMaxJd)
                {
                    dataMaxJd=rasterFileJd;
                }
                gain/=numberOfSolutionsByJd;
                offset/=numberOfSolutionsByJd;
                gainByJd[rasterFileJd]=gain;
                offsetByJd[rasterFileJd]=offset;
            }
            iterTotalRasterFilesByJd++;
        }
        double* x;
        double* yGains;
        double* yOffsets;
        x= new double[gainByJd.size()];
        yGains= new double[gainByJd.size()];
        yOffsets= new double[gainByJd.size()];
        int contData=0;
        QMap<int,double>::const_iterator iterGainByJd=gainByJd.begin();
        while(iterGainByJd!=gainByJd.end())
        {
            x[contData]=iterGainByJd.key()-dataMinJd;
            yGains[contData]=iterGainByJd.value();
            yOffsets[contData]=offsetByJd[iterGainByJd.key()];
            contData++;
            iterGainByJd++;
        }
        Splines::AkimaSpline* ptrGainsAkimaSpline=NULL;
        Splines::AkimaSpline* ptrOffsetsAkimaSpline=NULL;
        Splines::CubicSpline* ptrGainsCubicSpline=NULL;
        Splines::CubicSpline* ptrOffsetsCubicSpline=NULL;
        Splines::BesselSpline* ptrGainsBesselSpline=NULL;
        Splines::BesselSpline* ptrOffsetsBesselSpline=NULL;
        if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
        {
            ptrGainsAkimaSpline=new Splines::AkimaSpline();
            ptrOffsetsAkimaSpline=new Splines::AkimaSpline();
            ptrGainsAkimaSpline->build(x,yGains,contData) ;
            ptrOffsetsAkimaSpline->build(x,yOffsets,contData) ;
        }
        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
        {
            ptrGainsCubicSpline=new Splines::CubicSpline();
            ptrOffsetsCubicSpline=new Splines::CubicSpline();
            ptrGainsCubicSpline->build(x,yGains,contData) ;
            ptrOffsetsCubicSpline->build(x,yOffsets,contData) ;
        }
        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
        {
            ptrGainsBesselSpline=new Splines::BesselSpline();
            ptrOffsetsBesselSpline=new Splines::BesselSpline();
            ptrGainsBesselSpline->build(x,yGains,contData) ;
            ptrOffsetsBesselSpline->build(x,yOffsets,contData) ;
        }
        iterTotalRasterFilesByJd=totalRasterFilesByJd.begin();
        while(iterTotalRasterFilesByJd!=totalRasterFilesByJd.end())
        {
            int rasterFileJd=iterTotalRasterFilesByJd.key();
            QVector<QString> auxRasterFilesByJd=iterTotalRasterFilesByJd.value();
            for(int nrf=0;nrf<auxRasterFilesByJd.size();nrf++)
            {
                QString rasterFile=auxRasterFilesByJd.at(nrf);
                double gain=-1.0;
                double offset=-100000.0;
                bool interpolated=true;
                bool toInterpolation=false;
                if(rasterFile.compare(rasterFileReference,Qt::CaseInsensitive)==0)
                {
                    gain=1.0;
                    offset=0.0;
                    interpolated=false;
                    gainByRasterFile[rasterFile]=gain;
                    offsetByRasterFile[rasterFile]=offset;
                    interpolatedByRasterFile[rasterFile]=false;
                }
                else if(gainByRasterFile.contains(rasterFile)
                        &&offsetByRasterFile.contains(rasterFile))
                {
                    gain=gainByRasterFile[rasterFile];
                    offset=offsetByRasterFile[rasterFile];
                    interpolated=false;
                    if(fabs(gain+1.0)<0.01&&fabs(offset+100000)<0.01) // caso raro que no controlo
                    {
                        toInterpolation=true;
                    }
                }
                else if(gainByJd.contains(rasterFileJd)
                        &&offsetByJd.contains(rasterFileJd))
                {
                    gain=gainByJd[rasterFileJd];
                    offset=offsetByJd[rasterFileJd];
                    interpolated=false;
                    if(fabs(gain+1.0)<0.01&&fabs(offset+100000)<0.01) // caso raro que no controlo
                    {
                        toInterpolation=true;
                    }
                    gainByRasterFile[rasterFile]=gain;
                    offsetByRasterFile[rasterFile]=offset;
                    interpolatedByRasterFile[rasterFile]=true;
                }
                else
                {
                    toInterpolation=true;
                }
                if(toInterpolation)
                {
                    if(rasterFileJd<=dataMinJd)
                    {
                        gain=gainByJd[dataMinJd];
                        offset=offsetByJd[dataMinJd];
                    }
                    else if(rasterFileJd>=dataMaxJd)
                    {
                        gain=gainByJd[dataMaxJd];
                        offset=offsetByJd[dataMaxJd];
                    }
                    else // ahora interpolo
                    {
                        double xJd=(double)(rasterFileJd-dataMinJd);
                        if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
                        {
                            gain=(*ptrGainsAkimaSpline)(xJd);
                            offset=(*ptrOffsetsAkimaSpline)(xJd);
                        }
                        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
                        {
                            gain=(*ptrGainsCubicSpline)(xJd);
                            offset=(*ptrOffsetsCubicSpline)(xJd);
                        }
                        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
                        {
                            gain=(*ptrGainsBesselSpline)(xJd);
                            offset=(*ptrOffsetsBesselSpline)(xJd);
                        }
                    }
                    gainByRasterFile[rasterFile]=gain;
                    offsetByRasterFile[rasterFile]=offset;
                    interpolatedByRasterFile[rasterFile]=true;
                }
                if(!mPtrPersistenceManager->insertIntercalibration(rasterFile,
                                                                   rasterFileReference,
                                                                   bandId,
                                                                   gain,
                                                                   offset,
                                                                   to8Bits,
                                                                   toReflectance,
                                                                   interpolated,
                                                                   strAuxError))
                {
                    strError=QObject::tr("Algorithms::insertIntercalibration");
                    strError+=QObject::tr("\nError storing intercalibration in database:\nBand: %1\nScene: %2\nError:\n%3")
                            .arg(bandId).arg(rasterFile).arg(strAuxError);
                    return(false);
                }
            }
            iterTotalRasterFilesByJd++;
        }
        if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE,Qt::CaseInsensitive)==0)
        {
            delete(ptrGainsAkimaSpline);
            delete(ptrOffsetsAkimaSpline);
        }
        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE,Qt::CaseInsensitive)==0)
        {
            delete(ptrGainsCubicSpline);
            delete(ptrOffsetsCubicSpline);
        }
        else if(interpolationMethod.compare(ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE,Qt::CaseInsensitive)==0)
        {
            delete(ptrGainsBesselSpline);
            delete(ptrOffsetsBesselSpline);
        }

        if(printLsData)
        {
            QString resultsFileName=resultsFile.fileName();
            QFileInfo resultsFileInfo(resultsFileName);
            QString aMatrixFileName=resultsFileInfo.absolutePath()+"/A_"+bandId+".dat";
            QFile aMatrixFile(aMatrixFileName);
            if (!aMatrixFile.open(QFile::WriteOnly |QFile::Text))
            {
                strError=QObject::tr("Algorithms::intercalibrationComputation");
                strError+=QObject::tr("\nError opening matrix A file: \n %1").arg(aMatrixFileName);
                return(false);
            }
            QTextStream aMatrixOut(&aMatrixFile);
            QString bVectorFileName=resultsFileInfo.absolutePath()+"/b_"+bandId+".dat";
            QFile bVectorFile(bVectorFileName);
            if (!bVectorFile.open(QFile::WriteOnly |QFile::Text))
            {
                strError=QObject::tr("Algorithms::intercalibrationComputation");
                strError+=QObject::tr("\nError opening vector b file: \n %1").arg(aMatrixFileName);
                return(false);
            }
            QTextStream bVectorOut(&bVectorFile);
            for(int row=0;row<A.rows();row++)
            {
                for(int column=0;column<A.cols();column++)
                {
                    aMatrixOut<<QString::number(A(row,column),'f',15).rightJustified(25);
                }
                aMatrixOut<<"\n";
                bVectorOut<<QString::number(b(row,0),'f',15).rightJustified(25)<<"\n";
            }
            aMatrixFile.close();
            bVectorFile.close();
            QString residualsFileName=resultsFileInfo.absolutePath()+"/Residuals_"+bandId+".dat";
            QFile residualsFile(residualsFileName);
            if (!residualsFile.open(QFile::WriteOnly |QFile::Text))
            {
                strError=QObject::tr("Algorithms::intercalibrationComputation");
                strError+=QObject::tr("\nError opening residuals file: \n %1").arg(residualsFileName);
                return(false);
            }
            QTextStream residualsOut(&residualsFile);
            iterStatisticsByScenes=statisticsByScenes.begin();
            numberOfEquation=0;
            residualsOut<<"      Band                  RasterFile_1                  RasterFile_2   Num.Valores   Media_1   Sigma_1   Media_2   Sigma_2    ResEq1    ResEq2\n";
            while(iterStatisticsByScenes!=statisticsByScenes.end())
            {
                QString firstScene=iterStatisticsByScenes.key();
                QMap<QString,QVector<double> > statisticsByScene=iterStatisticsByScenes.value();
                QMap<QString,QVector<double> >::const_iterator iterStatisticsByScene=statisticsByScene.begin();
                while(iterStatisticsByScene!=statisticsByScene.end())
                {
                    QString secondScene=iterStatisticsByScene.key();
                    double firstMean=iterStatisticsByScene.value()[0];
                    double firstStd=iterStatisticsByScene.value()[1];
                    double secondMean=iterStatisticsByScene.value()[2];
                    double secondStd=iterStatisticsByScene.value()[3];
                    int numberOfValues=(int)iterStatisticsByScene.value()[4];
                    double residualFirstEquation=residuals(numberOfEquation);
                    double residualSecondEquation=residuals(numberOfEquation+1);
                    residualsOut<<bandId.rightJustified(10);
                    residualsOut<<firstScene.rightJustified(30);
                    residualsOut<<secondScene.rightJustified(30);
                    residualsOut<<QString::number(numberOfValues).rightJustified(14);
                    residualsOut<<QString::number(firstMean,'f',2).rightJustified(10);
                    residualsOut<<QString::number(firstStd,'f',4).rightJustified(10);
                    residualsOut<<QString::number(secondMean,'f',2).rightJustified(10);
                    residualsOut<<QString::number(secondStd,'f',4).rightJustified(10);
                    residualsOut<<QString::number(residualFirstEquation,'f',4).rightJustified(10);
                    residualsOut<<QString::number(residualSecondEquation,'f',4).rightJustified(10)<<"\n";
                    numberOfEquation=numberOfEquation+2;
                    iterStatisticsByScene++;
                }
                iterStatisticsByScenes++;
            }
            residualsFile.close();
        }
        double sumRes2=0;
        for(int i=0;i<numberOfEquations;i++)
        {
            double residual=residuals(i);
            sumRes2+=(residual*residual);
        }
//        for(int ns=0;ns<scenes.size();ns++)
//        {
//            if(ns==0)
//            {
//                out2<<bandId.rightJustified(10);
//            }
//            else
//            {
//                out2<<"          ";
//            }
//            QString sceneId=scenes.at(ns);
//            out2<<sceneId.rightJustified(30);
////            out2<<" "<<numberOfEquations<<" "<<numberOfUnknowns<<"\n";
//            double gain=sol(ns*2);
//            double offset=sol(ns*2+1);
//            out2<<QString::number(gain,'f',12).rightJustified(20);
//            out2<<QString::number(offset,'f',9).rightJustified(20)<<"\n";
//        }
        iterTotalRasterFilesByJd=totalRasterFilesByJd.begin();
        int contAux=0;
        while(iterTotalRasterFilesByJd!=totalRasterFilesByJd.end())
        {
            int rasterFileJd=iterTotalRasterFilesByJd.key();
            QVector<QString> auxRasterFilesByJd=iterTotalRasterFilesByJd.value();
            for(int nrf=0;nrf<auxRasterFilesByJd.size();nrf++)
            {
                QString rasterFile=auxRasterFilesByJd.at(nrf);
                if(contAux==0)
                {
                    out2<<bandId.rightJustified(10);
                }
                else
                {
                    out2<<"          ";
                }
                out2<<rasterFile.rightJustified(30);
    //            out2<<" "<<numberOfEquations<<" "<<numberOfUnknowns<<"\n";
                double gain=gainByRasterFile[rasterFile];
                double offset=offsetByRasterFile[rasterFile];
                bool interpolated=interpolatedByRasterFile[rasterFile];
//                double gain=-1.0;
//                double offset=-100000.0;
//                if(rasterFile.compare(rasterFileReference,Qt::CaseInsensitive)==0)
//                {
//                    gain=1.0;
//                    offset=0.0;
//                }
//                if(gainByRasterFile.contains(rasterFile))
//                {
//                    gain=gainByRasterFile[rasterFile];
//                }
//                if(offsetByRasterFile.contains(rasterFile))
//                {
//                    offset=offsetByRasterFile[rasterFile];
//                }
//    //            double gain=sol(ns*2);
//    //            double offset=sol(ns*2+1);
                out2<<QString::number(gain,'f',12).rightJustified(20);
                out2<<QString::number(offset,'f',9).rightJustified(20);
                QString strInterpolated="No";
                if(interpolated)
                {
                    strInterpolated="Yes";
                }
                out2<<strInterpolated.rightJustified(14);
                if(toReflectance)
                {
                    out2<<QString::number(sunElevationByRasterFile[rasterFile],'f',12).rightJustified(20);
                    out2<<QString::number(reflectanceMultValueByRasterFileAndByBand[rasterFile][bandId],'f',12).rightJustified(20);
                    out2<<QString::number(reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId],'f',8).rightJustified(20);
                }
                out2<<"\n";
                if(writeImageFiles)
                {
                    QMap<QString, QMap<QString, QMap<QString, QString> > >::const_iterator iterWITuplekey =tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.begin();
                    while(iterWITuplekey!=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand.end())
                    {
                        QMap<QString, QMap<QString, QString> >::const_iterator iterWIRasterFile=iterWITuplekey.value().begin();
                        while(iterWIRasterFile!=iterWITuplekey.value().end())
                        {
                            QString wiRasterFile=iterWIRasterFile.key();
                            if(wiRasterFile.compare(rasterFile,Qt::CaseInsensitive)==0)
                            {
                                QMap<QString, QString>::const_iterator iterWIBand=iterWIRasterFile.value().begin();
                                while(iterWIBand!=iterWIRasterFile.value().end())
                                {
                                    QString wiBand=iterWIBand.key();
                                    if(wiBand.compare(bandId,Qt::CaseInsensitive)==0)
                                    {
                                        QString rasterFileName=iterWIBand.value();
                                        QFileInfo rasterFileInfo(rasterFileName);
                                        QString intercalibratedRasterFileName=rasterFileInfo.absolutePath()+"/";
                                        intercalibratedRasterFileName+=ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILES_FOLDER;
                                        intercalibratedRasterFileName+="/";
                                        intercalibratedRasterFileName+=bandId;
                                        QString intercalibratedRasterFilePath=intercalibratedRasterFileName;
                                        QDir rasterFileDir(rasterFileInfo.absolutePath());
                                        if(!rasterFileDir.exists(intercalibratedRasterFilePath))
                                        {
                                            if(!rasterFileDir.mkpath(intercalibratedRasterFilePath))
                                            {
                                                strError=QObject::tr("Algorithms::intercalibrationComputation");
                                                strError+=QObject::tr("\nError making path for intercalibrated file:\n%1")
                                                        .arg(intercalibratedRasterFilePath);
                                                return(false);
                                            }
                                        }
                                        intercalibratedRasterFileName+=("/"+rasterFileInfo.fileName());
                                        QString auxStrError;
                                        double multValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][bandId];
                                        double addValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId];
                                        if(!applyIntercalibration(rasterFileName,
                                                                  intercalibratedRasterFileName,writeImageFilesTo8Bits,
                                                                  gain,offset,to8Bits,
                                                                  toReflectance,multValue,addValue,
                                                                  auxStrError))
                                        {
                                            strError=QObject::tr("Algorithms::intercalibrationComputation");
                                            strError+=QObject::tr("\nError applying intercalibration to file:\n%1\nError:\n%2")
                                                    .arg(rasterFileName).arg(auxStrError);
                                            return(false);
                                        }
                                    }
                                    iterWIBand++;
                                }
                            }
                            iterWIRasterFile++;
                        }
                        iterWITuplekey++;
                    }
//                    QMap<QString, QString> tuplekeysRasterFilesByTuplekey=tuplekeysRasterFilesByTuplekeyByRasterFileAndByBand[bandId][rasterFile];
//                    QMap<QString, QString>::const_iterator iterTuplekey=tuplekeysRasterFilesByTuplekey.begin();
//                    while(iterTuplekey!=tuplekeysRasterFilesByTuplekey.end())
//                    {
//                        QString tuplekey=iterTuplekey.key();

//                        QString rasterFileName=iterTuplekey.value();
//                        QFileInfo rasterFileInfo(rasterFileName);
//                        QString intercalibratedRasterFileName=rasterFileInfo.absolutePath()+"/";
//                        intercalibratedRasterFileName+=ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILES_FOLDER;
//                        QString intercalibratedRasterFilePath=intercalibratedRasterFileName;
//                        QDir rasterFileDir(rasterFileInfo.absolutePath());
//                        if(!rasterFileDir.exists(intercalibratedRasterFilePath))
//                        {
//                            if(!rasterFileDir.mkpath(intercalibratedRasterFilePath))
//                            {
//                                strError=QObject::tr("Algorithms::intercalibrationComputation");
//                                strError+=QObject::tr("\nError making path for intercalibrated file:\n%1")
//                                        .arg(intercalibratedRasterFilePath);
//                                return(false);
//                            }
//                        }
//                        intercalibratedRasterFileName+=("/"+rasterFileInfo.fileName());
//                        QString auxStrError;
//                        double multValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][bandId];
//                        double addValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][bandId];
//                        if(!applyIntercalibration(rasterFileName,
//                                                  intercalibratedRasterFileName,writeImageFilesTo8Bits,
//                                                  gain,offset,to8Bits,
//                                                  toReflectance,multValue,addValue,
//                                                  auxStrError))
//                        {
//                            strError=QObject::tr("Algorithms::intercalibrationComputation");
//                            strError+=QObject::tr("\nError applying intercalibration to file:\n%1\nError:\n%2")
//                                    .arg(rasterFileName).arg(auxStrError);
//                            return(false);
//                        }
//                        iterTuplekey++;
//                    }
                }
                contAux++;
            }
            iterTotalRasterFilesByJd++;
        }
    }
    progress4.setValue(bandsToProcess.size());
    progress4.close();
    resultsFile.close();
    return(true);
}

bool Algorithms::ndviComputation(QString quadkey,
                                 QString rasterFile,
                                 QString computationMethod,
                                 bool to8Bits, // tiene sentido para calculo con niveles digitales, si es true se supone que es asi
                                 QString rasterUnitConversion,
                                 QString redFileName,
                                 QString nirFileName,
                                 QString outputFileName,
                                 bool reprocess,
                                 QString &strError,
                                 QVector<QVector<float> >& values,
                                 bool readValues)
{
    QString strAuxError;
    if(rasterUnitConversion.compare(PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_NONE)!=0)
    {
        strError=QObject::tr("Algorithms::ndviComputation");
        strError+=QObject::tr("\nRaster unit conversion invalid: %1").arg(rasterUnitConversion);
        return(false);
    }
    if(!QFile::exists(outputFileName)
            ||reprocess)
    {
        QString strNoDataValue;
        mPtrParametersManager->getParameter(ALGORITHMS_REFL_PARAMETER_NO_DATA_VALUE)->getValue(strNoDataValue);
        double refNoDataValue=strNoDataValue.toDouble();
        mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_NO_DATA_VALUE)->getValue(strNoDataValue);
        double ndviNoDataValue=strNoDataValue.toDouble();
        QMap<QString,QString> ndviImageOptions;
        mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_IMAGE_OPTIONS)->getValue(ndviImageOptions);
        QString strBuildOverviews;
        mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_BUILD_OVERVIEWS)->getValue(strBuildOverviews);
        bool buildOverviews=false;
        if(strBuildOverviews.compare("yes",Qt::CaseInsensitive)==0)
            buildOverviews=true;
        IGDAL::Raster* ptrRedRasterFile=new IGDAL::Raster(mPtrCrsTools);
        if(!ptrRedRasterFile->setFromFile(redFileName,strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                    .arg(redFileName).arg(strAuxError);
            return(false);
        }
        IGDAL::Raster* ptrNirRasterFile=new IGDAL::Raster(mPtrCrsTools);
        if(!ptrNirRasterFile->setFromFile(nirFileName,strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                    .arg(nirFileName).arg(strAuxError);
            return(false);
        }
        // Si es imagen de sentinel2 el no data value no es el que yo pongo al calcular
        // la imagen de reflectividades porque ya está en reflectividades
        double refNoDataValueInNir;
        if(!ptrNirRasterFile->getNoDataValue(0,refNoDataValueInNir,strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                    .arg(nirFileName).arg(strAuxError);
            return(false);
        }
        double refNoDataValueInRed;
        if(!ptrRedRasterFile->getNoDataValue(0,refNoDataValueInRed,strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                    .arg(redFileName).arg(strAuxError);
            return(false);
        }
        double redRasterFilteTo8BitsFactor=1.0;
        if(to8Bits)
        {
            if(!ptrRedRasterFile->getFactorTo8Bits(redRasterFilteTo8BitsFactor,strAuxError))
            {
                strError=QObject::tr("Algorithms::ndviComputation");
                strError+=QObject::tr("\nError getting factor to 8 bits from raster file:\n%1\nError:\n%2")
                        .arg(redFileName).arg(strAuxError);
                return(false);
            }
        }
        double nirRasterFilteTo8BitsFactor=1.0;
        if(to8Bits)
        {
            if(!ptrNirRasterFile->getFactorTo8Bits(nirRasterFilteTo8BitsFactor,strAuxError))
            {
                strError=QObject::tr("Algorithms::ndviComputation");
                strError+=QObject::tr("\nError getting factor to 8 bits from raster file:\n%1\nError:\n%2")
                        .arg(nirFileName).arg(strAuxError);
                return(false);
            }
        }
        IGDAL::ImageTypes imageType=IGDAL::GEOTIFF;
        GDALDataType gdalDataType=GDT_Float32;
        int numberOfBands=1;
        int columns,rows;
        if(!ptrRedRasterFile->getSize(columns,rows,strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                    .arg(redFileName).arg(strAuxError);
            return(false);
        }
        int nirColumns,nirRows;
        if(!ptrNirRasterFile->getSize(nirColumns,nirRows,strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nDifferent dimensions in two files:\n%1\nand:\n%2")
                    .arg(redFileName).arg(nirFileName);
            return(false);
        }
        bool internalGeoRef=true;
        bool externalGeoRef=false;
        QString crsDescription=ptrRedRasterFile->getCrsDescription();
        QString crsNirDescription=ptrNirRasterFile->getCrsDescription();
        if(crsNirDescription.compare(crsDescription,Qt::CaseInsensitive)!=0)
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nDifferent crs in two files:\n%1\nand:\n%2")
                    .arg(redFileName).arg(nirFileName);
            return(false);
        }
        double nwFc,nwSc,seFc,seSc;
        if(!ptrRedRasterFile->getBoundingBox(nwFc,nwSc,seFc,seSc,
                                            strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError getting bounding box from raster file:\n%1\nError:\n%2")
                    .arg(redFileName).arg(strAuxError);
            return(false);
        }
        double nirNwFc,nirNwSc,nirSeFc,nirSeSc;
        if(!ptrNirRasterFile->getBoundingBox(nirNwFc,nirNwSc,nirSeFc,nirSeSc,
                                            strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError getting bounding box from raster file:\n%1\nError:\n%2")
                    .arg(nirFileName).arg(strAuxError);
            return(false);
        }
        if(nwFc!=nirNwFc||nwSc!=nirNwSc||seFc!=nirSeFc||seSc!=nirSeSc)
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nDifferent bounding box in two files:\n%1\nand:\n%2")
                    .arg(redFileName).arg(nirFileName);
            return(false);
        }
        int numberOfBand=0;
        int initialColumn=0;
        int initialRow=0;
        int columnsToRead=columns;
        int rowsToRead=rows;
        int numberOfPixels=columnsToRead*rowsToRead;
        float* redData;//=(float*)malloc(numberOfPixels*sizeof(GDT_Float32));
    //    =(GDT_Float32 *) CPLMalloc(numberOfPixels*sizeof(GDT_Float32));
        if(!ptrRedRasterFile->readValues(numberOfBand, // desde 0
                                         initialColumn,
                                         initialRow,
                                         columnsToRead,
                                         rowsToRead,
                                         redData,
                                         strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError reading values from raster file:\n%1\nError:\n%2")
                    .arg(redFileName).arg(strAuxError);
            return(false);
        }
        delete(ptrRedRasterFile);
        float* nirData;//=(float*)malloc(numberOfPixels*sizeof(GDT_Float32));
        if(!ptrNirRasterFile->readValues(numberOfBand, // desde 0
                                         initialColumn,
                                         initialRow,
                                         columnsToRead,
                                         rowsToRead,
                                         nirData,
                                         strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError reading values from raster file:\n%1\nError:\n%2")
                    .arg(redFileName).arg(strAuxError);
            return(false);
        }
        delete(ptrNirRasterFile);
        // https://landsat.usgs.gov/using-usgs-landsat-8-product
        float* pData=(float*)malloc(numberOfPixels*sizeof(GDT_Float32));
        if(readValues)
        {
            values.clear();
        }
        values.resize(rows);
        for(int row=0;row<rows;row++)
        {
            values[row].resize(columns);
            for(int column=0;column<columns;column++)
            {
                int posInData=row*columns+column;
                float nirValue=nirData[posInData]*nirRasterFilteTo8BitsFactor;
                float redValue=redData[posInData]*redRasterFilteTo8BitsFactor;
                float ndviValue=(nirValue-redValue)/(nirValue+redValue);
                if(fabs(nirValue-refNoDataValueInNir)<0.01
                        ||fabs(redValue-refNoDataValueInRed)<0.01
                        ||fabs(nirValue-refNoDataValue)<0.01
                        ||fabs(redValue-refNoDataValue)<0.01)
                {
                    ndviValue=ndviNoDataValue;
                }
                pData[posInData]=ndviValue;
                values[row][column]=ndviValue;
            }
        }
//        for(int i=0;i<numberOfPixels;i++)
//        {
//            if(fabs(nirData[i]-refNoDataValue)<0.1
//                    ||fabs(redData[i]-refNoDataValue)<0.1)
//            {
//                pData[i]=ndviNoDataValue;
//            }
//            else
//            {
//                pData[i]=(nirData[i]-redData[i])/(nirData[i]+redData[i]);
//            }
//        }
    //    for(int row=0;row<rows;row++)
    //    {
    //        for(int column=0;column<columns;column++)
    //        {
    //            int posInData=row*columns+column;
    //            pData[posInData]=1.0/sin(sunElevation*pi/180.)*(pData[posInData]*multValue+addValue);
    //        }
    //    }
        QVector<double> georef;
        bool closeAfterCreate=false;
        IGDAL::Raster* ptrNdviRasterFile=new IGDAL::Raster(mPtrCrsTools);
        if(!ptrNdviRasterFile->createRaster(outputFileName, // Se le añade la extension
                                            imageType,gdalDataType,
                                            numberOfBands,
                                            columns,rows,
                                            internalGeoRef,externalGeoRef,crsDescription,
                                            nwFc,nwSc,seFc,seSc,
                                            georef, // vacío si se georeferencia con las esquinas
                                            ndviNoDataValue,
                                            closeAfterCreate,
                                            ndviImageOptions,
//                                            buildOverviews,
                                            strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError creating raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            return(false);
        }
        if(!ptrNdviRasterFile->writeValues(numberOfBand, // desde 0
                                           initialColumn,
                                           initialRow,
                                           columnsToRead,
                                           rowsToRead,
                                           pData,
                                           strError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError writing raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            return(false);
        }
        if(buildOverviews)
        {
            if(!ptrNdviRasterFile->buildOverviews(strAuxError))
            {
                return(false);
            }
        }
        free(redData);
        free(nirData);
        free(pData);
        delete(ptrNdviRasterFile);
        if(!mPtrPersistenceManager->insertNdviTuplekeyFile(quadkey,
                                                          rasterFile,
                                                          computationMethod,
                                                          rasterUnitConversion,
                                                          outputFileName,
                                                          strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError storing ndvi file in database:\nNdvi file:%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            return(false);
        }
    }
    else if(readValues)
    {
        values.clear();
        QString strNoDataValue;
        mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_NO_DATA_VALUE)->getValue(strNoDataValue);
        double ndviNoDataValue=strNoDataValue.toDouble();
        IGDAL::Raster* ptrNdviRasterFile=new IGDAL::Raster(mPtrCrsTools);
        if(!ptrNdviRasterFile->setFromFile(outputFileName,strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            return(false);
        }
//        IGDAL::ImageTypes imageType=IGDAL::GEOTIFF;
//        GDALDataType gdalDataType=GDT_Float32;
        int columns,rows;
        if(!ptrNdviRasterFile->getSize(columns,rows,strAuxError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            return(false);
        }
        int numberOfBand=0;
        int initialColumn=0;
        int initialRow=0;
        int columnsToRead=columns;
        int rowsToRead=rows;
        int numberOfPixels=columnsToRead*rowsToRead;
        float* pData;//=(float*)malloc(numberOfPixels*sizeof(GDT_Float32));
    //    =(GDT_Float32 *) CPLMalloc(numberOfPixels*sizeof(GDT_Float32));
        if(!ptrNdviRasterFile->readValues(numberOfBand, // desde 0
                                         initialColumn,
                                         initialRow,
                                         columnsToRead,
                                         rowsToRead,
                                         pData,
                                         strError))
        {
            strError=QObject::tr("Algorithms::ndviComputation");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            return(false);
        }
        delete(ptrNdviRasterFile);
        values.resize(rows);
        for(int row=0;row<rows;row++)
        {
            values[row].resize(columns);
            for(int column=0;column<columns;column++)
            {
                int posInData=row*columns+column;
                float ndviValue=pData[posInData];
                if(fabs(ndviValue-ndviNoDataValue)<0.01)
                {
                    ndviValue=ndviNoDataValue;
                }
                values[row][column]=ndviValue;
            }
        }
        free(pData);
    }
    return(true);
}

bool Algorithms::piasComputation(QVector<QString> &rasterFiles,
                                 QMap<QString, QString> &rasterTypesByRasterFile,
                                 QMap<QString, QVector<QString> > &rasterFilesByQuadkey,
                                 QMap<QString, QMap<QString, QMap<QString, QString> > > &quadkeysRasterFilesByQuadkeyByRasterFileAndByBand,
                                 QMap<QString, int> &jdByRasterFile,
                                 QMap<QString, double> &sunAzimuthByRasterFile,
                                 QMap<QString, double> &sunElevationByRasterFile,
                                 QMap<QString, QMap<QString, double> > &reflectanceAddValueByRasterFileAndByBand,
                                 QMap<QString, QMap<QString, double> > &reflectanceMultValueByRasterFileAndByBand,
                                 bool mergeFiles,
                                 bool reprocessFiles,
                                 bool reprojectFiles,
                                 QFile& resultsFile,
                                 QString &strError)
{
    bool debugMode=true;
    QString landsat8IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8;
    QString sentinel2IdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2;
    QString orthoimageIdDb=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_ORTHOIMAGE;
    QString ndviRasterUnitConversion=PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_NONE;
    QString strAuxError;
    // Obtener la ruta del workspace
    QMap<QString, QVector<QString> >::const_iterator iterRasterFilesByQuadkey=rasterFilesByQuadkey.begin();
    QString workspaceBasePath;
    bool existsSentinel2=false;
    while(iterRasterFilesByQuadkey!=rasterFilesByQuadkey.end())
    {
        QString quadkey=iterRasterFilesByQuadkey.key();
        QVector<QString> rasterFilesInQuadkey=iterRasterFilesByQuadkey.value();
        int numberOfRasterFilesInQuadkey=rasterFilesInQuadkey.size();
        for(int nrf=0;nrf<numberOfRasterFilesInQuadkey;nrf++)
        {
            QString rasterFile=rasterFilesInQuadkey[nrf];
            QString rasterType=rasterTypesByRasterFile[rasterFile];
            if(rasterType.compare(orthoimageIdDb)==0)
            {
                continue;
            }
            if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand.contains(quadkey))
            {
                continue;
            }
            if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey].contains(rasterFile))
            {
                continue;
            }
            QString redBandRasterFile,nirBandRasterFile;
            bool validRasterType=false;
            if(rasterType.compare(landsat8IdDb)==0)
            {
                if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_LANDSAT8_BAND_B4_CODE)
                        ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_LANDSAT8_BAND_B5_CODE))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                redBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_LANDSAT8_BAND_B4_CODE];
                nirBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_LANDSAT8_BAND_B5_CODE];
                validRasterType=true;
            }
            if(rasterType.compare(sentinel2IdDb)==0)
            {
                if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_SENTINEL2_BAND_B4_CODE)
                        ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(REMOTESENSING_SENTINEL2_BAND_B8_CODE))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                redBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_SENTINEL2_BAND_B4_CODE];
                nirBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][REMOTESENSING_SENTINEL2_BAND_B8_CODE];
                validRasterType=true;
                if(!existsSentinel2)
                {
                    existsSentinel2=true;
                }
            }
            if(!validRasterType)
            {
                strError=QObject::tr("Algorithms::piasComputation");
                strError+=QObject::tr("\nInvalid raste type: %1\nfor raster file: %2")
                        .arg(rasterType).arg(rasterFile);
                return(false);
            }
            if(QFile::exists(redBandRasterFile))
            {
                QFileInfo redBandRasterFileInfo(redBandRasterFile);
                QDir redBandDir=redBandRasterFileInfo.absoluteDir();
                redBandDir.cdUp();
                workspaceBasePath=redBandDir.absolutePath();
                break;
            }
            else if(QFile::exists(nirBandRasterFile))
            {
                QFileInfo nirBandRasterFileInfo(nirBandRasterFile);
                QDir nirBandDir=nirBandRasterFileInfo.absoluteDir();
                nirBandDir.cdUp();
                workspaceBasePath=nirBandDir.absolutePath();
                break;
            }
        }
        if(!workspaceBasePath.isEmpty())
        {
            iterRasterFilesByQuadkey=rasterFilesByQuadkey.end();
        }
        else
        {
            iterRasterFilesByQuadkey++;
        }
    }

    QString reflectanceSuffixFileName,reflectanceImageFormat,reflectanceComputationMethod,reflectanceImageFileExtension,strCloudValue;
    QString strPiaNdviStdMax,strPiaNdviMax,strPiaNdviMin,piasFileBaseName,piasImageFormat,strPiaNoDataValue,strPiaValue,piasComputationMethod;
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD)->getValue(piasComputationMethod);
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_IMAGE_BASE_NAME)->getValue(piasFileBaseName);
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_IMAGE_FORMAT)->getValue(piasImageFormat);
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_NDVI_STD_MAX)->getValue(strPiaNdviStdMax);
    double piaNdviStdMax=strPiaNdviStdMax.toDouble();
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_NDVI_MAX)->getValue(strPiaNdviMax);
    double piaNdviMax=strPiaNdviMax.toDouble();
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_NDVI_MIN)->getValue(strPiaNdviMin);
    double piaNdviMin=strPiaNdviMin.toDouble();
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_PIA_NO_DATA_VALUE)->getValue(strPiaNoDataValue);
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_PIA_VALUE)->getValue(strPiaValue);
    int piaValue=strPiaValue.toInt();
    double piaNoDataValue=strPiaNoDataValue.toDouble();
    mPtrParametersManager->getParameter(ALGORITHMS_REFL_PARAMETER_IMAGE_SUFFIX)->getValue(reflectanceSuffixFileName);
    mPtrParametersManager->getParameter(ALGORITHMS_REFL_PARAMETER_IMAGE_FORMAT)->getValue(reflectanceImageFormat);
    mPtrParametersManager->getParameter(ALGORITHMS_REFL_PARAMETER_COMPUTATION_METHOD)->getValue(reflectanceComputationMethod);
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_PIA_CLOUD_VALUE)->getValue(strCloudValue);
    QMap<QString,QString> piaImageOptions;
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_IMAGE_OPTIONS)->getValue(piaImageOptions);
    QMap<QString,QString> ndviImageOptions;
    mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_IMAGE_OPTIONS)->getValue(ndviImageOptions);
    QString strBuildOverviews;
    mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_BUILD_OVERVIEWS)->getValue(strBuildOverviews);
    bool ndviBuildOverviews=false;
    if(strBuildOverviews.compare("yes",Qt::CaseInsensitive)==0)
        ndviBuildOverviews=true;
    mPtrParametersManager->getParameter(ALGORITHMS_PIAS_PARAMETER_BUILD_OVERVIEWS)->getValue(strBuildOverviews);
    bool piasBuildOverviews=false;
    if(strBuildOverviews.compare("yes",Qt::CaseInsensitive)==0)
        piasBuildOverviews=true;
    int cloudValue=strCloudValue.toInt();
    QString strNdviNoDataValue,piasImageFileExtension;
    if(piasComputationMethod.compare(ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD_1)!=0)
    {
        strError=QObject::tr("Algorithms::piasComputation");
        strError+=QObject::tr("\nParameter value for pias computation method: %1\n is not it is not implemented yet")
                .arg(piasComputationMethod);
        return(false);
    }
//    if(existsSentinel2
//            &&piasComputationMethod.compare(ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD_3)==0)
//    {
//        strError=QObject::tr("Algorithms::piasComputation");
//        strError+=QObject::tr("\nParameter value for pias computation method: %1\n is not valid with sentinel2 data")
//                .arg(piasComputationMethod);
//        return(false);
//    }
    if(piasImageFormat.compare(ALGORITHMS_PIAS_PARAMETER_IMAGE_FORMAT_1)!=0)
    {
        strError=QObject::tr("Algorithms::piasComputation");
        strError+=QObject::tr("\nParameter value for pias image format: %1\n is not in valid domain: \n[%2]")
                .arg(reflectanceImageFormat).arg(ALGORITHMS_PIAS_PARAMETER_IMAGE_FORMAT_1);
        return(false);
    }
    if(piasImageFormat.compare(ALGORITHMS_PIAS_PARAMETER_IMAGE_FORMAT_1)==0)
    {
        piasImageFileExtension=RASTER_TIFF_FILE_EXTENSION;
    }
    mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_NO_DATA_VALUE)->getValue(strNdviNoDataValue);
    double ndviNoDataValue=strNdviNoDataValue.toDouble();
    if(reflectanceImageFormat.compare(ALGORITHMS_REFL_PARAMETER_IMAGE_FORMAT_1)!=0)
    {
        strError=QObject::tr("Algorithms::piasComputation");
        strError+=QObject::tr("\nParameter value for reflectance image format: %1\n is not in valid domain: \n[%2]")
                .arg(reflectanceImageFormat).arg(ALGORITHMS_REFL_PARAMETER_IMAGE_FORMAT_1);
        return(false);
    }
    if(reflectanceImageFormat.compare(ALGORITHMS_REFL_PARAMETER_IMAGE_FORMAT_1)==0)
    {
        reflectanceImageFileExtension=RASTER_TIFF_FILE_EXTENSION;
    }
    if(reflectanceComputationMethod.compare(ALGORITHMS_REFL_COMPUTATION_METHOD_1)!=0)
    {
        strError=QObject::tr("Algorithms::piasComputation");
        strError+=QObject::tr("\nParameter value for reflectance computation method: %1\n is not in valid domain: \n[%2]")
                .arg(reflectanceComputationMethod).arg(ALGORITHMS_REFL_COMPUTATION_METHOD_1);
        return(false);
    }
    QString ndviSuffixFileName,ndviImageFormat,ndviComputationMethod,ndviImageFileExtension;
    mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_IMAGE_SUFFIX)->getValue(ndviSuffixFileName);
    mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_IMAGE_FORMAT)->getValue(ndviImageFormat);
    mPtrParametersManager->getParameter(ALGORITHMS_NDVI_PARAMETER_COMPUTATION_METHOD)->getValue(ndviComputationMethod);
    if(ndviImageFormat.compare(ALGORITHMS_NDVI_PARAMETER_IMAGE_FORMAT_1)!=0)
    {
        strError=QObject::tr("Algorithms::piasComputation");
        strError+=QObject::tr("\nParameter value for reflectance image format: %1\n is not in valid domain: \n[%2]")
                .arg(ndviImageFormat).arg(ALGORITHMS_NDVI_PARAMETER_IMAGE_FORMAT_1);
        return(false);
    }
    if(ndviImageFormat.compare(ALGORITHMS_NDVI_PARAMETER_IMAGE_FORMAT_1)==0)
    {
        ndviImageFileExtension=RASTER_TIFF_FILE_EXTENSION;
    }
    if(ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_1)!=0
            &&ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_2)!=0
            &&ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_3)!=0)
    {
        strError=QObject::tr("Algorithms::piasComputation");
        strError+=QObject::tr("\nParameter value for NDVI computation method: %1\n is not in valid domain: \n[%2]")
                .arg(ndviComputationMethod).arg(ALGORITHMS_NDVI_COMPUTATION_METHOD_1);
        return(false);
    }
    if(existsSentinel2
            &&(ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_2)==0
               ||ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_3)==0))
    {
        strError=QObject::tr("Algorithms::piasComputation");
        strError+=QObject::tr("\nParameter value for NDVI computation method: %1\n is not valid with sentinel2 data")
                .arg(piasComputationMethod);
        return(false);
    }
    bool ndviByDN=false;
    bool to8Bits=false;
    if(ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_2)==0
                   ||ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_3)==0)
    {
        ndviByDN=true;
        if(ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_3)==0)
        {
            to8Bits=true;
        }
    }

    // Paso 1: Crear imágenes de NDVI, si no existen o si se ha indicado que se desea recalcularlas
    int numberOfQuadkeys=rasterFilesByQuadkey.size();
    int quadkeysCont=0;
    QString title=QObject::tr("Computing NDVI raster files");
    QString msgGlobal=QObject::tr("Processing %1 quadkeys ...").arg(QString::number(numberOfQuadkeys));
    QProgressDialog progress(title, QObject::tr("Abort"),0,numberOfQuadkeys, mPtrWidgetParent);
    progress.setWindowModality(Qt::WindowModal);
    progress.setLabelText(msgGlobal);
    progress.show();
    qApp->processEvents();
    if (!resultsFile.open(QFile::Append |QFile::Text))
    {
        strError=QObject::tr("Algorithms::piasComputation");
        strError+=QObject::tr("\nError opening results file: \n %1").arg(resultsFile.fileName());
        return(false);
    }
    QTextStream out(&resultsFile);
    out<<"- Metodo de calculo de PIAS ................: "<<piasComputationMethod<<"\n";
    if(piasComputationMethod.compare(ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD_1)==0)
    {
        out<<"  - NDVI medio mayor a .....................: "<<strPiaNdviMax<<"\n";
        out<<"  - NDVI medio menor o igual a .............: "<<strPiaNdviMin<<"\n";
        out<<"  - STD de NDVI menor a ....................: "<<strPiaNdviStdMax<<"\n";
    }
    out<<"- Formato de imagenes de PIAS ..............: "<<piasImageFormat<<"\n";
    out<<"- Valor de PIA en imagen de PIAS ...........: "<<strPiaValue<<"\n";
    out<<"- Valor para no dato en imagen de PIAS .....: "<<strPiaNoDataValue<<"\n";
    out<<"- Metodo de calculo de NDVI ................: "<<ndviComputationMethod<<"\n";
    out<<"- Formato de imagenes de NDVI ..............: "<<ndviImageFormat<<"\n";
    out<<"- Valor para no dato en imagen de NDVI .....: "<<strNdviNoDataValue<<"\n";
    if(ndviComputationMethod.compare(ALGORITHMS_NDVI_COMPUTATION_METHOD_1)==0)
    {
        out<<"- Metodo de calculo de Reflectividades .....: "<<reflectanceComputationMethod<<"\n";
        out<<"- Formato de imagenes de Reflectividades ...: "<<reflectanceImageFormat<<"\n";
    }
    out<<"- Numero de quadkeys a procesar ............: "<<QString::number(numberOfQuadkeys)<<"\n";
    iterRasterFilesByQuadkey=rasterFilesByQuadkey.begin();
    while(iterRasterFilesByQuadkey!=rasterFilesByQuadkey.end())
    {
        quadkeysCont++;
        progress.setValue(quadkeysCont);
        qApp->processEvents();
        if (progress.wasCanceled())
            break;
        QString quadkey=iterRasterFilesByQuadkey.key();
        QVector<QString> rasterFilesInQuadkey=iterRasterFilesByQuadkey.value();
        int numberOfRasterFilesInQuadkey=rasterFilesInQuadkey.size();
        QString piasFileName=workspaceBasePath+"/"+quadkey;
        piasFileName+=("/"+piasFileBaseName);
        piasFileName+=("_"+piasComputationMethod);
        QString strCurrentDateTime=QDateTime::currentDateTime().toString(ALGORITHMS_DATE_TIME_FILE_NAME_STRING_FORMAT);
        piasFileName+=("_"+strCurrentDateTime);
        piasFileName+=("."+piasImageFileExtension);
        QString ndviMeanFileName=workspaceBasePath+"/"+quadkey;
        ndviMeanFileName+=("/ndvi_"+ndviComputationMethod+"_mean."+piasImageFileExtension);
        QString ndviStdFileName=workspaceBasePath+"/"+quadkey;
        ndviStdFileName+=("/ndvi_"+ndviComputationMethod+"_std."+piasImageFileExtension);
        out<<"  - Quadkey ................................: "<<quadkey;
        // Comprobar si existe: con los mismos ficheros y fechas
        // compruebo si para cada rasterfile existen las bandas red y nir
        // si se han eliminado dira que son distintos porque se va a calcular con otros
        QString previousPiasFileName;
        int previousPiasFileId;
        {
            QVector<QString> usedRasterFilesInQuadkey;
            for(int nrf=0;nrf<numberOfRasterFilesInQuadkey;nrf++)
            {
                QString rasterFile=rasterFilesInQuadkey[nrf];
                QString rasterType=rasterTypesByRasterFile[rasterFile];
                QString redBandCode,nirBandCode,maskBandCode;
                if(rasterType.compare(landsat8IdDb)==0)
                {
                    redBandCode=REMOTESENSING_LANDSAT8_BAND_B4_CODE;
                    nirBandCode=REMOTESENSING_LANDSAT8_BAND_B5_CODE;
                    maskBandCode=REMOTESENSING_LANDSAT8_BAND_B0_CODE;
                }
                if(rasterType.compare(sentinel2IdDb)==0)
                {
                    redBandCode=REMOTESENSING_SENTINEL2_BAND_B4_CODE;
                    nirBandCode=REMOTESENSING_SENTINEL2_BAND_B8_CODE;
                    maskBandCode=REMOTESENSING_SENTINEL2_BAND_B0_CODE;
                }
                int jd=jdByRasterFile[rasterFile];
                if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand.contains(quadkey))
                {
                    continue;
                }
                if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey].contains(rasterFile))
                {
                    continue;
                }
                if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(redBandCode)
                        ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(nirBandCode))
    //                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
                {
                    continue;
                }
                QString redBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][redBandCode];
                QString nirBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][nirBandCode];
                bool existsFiles=true;
                if(!QFile::exists(redBandRasterFile))
                {
                    existsFiles=false;
                }
                if(!QFile::exists(nirBandRasterFile))
                {
                    existsFiles=false;
                }
                if(!existsFiles) // estos tienen que existir pero el de ndvi se puede recalcular
                {
                    continue;
                }
                usedRasterFilesInQuadkey.push_back(rasterFile);
            }
            if(usedRasterFilesInQuadkey.size()<2)
            {
                iterRasterFilesByQuadkey++;
                out<<" Not enough information\n";
                continue;
            }
            if(!mPtrPersistenceManager->getExistsPiasTuplekeyFile(quadkey,
                                                                  usedRasterFilesInQuadkey,
                                                                  piasComputationMethod,
                                                                  previousPiasFileName,
                                                                  previousPiasFileId,
                                                                  strAuxError))
            {
                strError=QObject::tr("Algorithms::piasComputation");
                strError+=QObject::tr("\nError recovering pias file in database:\nPias file:%1\nError:\n%2")
                        .arg(piasFileName).arg(strAuxError);
                return(false);
            }
        }
        if(!previousPiasFileName.isEmpty())
        {
            if(!reprocessFiles)
            {
                out<<"\n"<<"    - PIAS file name .......................: Exits file, "<<previousPiasFileName<<"\n";
                iterRasterFilesByQuadkey++;
                continue;
            }
            else
            {
                QFile::remove(previousPiasFileName);
                // Eliminar en la base de datos
                if(!mPtrPersistenceManager->deletePiasFile(previousPiasFileId,
                                                           strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError storing deleting pias file in database:\nPias file:%1\nError:\n%2")
                            .arg(previousPiasFileName).arg(strAuxError);
                    return(false);
                }
            }
        }
        out<<"\n"<<"    - PIAS file name .......................: "<<piasFileName<<"\n";
        QVector<QString> usedRasterFilesInQuadkey;
        IGDAL::Raster* ptrPiasRasterFile=NULL;
        GByte*  piasData=NULL;
        // Para depuracion
        IGDAL::Raster* ptrNdviMeanRasterFile=NULL;
        float* ndviMeanData=NULL;
        IGDAL::Raster* ptrNdviStdRasterFile=NULL;
        float* ndviStdData=NULL;
        int piasColumns,piasRows;
        out<<"    - Numero de imagenes ...................: "<<QString::number(numberOfRasterFilesInQuadkey)<<"\n";
        QMap<int,QMap<int,QVector<int> > > cloudyPixelsByJdByRow;
        QMap<int,QVector<QVector<float> > > ndvisByJd; // [row][column]
        QString titleBis=QObject::tr("Computing NDVI raster files for quadkey: %1").arg(quadkey);
        QString msgGlobalBis=QObject::tr("Processing %1 raster files ...").arg(QString::number(numberOfRasterFilesInQuadkey));
        QProgressDialog progressBis(titleBis, QObject::tr("Abort"),0,numberOfRasterFilesInQuadkey, mPtrWidgetParent);
        progressBis.setWindowModality(Qt::WindowModal);
        progressBis.setLabelText(msgGlobalBis);
        progressBis.show();
        qApp->processEvents();
        for(int nrf=0;nrf<numberOfRasterFilesInQuadkey;nrf++)
        {
            progressBis.setValue(nrf+1);
            qApp->processEvents();
            if (progressBis.wasCanceled())
                break;
            QString rasterFile=rasterFilesInQuadkey[nrf];
            QString rasterType=rasterTypesByRasterFile[rasterFile];
            QString redBandCode,nirBandCode,maskBandCode;
            if(rasterType.compare(landsat8IdDb)==0)
            {
                redBandCode=REMOTESENSING_LANDSAT8_BAND_B4_CODE;
                nirBandCode=REMOTESENSING_LANDSAT8_BAND_B5_CODE;
                maskBandCode=REMOTESENSING_LANDSAT8_BAND_B0_CODE;
            }
            if(rasterType.compare(sentinel2IdDb)==0)
            {
                redBandCode=REMOTESENSING_SENTINEL2_BAND_B4_CODE;
                nirBandCode=REMOTESENSING_SENTINEL2_BAND_B8_CODE;
                maskBandCode=REMOTESENSING_SENTINEL2_BAND_B0_CODE;
            }
            int jd=jdByRasterFile[rasterFile];
            out<<"      - Imagen .............................: "<<rasterFile<<"\n";
            if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand.contains(quadkey))
            {
                out<<"        *** There are no files for all bands"<<"\n";
                continue;
            }
            if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey].contains(rasterFile))
            {
                out<<"        *** There are no files for all bands"<<"\n";
                continue;
            }
            if(!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(redBandCode)
                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(nirBandCode))
//                    ||!quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[rasterFile][quadkey].contains(REMOTESENSING_LANDSAT8_BAND_B0_CODE))
            {
                out<<"        *** There are no files for all bands"<<"\n";
                continue;
            }
            QString redBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][redBandCode];
            QString nirBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][nirBandCode];
            bool existsFiles=true;
            if(!QFile::exists(redBandRasterFile))
            {
                out<<"        *** File not found: "<<redBandRasterFile<<"\n";
                existsFiles=false;
            }
            if(!QFile::exists(nirBandRasterFile))
            {
                out<<"        *** File not found: "<<nirBandRasterFile<<"\n";
                existsFiles=false;
            }
            if(!existsFiles)
            {
                continue;
            }
            // Compruebo si existe el fichero NDVI
            QFileInfo redBandRasterFileInfo(redBandRasterFile);
            QString ndviRasterFile=redBandRasterFileInfo.absolutePath()+"/"+rasterFile;
            ndviRasterFile+=(ndviSuffixFileName+"_"+ndviComputationMethod+"."+ndviImageFileExtension);
//            bool existsNdviRasterFile=false;
//            if(QFile::exists(ndviRasterFile))
//            {
//                existsNdviRasterFile=true;
//            }
//            if(existsNdviRasterFile&&!reprocessFiles)
//            {
//                continue;
//            }
            QString reflectanceRedBandRasterFile,reflectanceNirBandRasterFile;
            if(rasterType.compare(landsat8IdDb)==0
                    &&!ndviByDN)
            {
                reflectanceRedBandRasterFile=redBandRasterFileInfo.absolutePath()+"/"+redBandRasterFileInfo.baseName();
                reflectanceRedBandRasterFile+=(reflectanceSuffixFileName+"_"+reflectanceComputationMethod+"."+reflectanceImageFileExtension);
                double sunElevation=sunElevationByRasterFile[rasterFile];
                if(!QFile::exists(reflectanceRedBandRasterFile)
                        ||reprocessFiles)
//                if(!QFile::exists(reflectanceRedBandRasterFile))
                {
                    double addValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][REMOTESENSING_LANDSAT8_BAND_B4_CODE];
                    double multValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][REMOTESENSING_LANDSAT8_BAND_B4_CODE];
                    if(!reflectanceComputation(redBandRasterFile,
                                               reflectanceRedBandRasterFile,
                                               sunElevation,
                                               addValue,
                                               multValue,
                                               strAuxError))
                    {
                        strError=QObject::tr("Algorithms::piasComputation");
                        strError+=QObject::tr("\nError computing reflectance for raster file:\n%1\nError:\n%2")
                                .arg(redBandRasterFile).arg(strAuxError);
                        return(false);
                    }
                }
                QFileInfo nirBandRasterFileInfo(nirBandRasterFile);
                reflectanceNirBandRasterFile=nirBandRasterFileInfo.absolutePath()+"/"+nirBandRasterFileInfo.baseName();
                reflectanceNirBandRasterFile+=(reflectanceSuffixFileName+"_"+reflectanceComputationMethod+"."+reflectanceImageFileExtension);
                if(!QFile::exists(reflectanceNirBandRasterFile)
                    ||reprocessFiles)
//                if(!QFile::exists(reflectanceNirBandRasterFile))
                {
                    double addValue=reflectanceAddValueByRasterFileAndByBand[rasterFile][REMOTESENSING_LANDSAT8_BAND_B5_CODE];
                    double multValue=reflectanceMultValueByRasterFileAndByBand[rasterFile][REMOTESENSING_LANDSAT8_BAND_B5_CODE];
                    if(!reflectanceComputation(nirBandRasterFile,
                                               reflectanceNirBandRasterFile,
                                               sunElevation,
                                               addValue,
                                               multValue,
                                               strAuxError))
                    {
                        strError=QObject::tr("Algorithms::piasComputation");
                        strError+=QObject::tr("\nError computing reflectance for raster file:\n%1\nError:\n%2")
                                .arg(nirBandRasterFile).arg(strAuxError);
                        return(false);
                    }
                }
            }
            else
            {
                reflectanceRedBandRasterFile=redBandRasterFile;
                reflectanceNirBandRasterFile=nirBandRasterFile;
            }
            QVector<QVector<float> > ndvis;
            bool readValues=true;
            if(!ndviComputation(quadkey,
                                rasterFile,
                                ndviComputationMethod,
                                to8Bits,
                                ndviRasterUnitConversion,
                                reflectanceRedBandRasterFile,
                                reflectanceNirBandRasterFile,
                                ndviRasterFile,
                                reprocessFiles,
                                strAuxError,
                                ndvis,
                                readValues))
            {
                strError=QObject::tr("Algorithms::piasComputation");
                strError+=QObject::tr("\nError computing ndvi for raster file:\n%1\nError:\n%2")
                        .arg(ndviRasterFile).arg(strAuxError);
                return(false);
            }
            ndvisByJd[jd]=ndvis;
            if(ptrPiasRasterFile==NULL)
            {
                ptrPiasRasterFile=new IGDAL::Raster(mPtrCrsTools);
                IGDAL::Raster* ptrNdviRasterFile=new IGDAL::Raster(mPtrCrsTools);
                if(!ptrNdviRasterFile->setFromFile(ndviRasterFile,strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                            .arg(ndviRasterFile).arg(strAuxError);
                    return(false);
                }
                if(!ptrNdviRasterFile->getSize(piasColumns,piasRows,strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError getting size fro raster file:\n%1\nError:\n%2")
                            .arg(ndviRasterFile).arg(strAuxError);
                    return(false);
                }
                bool piasInternalGeoRef=true;
                bool piasExternalGeoRef=false;
                QString piasCrsDescription=ptrNdviRasterFile->getCrsDescription();
                double piasNwFc,piasNwSc,piasSeFc,piasSeSc;
                if(!ptrNdviRasterFile->getBoundingBox(piasNwFc,piasNwSc,piasSeFc,piasSeSc,
                                                      strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError getting bounding box from raster file:\n%1\nError:\n%2")
                            .arg(ndviRasterFile).arg(strAuxError);
                    return(false);
                }
                delete(ptrNdviRasterFile);
                IGDAL::ImageTypes piasImageType=IGDAL::GEOTIFF;
                GDALDataType piasGdalDataType=ALGORITHMS_PIAS_GDAL_DATA_TYPE;
                int piasNumberOfBands=1;
                bool piasCloseAfterCreate=false;
                QVector<double> piasGeoref;
                if(!ptrPiasRasterFile->createRaster(piasFileName, // Se le añade la extension
                                                    piasImageType,piasGdalDataType,
                                                    piasNumberOfBands,
                                                    piasColumns,piasRows,
                                                    piasInternalGeoRef,piasExternalGeoRef,piasCrsDescription,
                                                    piasNwFc,piasNwSc,piasSeFc,piasSeSc,
                                                    piasGeoref, // vacío si se georeferencia con las esquinas
                                                    piaNoDataValue,
                                                    piasCloseAfterCreate,
                                                    piaImageOptions,
//                                                    piasBuildOverviews,
                                                    strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError creating raster file:\n%1\nError:\n%2")
                            .arg(piasFileName).arg(strAuxError);
                    return(false);
                }
                piasData=(GByte *) CPLMalloc(piasColumns*piasRows*sizeof(GByte));
                if(debugMode)
                {
                    ptrNdviMeanRasterFile=new IGDAL::Raster(mPtrCrsTools);
                    if(!ptrNdviMeanRasterFile->createRaster(ndviMeanFileName, // Se le añade la extension
                                                            piasImageType,GDT_Float32,
                                                            piasNumberOfBands,
                                                            piasColumns,piasRows,
                                                            piasInternalGeoRef,piasExternalGeoRef,piasCrsDescription,
                                                            piasNwFc,piasNwSc,piasSeFc,piasSeSc,
                                                            piasGeoref, // vacío si se georeferencia con las esquinas
                                                            ndviNoDataValue,
                                                            piasCloseAfterCreate,
                                                            ndviImageOptions,
//                                                            ndviBuildOverviews,
                                                            strAuxError))
                    {
                        strError=QObject::tr("Algorithms::piasComputation");
                        strError+=QObject::tr("\nError creating raster file:\n%1\nError:\n%2")
                                .arg(ndviMeanFileName).arg(strAuxError);
                        return(false);
                    }
                    ndviMeanData=(float*)CPLMalloc(piasColumns*piasRows*sizeof(GDT_Float32));
                    ptrNdviStdRasterFile=new IGDAL::Raster(mPtrCrsTools);
                    if(!ptrNdviStdRasterFile->createRaster(ndviStdFileName, // Se le añade la extension
                                                           piasImageType,GDT_Float32,
                                                           piasNumberOfBands,
                                                           piasColumns,piasRows,
                                                           piasInternalGeoRef,piasExternalGeoRef,piasCrsDescription,
                                                           piasNwFc,piasNwSc,piasSeFc,piasSeSc,
                                                           piasGeoref, // vacío si se georeferencia con las esquinas
                                                           ndviNoDataValue,
                                                           piasCloseAfterCreate,
                                                           ndviImageOptions,
//                                                           ndviBuildOverviews,
                                                           strAuxError))
                    {
                        strError=QObject::tr("Algorithms::piasComputation");
                        strError+=QObject::tr("\nError creating raster file:\n%1\nError:\n%2")
                                .arg(ndviStdFileName).arg(strAuxError);
                        return(false);
                    }
                    ndviStdData=(float*)CPLMalloc(piasColumns*piasRows*sizeof(GDT_Float32));
                }
            }
            if(quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile].contains(maskBandCode))
            {
                QString maskBandRasterFile=quadkeysRasterFilesByQuadkeyByRasterFileAndByBand[quadkey][rasterFile][maskBandCode];
                if(!QFile::exists(maskBandRasterFile))
                {
                    out<<"        *** File not found: "<<maskBandRasterFile<<"\n";
                    existsFiles=false;
                }
                IGDAL::Raster* ptrMaskRasterFile=NULL;
                ptrMaskRasterFile=new IGDAL::Raster(mPtrCrsTools);
                if(!ptrMaskRasterFile->setFromFile(maskBandRasterFile,strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                            .arg(maskBandRasterFile).arg(strAuxError);
                    return(false);
                }
                GDALDataType maskRasterDataType;
                if(!ptrMaskRasterFile->getDataType(maskRasterDataType,strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError getting data type for raster file:\n%1\nError:\n%2")
                            .arg(maskBandRasterFile).arg(strAuxError);
                    return(false);
                }
                if(maskRasterDataType!=GDT_Byte)
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nInvalid data type for raster file:\n%1\nError:\n%2")
                            .arg(maskBandRasterFile).arg(strAuxError);
                    return(false);
                }
                int maskColumns,maskRows;
                if(!ptrMaskRasterFile->getSize(maskColumns,maskRows,strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError getting size for raster file:\n%1\nError:\n%2")
                            .arg(maskBandRasterFile).arg(strAuxError);
                    return(false);
                }
                int numberOfPixels=maskColumns*maskRows;
                GDALRasterBand* ptrMaskRasterBand=NULL;
                if(!ptrMaskRasterFile->getRasterBand(0,ptrMaskRasterBand,strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError getting band for raster file:\n%1\nError:\n%2")
                            .arg(maskBandRasterFile).arg(strAuxError);
                    return(false);
                }
                GByte*  pAuxData=NULL;
                pAuxData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
                if(CE_None!=ptrMaskRasterBand->RasterIO( GF_Read, 0, 0, maskColumns, maskRows,
                                                         pAuxData, maskColumns, maskRows, maskRasterDataType, 0, 0 ))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError reading band for raster file:\n%1\nError:\n%2")
                            .arg(maskBandRasterFile).arg(strAuxError);
                    return(false);
                }
                QMap<int,QVector<int> > cloudyPixelsByRow;
                for(int row=0;row<maskRows;row++)
                {
                    for(int column=0;column<maskColumns;column++)
                    {
                        int posInData=row*maskColumns+column;
                        int value=pAuxData[posInData];
                        if(value==cloudValue)
                        {
                            if(!cloudyPixelsByRow.contains(row))
                            {
                                QVector<int> auxVector;
                                cloudyPixelsByRow[row]=auxVector;
                            }
                            cloudyPixelsByRow[row].push_back(column);
                        }
                    }
                }
                if(cloudyPixelsByRow.size()>0)
                {
                    cloudyPixelsByJdByRow[jd]=cloudyPixelsByRow;
                }
                CPLFree(pAuxData);
                delete(ptrMaskRasterBand);
            }
            usedRasterFilesInQuadkey.push_back(rasterFile);
        }
        progressBis.setValue(numberOfRasterFilesInQuadkey);
        progressBis.close();
        qApp->processEvents();
        if(ndvisByJd.size()>1)
        {
            QMap<int,QVector<QVector<float> > >::const_iterator iterNdvis=ndvisByJd.begin(); // [row][column]
            QVector<QVector<float> > ndviMeanValues;
            QVector<QVector<float> > ndviStdValues;
            QVector<QVector<int> > ndviNumberOfValues;
            int cont=0;
            while(iterNdvis!=ndvisByJd.end())
            {
                int jd=iterNdvis.key();
                QVector<QVector<float> > ndvis=iterNdvis.value();
                if(cont==0)
                {
                    for(int row=0;row<ndvis.size();row++)
                    {
                        QVector<float> auxFloat;
                        QVector<int> auxInt;
                        for(int col=0;col<ndvis[row].size();col++)
                        {
                            auxFloat.push_back(ndviNoDataValue);
                            auxInt.push_back(0);
                        }
                        ndviMeanValues.push_back(auxFloat);
                        ndviStdValues.push_back(auxFloat);
                        ndviNumberOfValues.push_back(auxInt);
                    }
                }
                cont++;
                for(int row=0;row<ndvis.size();row++)
                {
                    for(int col=0;col<ndvis[row].size();col++)
                    {
                        float ndviValue=ndvis[row][col];
                        if(fabs(ndviValue-ndviNoDataValue)<0.01)
                        {
                            continue;
                        }
                        if(cloudyPixelsByJdByRow.contains(jd))
                        {
                            if(cloudyPixelsByJdByRow[jd].contains(row))
                            {
                                if(cloudyPixelsByJdByRow[jd][row].contains(col))
                                {
                                    continue;
                                }
                            }
                        }
                        ndviNumberOfValues[row][col]++;
                        if(fabs(ndviMeanValues[row][col]-ndviNoDataValue)<0.01)
                        {
                            ndviMeanValues[row][col]=ndviValue;
                        }
                        else
                        {
                            ndviMeanValues[row][col]+=ndviValue;
                        }
                    }
                }
                iterNdvis++;
            }
            // Calcular el valor medio
            for(int row=0;row<ndviMeanValues.size();row++)
            {
                for(int col=0;col<ndviMeanValues[0].size();col++)
                {
                    if(ndviNumberOfValues[row][col]>0)
                    {
                        ndviMeanValues[row][col]=ndviMeanValues[row][col]/((float)ndviNumberOfValues[row][col]);
                        if(debugMode)
                        {
                            int posInData=row*ndviMeanValues[0].size()+col;
                            ndviMeanData[posInData]=ndviMeanValues[row][col];
                        }
                    }
                }
            }
            // Calcular el std
            iterNdvis=ndvisByJd.begin();
            while(iterNdvis!=ndvisByJd.end())
            {
                QVector<QVector<float> > ndvis=iterNdvis.value();
                for(int row=0;row<ndvis.size();row++)
                {
                    for(int col=0;col<ndvis[row].size();col++)
                    {
                        if(ndviNumberOfValues[row][col]==0)
                        {
                            continue;
                        }
                        float ndviMeanValue=ndviMeanValues[row][col];
                        if(fabs(ndviMeanValue-ndviNoDataValue)<0.01)
                        {
                            continue;
                        }
                        float ndviValue=ndvis[row][col];
                        if(fabs(ndviValue-ndviNoDataValue)<0.01)
                        {
                            continue;
                        }
                        float ndviDiff=ndviMeanValue-ndviValue;
                        if(fabs(ndviStdValues[row][col]-ndviNoDataValue)<0.01)
                        {
                            ndviStdValues[row][col]=ndviDiff*ndviDiff;
                        }
                        else
                        {
                            ndviStdValues[row][col]+=(ndviDiff*ndviDiff);
                        }
                    }
                }
                iterNdvis++;
            }
            // No /(n-1)
            for(int row=0;row<ndviMeanValues.size();row++)
            {
                for(int col=0;col<ndviMeanValues[0].size();col++)
                {
                    int posInData=row*ndviMeanValues[0].size()+col;
                    piasData[posInData]=piaNoDataValue;
                    if(ndviNumberOfValues[row][col]>0)
                    {
                        ndviStdValues[row][col]=sqrt(ndviStdValues[row][col]/((float)(ndviNumberOfValues[row][col])));
                        if(debugMode)
                        {
                            ndviStdData[posInData]=ndviStdValues[row][col];
                        }
                        if(ndviStdValues[row][col]<piaNdviStdMax
                                &&ndviMeanValues[row][col]>piaNdviMin
                                &&ndviMeanValues[row][col]<=piaNdviMax)
                        {
                            piasData[posInData]=piaValue;
                        }
                    }
                }
            }
            int piasNumberOBand=0;
            int piasInitialColumn=0;
            int piasInitialRow=0;
            GDALRasterBand* ptrPiasRasterBand;
            if(!ptrPiasRasterFile->getRasterBand(piasNumberOBand,ptrPiasRasterBand,strAuxError))
            {
                strError=QObject::tr("Algorithms::piasComputation");
                strError+=QObject::tr("\nError getting band for image:%1").arg(piasFileName);
                return(false);
            }
            if(CE_None!=ptrPiasRasterBand->RasterIO(GF_Write,piasInitialColumn,piasInitialRow,
                                                    piasColumns,piasRows,
                                                    piasData,piasColumns,piasRows,ALGORITHMS_PIAS_GDAL_DATA_TYPE,0,0))
            {
                strError=QObject::tr("Algorithms::piasComputation");
                strError+=QObject::tr("\nError writting values in image:%1").arg(piasFileName);
                return(false);
            }
            if(debugMode)
            {
                GDALRasterBand* ptrNdviMeanRasterBand;
                if(!ptrNdviMeanRasterFile->getRasterBand(piasNumberOBand,ptrNdviMeanRasterBand,strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError getting band for image:%1").arg(ndviMeanFileName);
                    return(false);
                }
                if(CE_None!=ptrNdviMeanRasterBand->RasterIO(GF_Write,piasInitialColumn,piasInitialRow,
                                                        piasColumns,piasRows,
                                                        ndviMeanData,piasColumns,piasRows,GDT_Float32,0,0))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError writting values in image:%1").arg(ndviMeanFileName);
                    return(false);
                }
                GDALRasterBand* ptrNdviStdRasterBand;
                if(!ptrNdviStdRasterFile->getRasterBand(piasNumberOBand,ptrNdviStdRasterBand,strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError getting band for image:%1").arg(ndviStdFileName);
                    return(false);
                }
                if(CE_None!=ptrNdviStdRasterBand->RasterIO(GF_Write,piasInitialColumn,piasInitialRow,
                                                        piasColumns,piasRows,
                                                        ndviStdData,piasColumns,piasRows,GDT_Float32,0,0))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError writting values in image:%1").arg(ndviStdFileName);
                    return(false);
                }
            }
        }
        if(debugMode)
        {
            if(ptrNdviMeanRasterFile!=NULL
                    &&ptrNdviStdRasterFile!=NULL)
            {
                if(ndviBuildOverviews)
                {
                    if(!ptrNdviMeanRasterFile->buildOverviews(strAuxError))
                    {
                        return(false);
                    }
                    if(!ptrNdviStdRasterFile->buildOverviews(strAuxError))
                    {
                        return(false);
                    }
                }
                delete(ptrNdviMeanRasterFile);
                CPLFree(ndviMeanData);
                delete(ptrNdviStdRasterFile);
                CPLFree(ndviStdData);
            }
        }
        if(ptrPiasRasterFile!=NULL) // si no se ha calculado no hay que hacer lo que sigue
        {
            if(piasBuildOverviews)
            {
                if(!ptrPiasRasterFile->buildOverviews(strAuxError))
                {
                    return(false);
                }
            }
            delete(ptrPiasRasterFile);
            CPLFree(piasData);
            if(ndvisByJd.size()>1)
            {
                QMap<QString,int> jdByUsedRasterFilesInQuadkey;
                for(int uf=0;uf<usedRasterFilesInQuadkey.size();uf++)
                {
                    QString auxRasterFile=usedRasterFilesInQuadkey.at(uf);
                    int auxJd=jdByRasterFile[auxRasterFile];
                    jdByUsedRasterFilesInQuadkey[auxRasterFile]=auxJd;
                }
                if(quadkey=="0331110303"
                        ||quadkey=="0331110023")
                {
                    int yo=1;
                }
                if(!mPtrPersistenceManager->insertPiasTuplekeyFile(quadkey,
                                                                   usedRasterFilesInQuadkey,
                                                                   jdByUsedRasterFilesInQuadkey,
                                                                   piasComputationMethod,
                                                                   piaValue,
                                                                   piasFileName,
                                                                   reprocessFiles,
                                                                   strAuxError))
                {
                    strError=QObject::tr("Algorithms::piasComputation");
                    strError+=QObject::tr("\nError storing pias file in database:\nPias file:%1\nError:\n%2")
                            .arg(piasFileName).arg(strAuxError);
                    return(false);
                }
            }
    //        break;
        }
        iterRasterFilesByQuadkey++;
    }
    progress.setValue(numberOfQuadkeys);
    progress.close();
    qApp->processEvents();
    resultsFile.close();
    return(true);
}

bool Algorithms::readMaskRasterFile(QString maskBandRasterFile,
                                    int maskValue,
                                    QVector<double> &georef,
                                    QVector<QVector<bool> > &values,
                                    float &maskValuesPercentage,
                                    QString &strError)
{
    QString strAuxError;
    values.clear();
    georef.clear();
    if(!QFile::exists(maskBandRasterFile))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nNot exists raster file:\n%1")
                .arg(maskBandRasterFile);
        return(false);
    }
    IGDAL::Raster* ptrMaskRasterFile=NULL;
    ptrMaskRasterFile=new IGDAL::Raster(mPtrCrsTools);
    if(!ptrMaskRasterFile->setFromFile(maskBandRasterFile,strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    if(!ptrMaskRasterFile->getGeoRef(georef,
                                     strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError getting georef from raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    GDALDataType maskRasterDataType;
    if(!ptrMaskRasterFile->getDataType(maskRasterDataType,strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError getting data type for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    if(maskRasterDataType!=GDT_Byte)
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nInvalid data type for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    int maskColumns,maskRows;
    if(!ptrMaskRasterFile->getSize(maskColumns,maskRows,strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError getting size for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    int numberOfPixels=maskColumns*maskRows;
    GDALRasterBand* ptrMaskRasterBand=NULL;
    if(!ptrMaskRasterFile->getRasterBand(0,ptrMaskRasterBand,strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError getting band for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    GByte*  pAuxData=NULL;
    pAuxData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
    if(CE_None!=ptrMaskRasterBand->RasterIO( GF_Read, 0, 0, maskColumns, maskRows,
                                             pAuxData, maskColumns, maskRows, maskRasterDataType, 0, 0 ))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError reading band for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    values.resize(maskRows);
    int numberOfMaskValues=0;
    for(int row=0;row<maskRows;row++)
    {
        values[row].resize(maskColumns);
        for(int column=0;column<maskColumns;column++)
        {
            int posInData=row*maskColumns+column;
            int value=pAuxData[posInData];
            if(value==maskValue)
            {
                values[row][column]=true;
                numberOfMaskValues++;
            }
            else
            {
                values[row][column]=false;
            }
        }
    }
    maskValuesPercentage=((float)numberOfMaskValues/(float)(maskColumns*maskRows))*100.0;
    CPLFree(pAuxData);
    delete(ptrMaskRasterFile);
    return(true);
}

bool Algorithms::readMaskRasterFile(QString maskBandRasterFile,
                                    int maskValue,
                                    QVector<double> &georef,
                                    QMap<int, QVector<int> > &values,
                                    float &maskValuesPercentage,
                                    QString &strError)
{
    QString strAuxError;
    values.clear();
    georef.clear();
    if(!QFile::exists(maskBandRasterFile))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nNot exists raster file:\n%1")
                .arg(maskBandRasterFile);
        return(false);
    }
    IGDAL::Raster* ptrMaskRasterFile=NULL;
    ptrMaskRasterFile=new IGDAL::Raster(mPtrCrsTools);
    if(!ptrMaskRasterFile->setFromFile(maskBandRasterFile,strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    if(!ptrMaskRasterFile->getGeoRef(georef,
                                     strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError getting georef from raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    GDALDataType maskRasterDataType;
    if(!ptrMaskRasterFile->getDataType(maskRasterDataType,strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError getting data type for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    if(maskRasterDataType!=GDT_Byte)
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nInvalid data type for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    int maskColumns,maskRows;
    if(!ptrMaskRasterFile->getSize(maskColumns,maskRows,strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError getting size for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    int numberOfPixels=maskColumns*maskRows;
    GDALRasterBand* ptrMaskRasterBand=NULL;
    if(!ptrMaskRasterFile->getRasterBand(0,ptrMaskRasterBand,strAuxError))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError getting band for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    GByte*  pAuxData=NULL;
    pAuxData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
    if(CE_None!=ptrMaskRasterBand->RasterIO( GF_Read, 0, 0, maskColumns, maskRows,
                                             pAuxData, maskColumns, maskRows, maskRasterDataType, 0, 0 ))
    {
        strError=QObject::tr("Algorithms::readMaskRasterFile");
        strError+=QObject::tr("\nError reading band for raster file:\n%1\nError:\n%2")
                .arg(maskBandRasterFile).arg(strAuxError);
        return(false);
    }
    int numberOfMaskValues=0;
    for(int row=0;row<maskRows;row++)
    {
        for(int column=0;column<maskColumns;column++)
        {
            int posInData=row*maskColumns+column;
            int value=pAuxData[posInData];
            if(value==maskValue)
            {
                if(!values.contains(row))
                {
                    QVector<int> aux;
                    values[row]=aux;
                }
                values[row].push_back(column);
                numberOfMaskValues++;
            }
        }
    }
    maskValuesPercentage=((float)numberOfMaskValues/(float)(maskColumns*maskRows))*100.0;
    CPLFree(pAuxData);
    delete(ptrMaskRasterFile);
    return(true);
}

bool Algorithms::reflectanceComputation(QString inputFileName,
                                        QString outputFileName,
                                        double sunElevation,
                                        double addValue,
                                        double multValue,
                                        QString &strError)
{
    QString strAuxError;
    QString strNoDataValue;
    mPtrParametersManager->getParameter(ALGORITHMS_REFL_PARAMETER_NO_DATA_VALUE)->getValue(strNoDataValue);
    double noDataValue=strNoDataValue.toDouble();
    QMap<QString,QString> refImageOptions;
    mPtrParametersManager->getParameter(ALGORITHMS_REFL_PARAMETER_IMAGE_OPTIONS)->getValue(refImageOptions);
    QString strBuildOverviews;
    mPtrParametersManager->getParameter(ALGORITHMS_REFL_PARAMETER_BUILD_OVERVIEWS)->getValue(strBuildOverviews);
    bool buildOverviews=false;
    if(strBuildOverviews.compare("yes",Qt::CaseInsensitive)==0)
        buildOverviews=true;
    IGDAL::Raster* ptrDlRasterFile=new IGDAL::Raster(mPtrCrsTools);
    if(!ptrDlRasterFile->setFromFile(inputFileName,strAuxError))
    {
        strError=QObject::tr("Algorithms::reflectanceComputation");
        strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        return(false);
    }
    IGDAL::Raster* ptrReflectanceRasterFile=new IGDAL::Raster(mPtrCrsTools);
    IGDAL::ImageTypes imageType=IGDAL::GEOTIFF;
    GDALDataType gdalDataType=GDT_Float32;
    int numberOfBands=1;
    int columns,rows;
    if(!ptrDlRasterFile->getSize(columns,rows,strAuxError))
    {
        strError=QObject::tr("Algorithms::reflectanceComputation");
        strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        return(false);
    }
    bool internalGeoRef=true;
    bool externalGeoRef=false;
    QString crsDescription=ptrDlRasterFile->getCrsDescription();
    double nwFc,nwSc,seFc,seSc;
    if(!ptrDlRasterFile->getBoundingBox(nwFc,nwSc,seFc,seSc,
                                        strAuxError))
    {
        strError=QObject::tr("Algorithms::reflectanceComputation");
        strError+=QObject::tr("\nError getting bounding box from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        return(false);
    }
    int numberOfBand=0;
    double dlNoDataValue;
    if(!ptrDlRasterFile->getNoDataValue(numberOfBand,dlNoDataValue,strAuxError))
    {
        strError=QObject::tr("Algorithms::reflectanceComputation");
        strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        return(false);
    }
    int initialColumn=0;
    int initialRow=0;
    int columnsToRead=columns;
    int rowsToRead=rows;
    int numberOfPixels=columnsToRead*rowsToRead;
    float* pData=(float*)malloc(numberOfPixels*sizeof(GDT_Float32));
//    =(GDT_Float32 *) CPLMalloc(numberOfPixels*sizeof(GDT_Float32));
    int* pIntData;
    if(!ptrDlRasterFile->readValues(numberOfBand, // desde 0
                                    initialColumn,
                                    initialRow,
                                    columnsToRead,
                                    rowsToRead,
                                    pIntData,
                                    strError))
    {
        strError=QObject::tr("Algorithms::reflectanceComputation");
        strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        return(false);
    }
    delete(ptrDlRasterFile);
    double pi=4.*atan(1.);
    // https://landsat.usgs.gov/using-usgs-landsat-8-product
    for(int i=0;i<numberOfPixels;i++)
    {
        if(fabs((double)pIntData[i]-dlNoDataValue)<0.01)
        {
            pData[i]=noDataValue;
        }
        else
        {
            pData[i]=1.0/sin(sunElevation*pi/180.)*(((float)pIntData[i])*multValue+addValue);
        }
    }
    free(pIntData);
//    for(int row=0;row<rows;row++)
//    {
//        for(int column=0;column<columns;column++)
//        {
//            int posInData=row*columns+column;
//            pData[posInData]=1.0/sin(sunElevation*pi/180.)*(pData[posInData]*multValue+addValue);
//        }
//    }
    QVector<double> georef;
    bool closeAfterCreate=false;
    if(!ptrReflectanceRasterFile->createRaster(outputFileName, // Se le añade la extension
                                               imageType,gdalDataType,
                                               numberOfBands,
                                               columns,rows,
                                               internalGeoRef,externalGeoRef,crsDescription,
                                               nwFc,nwSc,seFc,seSc,
                                               georef, // vacío si se georeferencia con las esquinas
                                               noDataValue,
                                               closeAfterCreate,
                                               refImageOptions,
//                                               buildOverviews,
                                               strAuxError))
    {
        strError=QObject::tr("Algorithms::reflectanceComputation");
        strError+=QObject::tr("\nError creating raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strAuxError);
        return(false);
    }
    if(!ptrReflectanceRasterFile->writeValues(numberOfBand, // desde 0
                                              initialColumn,
                                              initialRow,
                                              columnsToRead,
                                              rowsToRead,
                                              pData,
                                              strError))
    {
        strError=QObject::tr("Algorithms::reflectanceComputation");
        strError+=QObject::tr("\nError writing raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strAuxError);
        return(false);
    }
    free(pData);
    if(buildOverviews)
    {
        if(!ptrReflectanceRasterFile->buildOverviews(strAuxError))
        {
            return(false);
        }
    }
    delete(ptrReflectanceRasterFile);
    return(true);
}

bool Algorithms::setAlgorithms(QString &strError)
{
    mAlgorithmsCodes.clear();
    mAlgorithmsGuiTagsByCode.clear();
    mAlgorithmsCodes.push_back(ALGORITHMS_REFL_CODE);
    mAlgorithmsCodes.push_back(ALGORITHMS_NDVI_CODE);
    mAlgorithmsCodes.push_back(ALGORITHMS_PIAS_CODE);
    mAlgorithmsCodes.push_back(ALGORITHMS_INTC_CODE);
    mAlgorithmsCodes.push_back(ALGORITHMS_CLOUDREMOVAL_CODE);
    mAlgorithmsGuiTagsByCode[ALGORITHMS_REFL_CODE]=ALGORITHMS_REFL_GUI_TAG;
    mAlgorithmsGuiTagsByCode[ALGORITHMS_NDVI_CODE]=ALGORITHMS_NDVI_GUI_TAG;
    mAlgorithmsGuiTagsByCode[ALGORITHMS_PIAS_CODE]=ALGORITHMS_PIAS_GUI_TAG;
    mAlgorithmsGuiTagsByCode[ALGORITHMS_INTC_CODE]=ALGORITHMS_INTC_GUI_TAG;
    mAlgorithmsGuiTagsByCode[ALGORITHMS_CLOUDREMOVAL_CODE]=ALGORITHMS_CLOUDREMOVAL_GUI_TAG;
    return(true);
}

bool Algorithms::setParametersForAlgorithm(QString command,
                                           QString &strError,
                                           QWidget *ptrWidget)
{
    if(mPtrParametersManager==NULL)
    {
        strError=QObject::tr("Algorithms::setParametersForCommand, parameters is NULL");
        return(false);
    }
    return(mPtrParametersManager->setParametersForCommand(command,ptrWidget));
}

bool Algorithms::setParametersManager(QString fileName,
                                      QString &strError)
{
    if(!QFile::exists(fileName))
    {
        strError=QObject::tr("Algorithms::setParametersManager,\nNot exists parameters file:\n%1").arg(fileName);
        return(false);
    }
    if(mPtrParametersManager!=NULL)
    {
        delete(mPtrParametersManager);
        mPtrParametersManager=NULL;
    }
    mPtrParametersManager=new ParametersManager();
    if(!mPtrParametersManager->loadFromXml(fileName,strError))
    {
        delete(mPtrParametersManager);
        mPtrParametersManager=NULL;
        return(false);
    }
    return(true);
}

bool Algorithms::writeCloudRemovedFile(QString inputFileName,
                                       QString outputFileName,
                                       QVector<QVector<double> > &bandData,
                                       QString &strError)
{
    //  bandData debe venir preparado para cambiar de tipo de dato y escribir, pero en la unidad correcta
    if(!QFile::exists(inputFileName))
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nNot exists input image file: \n %1").arg(inputFileName);
        return(false);
    }
    IGDAL::ImageTypes imageType=IGDAL::TIFF;
    QFileInfo inputFileInfo(inputFileName);
    QString inputFileExtension=inputFileInfo.suffix();
    bool validExtension=false;
    if(inputFileExtension.compare(RASTER_TIFF_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_ECW_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::ECW;
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_JPEG_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::JPEG;
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_BMP_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::BMP;
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_PNG_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::PNG;
        validExtension=true;
    }
    else if(inputFileExtension.compare(RASTER_ERDAS_IMAGINE_FILE_EXTENSION,Qt::CaseInsensitive)==0)
    {
        imageType=IGDAL::HFA;
        validExtension=true;
    }
    if(!validExtension)
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nNot valid type for input image file: \n %1").arg(inputFileName);
        return(false);
    }
    if(QFile::exists(outputFileName))
    {
        if(!QFile::remove(outputFileName))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError removing existing intercalibrated image file: \n %1").arg(outputFileName);
            return(false);
        }
    }
    IGDAL::Raster* ptrInputRasterFile=new IGDAL::Raster(mPtrCrsTools);
    bool inputUpdate=false;
    QString strAuxError;
    if(!ptrInputRasterFile->setFromFile(inputFileName,strAuxError,inputUpdate))
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
    GDALDataType inputGdalDataType;//GDT_Float32;
    if(!ptrInputRasterFile->getDataType(inputGdalDataType,strAuxError))
    {
        strError=QObject::tr("Algorithms::applyIntercalibration");
        strError+=QObject::tr("\nError recovering data type from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
//    if(gdalDataType!=GDT_Float32
//            &&gdalDataType!=GDT_Byte
//            &&gdalDataType!=GDT_UInt16
//            &&gdalDataType!=GDT_Int16
//            &&gdalDataType!=GDT_UInt32
//            &&gdalDataType!=GDT_Int32)
    if(inputGdalDataType!=GDT_Float32
            &&inputGdalDataType!=GDT_Byte
            &&inputGdalDataType!=GDT_UInt16
            &&inputGdalDataType!=GDT_Int16)
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nInvalid type of data: not float32, byte, uint16, int16, uint32 or int32\nin file:\n%1")
                .arg(inputFileName);
        delete(ptrInputRasterFile);
        return(false);
    }
    GDALDataType outputGdalDataType=inputGdalDataType;
//    if(outputTo8bits
//            &&outputGdalDataType!=GDT_Byte)
//    {
//        outputGdalDataType=GDT_Byte;
//    }
//    double inputFactorTo8Bits=1.0;
//    if(outputTo8bits)
//    {
//        if(!ptrInputRasterFile->getFactorTo8Bits(inputFactorTo8Bits,strAuxError))
//        {
//            strError=QObject::tr("Algorithms::applyIntercalibration");
//            strError+=QObject::tr("\nError recovering factor to 8 bits from raster file:\n%1\nError:\n%2")
//                    .arg(inputFileName).arg(strAuxError);
//            return(false);
//        }
//    }
    int numberOfBands=1;
    if(!ptrInputRasterFile->getNumberOfBands(numberOfBands,strAuxError))
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nError recovering number of bands from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        return(false);
    }
    if(numberOfBands!=1)
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nThere is not a unique band in raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
    int columns,rows;
    if(!ptrInputRasterFile->getSize(columns,rows,strAuxError))
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nError getting dimension from raster file:\n%1\nError:\n%2")
                .arg(inputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
//    bool internalGeoRef=true;
//    bool externalGeoRef=false;
//    QString crsDescription=ptrDlRasterFile->getCrsDescription();
//    double nwFc,nwSc,seFc,seSc;
//    if(!ptrDlRasterFile->getBoundingBox(nwFc,nwSc,seFc,seSc,
//                                        strAuxError))
//    {
//        strError=QObject::tr("Algorithms::applyIntercalibration");
//        strError+=QObject::tr("\nError getting bounding box from raster file:\n%1\nError:\n%2")
//                .arg(outputFileName).arg(strAuxError);
//        return(false);
//    }
    int numberOfBand=0;
    double inputDlNoDataValue;
    if(!ptrInputRasterFile->getNoDataValue(numberOfBand,inputDlNoDataValue,strAuxError))
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nError reading no data value in raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        return(false);
    }
    delete(ptrInputRasterFile);
//    if(outputTo8bits)
//    {
//        if(inputDlNoDataValue<0||inputDlNoDataValue>255)
//        {
//            strError=QObject::tr("Algorithms::writeCloudRemovedFile");
//            strError+=QObject::tr("\nInvalid no data value with to8bits option: %1\nfor input file:\n%2")
//                    .arg(QString::number(inputDlNoDataValue,'f',2)).arg(inputFileName);
//            delete(ptrInputRasterFile);
//            return(false);
//        }
//    }

    IGDAL::Raster* ptrOutputRasterFile=new IGDAL::Raster(mPtrCrsTools);
//    if(outputTo8bits
//            &&inputGdalDataType!=GDT_Byte)
//    {
//        QVector<double> georef;
//        if(!ptrInputRasterFile->getGeoRef(georef,strAuxError))
//        {
//            strError=QObject::tr("Algorithms::reflectanceComputation");
//            strError+=QObject::tr("\nError recovering GeoRef from input raster file:\n%1\nError:\n%2")
//                    .arg(inputFileName).arg(strAuxError);
//            delete(ptrInputRasterFile);
//            delete(ptrOutputRasterFile);
//            return(false);
//        }
//        bool closeAfterCreate=false;
//        double nwFc,nwSc,seFc,seSc;
//        if(!ptrInputRasterFile->getBoundingBox(nwFc,nwSc,seFc,seSc,strAuxError))
//        {
//            strError=QObject::tr("Algorithms::reflectanceComputation");
//            strError+=QObject::tr("\nError recovering bounding box from input raster file:\n%1\nError:\n%2")
//                    .arg(inputFileName).arg(strAuxError);
//            delete(ptrInputRasterFile);
//            delete(ptrOutputRasterFile);
//            return(false);
//        }
//        QString crsDescription=ptrInputRasterFile->getCrsDescription();
//        bool internalGeoRef=true;
//        bool externalGeoRef=false;
//        QMap<QString, QString> refImageOptions; // por implementar
//        if(!ptrOutputRasterFile->createRaster(outputFileName, // Se le añade la extension
//                                              imageType,
//                                              outputGdalDataType,
//                                              numberOfBands,
//                                              columns,rows,
//                                              internalGeoRef,externalGeoRef,crsDescription,
//                                              nwFc,nwSc,seFc,seSc,
//                                              georef, // vacío si se georeferencia con las esquinas
//                                              inputDlNoDataValue,
//                                              closeAfterCreate,
//                                              refImageOptions,
//                                              // buildOverviews,
//                                              strAuxError))
//        {
//            strError=QObject::tr("Algorithms::reflectanceComputation");
//            strError+=QObject::tr("\nError creating raster file:\n%1\nError:\n%2")
//                    .arg(outputFileName).arg(strAuxError);
//            delete(ptrInputRasterFile);
//            delete(ptrOutputRasterFile);
//            return(false);
//        }
//    }
//    else
//    {
        if(!QFile::copy(inputFileName,outputFileName))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError copying file:\n%1\nto intercalibrated image file:\n%2")
                    .arg(inputFileName).arg(outputFileName);
            return(false);
        }
        bool update=true;
//        QString strAuxError;
        if(!ptrOutputRasterFile->setFromFile(outputFileName,strAuxError,update))
        {
            strError=QObject::tr("Algorithms::writeCloudRemovedFile");
            strError+=QObject::tr("\nError opening raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
//    }
    GDALRasterBand* ptrOutputRasterBand=NULL;
    if(!ptrOutputRasterFile->getRasterBand(numberOfBand,ptrOutputRasterBand,strAuxError))
    {
        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
        strError+=QObject::tr("\nError recovering raster band from raster file:\n%1\nError:\n%2")
                .arg(outputFileName).arg(strAuxError);
        delete(ptrInputRasterFile);
        delete(ptrOutputRasterFile);
        return(false);
    }

    int initialColumn=0;
    int initialRow=0;
    int columnsToRead=columns;
    int rowsToRead=rows;
    int numberOfPixels=columnsToRead*rowsToRead;

//    GDALRasterBand* ptrInputRasterBand;
//    if(!ptrInputRasterFile->getRasterBand(numberOfBand,ptrInputRasterBand,strAuxError))
//    {
//        strError=QObject::tr("Algorithms::writeCloudRemovedFile");
//        strError+=QObject::tr("\nError recovering raster band from raster file:\n%1\nError:\n%2")
//                .arg(inputFileName).arg(strAuxError);
//        delete(ptrInputRasterFile);
//        delete(ptrOutputRasterFile);
//        return(false);
//    }
    if(inputGdalDataType==GDT_Byte)
    {
        GByte*  pData=NULL;
        pData=(GByte *) CPLMalloc(numberOfPixels*sizeof(GByte));
        int i=0;
        for(int row=0;row<bandData.size();row++)
        {
            for(int column=0;column<bandData[row].size();column++)
            {
                if(fabs(bandData[row][column]-inputDlNoDataValue)<0.01)
                {
                    pData[i]=(int)inputDlNoDataValue;
                }
                else
                {
                    int intValue=qRound(bandData[row][column]);
                    if(intValue<0)
                    {
                        intValue=0;
                    }
                    else if(intValue>255)
                    {
                        intValue=255;
                    }
                    pData[i]=intValue;
                }
                i++;
            }
        }
        if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                  rowsToRead,pData,columnsToRead,rowsToRead,GDT_Byte,0,0))
        {
            strError=QObject::tr("Algorithms::writeCloudRemovedFile");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
//            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        CPLFree(pData);
    }
    else if(inputGdalDataType==GDT_UInt16)
    {
        GUInt16*  pData=NULL;
        pData=(GUInt16 *) CPLMalloc(numberOfPixels*sizeof(GUInt16));
        int i=0;
        for(int row=0;row<bandData.size();row++)
        {
            for(int column=0;column<bandData[row].size();column++)
            {
                if(fabs(bandData[row][column]-inputDlNoDataValue)<0.01)
                {
                    pData[i]=(int)inputDlNoDataValue;
                }
                else
                {
                    int intValue=qRound(bandData[row][column]);
                    if(intValue<0)
                    {
                        intValue=0;
                    }
                    else if(intValue>65535)
                    {
                        intValue=65535;
                    }
                    pData[i]=intValue;
                }
                i++;
            }
        }
        if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                  rowsToRead,pData,columnsToRead,rowsToRead,GDT_UInt16,0,0))
        {
            strError=QObject::tr("Algorithms::writeCloudRemovedFile");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
//            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        CPLFree(pData);
    }
    else if(inputGdalDataType==GDT_Int16)
    {
        GInt16*  pData=NULL;
        pData=(GInt16 *) CPLMalloc(numberOfPixels*sizeof(GInt16));
        int i=0;
        for(int row=0;row<bandData.size();row++)
        {
            for(int column=0;column<bandData[row].size();column++)
            {
                if(fabs(bandData[row][column]-inputDlNoDataValue)<0.01)
                {
                    pData[i]=(int)inputDlNoDataValue;
                }
                else
                {
                    int intValue=qRound(bandData[row][column]);
                    if(intValue<-32768)
                    {
                        intValue=-32768;
                    }
                    else if(intValue>32767)
                    {
                        intValue=32767;
                    }
                    pData[i]=intValue;
                }
                i++;
            }
        }
        if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                  rowsToRead,pData,columnsToRead,rowsToRead,GDT_Int16,0,0))
        {
            strError=QObject::tr("Algorithms::writeCloudRemovedFile");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
//            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
        CPLFree(pData);
    }
    else if(inputGdalDataType==GDT_Float32)
    {
        float* pData;
        pData=(float*)malloc(numberOfPixels*sizeof(GDT_Float32));
        int i=0;
        for(int row=0;row<bandData.size();row++)
        {
            for(int column=0;column<bandData[row].size();column++)
            {
                if(fabs(bandData[row][column]-inputDlNoDataValue)<0.01)
                {
                    pData[i]=inputDlNoDataValue;
                }
                else
                {
                    pData[i]=bandData[row][column];
                }
                i++;
            }
        }
        if(CE_None!=ptrOutputRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                                  rowsToRead,pData,columnsToRead,rowsToRead,GDT_Float32,0,0))
        {
            strError=QObject::tr("Algorithms::writeCloudRemovedFile");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
//            delete(ptrInputRasterFile);
            delete(ptrOutputRasterFile);
            return(false);
        }
    }
    /*
    else if(gdalDataType==GDT_UInt32)
    {
        GUInt32*  pData=NULL;
        pData=(GUInt32 *) CPLMalloc(numberOfPixels*sizeof(GUInt32));
        if(CE_None!=ptrRasterBand->RasterIO( GF_Read, initialColumn, initialRow, columnsToRead, rowsToRead,
                                                 pData, columnsToRead, rowsToRead, GDT_UInt32, 0, 0 ))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrRasterFile);
            return(false);
        }
        for(int i=0;i<numberOfPixels;i++)
        {
            int initialValue=(int)pData[i];
            if(fabs((double)initialValue-dlNoDataValue)<0.01)
            {
                pData[i]=(int)dlNoDataValue;
            }
            else
            {
                int intValue=qRound(((double)initialValue)*gain+offset);
                if(intValue<0)
                {
                    intValue=0;
                }
                else if(intValue>4294967295)
                {
                    intValue=4294967295;
                }
                pData[i]=intValue;
            }
        }
        if(CE_None!=ptrRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                            rowsToRead,pData,columnsToRead,rowsToRead,GDT_UInt32,0,0))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrRasterFile);
            return(false);
        }
        CPLFree(pData);
    }
    else if(gdalDataType==GDT_Int32)
    {
        GInt32*  pData=NULL;
        pData=(GInt32 *) CPLMalloc(numberOfPixels*sizeof(GInt32));
        if(CE_None!=ptrRasterBand->RasterIO( GF_Read, initialColumn, initialRow, columnsToRead, rowsToRead,
                                                 pData, columnsToRead, rowsToRead, GDT_Int32, 0, 0 ))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError reading raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrRasterFile);
            return(false);
        }
        for(int i=0;i<numberOfPixels;i++)
        {
            int initialValue=(int)pData[i];
            if(fabs((double)initialValue-dlNoDataValue)<0.01)
            {
                pData[i]=(int)dlNoDataValue;
            }
            else
            {
                int intValue=qRound(((double)initialValue)*gain+offset);
                if(intValue<-2147483648)
                {
                    intValue=-2147483648;
                }
                else if(intValue>2147483647)
                {
                    intValue=2147483647;
                }
                pData[i]=intValue;
            }
        }
        if(CE_None!=ptrRasterBand->RasterIO(GF_Write,initialColumn,initialRow,columnsToRead,
                                            rowsToRead,pData,columnsToRead,rowsToRead,GDT_Int32,0,0))
        {
            strError=QObject::tr("Algorithms::applyIntercalibration");
            strError+=QObject::tr("\nError writting raster file:\n%1\nError:\n%2")
                    .arg(outputFileName).arg(strAuxError);
            delete(ptrRasterFile);
            return(false);
        }
        CPLFree(pData);
    }
    */
    delete(ptrOutputRasterFile);
//    delete(ptrInputRasterFile);
    return(true);
}
