#-------------------------------------------------
#
# Project created by QtCreator 2016-07-11T13:10:55
#
#-------------------------------------------------
debug_and_release {
    CONFIG -= debug_and_release
    CONFIG += debug_and_release
}

CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
    }
CONFIG(release, debug|release) {
        CONFIG -= debug release
        CONFIG += release
}

QT += widgets

QT       += xml
QT       += sql
QT       += gui

TARGET = libRemoteSensing
TEMPLATE = lib

DEFINES += LIBREMOTESENSING_LIBRARY

SOURCES += \
    Scene.cpp \
    ScenesManager.cpp \
    SceneLandsat8.cpp \
    SceneSentinel2.cpp \
    PersistenceManager.cpp \
    Algorithms.cpp \
    Parameter.cpp \
    ParametersManager.cpp \
    ParametersManagerDialog.cpp \
    TONIpbpProject.cpp \
    ClassificationProject.cpp

HEADERS +=\
        libremotesensing_global.h \
    remotesensing_definitions.h \
    Scene.h \
    ScenesManager.h \
    SceneLandsat8.h \
    SceneLandsat8_definitions.h \
    SceneSentinel2_definitions.h \
    SceneSentinel2.h \
    PersistenceManager.h \
    persistencemanager_definitions.h \
    Algorithms.h \
    Parameter.h \
    ParameterDefinitions.h \
    ParametersManager.h \
    ParametersManagerDialog.h \
    algorithms_definitions.h \
    TONIpbpProject.h \
    ClassificationProject.h

DESTDIR_RELEASE= ./../../../build/release
DESTDIR_DEBUG= ./../../../build/debug
#OSGEO4W_PATH="C:\Program Files\QGIS 2.18"
OSGEO4W_PATH="C:\Program Files\QGIS 3.10"
EIGEN_PATH= ./../../../depends/eigen-eigen-323c052e1731

debug{
    DESTDIR = $$DESTDIR_DEBUG
    LIBS += -L$$DESTDIR_DEBUG
}else{
    DESTDIR = $$DESTDIR_RELEASE
    LIBS += -L$$DESTDIR_RELEASE
}

INCLUDEPATH += $$EIGEN_PATH
#INCLUDEPATH += ../libPW
INCLUDEPATH += ../libProcessTools
INCLUDEPATH += ../libCRS
INCLUDEPATH += ../libIGDAL
INCLUDEPATH += ../libNestedGrid
INCLUDEPATH += ../ # por Eigen
INCLUDEPATH += ../libSplines

INCLUDEPATH += . $$OSGEO4W_PATH/include
LIBS += -L$$OSGEO4W_PATH/bin
#LIBS += $$OSGEO4W_PATH/lib/gsl.lib
LIBS += $$OSGEO4W_PATH/lib/proj_i.lib
LIBS += $$OSGEO4W_PATH/lib/gdal_i.lib
LIBS += $$OSGEO4W_PATH/lib/geos_c.lib

#LIBS += -lpw
LIBS += -llibProcessTools
LIBS += -llibCRS
LIBS += -llibIGDAL
LIBS += -llibNestedGrid
LIBS += -llibSplines

#debug{
#    win32-*{

#        !contains(QMAKE_TARGET.arch, x86_64) {
#            OSGEO4W_PATH = E:/OSGeo4W_x32
#            DESTDIR = build/debug
#        } else {
#            OSGEO4W_PATH = E:/OSGEO4W_64
#            LIBS += -L$$PWD/../../libs/libCRS/build64/debug #\
#            LIBS += -L$$PWD/../../libs/libIGDAL/build64/debug #\
#            LIBS += -L$$PWD/../../libs/libNestedGrid/build64/debug #\
#            DESTDIR = build64/debug
#        }
#    }

#}else{
#    win32-*{
#        !contains(QMAKE_TARGET.arch, x86_64) {
#            OSGEO4W_PATH = E:/OSGeo4W_x32
#            DESTDIR = build/release
#        } else {
#            OSGEO4W_PATH = E:/OSGEO4W_64
#            DESTDIR = $$DESTDIR_RELEASE
##            DESTDIR = build64/release
#        }
#    }
#}

#unix {
#    target.path = /usr/lib
#    INSTALLS += target
#}
