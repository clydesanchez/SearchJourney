/*
FileName: RasterCalculator.h
Author:JZH
Date:2024.11.28
Description:
    栅格计算器类的声明
*/

#ifndef RASTERCALCULATORTOOL_H
#define RASTERCALCULATORTOOL_H

#include "ui_RasterCalculatorTool.h"

#include <QDebug>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTemporaryFile>
#include <QTimer>
#include <QTreeWidget>

#include <QgsMapLayer.h>
#include <QgsProject.h>
#include <QgsRasterCalculator.h>
#include <QgsRasterFileWriter.h>
#include <QgsRasterLayer.h>
#include <QgsRasterPipe.h>

class RasterCalculatorTool : public QDialog
{
    Q_OBJECT

public:
    RasterCalculatorTool(QWidget* parent = nullptr);
    ~RasterCalculatorTool();

public:
    void setCalculationExpression(const QString& expression);// 修改计算表达式

private:
    Ui::RasterCalculatorToolClass ui;

// 成员变量声明
private:
    int mnSelectedBandNumber;                                // 存储当前选择的波段编号
    QString mqstrCalculationExpression;                      // 动态计算表达式
    QList<QgsMapLayer*> mqlAllCalcuRaster;                   // 存储所有栅格图层
    QList<QgsMapLayer*> mqlResultRaster;                     // 存储计算结果
    QList<QString> mqlAllRasterPaths;                        // 存储所有栅格图层的路径
    QgsRasterLayer* mqrlSelectedRasterLayer;                 // 存储当前选择的栅格图层
    QMap<QStandardItem*, QgsRasterLayer*> mqmItemToLayerMap; // 存储节点与栅格图层的映射

// 功能函数声明
private:
    //void initializeToolTree();                            // 初始化工具树           
    int addRasterLayer();                                   // 添加栅格图层
    int saveRasterResult();                                 // 保存结果
    int performRasterCalculation();                         // 执行栅格计算
    int analysisNDVI();                                     // 分析归一化差值植被指数NDVI
    int analysisMNDWI();                                    // 分析修正水体指数MNDWI
    int analysisNDBI();                                     // 分析非植被指数NDBI
    int analysisLST();                                      // 地表温度反演
    void onTreeViewDoubleClicked(const QModelIndex& index); // 树形视图双击事件

// 槽函数声明
private slots:
    void on_ctrlOpenRasterPushButton_clicked();  // 打开栅格
    void on_ctrlPlussPushButton_clicked();       // 加法
    void on_ctrlMinusPushButton_clicked();       // 减法
    void on_ctrlMultiplyPushButton_clicked();    // 乘法
    void on_ctrlDividePushButton_clicked();      // 除法
    void on_ctrlExpPushButton_clicked();         // 指数
    void on_ctrlLnPushButton_clicked();          // 自然对数
    void on_ctrlLog10PushButton_clicked();       // 10为底对数
    void on_ctrlLeftParenPushButton_clicked();   // 左括号
    void on_ctrlRightParenPushButton_clicked();  // 右括号
    void on_ctrlSetSavePathPushButton_clicked(); // 保存路径选择
    void on_ctrlRunPushButton_clicked();         // 执行计算
    void on_ctrlToolTreeWidget_doubleClicked(const QModelIndex& index); // 处理工具树双击事件
};

#endif // RASTERCALCULATORTOOL_H
