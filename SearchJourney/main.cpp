#include "MainWidget.h"
#include <QtWidgets/QApplication>
//#include "ogrsf_frmts.h"
int main(int argc, char *argv[])
{
    //GDALAllRegister();
    //CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");
    if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    MainWidget w;
    w.show();
    return a.exec();
}
