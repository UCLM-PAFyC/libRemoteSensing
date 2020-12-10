#include <QtWidgets>
#include <QDomDocument>
#include <QString>

#include "ParameterDefinitions.h"
#include "Parameter.h"
#include "ParametersManager.h"
#include "ParametersManagerDialog.h"

ParametersManager::ParametersManager()
{
    _fileName="";
    _path="";
    _parameterTypes.push_back(PARAMETER_TYPE_INTEGER);
    _parameterTypes.push_back(PARAMETER_TYPE_DOUBLE);
    _parameterTypes.push_back(PARAMETER_TYPE_STRING);
    _parameterTypes.push_back(PARAMETER_TYPE_ENUM);
    _parameterTypes.push_back(PARAMETER_TYPE_VECTOR_INTEGER);
    _parameterTypes.push_back(PARAMETER_TYPE_VECTOR_TAG_VALUES);
}

bool ParametersManager::deleteParameter(QString code)
{
    _parameters.remove(code);
    return(true);
}

bool ParametersManager::setParameter(Parameter* ptrParameter)
{
    QString parameterCode=ptrParameter->getCode();
    bool success=false;
    if(_parameters.contains(parameterCode))
    {
        Parameter* ptrParContent=_parameters.find(parameterCode).value();
        ptrParameter->setCode(parameterCode);
        ptrParameter->setTag(ptrParContent->getTag());
        ptrParameter->setExplanation(ptrParContent->getExplanation());
        ptrParameter->setType(ptrParContent->getType());
        double doubleValue;
        ptrParContent->getMinValue(doubleValue);
        ptrParameter->setMinValue(doubleValue);
        ptrParContent->getMaxValue(doubleValue);
        ptrParameter->setMaxValue(doubleValue);
        ptrParContent->getValue(doubleValue);
        ptrParameter->setValue(doubleValue);
        ptrParContent->getValue(doubleValue);
        ptrParameter->setValue(doubleValue);
        int intValue;
        ptrParContent->getMinValue(intValue);
        ptrParameter->setMinValue(intValue);
        ptrParContent->getMaxValue(intValue);
        ptrParameter->setMaxValue(intValue);
        ptrParContent->getValue(intValue);
        ptrParameter->setValue(intValue);
        ptrParContent->getValue(intValue);
        ptrParameter->setValue(intValue);
        QString strValue;
        ptrParContent->getValue(strValue);
        ptrParameter->setValue(strValue);
        QString strDefaultValue;
        ptrParContent->getValue(strDefaultValue);
        ptrParameter->setValue(strDefaultValue);
        ptrParameter->setEmc(ptrParContent->getEmc());
        //ptrParameter->setFactorConversion(ptrParContent->getFactorConversion());
        ptrParameter->setEnabled(ptrParContent->isEnabled());
        ptrParameter->setPrintWidth(ptrParContent->getPrintWidth());
        ptrParameter->setPrintPrecision(ptrParContent->getPrintPrecision());
        //ptrParameter->setPrintUnit(ptrParContent->getPrintUnit());
        ptrParameter->setUnit(ptrParContent->getUnit());
        success=true;
    }
    return(success);
}

Parameter* ParametersManager::getParameter(int pos)
{
    Parameter* ptrParameter=NULL;
    if(pos<_parameters.size())
    {
        QMap<QString, Parameter*>::iterator i = _parameters.begin();
        int cont=0;
        while(cont<pos)
        {
            cont++;
            i++;
        }
        ptrParameter=i.value();
    } 
    return(ptrParameter);
}

Parameter* ParametersManager::getParameter(QString name)
{
    Parameter* ptrParameter=NULL;
    if(_parameters.contains(name))
        ptrParameter=_parameters.find(name).value();
    return(ptrParameter);
}

QString ParametersManager::getFileName(void)
{
    return _fileName;
}

int ParametersManager::getNumberOfParameters(void)
{
    return _parameters.size();
}

Parameter* ParametersManager::insertParameter(Parameter* ptrParameter)
{
    QString code=ptrParameter->getCode();
    return _parameters[code]=ptrParameter;
}

bool ParametersManager::isOk(void)
{
    QString noStr;
    if(_fileName.compare(noStr)==0)
    {
        return false;
    }
    return true;
}

bool ParametersManager::loadFromXml(QString fileName,
                                    QString& strError)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return false;
    _fileName=fileName;
    QString errorStr;
    QString noStr;
    int errorLine;
    int errorColumn;

    QDomDocument doc;
    if (!doc.setContent(&file,true,&errorStr,&errorLine,&errorColumn))
    {
        strError="Error al construir el proyecto";
        strError+="\ndesde el fichero XML:\n";
        strError+=fileName;
        strError+="\nAl parsear el fichero se ha producido";
        strError+=("\nel error: "+errorStr);
        strError+=("\nen la linea "+QString::number(errorLine));
        strError+=("\nen la columna "+QString::number(errorColumn));
        return false;
    }
    QDomElement root = doc.documentElement();

    if (root.tagName() != "GestorDeParametros")
    {
        strError="Error al construir el proyecto";
        strError+="\ndesde el fichero XML:\n";
        strError+=fileName;
        strError+="\nEl tipo de documento no es del tipo: EspecificacionesTecnicasVuelo";
        return false;
    }
    else if (root.hasAttribute("version") && root.attribute("version") != "1.1")
    {
        strError="Error al construir el proyecto";
        strError+="\ndesde el fichero XML:\n";
        strError+=fileName;
        strError+="\nEl documento no es de la version: 1.1";
        return false;
    }
    
    QDomNode node = root.firstChild();
    int numberOfParameter=0;
    while(!node.isNull())
    {
        QDomElement element = node.toElement(); // try to convert the node to an element.
        if(!element.isNull())
        {
            numberOfParameter++;
            QString tag=element.tagName();
            if(tag.compare("Parametro")==0)
            {
                bool parameterIsOk=true;
                QString command,code,tag,explanation,strMinValue,strMaxValue,strEnabled;
                QString type,enumValues,strValue;
                QString unit,strPrintWidth;
                bool enabled=false;
                //QString strFactorConversion,unit,strPrintWidth;
                //QString strPrintPrecision,printUnit;
                QString strPrintPrecision;
                QDomNode parameterDataNode=element.firstChild();
                while(!parameterDataNode.isNull())
                {
                    QDomElement parameterDataElement=parameterDataNode.toElement();
                    if(!parameterDataElement.isNull())
                    {
                        QString tagParameterDataElement=parameterDataElement.tagName().trimmed();
                        if(tagParameterDataElement.compare("Comando")==0)
                        {
                            command=parameterDataElement.text();
                            if(command.isEmpty())
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Comando\" esta vacio";
                                return(false);
                            }
                        }
                        if(tagParameterDataElement.compare("Habilitado")==0)
                        {
                            strEnabled=parameterDataElement.text();
                            if(strEnabled.isEmpty())
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Habilitado\" esta vacio";
                                return(false);
                            }
                            if(strEnabled.compare("Si",Qt::CaseInsensitive)!=0
                                    &&strEnabled.compare("No",Qt::CaseInsensitive)!=0)
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Habilitado\"";
                                strError+="\ndebe ser Si o No";
                                return(false);
                            }
                            if(strEnabled.compare("Si",Qt::CaseInsensitive)==0)
                                enabled=true;
                        }
                        if(tagParameterDataElement.compare("Codigo")==0)
                        {
                            code=parameterDataElement.text();
                            if(!code.isEmpty())
                            {
                                if(_parameters.contains(code))
                                    parameterIsOk=false;
                            }
                            else
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Codigo\" esta vacio";
                                return(false);
                            }
                        }
                        if(tagParameterDataElement.compare("Tipo")==0)
                        {
                            type=parameterDataElement.text();
                            if(!_parameterTypes.contains(type))
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Tipo\" no es valido";
                                return(false);
                            }
                        }
                        if(tagParameterDataElement.compare("Etiqueta")==0)
                        {
                            tag=parameterDataElement.text();
//                            if(tag.isEmpty())
//                            {
//                                strError="En el fichero de parametros:\n";
//                                strError+=fileName;
//                                strError+="\nen el parametro numero: ";
//                                strError+=QString::number(numberOfParameter);
//                                strError+="\nel elemento \"Etiqueta\" esta vacio";
//                                return(false);
//                            }
                        }
                        if(tagParameterDataElement.compare("Descripcion")==0)
                        {
                            explanation=parameterDataElement.text();
                            if(explanation.isEmpty())
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Descripcion\" esta vacio";
                                return(false);
                            }
                        }
                        if(tagParameterDataElement.compare("ListaDeValores")==0)
                            enumValues=parameterDataElement.text();
                        if(tagParameterDataElement.compare("Valor")==0)
                        {
                            strValue=parameterDataElement.text();
                            if(strValue.isEmpty())
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Valor\" esta vacio";
                                return(false);
                            }
                        }
                        if(tagParameterDataElement.compare("ValorMinimo")==0)
                            strMinValue=parameterDataElement.text();
                        if(tagParameterDataElement.compare("ValorMaximo")==0)
                            strMaxValue=parameterDataElement.text();
                        //if(tagParameterDataElement.compare("FactorDeConversion")==0)
                        //    strFactorConversion=parameterDataElement.text();
                        if(tagParameterDataElement.compare("Unidad")==0)
                            unit=parameterDataElement.text();
                        //if(tagParameterDataElement.compare("UnidadDeImpresion")==0)
                        //    printUnit=parameterDataElement.text();
                        if(tagParameterDataElement.compare("DimensionDeImpresion")==0)
                        {
                            strPrintWidth=parameterDataElement.text();
                            if(strPrintWidth.isEmpty())
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"DimensionDeImpresion\" esta vacio";
                                return(false);
                            }
                        }
                        if(tagParameterDataElement.compare("PrecisionDeImpresion")==0)
                            strPrintPrecision=parameterDataElement.text();
                    }
                    parameterDataNode=parameterDataNode.nextSibling();
                }
                double doubleValue,doubleMinValue,doubleMaxValue;
//                double doubleDefaultValue,doubleMinValue,doubleMaxValue,factorConversion;
                int intValue,intMinValue,intMaxValue;
                QVector<int> intMaxValues,intMinValues,intValues;
                int printWidth,printPrecision;
                bool okConversion;
                printWidth=strPrintWidth.toInt(&okConversion);
                if(!okConversion)
                {
                    strError="En el fichero de parametros:\n";
                    strError+=fileName;
                    strError+="\nen el parametro numero: ";
                    strError+=QString::number(numberOfParameter);
                    strError+="\nel elemento \"DimensionDeImpresion\" no es un valor entero";
                    return(false);
                }
                if(type.compare(PARAMETER_TYPE_VECTOR_TAG_VALUES,Qt::CaseInsensitive)==0)
                {
                    if(strValue.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"Valor\" esta vacio";
                        return(false);
                    }
                    QStringList values=strValue.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                    if(values.size()==0)
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"Valor\" no contiene valores";
                        return(false);
                    }
                    else
                    {
                        for(int k=0;k<values.size();k++)
                        {
                            QStringList tagAndValue=values.at(k).split(TAG_VALUE_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                            if(tagAndValue.size()!=2)
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Valor\" no contiene valor para etiqueta y valor en el item ";
                                strError+=QString::number(k+1);
                                return(false);
                            }
                        }
                    }
                }
                if(type.compare(PARAMETER_TYPE_ENUM,Qt::CaseInsensitive)==0)
                {
                    if(enumValues.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ListaDeValores\" esta vacio";
                        return(false);
                    }
                    QStringList values=enumValues.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                    if(values.size()==0)
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ListaDeValores\" no contiene valores";
                        return(false);
                    }
                    else
                    {
                        if(!strValue.contains(ENUM_CHARACTER_SEPARATOR))
                        {
                            if(values.indexOf(strValue)==-1)
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Valor\" no esta en la lista";
                                return(false);
                            }
                        }
                        else
                        {
                            QStringList strValues=strValue.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                            for(int nv=0;nv<strValues.size();nv++)
                            {
                                if(values.indexOf(strValues.at(nv))==-1)
                                {
                                    strError="En el fichero de parametros:\n";
                                    strError+=fileName;
                                    strError+="\nen el parametro numero: ";
                                    strError+=QString::number(numberOfParameter);
                                    strError+="\nun elemento \"Valor\" no esta en la lista";
                                    return(false);
                                }
                            }
                        }
                    }
                }
                if(type.compare(PARAMETER_TYPE_DOUBLE,Qt::CaseInsensitive)==0)
                {
                    doubleValue=strValue.toDouble(&okConversion);
                    if(!okConversion)
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"Valor\" no es un double";
                        return(false);
                    }
                    if(strMaxValue.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ValorMaximo\" esta vacio";
                        return(false);
                    }
                    else
                    {
                        doubleMaxValue=strMaxValue.toDouble(&okConversion);
                        if(!okConversion)
                        {
                            strError="En el fichero de parametros:\n";
                            strError+=fileName;
                            strError+="\nen el parametro numero: ";
                            strError+=QString::number(numberOfParameter);
                            strError+="\nel elemento \"ValorMaximo\" no es un double";
                            return(false);
                        }
                    }
                    if(strMinValue.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ValorMinimo\" esta vacio";
                        return(false);
                    }
                    else
                    {
                        doubleMinValue=strMinValue.toDouble(&okConversion);
                        if(!okConversion)
                        {
                            strError="En el fichero de parametros:\n";
                            strError+=fileName;
                            strError+="\nen el parametro numero: ";
                            strError+=QString::number(numberOfParameter);
                            strError+="\nel elemento \"ValorMinimo\" no es un double";
                            return(false);
                        }
                    }
                    if(doubleValue<doubleMinValue||doubleValue>doubleMaxValue)
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"Valor\" esta fuera de dominio";
                        return(false);
                    }
                    if(strPrintPrecision.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"PrecisionDeImpresion\" esta vacio";
                        return(false);
                    }
                    else
                    {
                        printPrecision=strPrintPrecision.toInt(&okConversion);
                        if(!okConversion)
                        {
                            strError="En el fichero de parametros:\n";
                            strError+=fileName;
                            strError+="\nen el parametro numero: ";
                            strError+=QString::number(numberOfParameter);
                            strError+="\nel elemento \"PrecisionDeImpresion\" no es un entero";
                            return(false);
                        }
                    }
                }
                if(type.compare(PARAMETER_TYPE_INTEGER,Qt::CaseInsensitive)==0)
                {
                    intValue=strValue.toInt(&okConversion);
                    if(!okConversion)
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"Valor\" no es un entero";
                        return(false);
                    }
                    if(strMaxValue.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ValorMaximo\" esta vacio";
                        return(false);
                    }
                    else
                    {
                        intMaxValue=strMaxValue.toInt(&okConversion);
                        if(!okConversion)
                        {
                            strError="En el fichero de parametros:\n";
                            strError+=fileName;
                            strError+="\nen el parametro numero: ";
                            strError+=QString::number(numberOfParameter);
                            strError+="\nel elemento \"ValorMaximo\" no es un entero";
                            return(false);
                        }
                    }
                    if(strMinValue.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ValorMinimo\" esta vacio";
                        return(false);
                    }
                    else
                    {
                        intMinValue=strMinValue.toDouble(&okConversion);
                        if(!okConversion)
                        {
                            strError="En el fichero de parametros:\n";
                            strError+=fileName;
                            strError+="\nen el parametro numero: ";
                            strError+=QString::number(numberOfParameter);
                            strError+="\nel elemento \"ValorMinimo\" no es un entero";
                            return(false);
                        }
                    }
                    if(intValue<intMinValue||intValue>intMaxValue)
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"Valor\" esta fuera de dominio";
                        return(false);
                    }
                }
                if(type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
                {
                    if(strMaxValue.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ValorMaximo\" esta vacio";
                        return(false);
                    }
                    else
                    {
                        QStringList auxStrValues=strMaxValue.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                        if(auxStrValues.size()==0)
                        {
                            strError="En el fichero de parametros:\n";
                            strError+=fileName;
                            strError+="\nen el parametro numero: ";
                            strError+=QString::number(numberOfParameter);
                            strError+="\nel elemento \"ValorMaximo\" no contiene valores enteros";
                            return(false);
                        }
                        else
                        {
                            for(int nv=0;nv<auxStrValues.size();nv++)
                            {
                                QString auxStrValue=auxStrValues.at(nv);
                                int intAuxValue=auxStrValue.toDouble(&okConversion);
                                if(!okConversion)
                                {
                                    strError="En el fichero de parametros:\n";
                                    strError+=fileName;
                                    strError+="\nen el parametro numero: ";
                                    strError+=QString::number(numberOfParameter);
                                    strError+="\nel elemento \"ValorMaximo\"";
                                    strError+="\nen la posicion: ";
                                    strError+=QString::number(nv+1);
                                    strError+="\n no es un entero";
                                    return(false);
                                }
                                if(intAuxValue<intMinValue||intValue>intMaxValue)
                                {
                                    strError="En el fichero de parametros:\n";
                                    strError+=fileName;
                                    strError+="\nen el parametro numero: ";
                                    strError+=QString::number(numberOfParameter);
                                    strError+="\nel elemento \"ValorMaximo\"";
                                    strError+="\nen la posicion: ";
                                    strError+=QString::number(nv+1);
                                    strError+="\n esta fuera de dominio";
                                    return(false);
                                }
                                intMaxValues.push_back(intAuxValue);
                            }
                        }
                    }
                    if(strMinValue.isEmpty())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ValorMinimo\" esta vacio";
                        return(false);
                    }
                    else
                    {
                        QStringList auxStrValues=strMinValue.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                        if(auxStrValues.size()==0)
                        {
                            strError="En el fichero de parametros:\n";
                            strError+=fileName;
                            strError+="\nen el parametro numero: ";
                            strError+=QString::number(numberOfParameter);
                            strError+="\nel elemento \"ValorMinimo\" no contiene valores enteros";
                            return(false);
                        }
                        else
                        {
                            for(int nv=0;nv<auxStrValues.size();nv++)
                            {
                                QString auxStrValue=auxStrValues.at(nv);
                                int intAuxValue=auxStrValue.toDouble(&okConversion);
                                if(!okConversion)
                                {
                                    strError="En el fichero de parametros:\n";
                                    strError+=fileName;
                                    strError+="\nen el parametro numero: ";
                                    strError+=QString::number(numberOfParameter);
                                    strError+="\nel elemento \"ValorMinimo\"";
                                    strError+="\nen la posicion: ";
                                    strError+=QString::number(nv+1);
                                    strError+="\n no es un entero";
                                    return(false);
                                }
                                if(intAuxValue<intMinValue||intValue>intMaxValue)
                                {
                                    strError="En el fichero de parametros:\n";
                                    strError+=fileName;
                                    strError+="\nen el parametro numero: ";
                                    strError+=QString::number(numberOfParameter);
                                    strError+="\nel elemento \"ValorMinimo\"";
                                    strError+="\nen la posicion: ";
                                    strError+=QString::number(nv+1);
                                    strError+="\n esta fuera de dominio";
                                    return(false);
                                }
                                intMinValues.push_back(intAuxValue);
                            }
                        }
                    }
                    if(intMaxValues.size()!=intMinValues.size())
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"ValorMaximo\"";
                        strError+="\ny el elemento \"ValorMinimo\"";
                        strError+="\n tienen distinto numero de valores";
                        return(false);
                    }
                    QStringList auxStrValues=strValue.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                    if(auxStrValues.size()==0)
                    {
                        strError="En el fichero de parametros:\n";
                        strError+=fileName;
                        strError+="\nen el parametro numero: ";
                        strError+=QString::number(numberOfParameter);
                        strError+="\nel elemento \"Valor\" no contiene valores enteros";
                        return(false);
                    }
                    else
                    {
                        if(auxStrValues.size()!=intMaxValues.size())
                        {
                            strError="En el fichero de parametros:\n";
                            strError+=fileName;
                            strError+="\nen el parametro numero: ";
                            strError+=QString::number(numberOfParameter);
                            strError+="\nel elemento \"Valor\"";
                            strError+="\ny el elemento \"ValorMaximo\"";
                            strError+="\n tienen distinto numero de valores";
                            return(false);
                        }
                        for(int nv=0;nv<auxStrValues.size();nv++)
                        {
                            QString auxStrValue=auxStrValues.at(nv);
                            int intAuxValue=auxStrValue.toDouble(&okConversion);
                            if(!okConversion)
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Valor\"";
                                strError+="\nen la posicion: ";
                                strError+=QString::number(nv+1);
                                strError+="\n no es un entero";
                                return(false);
                            }
                            if(intAuxValue<intMinValues.at(nv)
                                    ||intValue>intMaxValues.at(nv))
                            {
                                strError="En el fichero de parametros:\n";
                                strError+=fileName;
                                strError+="\nen el parametro numero: ";
                                strError+=QString::number(numberOfParameter);
                                strError+="\nel elemento \"Valor\"";
                                strError+="\nen la posicion: ";
                                strError+=QString::number(nv+1);
                                strError+="\n esta fuera de dominio";
                                return(false);
                            }
                            intValues.push_back(intAuxValue);
                        }
                    }
                }
                Parameter* ptrParameter=new Parameter();
                ptrParameter->setCode(code);
                ptrParameter->setCommand(command);
                ptrParameter->setTag(tag);
                ptrParameter->setType(type);
                ptrParameter->setExplanation(explanation);
                ptrParameter->setPrintWidth(printWidth);
                ptrParameter->setEnabled(enabled);
                if(type.compare(PARAMETER_TYPE_VECTOR_TAG_VALUES,Qt::CaseInsensitive)==0)
                {
                    QStringList values=strValue.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                    QMap<QString,QString> tagValues;
                    for(int k=0;k<values.size();k++)
                    {
                        QStringList tagAndValue=values.at(k).split(TAG_VALUE_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
                        QString tag=tagAndValue.at(0).trimmed();
                        QString value=tagAndValue.at(1).trimmed();
                        tagValues[tag]=value;
                    }
                    ptrParameter->setValue(tagValues);
                }
                if(type.compare(PARAMETER_TYPE_STRING,Qt::CaseInsensitive)==0)
                {
                    ptrParameter->setValue(strValue);
                }
                if(type.compare(PARAMETER_TYPE_ENUM,Qt::CaseInsensitive)==0)
                {
                    ptrParameter->setEnumValues(enumValues);
                    ptrParameter->setValue(strValue);
                }
                if(type.compare(PARAMETER_TYPE_DOUBLE,Qt::CaseInsensitive)==0)
                {
                    ptrParameter->setValue(doubleValue);
                    ptrParameter->setMinValue(doubleMinValue);
                    ptrParameter->setMaxValue(doubleMaxValue);
                    //ptrParameter->setFactorConversion(factorConversion);
                    ptrParameter->setPrintWidth(printWidth);
                    ptrParameter->setPrintPrecision(printPrecision);
                    //ptrParameter->setPrintUnit(printUnit);
                    ptrParameter->setUnit(unit);
                }
                if(type.compare(PARAMETER_TYPE_INTEGER,Qt::CaseInsensitive)==0)
                {
                    ptrParameter->setValue(intValue);
                    ptrParameter->setMinValue(intMinValue);
                    ptrParameter->setMaxValue(intMaxValue);
                }
                if(type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
                {
                    ptrParameter->setIntValues(strValue);
                    ptrParameter->setIntMinValues(strMinValue);
                    ptrParameter->setIntMaxValues(strMaxValue);
                }
                _parameters[code]=ptrParameter;
            }
        }
        node = node.nextSibling();
    }
    _fileName=fileName;
    return true;
}

bool ParametersManager::saveAsXml(QWidget* parent)
{
    QWidget* ptrWidget=parent;
    if(parent==NULL)
        ptrWidget=new QWidget();
    if(!isOk())
        return false;
    QFile file(_fileName);
    if (!file.open(QFile::WriteOnly |QFile::Text))
        return false;
    
    QDomDocument doc("GestorDeParametros");
    QDomElement root = doc.createElement("GestorDeParametros");
    root.setAttribute("version","1.1");
    doc.appendChild(root);
    
    /*QDomElement tag=doc.createElement("Name");
    root.appendChild(tag);
    QDomText t = doc.createTextNode(_name);
    tag.appendChild(t);*/

    int numberOfParameters=_parameters.size();

    //QDomElement tagParameters = doc.createElement("Parameters");
    //root.appendChild(tagParameters);
    QDomText t;
    int numberOfParametersNoDeleted=0;
    QMap<QString, Parameter*>::iterator i = _parameters.begin();
    while(i!=_parameters.end())
    {
        Parameter* ptrParameter=i.value();
        QDomElement tagParameter=doc.createElement("Parametro");
        root.appendChild(tagParameter);

        QDomElement tagCommand=doc.createElement("Comando");
        t=doc.createTextNode(ptrParameter->getCommand());
        tagCommand.appendChild(t);
        tagParameter.appendChild(tagCommand);

        QDomElement tagName=doc.createElement("Codigo");
        t=doc.createTextNode(ptrParameter->getCode());
        tagName.appendChild(t);
        tagParameter.appendChild(tagName);

        QDomElement tagEnabled=doc.createElement("Habilitado");
        QString strEnabled="Si";
        if(!ptrParameter->isEnabled())
            strEnabled="No";
        t=doc.createTextNode(strEnabled);
        tagEnabled.appendChild(t);
        tagParameter.appendChild(tagEnabled);

        QDomElement tagTag=doc.createElement("Etiqueta");
        t=doc.createTextNode(ptrParameter->getTag());
        tagTag.appendChild(t);
        tagParameter.appendChild(tagTag);
            
        QDomElement tagType=doc.createElement("Tipo");
        t=doc.createTextNode(ptrParameter->getType());
        tagType.appendChild(t);
        tagParameter.appendChild(tagType);
            
        QDomElement tagExplanation=doc.createElement("Descripcion");
        t=doc.createTextNode(ptrParameter->getExplanation());
        tagExplanation.appendChild(t);
        tagParameter.appendChild(tagExplanation);
            
        QDomElement tagPrintWidth=doc.createElement("DimensionDeImpresion");
        t=doc.createTextNode(QString::number(ptrParameter->getPrintWidth(),10));
        tagPrintWidth.appendChild(t);
        tagParameter.appendChild(tagPrintWidth);
            
        QDomElement tagUnit=doc.createElement("Unidad");
        t=doc.createTextNode(ptrParameter->getUnit());
        tagUnit.appendChild(t);
        tagParameter.appendChild(tagUnit);
            
        QString type=ptrParameter->getType();
        if(type.compare(PARAMETER_TYPE_STRING,Qt::CaseInsensitive)==0)
        {
            QDomElement tagDefaultValue=doc.createElement("Valor");
            QString strDefaultValue;
            ptrParameter->getValue(strDefaultValue);
            t=doc.createTextNode(strDefaultValue);
            tagDefaultValue.appendChild(t);
            tagParameter.appendChild(tagDefaultValue);
        }
        if(type.compare(PARAMETER_TYPE_VECTOR_TAG_VALUES,Qt::CaseInsensitive)==0)
        {
            QDomElement tagTagAndValues=doc.createElement("Valor");
            QMap<QString,QString> tagAndValues;
            ptrParameter->getValue(tagAndValues);
            QString strTagAndValues;
            QMap<QString,QString>::const_iterator iterTagAndValues=tagAndValues.begin();
            int cont=0;
            while(iterTagAndValues!=tagAndValues.end())
            {
                strTagAndValues+=iterTagAndValues.key();
                strTagAndValues+=TAG_VALUE_CHARACTER_SEPARATOR;
                strTagAndValues+=iterTagAndValues.value();
                cont++;
                if(cont<tagAndValues.size())
                {
                    strTagAndValues+=ENUM_CHARACTER_SEPARATOR;
                }
                iterTagAndValues++;
            }
            t=doc.createTextNode(strTagAndValues);
            tagTagAndValues.appendChild(t);
            tagParameter.appendChild(tagTagAndValues);
//            QDomElement tagDefaultValue=doc.createElement("Valor");
//            QString strDefaultValue;
//            ptrParameter->getValue(strDefaultValue);
//            t=doc.createTextNode(strDefaultValue);
//            tagDefaultValue.appendChild(t);
//            tagParameter.appendChild(tagDefaultValue);
        }
        if(type.compare(PARAMETER_TYPE_ENUM,Qt::CaseInsensitive)==0)
        {
            QDomElement tagEnumValues=doc.createElement("ListaDeValores");
            QString enumValues;
            ptrParameter->getEnumValues(enumValues);
            t=doc.createTextNode(enumValues);
            tagEnumValues.appendChild(t);
            tagParameter.appendChild(tagEnumValues);

            QDomElement tagDefaultValue=doc.createElement("Valor");
            QString strDefaultValue;
            ptrParameter->getValue(strDefaultValue);
            t=doc.createTextNode(strDefaultValue);
            tagDefaultValue.appendChild(t);
            tagParameter.appendChild(tagDefaultValue);
        }

        if(type.compare(PARAMETER_TYPE_DOUBLE,Qt::CaseInsensitive)==0)
        {
            int precision=ptrParameter->getPrintPrecision();
            QDomElement tagDefaultValue=doc.createElement("Valor");
            double doubleValue;
            ptrParameter->getValue(doubleValue);
            t=doc.createTextNode(QString::number(doubleValue,'f',precision));
            tagDefaultValue.appendChild(t);
            tagParameter.appendChild(tagDefaultValue);

            QDomElement tagMaxValue=doc.createElement("ValorMaximo");
            ptrParameter->getMaxValue(doubleValue);
            t=doc.createTextNode(QString::number(doubleValue,'f',precision));
            tagMaxValue.appendChild(t);
            tagParameter.appendChild(tagMaxValue);
                
            QDomElement tagMinValue=doc.createElement("ValorMinimo");
            ptrParameter->getMinValue(doubleValue);
            t=doc.createTextNode(QString::number(doubleValue,'f',precision));
            tagMinValue.appendChild(t);
            tagParameter.appendChild(tagMinValue);

            //QDomElement tagPrintUnit=doc.createElement("UnidadDeImpresion");
            //t=doc.createTextNode(ptrParameter->getPrintUnit());
            //tagPrintUnit.appendChild(t);
            //    tagParameter.appendChild(tagPrintUnit);
                
            //QDomElement tagFactorConversion=doc.createElement("FactorDeConversion");
            //t=doc.createTextNode(QString::number(ptrParameter->getFactorConversion(),'f',20));
            //tagFactorConversion.appendChild(t);
            //tagParameter.appendChild(tagFactorConversion);
                
            QDomElement tagPrintPrecision=doc.createElement("PrecisionDeImpresion");
            t=doc.createTextNode(QString::number(ptrParameter->getPrintPrecision(),10));
            tagPrintPrecision.appendChild(t);
            tagParameter.appendChild(tagPrintPrecision);
        }

        if(type.compare(PARAMETER_TYPE_INTEGER,Qt::CaseInsensitive)==0)
        {
            int precision=ptrParameter->getPrintPrecision();
            QDomElement tagDefaultValue=doc.createElement("Valor");
            int intValue;
            ptrParameter->getValue(intValue);
            t=doc.createTextNode(QString::number(intValue));
            tagDefaultValue.appendChild(t);
            tagParameter.appendChild(tagDefaultValue);

            QDomElement tagMaxValue=doc.createElement("ValorMaximo");
            ptrParameter->getMaxValue(intValue);
            t=doc.createTextNode(QString::number(intValue));
            tagMaxValue.appendChild(t);
            tagParameter.appendChild(tagMaxValue);
                
            QDomElement tagMinValue=doc.createElement("ValorMinimo");
            ptrParameter->getMinValue(intValue);
            t=doc.createTextNode(QString::number(intValue));
            tagMinValue.appendChild(t);
            tagParameter.appendChild(tagMinValue);

            //QDomElement tagPrintUnit=doc.createElement("UnidadDeImpresion");
            //t=doc.createTextNode(ptrParameter->getPrintUnit());
            //tagPrintUnit.appendChild(t);
            //    tagParameter.appendChild(tagPrintUnit);
            //    
            //QDomElement tagFactorConversion=doc.createElement("FactorDeConversion");
            //t=doc.createTextNode(QString::number(ptrParameter->getFactorConversion(),'f',20));
            //tagFactorConversion.appendChild(t);
            //tagParameter.appendChild(tagFactorConversion);
            //    
            //QDomElement tagPrintPrecision=doc.createElement("PrecisionDeImpresion");
            //t=doc.createTextNode(QString::number(ptrParameter->getPrintPrecision(),10));
            //tagPrintPrecision.appendChild(t);
            //tagParameter.appendChild(tagPrintPrecision);
        }

        if(type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
        {
            int precision=ptrParameter->getPrintPrecision();
            QDomElement tagDefaultValue=doc.createElement("Valor");
            QString strValues;
            ptrParameter->getIntValues(strValues);
            t=doc.createTextNode(strValues);
            tagDefaultValue.appendChild(t);
            tagParameter.appendChild(tagDefaultValue);

            QDomElement tagMaxValue=doc.createElement("ValorMaximo");
            QString strIntMaxValues;
            ptrParameter->getIntMaxValues(strIntMaxValues);
            t=doc.createTextNode(strIntMaxValues);
            tagMaxValue.appendChild(t);
            tagParameter.appendChild(tagMaxValue);

            QDomElement tagMinValue=doc.createElement("ValorMinimo");
            QString strIntMinValues;
            ptrParameter->getIntMinValues(strIntMinValues);
            t=doc.createTextNode(strIntMinValues);
            tagMinValue.appendChild(t);
            tagParameter.appendChild(tagMinValue);
        }
        i++;
    }
    //if(numberOfParametersNoDeleted==0)
    //{
    //    t=doc.createTextNode("");
    //    tagParameters.appendChild(t);
    //}
    const int IndentSize = 4;
    QTextStream out(&file);
    doc.save(out, IndentSize);
    return true;
}

//bool ParametersManager::setDefault(QString fileName)
//{
//    QString parametersFileName=fileName;
//    QString path;
//    if(fileName.isEmpty())
//    {
//        path=QDir::currentPath()+DEFAULT_PARAMETERS_PATH;
//        parametersFileName=path+"/"+DEFAULT_PARAMETERS_FILE;
//    }
//    else
//    {
//        QFileInfo fileInfo(fileName);
//        path=fileInfo.absolutePath();
//    }
//    if(!QFile::exists(parametersFileName))
//    {
//        QString title="Error creando el objeto ParametersManager por defecto";
//        QString strError="No existe el fichero:\n";
//        strError+=parametersFileName;
//        strError+="\nContacte con los autores\n";
//        QMessageBox::warning(new QWidget(),title,strError);
//        return(false);
//    }
//    if(!loadFromXml(parametersFileName))
//    {
//        QString title="Error creando el objeto ParametersManager por defecto";
//        QString strError="Se ha producico un error al construir\n";
//        strError+="el objeto desde el fichero:\n";
//        strError+=parametersFileName;
//        strError+="\nContacte con los autores\n";
//        QMessageBox::warning(new QWidget(),title,strError);
//        return(false);
//    }
//    _path=path;
//    _fileName=parametersFileName;
//    return(true);
//}

void ParametersManager::setFileName(QString fileName)
{
    _fileName=fileName;
}

QVector<QString> ParametersManager::getParameterCodes()
{
    QVector<QString> parameterCodes;    
    QMap<QString, Parameter*>::iterator i = _parameters.begin();
    while(i!=_parameters.end())
    {
        Parameter* ptrParameter=i.value();
        parameterCodes.push_back(ptrParameter->getCode());
        i++;
    }
    return(parameterCodes);
}

Parameter* ParametersManager::getParameterFromTag(QString tag)
{
    Parameter* ptrParameter=NULL;
    QMap<QString, Parameter*>::iterator i = _parameters.begin();
    while(i!=_parameters.end())
    {
        Parameter* ptrAuxParameter=i.value();
        QString auxTag=ptrAuxParameter->getTag();
        if(auxTag.compare(tag,Qt::CaseInsensitive)==0)
        {
            ptrParameter=ptrAuxParameter;
            break;
        }
        i++;
    }
    return(ptrParameter);
}

QVector<QString> ParametersManager::getParameterTags()
{
    QVector<QString> parameterTags;    
    QMap<QString, Parameter*>::iterator i = _parameters.begin();
    while(i!=_parameters.end())
    {
        Parameter* ptrParameter=i.value();
        parameterTags.push_back(ptrParameter->getTag());
        i++;
    }
    return(parameterTags);
}

QVector<QString> ParametersManager::getParameterTypes()
{
    return(_parameterTypes);
}

QString ParametersManager::getPath(void)
{
    return(_path);
}

void ParametersManager::setPath(QString path)
{
    _path=path;
}

bool ParametersManager::setParametersForCommand(QString command,
                                                QWidget *ptrWidget)
{
    ParametersManagerDialog dialog(this,command,ptrWidget);
//    dialog.show();
    return(true);
}

bool ParametersManager::getParametersByCommand(QString command, // si esta vacio devuelvo todos
                                               QVector<Parameter *>& ptrParameters,
                                               bool onlyEnabled)
{
    ptrParameters.clear();
    QMap<QString,Parameter*>::const_iterator iterParameters=_parameters.begin();
    while(iterParameters!=_parameters.end())
    {
        Parameter* ptrParameter=iterParameters.value();
        if(onlyEnabled&&!ptrParameter->isEnabled())
        {
            iterParameters++;
            continue;
        }
        QString parameterCommand=ptrParameter->getCommand();
        if(parameterCommand.compare(command,Qt::CaseInsensitive)==0)
        {
            ptrParameters.push_back(ptrParameter);
        }
        else if(command.isEmpty())
        {
            ptrParameters.push_back(ptrParameter);
        }
        iterParameters++;
    }
    return(true);
}

bool ParametersManager::getParameterValue(QString command,
                                          QString code,
                                          QString &value,
                                          QString &strError)
{
    QMap<QString,Parameter*>::const_iterator iterParameters=_parameters.begin();
    while(iterParameters!=_parameters.end())
    {
        Parameter* ptrParameter=iterParameters.value();
        QString parameterCommand=ptrParameter->getCommand();
        if(parameterCommand.compare(command,Qt::CaseInsensitive)==0)
        {
            QString parameterCode=ptrParameter->getCode();
            if(parameterCode.compare(code,Qt::CaseInsensitive)==0)
            {
                ptrParameter->getValue(value);
                return(true);
            }
        }
        iterParameters++;
    }
    strError="Parameter not found";
    return(false);
}

bool ParametersManager::getParametersTagAndValues(QString command,
                                                  QVector<QString> &codes,
                                                  QVector<QString> &tags,
                                                  QVector<QString> &values,
                                                  QString &strError)
{
    codes.clear();
    tags.clear();
    values.clear();
    QMap<QString,Parameter*>::const_iterator iterParameters=_parameters.begin();
    bool parameterFound=false;
    while(iterParameters!=_parameters.end())
    {
        Parameter* ptrParameter=iterParameters.value();
        QString parameterCommand=ptrParameter->getCommand();
        if(parameterCommand.compare(command,Qt::CaseInsensitive)==0)
        {
            if(!parameterFound)
                parameterFound=true;
            if(ptrParameter->isEnabled())
            {
                QString parameterCode=ptrParameter->getCode();
                QString parameterTag=ptrParameter->getTag();
                QString parameterValue;
                ptrParameter->getValue(parameterValue);
                codes.push_back(parameterCode);
                tags.push_back(parameterTag);
                values.push_back(parameterValue);
            }
        }
        iterParameters++;
    }
    if(!parameterFound)
    {
        strError="Parameter not found";
        return(false);
    }
    return(true);
}
