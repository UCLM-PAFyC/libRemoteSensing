// ControlVueloFotogrametrico Version 2.0.
// Copyright (C) 2006-09: Beatriz Felipe, David Hernandez
// Autores:
//            Beatriz Felipe Garcia, beatriz.felipe@uclm.es
//            David Hernandez Lopez, david.hernandez@uclm.es
// ultima actualizacion 2008-12

/**
\file
\brief Fichero de declaracion de la clase Parameter.
*/

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <QVector>
#include <QMap>
#include <QString>

/**
\class Parameter.
\brief Clase para definir y manipular cada parametro de entrada y/o salida.

Los objetos de esta clase se instancian en el HEAP.

La unica instancia del objeto ParametersManager que tiene el programa (instancia de la clase MainWindow) contiene 
un contenedor de instancias de esta clase: ParametersManager::_parameters.

Esta clase esta diseñada para la correcta introduccion, gestion y salida de un parametro utilizado en el programa.

La definicion de un parametro incluye:
- Un codigo, cadena de texto que lo identifica y que debe coincidir con alguna de las incluidas en 
  el fichero de definiciones, ParameterDefinitions.h.
  Este codigo debe ser distinto para cada parametro ya que es la variable por la que se indexa 
  el contenedor ParametersManager:_parameters.
- Un tipo, de entre los incluidos en ParameterDefinitions.h:
  - Cadena de texto, identificado con el texto incluido en la definicion de \ref PARAMETER_TYPE_STRING.
  - Numero entero, identificado con el texto incluido en la la definicion de \ref PARAMETER_TYPE_INTEGER.
  - Numero en doble precision, identificado con el texto incluido en la la definicion de \ref PARAMETER_TYPE_DOUBLE.
  - Fecha, identificado con el texto incluido en la la definicion de \ref PARAMETER_TYPE_DATE.
  - Conjunto de textos, identificado con el texto incluido en la la definicion de \ref PARAMETER_TYPE_ENUM.
- Una etiqueta, texto que aparecera en cualquier parte de la GUI. Esta etiqueta debe ser distinta para cada parametro.
- Una descripcion del significado del parametro.
- Un numero de caracteres para la impresion.

Cada parametro debe tener un valor acorde a su tipo y un valor por defecto, con el que se inicializa el parametro.

Los parametros de tipo \ref PARAMETER_TYPE_INTEGER o \ref PARAMETER_TYPE_DOUBLE incluyen tambien una definicion de su dominio
definido a traves de un valor minimo y un valor maximo.
El dominio se utiliza para comprobar la validez del valor introducido por el usuario, ya sea a traves de la GUI o
de algun fichero de entrada.

Los parametros de tipo \ref PARAMETER_TYPE_DOUBLE incluyen tambien:
- El numero de decimales para la impresion.
- Una unidad, de entre las incluidas en ParameterDefinitions.h:
  - Metros, identificado con el texto \ref UNIT_METER y con acronimo \ref ACRONYM_UNIT_METER.
  - Milimetros, identificado con el texto \ref UNIT_MILIMETER y con acronimo \ref ACRONYM_UNIT_MILIMETER.
  - Pixel, identificado con el texto \ref UNIT_PIXEL y con acronimo \ref ACRONYM_UNIT_PIXEL.
  - Radianes, identificado con el texto \ref UNIT_RADIAN y con acronimo \ref ACRONYM_UNIT_RADIAN.
  - Grados centesimales, identificado con el texto \ref UNIT_GRAD_CENTE y con acronimo \ref ACRONYM_GRAD_CENTE.
  - Grados sexagesimales (DEG), identificado con el texto \ref UNIT_GRAD_DEG y con acronimo \ref ACRONYM_GRAD_DEG.
  - Adimensional, identificado con el texto \ref UNIT_ADIMENSIONAL y con acronimo \ref ACRONYM_UNIT_ADIMENSIONAL.
  - Tanto por unidad, identificado con el texto \ref UNIT_PERCENTAGE_1 y con acronimo \ref ACRONYM_UNIT_PERCENTAGE_1.
  - Tanto por cien, identificado con el texto \ref UNIT_PERCENTAGE_100 y con acronimo \ref ACRONYM_UNIT_PERCENTAGE_100.
  - Tanto por mil, identificado con el texto \ref UNIT_PERCENTAGE_1000 y con acronimo \ref ACRONYM_UNIT_PERCENTAGE_1000.
  - Segundos, identificado con el texto \ref UNIT_SECONDS y con acronimo \ref ACRONYM_UNIT_SECONDS.
- Pueden incluir el error medio cuadratico (EMC) de su valor, en la misma unidad del valor.

El programa utiliza como unidades internas para los parametros de tipo \ref PARAMETER_TYPE_DOUBLE :
- Magnitudes lineales.- metros.
- Magnitudes superficiales.- metros cuadrados.
- Magnitudes angulares.- radianes.
- Magnitudes adimensionales.- tanto por unidad.
Esto implica que para cada unidad el programa aplica un factor de conversion en operaciones de entrada,
y su inverso en operaciones de salida.
*/

class Parameter
{
public:

    /**
    \brief Constructor por defecto que inicializa las variables.
    */
    Parameter(void);

    /**
    \brief Metodo para obtener el codigo del parametro.
    \return Codigo del parametro.
    */
    QString getCode(void) const;

    /**
    \brief Metodo para obtener el comando del parametro.
    \return Comando del parametro.
    */
    QString getCommand(void) const;

    /**
    \brief Metodo para obtener la etiqueta del parametro.
    \return Etiqueta del parametro.
    */
    QString getTag(void) const;

    /**
    \brief Metodo para obtener el tipo del parametro.
    \return Tipo del parametro.
    */
    QString getType(void) const;

    /**
    \brief Metodo para obtener el factor de conversion del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \return Factor de conversion.
    */
    double getFactorConversion(void) const;

    /**
    \brief Metodo para obtener el valor maximo del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[out] value Referencia donde se devuelve el valor.
    */
    void getMaxValue(double& value) const;

    /**
    \brief Metodo para obtener el valor minimo del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[out] value Referencia donde se devuelve el valor.
    */
    void getMinValue(double& value) const;

    /**
    \brief Metodo para obtener el valor minimo del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[out] value Referencia donde se devuelve el valor.
    */
    void getValue(double& value) const;

    /**
    \brief Metodo para obtener el valor maximo del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_INTEGER.
    \param[out] value Referencia donde se devuelve el valor.
    */
    void getMaxValue(int& value) const;

    /**
    \brief Metodo para obtener el valor minimo del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_INTEGER.
    \param[out] value Referencia donde se devuelve el valor.
    */
    void getMinValue(int& value) const;

    /**
    \brief Metodo para obtener el valor del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_INTEGER.
    \param[out] value Referencia donde se devuelve el valor.
    */
    void getValue(int& value) const;

    /**
    \brief Metodo para obtener el valor minimo del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_STRING o \ref PARAMETER_TYPE_ENUM.
    \param[out] value Referencia donde se devuelve el valor.
    */
    void getValue(QString& value) const;

    /**
    \brief Metodo para obtener el valor de impresion del parametro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \return Valor de impresion.
    */
    QString getPrintValue(void) const;

    /**
    \brief Metodo para obtener el valor del EMC del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \return EMC del parametro.
    */
    double getEmc(void) const;

    /**
    \brief Metodo para obtener el valor del EMC del parametro, en formato de impresion.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \return EMC del parametro.
    */
    double getPrintEmc(void) const;

    /**
    \brief Metodo para obtener la descripcion del parametro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \return Descripcion.
    */
    QString getExplanation(void) const;

    /**
    \brief Metodo para obtener el numero de decimales de impresion.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \return Numero de decimales.
    */
    int getPrintPrecision(void) const;

    /**
    \brief Metodo para obtener el numero de caracteres para la impresion del parametro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \return Numero de caracteres.
    */
    int getPrintWidth(void) const;

    /**
    \brief Metodo para obtener la unidad del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \return Unidad del parametro.
    */
    QString getUnit(void) const;

    /**
    \brief Metodo para obtener el acronimo de la unidad del parametro.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    \return Acronimo de la unidad del parametro.
    */
    QString getUnitAcronym(void) const;

    /**
    \brief Metodo para obtener si el parametro esta habilitado.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \return Estado de habilitacion: verdadero/falso.
    */
    bool isEnabled(void) const;

    /**
    \brief Metodo para obtener si el valor de un parametro esta en su dominio.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_INTEGER o \ref PARAMETER_TYPE_DOUBLE.
    \return Verdadero si esta en dominio.
    */
    bool isInDomain(void) const;

    /**
    \brief Metodo para insertar un texto en la coleccion de valores.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_ENUM.
    \param[in] value  Texto a insertar.
    \return Verdadero si se ha insertado (falso si el texto ya estaba en la coleccion).
    */
    bool insertEnum(QString value);

    /**
    \brief Metodo para comprobar si un texto esta en la coleccion de valores.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_ENUM.
    \param[in] value  Texto a consultar.
    \return Verdadero si el texto esta en la coleccion.
    */
    bool existsEnumValue(QString value);

    /**
    \brief Metodo para obtener la coleccion de valores como un vector de cadena de caracteres.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_ENUM.
    \return Vector con la coleccion.
    */
    QVector<QString> getEnumValues();

    /**
    \brief Metodo para establecer la coleccion de valores a partir de una cadena con los textos separados por el caracter \ref ENUM_CHARACTER_SEPARATOR.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_ENUM.
    \return Verdadero si se construye correctamente.
    */
    bool setEnumValues(QString values);

    /**
    \brief Metodo para obtener la coleccion de valores como una cadena con los textos separados por el caracter \ref ENUM_CHARACTER_SEPARATOR.
    \brief Este metodo se utiliza para parametros de tipo \ref PARAMETER_TYPE_ENUM.
    \param[out] enumValues  Referencia a una cadena de texto donde devolver los valores.
    */
    void getEnumValues(QString& enumValues);

    void getValue(QMap<QString,QString>& value);

    /**
    \brief Metodo para asignar el codigo al parametro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \param[in] value  Valor a asignar.
    */
    void setCode(QString value);

    /**
    \brief Metodo para asignar el comando al parametro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \param[in] value  Valor a asignar.
    */
    void setCommand(QString value);

    /**
    \brief Metodo para asignar la etiqueta al parametro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \param[in] value  Valor a asignar.
    */
    void setTag(QString value);

    /**
    \brief Metodo para asignar el tipo al parametro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \param[in] value  Valor a asignar.
    */
    void setType(QString value);

    /**
    \brief Metodo para asignar el EMC al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[in] value  Valor a asignar.
    */
    void setEmc(double value);
    void setEnabled(bool);
    void setExplanation(QString);
    void setFactorConversion(void);

    /**
    \brief Metodo para asignar el valor maximo al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[in] value  Valor a asignar.
    */
    void setMaxValue(double value);

    /**
    \brief Metodo para asignar el valor minimo al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[in] value  Valor a asignar.
    */
    void setMinValue(double value);

    /**
    \brief Metodo para asignar el valor al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[in] value  Valor a asignar.
    */
    void setValue(double value);

    /**
    \brief Metodo para asignar el valor maximo al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_INTEGER.
    \param[in] value  Valor a asignar.
    */
    void setMaxValue(int value);

    /**
    \brief Metodo para asignar el valor minimo al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_INTEGER.
    \param[in] value  Valor a asignar.
    */
    void setMinValue(int value);

    /**
    \brief Metodo para asignar el valor al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_INTEGER.
    \param[in] value  Valor a asignar.
    */
    void setValue(int value);

    /**
    \brief Metodo para asignar el valor al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_STRING o \ref PARAMETER_TYPE_ENUM.
    \param[in] value  Valor a asignar.
    */
    void setValue(QString value);

    void setValue(QMap<QString,QString>& value);

    /**
    \brief Metodo para establecer toda la informacion de un parametro igual a la otro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \param[in] other  Puntero al parametro del que se debe copiar la definicion.
    \return Verdadero si no se produce ningun error.
    */
    bool setParameter(Parameter* other);

    /**
    \brief Metodo para asignar el numero de decimales de impresion al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[in] value  Valor a asignar.
    */
    void setPrintPrecision(int value);

    /**
    \brief Metodo para asignar el numero de caracteres de impresion al parametro.
    \brief Este metodo se utiliza para cualquier tipo de parametro.
    \param[in] value  Valor a asignar.
    */
    void setPrintWidth(int value);

    /**
    \brief Metodo para asignar la unidad al parametro.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[in] value  Valor a asignar.
    */
    void setUnit(QString value);

    /**
    \brief Metodo para asignar el acronimo de la unidad al parametro.
    \brief El acronimo se asigna automaticamente en funcion del tipo de unidad.
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    */
    void setUnitAcronym(void);

    /**
    \brief Metodo para pasar un valor de la unidad del parametro a la unidad interna correspondiente a su magnitud.
    \brief Se utilizara el factor de conversion para pasar de la unidad del parametro a la interna (de DEG a RAD, por ejemplo).
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[out] value  Referencia al valor de parametro que entra en la unidad del parametro y se devuelve en la interna funcion de la magnitud.
    */
    void fromPrintUnit(double& value);

    /**
    \brief Metodo para pasar un valor de la unidad interna del parametro correspondiente a su magnitud a la unidad del parametro.
    \brief Se utilizara el factor de conversion para pasar de la unidad del parametro a la interna (de RAD a DEG, por ejemplo).
    \brief Este metodo se utiliza para parametros del tipo \ref PARAMETER_TYPE_DOUBLE.
    \param[out] value  Referencia al valor de parametro que entra en la unidad interna funcion de la magnitud y se devuelve en la unidad del parametro.
    */
    void toPrintUnit(double& value);

    void getIntValues(QString &values);
    void getIntMinValues(QString &values);
    void getIntMaxValues(QString &values);
    void setIntValues(QString &values);
    void setIntMinValues(QString &values);
    void setIntMaxValues(QString &values);

private:

    /// Commando que usa el parametro
    QString _command;

    /// Codigo que debe ser unico para cada especificacion tecnica ya que es su indice 
    /// en el contenedor de parametros de un objeto de la clase ParametersManager, ParametersManager::_parameters.
    QString _code;
    
    /// Etiqueta del parametro.
    QString _tag;

    /// Tipo del parametro.
    QString _type;

    /// Descripcion del parametro.
    QString _explanation;

    /// Valor minimo para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    double _doubleMinValue;

    /// Valor maximo para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    double _doubleMaxValue;

    /// Valor para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    double _doubleValue;

    /// Valor minimo para parametros de tipo \ref PARAMETER_TYPE_INTEGER.
    int _intMinValue;

    /// Valor maximo para parametros de tipo \ref PARAMETER_TYPE_INTEGER.
    int _intMaxValue;

    /// Valor para parametros de tipo \ref PARAMETER_TYPE_INTEGER.
    int _intValue;

    /// Valor para parametros de tipo \ref PARAMETER_TYPE_STRING o \ref PARAMETER_TYPE_ENUM.
    QString _strValue;

    /// EMC para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    double _emc;

    /// Factor de conversion para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    double _factorConversion;

    /// Estado de habilitacion del parametro.
    bool _isEnabled;

    /// Unidad parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    QString _unit;

    /// Acronimo de la unidad para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    QString _unitAcronym;
    
    /// Numero de caracteres de impresion.
    int _printWidth;

    /// Numero de decimales de impresion para parametros de tipo \ref PARAMETER_TYPE_DOUBLE.
    int _printPrecision;

    /// Coleccion de valores para parametros de tipo \ref PARAMETER_TYPE_ENUM.
    QVector<QString> _enum;

    QVector<int> _intValues;
    QVector<int> _intMinValues;
    QVector<int> _intMaxValues;

    QMap<QString,QString> _tagValues;

};
#endif
