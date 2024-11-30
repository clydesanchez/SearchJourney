/*
FileName: MainWidget.h
Author:RYB
Date:2024.7.25
Description:
    主窗口类的声明
*/
#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtWidgets/QMainWindow>
#include "ui_MainWidget.h"
#include "GISMapCanvas.h"
#include "RasterCalculatorTool.h"
#include <QgsMultiPolygon.h>
#include <QgsLayerTreeView.h>
#include <QgsVertexMarker.h>
#include <QLabel>
#include "PointEdit.h"
#include "LineEdit.h"

#include "Buffer.h"
#include "Clip.h"
class MainWidget : public QMainWindow
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

private:
    Ui::MainWidgetClass ui;

private:
    QgsProject *mppjProject;               // 工程项目
    GISMapCanvas *mcanMapCanvas;           // 地图画布
    QList<QgsMapLayer *> mliLayersList;    // 所有图层
    QList<QgsMapLayer *> mliVisibleLayers; // 可见图层

    int mnActiveLayerIndex = -1; // 激活图层索引
    QList<QgsFeature> mpfSelectFeature; // 激活要素
    int mnSelectVertexIndex = -1; // 激活顶点索引
    bool mbDragging = false; // 是否拖拽
    QVector<QgsVertexMarker*> mvVertices ; // 顶点集合
    QgsPointXY mLastMousePos; // 上一次鼠标地图坐标

    QWidget* mFullWidget=nullptr; // 全屏窗口
    bool mbCanvasFullScreen = false; // 画布是否全屏
    QTimer* mTimer; // 定时器
    int mnProgressValue = 0; // 进度值
private:
    PointEdit* mpePointEdit = nullptr; // 点编辑工具
    QList<QgsFeature> mSelectedFeatures;  // 用于存储选中的要素

    Buffer* mpBuffer; // 缓冲区分析
    Clip* mpClip; // 裁剪分析
private:
    QLabel* mqlbStatusBarCRS; // 状态栏坐标系
public:
    QString mstrRampPath; // 色带路径
    QString mstrStylePath; // 样式路径

public:
    void updateLayerList(); // 更新图层列表
public:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    // 地图交互
    void onMapCanvasClicked(const QPoint& point);
    void onMapCanvasReleased(const QPoint& point);
    void onMapCanvasMoved(const QPoint& point);
signals:
    void updateProgressValue(int nValue); // 进度值改变

    // 控件交互
public slots:
    void on_ctrlOpenVectorAction_triggered();             // 添加矢量图层
    void on_ctrlOpenRasterAction_triggered();             // 添加栅格图层
    void on_ctrlOpenTextAction_triggered();               // 读取CSV文件转点
    void on_ctrlEditableAction_triggered();               // 编辑图层
    void on_ctrlKMeansAction_triggered();                 // KMeans聚类
    void on_ctrlConnectAction_triggered();                // 空间连接
    void on_ctrlRasterStatisticsAction_triggered();       // 栅格统计
    void on_ctrlSaveProjectAction_triggered();            // 保存工程
    void on_ctrlOpenProjectAction_triggered();            // 打开工程
    void on_ctrlSaveAsSHPAction_triggered();              // 保存为SHP
    void on_ctrlSaveAsTxtAction_triggered();              // 保存为TXT
    void on_ctrlLayerListViewAction_triggered();          // 图层列表视图
    void on_ctrlStatisticsViewAction_triggered();         // 统计视图
    void on_ctrlToolViewAction_triggered();               // 工具视图
    void on_ctrlCRSAction_triggered();                    // 设置坐标系
    void on_ctrlDeleteAction_triggered();                 // 删除图元
    void on_ctrlEditAttriAction_triggered();              // 编辑属性
    void on_ctrlMoveAction_triggered();                   // 平移图元
    void on_ctrlCopyAction_triggered();                   // 复制图元
    void on_ctrlVecToRasAction_triggered();               // 矢量转栅格
    void on_ctrlOpenRasterCalculatorActionV2_triggered(); // 栅格计算器
    void on_ctrlActionCanvasFullScreen_triggered();
    void on_ctrlAddPointAction_triggered();               // 添加点
    void on_ctrlUndoAction_triggered();                   // 撤销
    void on_ctrlRedoAction_triggered();                   // 重做
    void on_ctrlSaveEditAction_triggered();                   // 保存编辑
    void on_ctrlSmoothLineAction_triggered();             // 光滑线
    void on_ctrlThiningLineAction_triggered();            // 线抽稀
    void on_ctrlPolygonToLineAction_triggered();          // 面转线
    void on_ctrlActionRamp_triggered();                   // 栅格色带
    void on_ctrlActionImportStyle_triggered();            // 导入样式

    void onTreeItemClicked(QTreeWidgetItem *ptwiItem, int nColumn); // 点击工具栏事件
    void onChangeLayerVisible(QgsLayerTreeNode *pltnNode);          // 改变图层可见性

    void slotApplySymbol(QString strLayerName, QgsSymbol* psSymbol); // 设置图层样式
    void slotApplyMark(QString strLayer, QgsVectorLayerSimpleLabeling* pMark);

    void on_actionbuffer_triggered(); // 缓冲区分析
    void on_actionClip_triggered(); // 裁剪分析

    // 分析工具
public:
    void createKMeans();                                                                 // 构建聚类分析图层
    void connectByLocation();                                                            // 构建空间连接图层
    void rasterStatistics();                                                             // 栅格图层统计
    QList<int> MainWidget::calculateKMeans(QList<QgsPoint *> liPoints, int nClusterNum); // 计算KMeans
    QList<QgsPoint *> getQgsPoints(QgsVectorLayer *pvlLayer);                            // 获取点集
    QList<QgsPolylineXY *> getQgsPolylines(QgsVectorLayer *pvlLayer);                    // 获取线集
    QList<QgsMultiPolygonXY *> getQgsPolygons(QgsVectorLayer *pvlLayer);                 // 获取面集
    QList<QgsGeometry *> getQgsGeometries(QgsVectorLayer *pvlLayer);                     // 获取几何体集

    void colorFeaturesByField(QgsVectorLayer *pvlLayer, const QString &qstrFieldName,int bClusterNum=0);   // 根据字段上色
    void connectFiled(QgsVectorLayer *pvlLayerResult, QgsVectorLayer *pvlLayerCompare, int nResultIdx, int nCompareIdx); // 连接字段
    // 图层显示
public:
    void addVectorLayer(); // 添加矢量图层
    void addRasterLayer(); // 添加栅格图层
    void addTextLayer();   // 添加CSV文本图层

    void setLayerToMap(QgsMapLayer *pmlNewLayer);                   // 设置图层到地图
    void setLayerColor(QgsVectorLayer *pvlChoose, QColor qcChoose); // 改变图层颜色

    static QgsVectorLayer* TransformCRS_Vec(QgsVectorLayer* veclayer, int newCRScode); // 坐标转换 矢量
    static QgsRasterLayer* TransformCRS_Ras(QgsRasterLayer* raslayer, int newCRScode); // 坐标转换 栅格

    // 文件操作
public:
    void saveProjectAsQGZ();   // 保存工程为QGZ
    void openProjectFromQGZ(); // 从QGZ打开工程
    void saveAsSHP();          // 将矢量图层保存为shp文件
    void saveAsTxt();          // 将栅格统计结果保存为txt文件
    // ui控制
public:
    QDockWidget* getToolDock() const; // 获取工具栏

    // 图层操作
public:
    void deleteFeature(const QList<QgsFeature>& selectedFeatures);
    void editAttribute(const QList<QgsFeature>& selectedFeatures);
    void moveFeature(const QList<QgsFeature>& selectedFeatures);
    void copyFeature(const QList<QgsFeature>& selectedFeatures);
    void selectFeatures(const QList<QgsFeature>& selectedFeatures);
    void thiningLines(const QList<QgsFeature>& selectedFeatures);			// 抽稀线
    void smoothLines(const QList<QgsFeature>& selectedFeatures);			// 平滑线
};
#endif