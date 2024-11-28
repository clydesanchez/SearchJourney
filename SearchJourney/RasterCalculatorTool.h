#ifndef RASTERCALCULATORTOOL_H
#define RASTERCALCULATORTOOL_H

#include "ui_RasterCalculatorTool.h"

#include <QDialog>
#include <QgsRasterLayer.h>
#include <QFileDialog>
#include <QgsRasterCalculator.h>
#include <QgsProject.h>
#include <QMessageBox>
#include <QPushButton>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDebug>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFileDialog>
#include <QDir>
#include <QList>

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
    QString m_calculationExpression;                         // 动态计算表达式
    QList<QgsMapLayer*> mqlAllCalcuRaster;                   // 存储所有栅格图层
    QList<QgsMapLayer*> mqlResultRaster;                     // 存储计算结果
    QList<QString> mqlAllRasterPaths;                        // 存储所有栅格图层的路径
    QgsRasterLayer* m_selectedRasterLayer;                   // 存储当前选择的栅格图层
    int m_selectedBandNumber;                                // 存储当前选择的波段编号
    QMap<QStandardItem*, QgsRasterLayer*> m_itemToLayerMap;  // 存储节点与栅格图层的映射



// 功能函数声明
private:
    int addRasterLayer();                                   // 添加栅格图层
    int saveRasterResult();                                 // 保存结果
    int performRasterCalculation();                         // 执行栅格计算
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
};

#endif // RASTERCALCULATORTOOL_H
