// ControlVueloFotogrametrico Version 2.0.
// Copyright (C) 2006-09: Beatriz Felipe, David Hernandez
// Autores:
//            Beatriz Felipe Garcia, beatriz.felipe@uclm.es
//            David Hernandez Lopez, david.hernandez@uclm.es
// ultima actualizacion 2008-12

/**
\file
\brief Fichero con definiciones utilizadas por las clases Parameter y ParametersManager.

\author Beatriz Felipe Garcia, beatriz.felipe@uclm.es.
\author David Hernandez Lopez, david.hernandez@uclm.es.
\version 1.4
\date    2008-12

*/

#ifndef PARAMETERS_DEFINITIONS_H
#define PARAMETERS_DEFINITIONS_H

/// Valor que representa un numero entero no valido.
#define PARAMETERS_NO_INTEGER            -999999

/// Valor que representa un numero en doble precision no valido.
#define PARAMETERS_NO_DOUBLE      -999999.999999

/// Numero de caracteres para imprimir cada columna generica de un listado.
#define COLUMN_WIDTH                          10

/// Numero de caracteres para imprimir una columna correspondiente a un numero de punto.
#define POINT_WIDTH                           10

/// Numero de caracteres para imprimir una columna correspondiente a una coordenada terreno de punto.
#define TERRAIN_COORDINATE_WIDTH              15

/// Numero de decimales para imprimir una coordenada terreno de punto.
#define TERRAIN_COORDINATE_PRECISION           4

/// Numero de caracteres para imprimir una columna correspondiente a una coordenada imagen de punto.
#define IMAGE_COORDINATE_WIDTH                15

/// Numero de decimales para imprimir una coordenada imagen de punto.
#define IMAGE_COORDINATE_PRECISION             4

/// Numero de decimales para imprimir una incognita adimensional.
#define UNKNOWN_ADIMENSIONAL_PRECISION        12

/// Numero de caracteres para imprimir el nombre de una incognita.
#define UNKNOW_NAME_WIDTH                     20

/// Numero de caracteres para imprimir la descripcion de un parametro.
#define EXPLANATION_WIDTH                     50

/// Numero de caracteres para imprimir el nombre de una unidad.
#define UNIT_NAME_WIDTH                       15

/// Numero de caracteres para imprimer el nombre de un parametro.
#define PARAMETER_NAME_WIDTH                  15

/// Numero de caracteres para imprimir el valor de un parametro.
#define PARAMETER_VALUE_WIDTH                 20

/// Numero de decimales para imprimir el valor de un parametro.
#define PARAMETER_PRECISION_WIDTH             20

/// Numero de caracteres para imprimir la descripcion de un parametro.
#define PARAMETER_EXPLANATION_WIDTH           50

/// Numero de decimales para imprimir una incognita lineal.
#define UNKNOWN_LINEAR_PRECISION               4

/// Numero de decimales para imprimir una incognita angular.
#define UNKNOWN_ANGULAR_PRECISION              6

/// Tipo de parametro por defecto.
#define DEFAULT_PARAMETER_TYPE      "Texto"

/// Numero de decimales por defecto para un parametro.
#define DEFAULT_PARAMETER_PRECISION 3

/// Etiqueta para el tipo de parametro enumeracion (lista de textos).
#define PARAMETER_TYPE_ENUM         "Lista de textos"

/// Etiqueta para el tipo de parametro texto (cadena de caracteres).
#define PARAMETER_TYPE_STRING       "Texto"

/// Etiqueta para el tipo de parametro numero en doble precision.
#define PARAMETER_TYPE_DOUBLE       "Numero en doble precision"

/// Etiqueta para el tipo de parametro numero entero.
#define PARAMETER_TYPE_VECTOR_DOUBLE      "Vector de numeros en doble precision"

/// Etiqueta para el tipo de parametro numero entero.
#define PARAMETER_TYPE_INTEGER      "Numero entero"

/// Etiqueta para el tipo de parametro numero entero.
#define PARAMETER_TYPE_VECTOR_INTEGER      "Vector de numeros enteros"

/// Etiqueta para el tipo de parametro numero entero.
#define PARAMETER_TYPE_VECTOR_TAG_VALUES      "Vector de etiqueta/valor"

/// Etiqueta para el tipo de parametro fecha.
#define PARAMETER_TYPE_DATE         "Fecha"

/// Caracter separador de textos en una lista de textos.
#define ENUM_CHARACTER_SEPARATOR    ";"

/// Caracter separador de textos en una lista de textos.
#define TAG_VALUE_CHARACTER_SEPARATOR    "/"

/// Etiqueta de la unidad metros.
#define UNIT_METER                  "metros"

/// Acronimo de la unidad metros.
#define ACRONYM_UNIT_METER          "m"

/// Etiqueta de la unidad milimetros.
#define UNIT_MILIMETER              "milimetros"

/// Acronimo de la unidad milimetros.
#define ACRONYM_UNIT_MILIMETER      "mm"

/// Etiqueta de la unidad nanometros.
#define UNIT_NANOMETER              "nanometros"

/// Acronimo de la unidad nanometros.
#define ACRONYM_UNIT_NANOMETER      "nm"

/// Etiqueta de la unidad pixel.
#define UNIT_PIXEL                  "pixel"

/// Acronimo de la unidad pixel.
#define ACRONYM_UNIT_PIXEL          "px"

/// Etiqueta de la unidad radian.
#define UNIT_RADIAN                 "radianes"

/// Acronimo de la unidad radian.
#define ACRONYM_UNIT_RADIAN         "rad"

/// Etiqueta de la unidad grados centesimales.
#define UNIT_GRAD_CENTE             "grados centesimales"

/// Acronimo de la unidad grados centesimales.
#define ACRONYM_GRAD_CENTE          "grad"

/// Etiqueta de la unidad grados sexagesimales (DEG)
#define UNIT_GRAD_DEG               "grados sexagesimales (DEG)"

/// Acronimo de la unidad grados sexagesimales (DEG)
#define ACRONYM_GRAD_DEG            "deg"

/// Etiqueta de la unidad adimensional.
#define UNIT_ADIMENSIONAL           "adimensional"

/// Acronimo de la unidad adimensional.
#define ACRONYM_UNIT_ADIMENSIONAL   "adi"

/// Etiqueta de la unidad tanto por uno.
#define UNIT_PERCENTAGE_1           "tanto por uno"

/// Acronimo de la unidad tanto por uno.
#define ACRONYM_UNIT_PERCENTAGE_1   "tpu"

/// Etiqueta de la unidad tanto por cien.
#define UNIT_PERCENTAGE_100         "tanto por cien"

/// Acronimo de la unidad tanto por cien.
#define ACRONYM_UNIT_PERCENTAGE_100 "tpc"

/// Etiqueta de la unidad tanto por mil.
#define UNIT_PERCENTAGE_1000        "tanto por mil"

/// Acronimo de la unidad tanto por mil.
#define ACRONYM_UNIT_PERCENTAGE_1000 "tpm"

/// Etiqueta de la unidad segundos.
#define UNIT_SECONDS                "segundos"

/// Acronimo de la unidad segundos.
#define ACRONYM_UNIT_SECONDS        "seg"

/// Etiqueta de la unidad metros por segundo.
#define UNIT_METERS_PER_SECOND                "metros por segundo"

/// Acronimo de la unidad segundos.
#define ACRONYM_UNIT_METERS_PER_SECONDS        "m/s"

/// Etiqueta de la unidad kilometros por hora.
#define UNIT_KILOMETERS_PER_HOUR                "kilometros por hora"

/// Acronimo de la unidad segundos.
#define ACRONYM_UNIT_KILOMETERS_PER_HOUR        "km/h"

/// Etiqueta de la unidad kilometros por hora.
#define UNIT_KNOTS                "nudos"

/// Acronimo de la unidad segundos.
#define ACRONYM_KNOTS        "kn"

/// Etiqueta de la unidad hercios.
#define UNIT_HERTZ                           "Hercios (Hz)"

/// Acronimo de la unidad hercios.
#define ACRONYM_UNIT_HERTZ                   "Hz"

/// Etiqueta de la unidad kilohercios.
#define UNIT_KILOHERTZ                       "Kilohercios (KHz)"

/// Acronimo de la unidad kilohercios.
#define ACRONYM_UNIT_KILOHERTZ               "KHz"

/// Etiqueta de la unidad megahercios.
#define UNIT_MEGAHERTZ                       "Megahercios (MHz)"

/// Acronimo de la unidad megahercios.
#define ACRONYM_UNIT_MEGAHERTZ               "MHz"

#endif
