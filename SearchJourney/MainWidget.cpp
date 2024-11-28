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
    19. void on_ctrlDeleteAction_triggered(); 删除图元
    20. void on_ctrlEditAttriAction_triggered(); 编辑属性
    21. void on_ctrlMoveAction_triggered(); 平移图元
    22. void onTreeItemClicked(QTreeWidgetItem *item, int column) 点击工具栏事件
    23. void updateLayerList() 更新图层列表
    24. void onChangeLayerVisible(QgsLayerTreeNode *pltnNode) 改变图层可见性
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
    this->resize(1940, 1230);
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
    // 编辑工具栏禁用
    ui.ctrlDeleteAction->setEnabled(false);
    ui.ctrlEditAttriAction->setEnabled(false);
    ui.ctrlMoveAction->setEnabled(false);
    // 重命名窗口标题
    this->setWindowTitle("OOP_GIS_System");
}

MainWidget::~MainWidget()
{
    if (mpBuffer)
    {
        delete mpBuffer;
        mpBuffer = nullptr;
    }
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
void MainWidget::on_actionbuffer_triggered() {
    mpBuffer = new Buffer(this, mcanMapCanvas, mppjProject);
    mpBuffer->setMapLayers(mliLayersList);
    mpBuffer->show();
}

void MainWidget::on_actionClip_triggered() {
    mpClip = new Clip(this, mcanMapCanvas, mppjProject);
    mpClip->setMapLayers(mliLayersList);
    mpClip->show();
}

// 更新图层列表
void MainWidget::updateLayerList()
{
    delete ui.ctrlLayerTreeView->model();
    QgsLayerTree* pqltLayerTreeRoot = mppjProject->layerTreeRoot();
    QgsLayerTreeModel* pqltLayerTreeModel = new QgsLayerTreeModel(pqltLayerTreeRoot, this);
    connect(pqltLayerTreeRoot, &QgsLayerTree::visibilityChanged, this, &MainWidget::onChangeLayerVisible);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeRename);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeReorder);  // TODO: 活动改变图层顺序
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::ShowLegendAsTree);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::UseEmbeddedWidgets);
    pqltLayerTreeModel->setFlag(QgsLayerTreeModel::UseTextFormatting);
    pqltLayerTreeModel->setAutoCollapseLegendNodes(10);
    ui.ctrlLayerTreeView->setModel(pqltLayerTreeModel); // 更新列表
    ui.ctrlLayerTreeView->setMenuProvider(new LayerItemMenu(ui.ctrlLayerTreeView, mcanMapCanvas,this,mppjProject));// 连接右键菜单
    // 重置图层顺序
    ui.ctrlLayerTreeView->setDragDropMode(QAbstractItemView::InternalMove);
    ui.ctrlLayerTreeView->setDefaultDropAction(Qt::MoveAction);
    ui.ctrlLayerTreeView->setAcceptDrops(true);
    ui.ctrlLayerTreeView->setDropIndicatorShown(true);
    ui.ctrlLayerTreeView->setDragEnabled(true);
    mppjProject->layerTreeRoot()->reorderGroupLayers(pqltLayerTreeRoot->layerOrder());
    mcanMapCanvas->setLayers(pqltLayerTreeRoot->layerOrder());
    // 查看
    mcanMapCanvas->refresh();
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
// 返回工具栏
QDockWidget *MainWidget::getToolDock() const
{
	return ui.ctrlToolDock;
}
// 全屏显示
void MainWidget::on_ctrlActionCanvasFullScreen_triggered()
{
    QGridLayout* layout = new QGridLayout;
    GISMapCanvas* Canvas = new GISMapCanvas(mcanMapCanvas);
    qDebug() << Canvas->extent().center().toQPointF() <<mcanMapCanvas->extent().center().toQPointF();
    if(mFullWidget==nullptr)
	{
		mFullWidget = new QWidget();
	}
    else {
        delete mFullWidget;
        mFullWidget = new QWidget();
    }
	if (!mbCanvasFullScreen)
    {
        // 将画布复制到新窗口
        layout->setSpacing(0);
        layout->setMargin(0);
        layout->addWidget(Canvas);
        mFullWidget->setLayout(layout);
        mFullWidget->showFullScreen();
        // 设置焦点
        Canvas->setFocus();
        QAction *action = ui.ctrlActionCanvasFullScreen;
        mFullWidget->addAction(action);

        mbCanvasFullScreen = true;
	}
	else
	{
        mFullWidget->close();
        layout->removeWidget(Canvas);
        delete Canvas;
        delete layout;
        mFullWidget->close();
        mbCanvasFullScreen = false;
	}
}
// 自定义栅格计算器V2
void MainWidget::on_ctrlOpenRasterCalculatorActionV2_triggered()
{
    // 创建 RasterCalculatorTool 对话框实例
    RasterCalculatorTool* rasterCalculatorDialog = new RasterCalculatorTool(this);

    // 显示对话框并等待用户交互
    if (rasterCalculatorDialog->exec() == QDialog::Accepted)
    {
        // 用户点击“确定”按钮后，可以在这里处理结果
        qDebug() << "Raster Calculator executed successfully.";
    }
    else
    {
        // 用户点击“取消”按钮后，可以在这里处理逻辑
        qDebug() << "Raster Calculator canceled.";
    }

    // 释放内存
    delete rasterCalculatorDialog;
}