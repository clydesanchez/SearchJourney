#pragma once
#ifndef LAYERITEMMENU_H
#define LAYERITEMMENU_H

#include <qgslayertreeview.h>
#include <qgsmapcanvas.h>
#include "MainWidget.h"
class LayerItemMenu :
    public QgsLayerTreeViewMenuProvider
{
public:
    LayerItemMenu(QgsLayerTreeView*view, QgsMapCanvas *canvas,MainWidget* widMain,QgsProject* prjSrc);
    ~LayerItemMenu() = default;

    QMenu *createContextMenu();
    QAction* actionZoomToLayer(QgsMapCanvas* canvas, QMenu* menu);//缩放到图层
    QAction* actionRemoveLayer(QString strLayerName);//移除图层 BUG:再次添加图层时会崩溃
    QAction* actionSymbolManger(QString strLayerName,Qgis::GeometryType layerType, QgsSymbolList Srcsymbol);//符号管理
    QAction* actionLabelManger(QString strLayerName);//标注管理
    QAction* actionShowProperties(QString strLayerName, QgsVectorLayer* veclayer);//显示属性
    QAction* actionCrsTransform_vec(QString strLayerName, QgsVectorLayer* veclayer);//坐标转换 矢量
    QAction* actionCrsTransform_ras(QString strLayerName, QgsRasterLayer* rasLayer);// 坐标转换 栅格
private:
    QgsMapCanvas* mcanMapCanvas;
    QgsLayerTreeView* mctrlLayerItem;
    MainWidget* mwidMain;
    QgsProject* mprjProject;
};

#endif
