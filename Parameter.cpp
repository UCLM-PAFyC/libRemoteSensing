#include <cmath>
#include <QtWidgets>

#include "ParameterDefinitions.h"
#include "Parameter.h"

Parameter::Parameter(void)
{
    _doubleValue=0.0;
    _doubleMinValue=-1000000.0;
    _doubleMaxValue=1000000.0;
    _intValue=0;
    _intMinValue=-1000000;
    _intMaxValue=1000000;
    _strValue="";
    _emc=0.0;
    _isEnabled=true;
    _tag="";
    _code="";
    _explanation="";
    _type=DEFAULT_PARAMETER_TYPE;
    _factorConversion=1.0;
    _printWidth=20;
    _printPrecision=3;
    _unit=UNIT_ADIMENSIONAL;
    _unitAcronym=ACRONYM_UNIT_ADIMENSIONAL;
    _intValues.clear();
    _intMinValues.clear();
    _intMaxValues.clear();
}

bool Parameter::insertEnum(QString value)
{
    bool success=false;
    if(_enum.contains(value))
        return(false);
    else
    {
        _enum.push_back(value);
    }
    return(success);
}

bool Parameter::existsEnumValue(QString value)
{
    bool success=false;
    if(_enum.contains(value))
        success=true;
    return(success);
}

QVector<QString> Parameter::getEnumValues()
{
    return(_enum);
}

void Parameter::getEnumValues(QString& values)
{
    values="";
    for(int i=0;i<_enum.size();i++)
    {
        values+=_enum.at(i);
        if(i<(_enum.size()-1))
            values+=ENUM_CHARACTER_SEPARATOR;
    }
}

void Parameter::getValue(QMap<QString, QString> &value)
{
    value=_tagValues;
}

bool Parameter::setEnumValues(QString values)
{
    _enum.clear();
    QStringList enumValues=values.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
    for(int i=0;i<enumValues.size();i++)
        _enum.push_back(enumValues.at(i).trimmed());
    return(true);
}

QString Parameter::getCode(void) const
{
    return _code;
}

QString Parameter::getTag(void) const
{
    return _tag;
}

double Parameter::getFactorConversion(void) const
{
    return _factorConversion;
}

void Parameter::getMaxValue(double& value) const
{
   value=_doubleMaxValue;
}

void Parameter::getMinValue(double& value) const
{
   value=_doubleMinValue;
}

void Parameter::getValue(double& value) const
{
   value=_doubleValue;
}

void Parameter::getMaxValue(int& value) const
{
   value=_intMaxValue;
}

void Parameter::getMinValue(int& value) const
{
   value=_intMinValue;
}

void Parameter::getValue(int& value) const
{
   value=_intValue;
}

void Parameter::getValue(QString& value) const
{
   if(_type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
   {
       value="";
       for(int i=0;i<_intValues.size();i++)
       {
           if(_type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
               value+=QString::number(_intValues.at(i));
           if(i<(_intValues.size()-1))
               value+=ENUM_CHARACTER_SEPARATOR;
       }
   }
   else if(_type.compare(PARAMETER_TYPE_INTEGER,Qt::CaseInsensitive)==0)
   {
       value=QString::number(_intValue);
   }
   else if(_type.compare(PARAMETER_TYPE_DOUBLE,Qt::CaseInsensitive)==0)
   {
       value=QString::number(_doubleValue,'f',_printPrecision);
   }
   else if(_type.compare(PARAMETER_TYPE_ENUM,Qt::CaseInsensitive)==0)
   {
       value=_strValue;
//       value="";
//       for(int i=0;i<_enum.size();i++)
//       {
//           value+=_enum.at(i);
//           if(i<(_enum.size()-1))
//               value+=ENUM_CHARACTER_SEPARATOR;
//       }
   }
   else
   {
       value=_strValue;
   }
}

QString Parameter::getPrintValue(void) const
{
    QString strValue;
    if(_unit==PARAMETER_TYPE_STRING
        ||_unit==PARAMETER_TYPE_ENUM
        ||_unit==PARAMETER_TYPE_DATE)
        strValue=_strValue.rightJustified(_printWidth);
    if(_unit==PARAMETER_TYPE_INTEGER)
        strValue=QString::number(_intValue).rightJustified(_printWidth);
    if(_unit==PARAMETER_TYPE_DOUBLE)
    {
        double value=_doubleValue*_factorConversion;
        strValue=QString::number(value,'f',_printPrecision).rightJustified(_printWidth);
    }
    return strValue;
}

double Parameter::getEmc(void) const
{
    return _emc;
}

double Parameter::getPrintEmc(void) const
{
    return _emc*_factorConversion;
}

QString Parameter::getExplanation(void) const
{
    return _explanation;
}

int Parameter::getPrintPrecision(void) const
{
    return _printPrecision;
}

int Parameter::getPrintWidth(void) const
{
    return _printWidth;
}

QString Parameter::getType(void) const
{
    return _type;
}

QString Parameter::getUnit(void) const
{
    return _unit;
}

QString Parameter::getUnitAcronym(void) const
{
    return _unitAcronym;
}

bool Parameter::isEnabled(void) const
{
    return _isEnabled;
}

bool Parameter::isInDomain(void) const
{
    if(_type.compare(PARAMETER_TYPE_DOUBLE,Qt::CaseInsensitive)==0)
        return _doubleValue<=_doubleMaxValue&&_doubleValue>=_doubleMinValue;
    if(_type.compare(PARAMETER_TYPE_INTEGER,Qt::CaseInsensitive)==0)
        return _intValue<=_intMaxValue&&_intValue>=_intMinValue;
    return(true);
}

void Parameter::setCode(QString name)
{
    _code=name;
}

void Parameter::setTag(QString name)
{
    _tag=name;
}

void Parameter::setEmc(double emc)
{
    _emc=emc;
}

void Parameter::setEnabled(bool isEnabled)
{
    _isEnabled=isEnabled;
}

void Parameter::setExplanation(QString explanation)
{
    _explanation=explanation;
}

void Parameter::setFactorConversion(void)
{
    if(_type.compare(PARAMETER_TYPE_DOUBLE,Qt::CaseInsensitive)==0
        &&_factorConversion==1.0)
    {
        if(_unit.compare(UNIT_GRAD_CENTE)==0)
            _factorConversion=200.0/(4.0*atan(1.0));
        if(_unit.compare(UNIT_GRAD_DEG)==0)
            _factorConversion=180.0/(4.0*atan(1.0));
        if(_unit.compare(UNIT_PERCENTAGE_100)==0)
            _factorConversion=100.0;
        if(_unit.compare(UNIT_PERCENTAGE_1000)==0)
            _factorConversion=1000.0;
        if(_unit.compare(UNIT_PIXEL)==0)
            _factorConversion=1.0/1000.0/_doubleValue;
        if(_unit.compare(UNIT_MILIMETER)==0)
            _factorConversion=1000.0;
        if(_unit.compare(UNIT_KILOHERTZ)==0)
            _factorConversion=1.0/1000.0;
        if(_unit.compare(UNIT_MEGAHERTZ)==0)
            _factorConversion=1.0/1000000.0;
        if(_unit.compare(UNIT_KILOMETERS_PER_HOUR)==0)
            _factorConversion=1.0/1000.0*60.0*60.0;
        if(_unit.compare(UNIT_KNOTS)==0)
            _factorConversion=1.0/1000.0*60.0*60.0/1.852;
    }
}

void Parameter::setMaxValue(double maxValue)
{
    _doubleMaxValue=maxValue;
}

void Parameter::setMinValue(double minValue)
{
    _doubleMinValue=minValue;
}

void Parameter::setValue(double value)
{
    _doubleValue=value;
}

void Parameter::setMaxValue(int maxValue)
{
    _intMaxValue=maxValue;
}

void Parameter::setMinValue(int minValue)
{
    _intMinValue=minValue;
}

void Parameter::setValue(int value)
{
    _intValue=value;
}

void Parameter::setValue(QString value)
{
    _strValue=value;
}

void Parameter::setValue(QMap<QString, QString> &value)
{
    _tagValues=value;
}

bool Parameter::setParameter(Parameter* other)
{
    if(other==NULL)
        return(false);
    _code=other->getCode();
    _tag=other->getTag();
    _explanation=other->getExplanation();
    _emc=other->getEmc();
    _factorConversion=other->getFactorConversion();
    _isEnabled=other->isEnabled();
    _unit=other->getUnit();
    _printWidth=other->getPrintWidth();
    _printPrecision=other->getPrintPrecision();
    other->getMinValue(_doubleMinValue);
    other->getMaxValue(_doubleMaxValue);
    other->getValue(_doubleValue);
    other->getMinValue(_intMinValue);
    other->getMaxValue(_intMaxValue);
    other->getValue(_intValue);
    other->getValue(_strValue);
    QString values;
    other->getIntMinValues(values);
    this->setIntMinValues(values);
    other->getIntMaxValues(values);
    this->setIntMaxValues(values);
    other->getIntValues(values);
    this->setIntValues(values);
    return(true);
}

void Parameter::setPrintPrecision(int printPrecision)
{
    _printPrecision=printPrecision;
}

void Parameter::setPrintWidth(int printWidth)
{
    _printWidth=printWidth;
}

void Parameter::setType(QString type)
{
    _type=type;
}

void Parameter::setUnit(QString unit)
{
    _unit=unit;
    setFactorConversion();
    setUnitAcronym();
}

void Parameter::setUnitAcronym(void)
{
    if(_unit.isEmpty())
        return;
    if(_unit.compare(UNIT_METER,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_METER;
        return;
    }
    if(_unit.compare(UNIT_MILIMETER,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_MILIMETER;
        return;
    }
    if(_unit.compare(UNIT_PIXEL,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_PIXEL;
        return;
    }
    if(_unit.compare(UNIT_RADIAN,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_RADIAN;
        return;
    }
    if(_unit.compare(UNIT_GRAD_CENTE,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_GRAD_CENTE;
        return;
    }
    if(_unit.compare(UNIT_GRAD_DEG,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_GRAD_DEG;
        return;
    }
    if(_unit.compare(UNIT_ADIMENSIONAL,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_ADIMENSIONAL;
        return;
    }
    if(_unit.compare(UNIT_PERCENTAGE_1,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_PERCENTAGE_1;
        return;
    }
    if(_unit.compare(UNIT_PERCENTAGE_1,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_PERCENTAGE_1;
        return;
    }
    if(_unit.compare(UNIT_PERCENTAGE_100,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_PERCENTAGE_100;
        return;
    }
    if(_unit.compare(UNIT_PERCENTAGE_1000,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_PERCENTAGE_1000;
        return;
    }
    if(_unit.compare(UNIT_HERTZ,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_HERTZ;
        return;
    }
    if(_unit.compare(UNIT_KILOHERTZ,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_KILOHERTZ;
        return;
    }
    if(_unit.compare(UNIT_MEGAHERTZ,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_MEGAHERTZ;
        return;
    }
    if(_unit.compare(UNIT_METERS_PER_SECOND,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_METERS_PER_SECONDS;
        return;
    }
    if(_unit.compare(UNIT_KILOMETERS_PER_HOUR,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_UNIT_KILOMETERS_PER_HOUR;
        return;
    }
    if(_unit.compare(UNIT_KNOTS,Qt::CaseInsensitive)==0)
    {
        _unitAcronym=ACRONYM_KNOTS;
        return;
    }
}

void Parameter::fromPrintUnit(double& value)
{
    value=value/_factorConversion;
}

void Parameter::toPrintUnit(double& value)
{
    value=value*_factorConversion;
}

void Parameter::getIntValues(QString &values)
{
    values="";
    for(int i=0;i<_intValues.size();i++)
    {
        if(_type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
            values+=QString::number(_intValues.at(i));
        if(i<(_intValues.size()-1))
            values+=ENUM_CHARACTER_SEPARATOR;
    }
}

void Parameter::getIntMinValues(QString &values)
{
    values="";
    for(int i=0;i<_intMinValues.size();i++)
    {
        if(_type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
            values+=QString::number(_intMinValues.at(i));
        if(i<(_intMinValues.size()-1))
            values+=ENUM_CHARACTER_SEPARATOR;
    }
}

void Parameter::getIntMaxValues(QString &values)
{
    values="";
    for(int i=0;i<_intMaxValues.size();i++)
    {
        if(_type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
            values+=QString::number(_intMaxValues.at(i));
        if(i<(_intMaxValues.size()-1))
            values+=ENUM_CHARACTER_SEPARATOR;
    }
}

void Parameter::setIntValues(QString &values)
{
    _intValues.clear();
    QStringList strIntValues=values.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
    for(int i=0;i<strIntValues.size();i++)
    {
        if(_type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
            _intValues.push_back(strIntValues.at(i).trimmed().toInt());
    }
}

void Parameter::setIntMinValues(QString &values)
{
    _intMinValues.clear();
    QStringList strIntValues=values.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
    for(int i=0;i<strIntValues.size();i++)
    {
        if(_type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
            _intMinValues.push_back(strIntValues.at(i).trimmed().toInt());
    }
}

void Parameter::setIntMaxValues(QString &values)
{
    _intMaxValues.clear();
    QStringList strIntValues=values.split(ENUM_CHARACTER_SEPARATOR,QString::SkipEmptyParts);
    for(int i=0;i<strIntValues.size();i++)
    {
        if(_type.compare(PARAMETER_TYPE_VECTOR_INTEGER,Qt::CaseInsensitive)==0)
            _intMaxValues.push_back(strIntValues.at(i).trimmed().toInt());
    }
}

QString Parameter::getCommand() const
{
    return(_command);
}

void Parameter::setCommand(QString value)
{
    _command=value;
}
