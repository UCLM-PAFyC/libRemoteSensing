// ControlVueloFotogrametrico Version 2.0.
// Copyright (C) 2006-09: Beatriz Felipe, David Hernandez
// Autores:
//            Beatriz Felipe Garcia, beatriz.felipe@uclm.es
//            David Hernandez Lopez, david.hernandez@uclm.es
// ultima actualizacion 2008-12

/**
\file
\brief Fichero de declaracion de la clase ParametersManager.
*/

#ifndef PARAMETERS_MANAGER_H
#define PARAMETERS_MANAGER_H

#include <QMap>
#include <QVector>
#include <QString>

class Parameter;
class QWidget;


/**
\class ParametersManager.
\brief Clase para la gestion de los parametros, objetos del tipo Parameter, del programa.

\author Beatriz Felipe Garcia, beatriz.felipe@uclm.es.
\author David Hernandez Lopez, david.hernandez@uclm.es.
\version 1.4
\date    2008-12

El programa mantiene una unica instancia de esta clase en el HEAP (region disponible para las solicitudes de memoria dinamica al sistema operativo).

Cada parametro gestionado por el programa es una instancia de la clase Parameter.

Esta clase es la responsable de mantener el contenedor con los parametros definidos, y permitir su construccion y modificacion.

La informacion de los parametros se almacena en un fichero XML. Este fichero lo podria editar directamente el usuario pero para
evitar errores se han diseñado dos clases interfaz:
- ParametersManagerDialog.- Que despliega la lista de parametros existentes y permite insertar nuevos y modificar existentes
  a traves de llamadas a ParameterDialog.
- ParameterDialog.- Que despliega la informacion de un parametro y permite su modificacion. Cualquier modificacion 
  de un parametro se hace persistir en el fichero XML cuando se cierra este dialogo pulsando sobre el boton OK.

El programa comprueba que el fichero de parametros incluye al menos los que son necesarios en el propio programa
mediante la llamada al metodo Photogrammetry::ProjectFlightControl::setParametersManager() en tres casos:
- Cuando arranca el programa, cargandose el fichero por defecto, conforme a: \ref DEFAULT_PARAMETERS_PATH y \ref DEFAULT_PARAMETERS_FILE.
- Cuando el usuario elige otro fichero de parametros, a traves de la interfaz de la clase ParametersManagerDialog.
- Cuando el usuario modifica la definicion de parametros, lo que se produce cada vez que se añade o modifica un parametro, 
  a traves de la interfaz de la clase ParameterDialog.
*/
class ParametersManager
{
public:

    /**
    \brief Constructor por defecto.
    \brief Este metodo unicamente rellena el contenedor de tipos de parametros, \ref _parameterTypes,  con los tipos:
           - Entero, \ref PARAMETER_TYPE_INTEGER.
           - Doble precision, /ref PARAMETER_TYPE_DOUBLE.
           - Texto, \ref PARAMETER_TYPE_STRING.
           - Lista de textos, \ref PARAMETER_TYPE_ENUM.
    */
    ParametersManager();

    /**
    \brief Metodo para eliminar un parametro.
    \param[in]  code       Codigo del parametro a eliminar.
    \return Verdadero si no se produce ningun error.
    */
    bool deleteParameter(QString code);

    /**
    \brief Metodo para copiar la informacion de un parametro en otro existente con el mismo codigo.
    \param[in]  ptrParameter       Puntero al parametro cuya informacion se quiere copiar.
    \return Verdadero si existe un parametro con el mismo codigo.
    */
    bool setParameter(Parameter* ptrParameter);

    /**
    \brief Metodo para obtener el nombre del fichero XML de definicion de los parametros.
    \return Nombre del fichero XML, con la ruta completa.
    */
    QString getFileName(void);

    /**
    \brief Metodo para obtener el numero de parametros del contenedor.
    \return Numero de parametros.
    */
    int getNumberOfParameters(void);

    /**
    \brief Metodo para obtener el puntero a un parametro por su posicion en el contenedor.
    \param[in] pos   Posicion en el contenedor del parametro que se desea obtener.
    \return Puntero al parametro o NULL si la posicion es menor que 0 o mayor al numero de parametos menos uno.
    */
    Parameter* getParameter(int pos);

    /**
    \brief Metodo para obtener el puntero a un parametro por su codigo.
    \param[in] code   Codigo del parametro que se desea obtener.
    \return Puntero al parametro o NULL si el contenedor no incluye ningun parametro con este codigo.
    */
    Parameter* getParameter(QString code);

    /**
    \brief Metodo para obtener el puntero a un parametro por su etiqueta.
    \param[in] tag   Etiqueta del parametro que se desea obtener.
    \return Puntero al parametro o NULL si el contenedor no incluye ningun parametro con este codigo.
    */
    Parameter* getParameterFromTag(QString tag);

    /**
    \brief Metodo para obtener la ruta del fichero XML de definicion de los parametros.
    \return Ruta completa del fichero XML.
    */
    QString getPath(void);

    /**
    \brief Metodo para insertar un parametro en el contenedor.
    \brief Como el parametro ha sido creado previamente en el HEAP, lo que se inserta
           en el contenedor es su direccion de memoria.
    \param[in]  ptrParameter       Puntero al parametro a insertar.
    \return La propia direccion de memoria del parametro insertado.
    */
    Parameter* insertParameter(Parameter* ptrParameter);

    /**
    \brief Metodo para obtener un vector con los codigos de los parametros del contenedor.
    \return Vector con los codigos.
    */
    QVector<QString> getParameterCodes();

    /**
    \brief Metodo para obtener un vector con los tipos de los parametros del contenedor.
    \return Vector con los tipos.
    */
    QVector<QString> getParameterTypes();

    /**
    \brief Metodo para obtener un vector con las etiquetas de los parametros del contenedor.
    \return Vector con las etiquetas.
    */
    QVector<QString> getParameterTags();

    /**
    \brief Metodo para comprobar si se existe el fichero XML de definicion de parametros.
    \return Falso si no se ha definido.
    */
    bool isOk(void);

    /**
    \brief Metodo para construir el contenedor de parametros desde el fichero XML de definicion.
    \param[in]  fileName       Nombre del fichero XML, con la ruta completa.
    \param[in]  ptrParent      Puntero al objeto QWidget padre.
    \return Falso si se produce algun error.
    */
    bool loadFromXml(QString fileName,
                     QString& strError);

    /**
    \brief Metodo para salvar la definicion de los parametros del contenedor de parametros en el fichero XML de definicion.
    \param[in]  ptrParent      Puntero al objeto QWidget padre.
    \return Falso si se produce algun error.
    */
    bool saveAsXml(QWidget* ptrParent=NULL);

//    /**
//    \brief Metodo para establecer los valores por defecto a partir del fichero XML de definicion
//           de parametros por defecto, conforme a: \ref DEFAULT_PARAMETERS_PATH y \ref DEFAULT_PARAMETERS_FILE.
//    \brief Este metodo es invocado por el constructor de la clase MainWindow.
//    \return Falso si se produce algun error.
//    */
//    bool setDefault(QString fileName="");

    /**
    \brief Metodo para asignar el nombre del fichero XML con la definicion de los parametros.
    \brief Este metodo es invocado al final del metodo \ref loadFromXml. 
    \param[in]  value       Nombre del fichero XML, con la ruta completa.
    */
    void setFileName(QString value);

    /**
    \brief Metodo para asignar la ruta del fichero XML con la definicion de los parametros.
    \brief Este metodo es invocado al final del metodo \ref setFileName. 
    \param[in]  value       Nombre del fichero XML, con la ruta completa.
    */
    void setPath(QString value);
    //bool setParameterDescription(Parameter* ptrParameter);

    bool setParametersForCommand(QString command,
                                 QWidget* ptrWidget=NULL);

    bool getParametersByCommand(QString command,
                                QVector<Parameter *> &ptrParameters,
                                bool onlyEnabled=false);
    bool getParameterValue(QString command,
                           QString code,
                           QString& value,
                           QString& strError);
    bool getParametersTagAndValues(QString command,
                                   QVector<QString>& codes,
                                   QVector<QString>& tags,
                                   QVector<QString>& values,
                                   QString& strError);

private:

    /// Fichero XML con la definicion de los parametros.
    QString _fileName;
    
    /// Contenedor de punteros a los parametros indexado por los codigos de los parametros.
    QMap<QString,Parameter*> _parameters;

    /// Contenedor de los tipos de parametros.
    QVector<QString> _parameterTypes;

    /// Ruta completa del fichero _fileName.
    QString _path;
};

#endif
