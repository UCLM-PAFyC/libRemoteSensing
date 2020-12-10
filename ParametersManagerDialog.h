// ControlVueloFotogrametrico Version 2.0.
// Copyright (C) 2006-09: Beatriz Felipe, David Hernandez
// Autores:
//            Beatriz Felipe Garcia, beatriz.felipe@uclm.es
//            David Hernandez Lopez, david.hernandez@uclm.es
// ultima actualizacion 2008-12

/**
\file
\brief Fichero de declaracion de la clase ParametersManagerDialog.
*/

#ifndef PARAMETERS_MANAGER_DIALOG_H
#define PARAMETERS_MANAGER_DIALOG_H

#include <QDialog>
#include <QWidget>

class QLabel;
class QPushButton;
class QDialogButtonBox;
class QTabWidget;
class QTableWidget;
class QTableWidgetItem;

class ParametersManager;

/**
\class ParametersManagerDialog.
\brief Clase que actua como interfaz grafica de usuario para la clase ParametersManager.

\author Beatriz Felipe Garcia, beatriz.felipe@uclm.es.
\author David Hernandez Lopez, david.hernandez@uclm.es.
\version 1.4
\date    2008-12

Esta clase permite modificar la informacion de la clase ParametersManager y acceder a la creacion y
modificacion de la definicion de los parametros, instancias de la clase Parameters, a traves de la clase ParameterDialog.

Cuando se construye una instancia de este objeto, al elegir la opcion correspondiente de un menu de la interfaz grafica de la clase
MainWindow, se despliega un dialogo con un diseño que incluye:
- Una etiqueta de texto con el nombre del fichero XML de definicion de parametros, con la ruta completa, y un
  boton para modificarlo, guardando la informacion de la definicion de parametros en el fichero seleccionado.
- Una tabla con toda la informacion de cada parametro en una fila. Pulsando sobre la cualquier celda se construye
  y despliega una instancia de la clase ParameterDialog para el parametro correspondiente a la fila de la celda pulsada.
- Un boton para añadir un nuevo parametro que construye y despliega una instancia de la clase ParameterDialog con una
  instancia de Parameter construida con la informacion por defecto.
- Un boton para eliminar los parametros correspondientes a las filas seleccionadas en la tabla.
- Un boton para aceptar (OK) y otro para cancelar (CANCEL).

Cuando se inserta o modifica una instancia de Parameter se almacena en el fichero XML y se comprueba 
la integridad con el programa, se comprueba que existen al menos los parametros requeridos por el programa.
*/
class ParametersManagerDialog : public QDialog
{
Q_OBJECT
private slots:

    /**
    \brief Metodo para cerrar el dialogo.
    */
    void onOk();

    /**
    \brief Metodo para modificar la informacion de un parametro mediante la construccion de un objeto ParameterDialog
           con la direccion de memoria del parametro correspondiente a la fila de la celda pulsada.
    \param[in] item  Puntero a la celda sobre la que se pulsa
    */
    void tableWidgetClicked(QTableWidgetItem* item);

public:

    /**
    \brief Constructor que despliega el dialogo si no se produce ningun error.
    \param[in]  ptrParametersManager Puntero al objeto ParametersManager que gestiona los parametros.
    \param[in]  ptrLanguagesManager  Puntero al objeto LanguagesManager que interactua con la base de datos de traducciones.
    \param[in]  ptrParent            Puntero al objeto QWidget padre.
    */
    ParametersManagerDialog(ParametersManager *ptrParametersManager,
                            QString command,
                            QWidget * ptrParent = NULL);
private:
    void createTabWidget();
    void fillTabWidget(void);
    QLabel *fileLabel;
    QTabWidget *tabWidget;
    QTableWidget *tableWidget;
    QString _command;
    ParametersManager* _ptrParametersManager;
};

#endif
