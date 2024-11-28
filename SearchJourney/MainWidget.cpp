/*
FileName: MainWidget.cpp
Author:RYB
Date:2024.7.25
Description:
    主窗口类地图画布初始化、图层列表初始化、控件槽函数的实现
Function Lists:
    1. MainWidget(QWidget *parent = nullptr) 构造函数
    2. ~MainWidget() 析构函数
窗口口事件
    3. void paintEvent(QPaintEvent *event) 绘制地图
    4. void resizeEvent(QResizeEvent *event) 更新画布大小
    5. void mousePressEvent(QMouseEvent *event) 图层列表点击事件:鼠标穿透
槽函数
    6. void on_ctrlOpenVectorAction_triggered() 添加矢量图层
    7. void on_ctrlOpenRasterAction_triggered() 添加栅格图层
    8. void on_ctrlOpenTextAction_triggered() 读取CSV文件转点
    9. void on_ctrlKMeansAction_triggered() KMeans聚类
    10. void on_ctrlConnectAction_triggered() 空间连接
    11. void on_ctrlRasterStatisticsAction_triggered() 栅格统计
    12. void on_ctrlSaveProjectAction_triggered() 保存工程
    13. void on_ctrlOpenProjectAction_triggered() 打开工程
    14. void on_ctrlSaveAsSHPAction_triggered() 保存为SHP
    15. void on_ctrlSaveAsTxtAction_triggered() 保存为TXT
    16. void on_ctrlLayerListViewAction_triggered() 图层列表视图
    17. void on_ctrlStatisticsViewAction_triggered() 统计视图
    18. void on_ctrlToolViewAction_triggered() 工具视图
    19. void onTreeItemClicked(QTreeWidgetItem *item, int column) 点击工具栏事件
    20. void updateLayerList() 更新图层列表
    21. void onChangeLayerVisible(QgsLayerTreeNode *pltnNode) 改变图层可见性
*/

#include "MainWidget.h"
#include <QMouseEvent>
#include <QPixMap>
#include <QPushButton>
#include <QgsLayerTreeModel.h>
#include <QgsLayerTreeView.h>
#include <QgsLayerTreeNode.h>
#include <QgsLayerTree.h>
#include <QMessageBox>
#include "LayerItemMenu.h"
#include <QGis.h>
#include <qgsapplication.h>
#include <gdal.h>
#include <QSqlDatabase>
#include <qgsstylemanagerdialog.h>
#include <QgsFeatureIterator.h>
#include <Qgsvectorlayer.h>
MainWidget::MainWidget(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    // 初始化工程项目
    mppjProject = new QgsProject();
    // 初始化地图画布
    mcanMapCanvas = ui.graphicsView;
    mcanMapCanvas->setStatusBar(ui.statusBar);
    mcanMapCanvas->resize(ui.ctrlMapView->size());
    mcanMapCanvas->setCanvasColor(QColor(255, 255, 255));
    mcanMapCanvas->setVisible(true);
    mcanMapCanvas->enableAntiAliasing(true);
    connect(mcanMapCanvas, &GISMapCanvas::pressed, this, &MainWidget::onMapCanvasClicked);
    connect(mcanMapCanvas, &GISMapCanvas::released, this, &MainWidget::onMapCanvasReleased);
    connect(mcanMapCanvas, &GISMapCanvas::moved, this, &MainWidget::onMapCanvasMoved);
    // 图层列表
    QgsLayerTreeModel *pqltLayerTreeModel = new QgsLayerTreeModel(QgsProject::instance()->layerTreeRoot(), this);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeRename);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeReorder);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::ShowLegendAsTree);
    pqltLayerTreeModel->setAutoCollapseLegendNodes(10);
    ui.ctrlLayerTreeView->setModel(pqltLayerTreeModel);
    //ui.ctrlLayerTreeView->setMenuProvider(new LayerItemMenu(ui.ctrlLayerTreeView,mcanMapCanvas));
    // listview禁用编辑
    ui.ctrlStatisticsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 重命名窗口标题
    this->setWindowTitle("OOP_GIS_System");
    //设置编辑控件可用性
    mGeometryEditTool = new GeometryEditTool(mcanMapCanvas);
	ui.ctrlAddPointAction->setEnabled(false);
	ui.ctrlUndoAction->setEnabled(false);
	ui.ctrlRedoAction->setEnabled(false);
    ui.ctrlCutLineAction->setEnabled(false);
    ui.ctrlSmoothLineAction->setEnabled(false);
}

MainWidget::~MainWidget()
{
}
// 绘制地图
void MainWidget::paintEvent(QPaintEvent *event)
{
    // 基类函数
    QMainWindow::paintEvent(event);
}
// 更新画布大小
void MainWidget::resizeEvent(QResizeEvent *event)
{
    // 基类函数
    QMainWindow::resizeEvent(event);
    mcanMapCanvas->resize(ui.ctrlMapView->size());
    mcanMapCanvas->refresh();
}
// 图层列表点击事件:鼠标穿透
void MainWidget::mousePressEvent(QMouseEvent *event)
{
    // 基类函数
    QMainWindow::mousePressEvent(event);
}
// 添加矢量图层
void MainWidget::on_ctrlOpenVectorAction_triggered()
{
    addVectorLayer();
}
// 添加栅格图层
void MainWidget::on_ctrlOpenRasterAction_triggered()
{
    addRasterLayer();
}
// 读取CSV文件转点
void MainWidget::on_ctrlOpenTextAction_triggered()
{
    addTextLayer();
}
// KMeans聚类
void MainWidget::on_ctrlKMeansAction_triggered()
{
    createKMeans();
}
// 空间连接
void MainWidget::on_ctrlConnectAction_triggered()
{
    connectByLocation();
}
// 栅格统计
void MainWidget::on_ctrlRasterStatisticsAction_triggered()
{
    rasterStatistics();
}
// 保存工程
void MainWidget::on_ctrlOpenProjectAction_triggered()
{
    openProjectFromQGZ();
}
// 打开工程
void MainWidget::on_ctrlSaveProjectAction_triggered()
{
    saveProjectAsQGZ();
}
// 保存为SHP
void MainWidget::on_ctrlSaveAsSHPAction_triggered()
{
    saveAsSHP();
}
// 保存为TXT
void MainWidget::on_ctrlSaveAsTxtAction_triggered()
{
    saveAsTxt();
}
// 图层列表视图
void MainWidget::on_ctrlLayerListViewAction_triggered()
{
    ui.ctrlLayerListDock->setVisible(!ui.ctrlLayerListDock->isVisible());
}
// 统计视图
void MainWidget::on_ctrlStatisticsViewAction_triggered()
{
    ui.ctrlStatisticsDock->setVisible(!ui.ctrlStatisticsDock->isVisible());
}
// 工具视图
void MainWidget::on_ctrlToolViewAction_triggered()
{
    ui.ctrlToolDock->setVisible(!ui.ctrlToolDock->isVisible());
}
// 点击工具栏
void MainWidget::onTreeItemClicked(QTreeWidgetItem *item, int column)
{
    // 获取工具名
    QString qstrLayerName = item->text(column);
    qDebug() << "分析工具：" << qstrLayerName;

    // 根据工具名执行相应操作
    if (qstrLayerName == "K-Means聚类")
    {
        createKMeans();
    }
    else if (qstrLayerName == "按空间连接")
    {
        connectByLocation();
    }
    else if (qstrLayerName == "栅格统计")
    {
        rasterStatistics();
    }
    else if (qstrLayerName == "保存统计结果")
    {
        saveAsTxt();
    }
    else if (qstrLayerName == "保存为SHP")
    {
        saveAsSHP();
    }
}
// 更新图层列表
void MainWidget::updateLayerList()
{
    delete ui.ctrlLayerTreeView->model();
    QgsLayerTree* pqltLayerTreeRoot = mppjProject->layerTreeRoot();
    QgsLayerTreeModel* pqltLayerTreeModel = new QgsLayerTreeModel(pqltLayerTreeRoot, this);
    connect(pqltLayerTreeRoot, &QgsLayerTree::visibilityChanged, this, &MainWidget::onChangeLayerVisible);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeRename);
    //pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeReorder);  // TODO: 活动改变图层顺序
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::ShowLegendAsTree);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::UseEmbeddedWidgets);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::UseTextFormatting);
    pqltLayerTreeModel->setAutoCollapseLegendNodes(10);
    ui.ctrlLayerTreeView->setModel(pqltLayerTreeModel); // 更新列表
    ui.ctrlLayerTreeView->setMenuProvider(new LayerItemMenu(ui.ctrlLayerTreeView, mcanMapCanvas,this,mppjProject));// 连接右键菜单
}
// 改变图层可见性
void MainWidget::onChangeLayerVisible(QgsLayerTreeNode *pltnNode)
{
    mliVisibleLayers.clear();

    QgsLayerTreeModel *model = ui.ctrlLayerTreeView->layerTreeModel();
    QgsLayerTreeNode *rootNode = model->rootGroup();

    // 遍历所有子节点
    for (int i = 0; i < rootNode->children().size(); ++i)
    {
        QgsLayerTreeNode *node = rootNode->children().at(i);
        if (node->nodeType() == QgsLayerTreeNode::NodeLayer)
        {
            QgsLayerTreeLayer *layerNode = static_cast<QgsLayerTreeLayer *>(node);
            QgsMapLayer *mapLayer = layerNode->layer();

            // 获取复选框状态
            bool isVisible = layerNode->isVisible();
            if (isVisible)
            {
                mliVisibleLayers.append(mapLayer);
            }
        }
    }

    // 让画布只显示可见图层
    mcanMapCanvas->destroyed();
    mcanMapCanvas->setLayers(mliVisibleLayers);
    mcanMapCanvas->refresh();
}