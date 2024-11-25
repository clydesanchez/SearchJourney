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
    QAction* actionZoomToLayer(QgsMapCanvas* canvas, QMenu* menu);
    QAction* actionRemoveLayer(QString strLayerName);
    QAction* actionSymbolManger(QString strLayerName,Qgis::GeometryType layerType, QgsSymbolList Srcsymbol);
    QAction* actionShowProperties(QString strLayerName, QgsVectorLayer* veclayer);
    QAction* actionCrsTransform_vec(QString strLayerName, QgsVectorLayer* veclayer);
    QAction* actionCrsTransform_ras(QString strLayerName, QgsRasterLayer* rasLayer);
private:
    QgsMapCanvas* mcanMapCanvas;
    QgsLayerTreeView* mctrlLayerItem;
    MainWidget* mwidMain;
    QgsProject* mprjProject;
};

#endif