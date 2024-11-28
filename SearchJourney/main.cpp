#include "MainWidget.h"
#include <QtWidgets/QApplication>
//#include "ogrsf_frmts.h"
int main(int argc, char *argv[])
{
    //GDALAllRegister();
    //CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
    QApplication a(argc, argv);

    QFile qssFile("./qss/MacOS.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        a.setStyleSheet(qssFile.readAll());
    }
    qssFile.close();

    MainWidget w;
    w.show();

    return a.exec();
}
