#include "MainWidget.h"
#include "SelectFeatureTool.h"

void MainWidget::on_ctrlDeleteAction_triggered() {
    // 获取当前的矢量图层
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::deleteFeature);
    mcanMapCanvas->setMapTool(pSelectTool);
}

void MainWidget::on_ctrlEditAttriAction_triggered() {
    // 获取当前的矢量图层
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::editAttribute);
    mcanMapCanvas->setMapTool(pSelectTool);
}