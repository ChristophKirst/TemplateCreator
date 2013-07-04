#-------------------------------------------------
#
# Project created by QtCreator 2012-12-05T00:26:20
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TemplateCreator
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp \
    heka.cpp \
    fft.cpp \
    numerics.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h \
    fft.h \
    heka.h \
    numerics.h \
    debug.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    icon.png


unix:LIBS += -lgsl -lgslcblas
unix:LIBS += -lfftw3



#win32:DEPENDPATH += C:/Programing/gsl-1.8
#win32:DEPENDPATH += C:/Programing/gsl-1.8/lib
#win32:DEPENDPATH += C:/Programing/gsl-1.8/include
#win32:INCLUDEPATH += C:/Programing/gsl-1.8/include
#win32:LIBS += -LC:/Programing/gsl-1.8/lib
#win32:LIBS += -LC:/Programing/gsl-1.8/bin
#win32:LIBS += -llibgsl -llibgslcblas
#win32:LIBS += -lgsl -lgslcblas


#win32:DEPENDPATH += C:/Programing/gsl-1.15
#win32:DEPENDPATH += C:/Programing/gsl-1.15/lib
#win32:DEPENDPATH += C:/Programing/gsl-1.15/include
#win32:INCLUDEPATH += C:/Programing/gsl-1.15/include
#win32:LIBS += -LC:/Programing/gsl-1.15/lib
#win32:LIBS += -LC:/Programing/gsl-1.15/bin
#win32:LIBS += -llibgsl -llibgslcblas
#win32:LIBS += -lgsl -lgslcblas

#win32:DEPENDPATH += C:/Programing/fftw-3.3.3-dll32
#win32:INCLUDEPATH += C:/Programing/fftw-3.3.3-dll32
#win32:LIBS += -LC:/Programing/fftw-3.3.3-dll32
#win32:LIBS += -lfftw3-3 -lfftw3f-3 -lfftw3l-3
#win32:LIBS += -lfftw3

