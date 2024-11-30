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
    // 加载样式表
    QFile qssFile("./qss/BrightTheme.qss");
    if (qssFile.open(QFile::ReadOnly)) {
        a.setStyleSheet(qssFile.readAll());
    }
    qssFile.close();
    // 加载字体文件
    //int fontId = QFontDatabase::addApplicationFont("./font/zhankukuaileti2016_xiudingban.ttf");
    int fontId = QFontDatabase::addApplicationFont("./font/SourceHanSansCN-Medium.ttf");
    if (fontId != -1) {
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont appFont(family);
        appFont.setPointSize(9); // 设置应用程序默认字体大小
        a.setFont(appFont);
    }
    else {
        qWarning("字体加载失败,检查字体文件路径！");
    }

    // 单独设置 QToolBar 的字体大小
    //QFont toolbarFont = a.font();
    //toolbarFont.setPointSize(); // 设置字体大小为 7
    //a.setFont(toolbarFont, "QToolBar"); // 应用到 QToolBar

    MainWidget w;
    w.show();

    return a.exec();
}
