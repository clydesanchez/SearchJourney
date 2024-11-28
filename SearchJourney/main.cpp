#include "MainWidget.h"
#include <QtWidgets/QApplication>
//#include "ogrsf_frmts.h"

#include <QMetaType>
#include <qgscolorramp.h>
int main(int argc, char *argv[])
{
    //GDALAllRegister();
    //CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

    // 注册 QgsColorRamp* 类型到 Qt 的元对象系统
    qRegisterMetaType<QgsColorRamp*>("QgsColorRamp*");

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
