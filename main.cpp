/*
    TemplateCreator: create stimulus templates for HEKA Patchmaster

    Author: Christoph Kirst (ckirst@nld.ds.mpg.de)
    Date:   2012, LMU Munich
*/

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
