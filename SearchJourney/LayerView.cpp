/*
FileName: LayerView.cpp
Author:RYB
Date:2024.7.25
Description:
    地图视图的显示功能
Function Lists:
    1. addVectorLayer() 添加矢量图层
    2. addRasterLayer() 添加栅格图层
    3. addTextLayer() 添加CSV文本图层
    4. setLayerToMap(QgsMapLayer *pmlNewLayer) 设置图层到地图
    5. setLayerColor(QgsVectorLayer *pmlChoose, QColor qcChoose) 修改图层颜色
*/

#include "MainWidget.h"
#include "SelectDialog.h"
#include "SymbolManger.h"
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h>
#include <QFileDialog>
#include <QMessageBox>
#include <Qgssymbol.h>
#include <Qgssinglesymbolrenderer.h>
#include <qgsmarkersymbol.h>
#include <qgsmarkersymbollayer.h>
#include <qgsfillsymbol.h>
#include <qgsfillsymbollayer.h>
#include <QgsSymbolLayerRegistry.h>

#include <qgslabeling.h>
// 添加矢量图层
void MainWidget::addVectorLayer()
{
    QString qstrFileName = QFileDialog::getOpenFileName(this, tr("打开shp文件"), "", "*.shp");
    QStringList _qstrSplit = qstrFileName.split('/');
    QString qstrBasename = _qstrSplit.at(_qstrSplit.size() - 1);
    QgsVectorLayer *qvlVectorLayer = new QgsVectorLayer(qstrFileName, qstrBasename, "ogr");

    Qgis::GeometryType layerType= qvlVectorLayer->geometryType();
    QgsSymbol* symbol = QgsSymbol::defaultSymbol(layerType);

    // 将符号应用到矢量图层的渲染器中
    QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(symbol);
    qvlVectorLayer->setRenderer(renderer);

    if (!qvlVectorLayer->isValid())
    {
        QMessageBox::critical(this, "error", QString("导入矢量图层失败： \n") + qstrFileName);
        return;
    }
    // 添加到当前qgz工程
    mppjProject->addMapLayer(qvlVectorLayer);
    setLayerToMap(static_cast<QgsMapLayer *>(qvlVectorLayer));
}
// 添加栅格图层
void MainWidget::addRasterLayer()
{
    QString qstrFileName = QFileDialog::getOpenFileName(this, tr("打开tif文件"), "", "*.tif");
    QStringList _qstrSplit = qstrFileName.split('/');
    QString qstrBasename = _qstrSplit.at(_qstrSplit.size() - 1);
    QgsRasterLayer *qvlRasterLayer = new QgsRasterLayer(qstrFileName, qstrBasename);
    if (!qvlRasterLayer->isValid())
    {
        QMessageBox::critical(this, "error", QString("导入栅格图层失败： \n") + qstrFileName);
        return;
    }
    // 添加图层透明度
    //qvlRasterLayer->setOpacity(0.5);
    // 添加到当前qgz工程
    mppjProject->addMapLayer(qvlRasterLayer);
    setLayerToMap(static_cast<QgsMapLayer *>(qvlRasterLayer));
}
// 添加CSV文本图层
void MainWidget::addTextLayer()
{
    // 打开CSV文件
    QString qstrFileName = QFileDialog::getOpenFileName(this, tr("打开csv文件"), "", "*.csv");
    QStringList _qstrSplit = qstrFileName.split('/');
    QString qstrBasename = _qstrSplit.at(_qstrSplit.size() - 1);
    QgsVectorLayer *pvlTextLayer = new QgsVectorLayer(qstrFileName, qstrBasename, "ogr");
    if (!pvlTextLayer->isValid())
    {
        QMessageBox::critical(this, "error", QString("文件打开失败: \n") + qstrFileName);
        return;
    }
    TextSelect *ctrlChooseWidget = new TextSelect();

    // 将字段名添加到comboBox中
    ctrlChooseWidget->addItems(pvlTextLayer);
    // 暂停当前窗口进程
    ctrlChooseWidget->exec();
    // 点击确认按钮后，以ctrlXComboBox字段为经度，ctrlYComboBox字段为纬度，添加点图层
    QString qstrNameX = ctrlChooseWidget->getNameX();
    QString qstrNameY = ctrlChooseWidget->getNameY();
    QString qstrNameCRS = ctrlChooseWidget->getNameCRS();

    // 创建一个内存图层
    QString qstrLayerUri = "Point?crs=epsg:4326:integer&index=yes";
    QgsVectorLayer *pvlMemoryLayer = new QgsVectorLayer(qstrLayerUri, qstrBasename, "memory");
    // 创建一个要素列表
    QgsFeatureList featureList;
    // 获取要素迭代器
    QgsFeatureIterator iter = pvlTextLayer->getFeatures();
    // 从pvlTextLayer中获取字段值
    QgsFields fields = pvlTextLayer->dataProvider()->fields();
    //// 添加字段到pvlMemoryLayer
    pvlMemoryLayer->startEditing();
    pvlMemoryLayer->dataProvider()->addAttributes(fields.toList());
    pvlMemoryLayer->commitChanges();
    // 读取csv文件中的经纬度字段
    QgsFeature feature;
    while (iter.nextFeature(feature))
    {
        // 读取经纬度数据
        double x = feature.attribute(qstrNameX).toDouble();
        double y = feature.attribute(qstrNameY).toDouble();

        // 创建一个点对象
        QgsPoint point(x, y);
        // 创建一个要素
        QgsFeature pointFeature;
        pointFeature.setGeometry(QgsGeometry::fromPoint(point));
        // 将字段值添加到要素中
        pointFeature.setAttributes(feature.attributes());
        //qDebug() << feature.attributes();
        // 将要素添加到要素列表中
        featureList.append(pointFeature);
    }
    // 将要素列表添加到图层
    pvlMemoryLayer->dataProvider()->addFeatures(featureList);
    pvlMemoryLayer->updateExtents();
    // 添加到当前qgz工程
     mppjProject->addMapLayer(pvlMemoryLayer);
    // 将图层添加到地图画布
    setLayerToMap(static_cast<QgsMapLayer *>(pvlMemoryLayer));
}
// 设置图层到地图
void MainWidget::setLayerToMap(QgsMapLayer *pmlNewLayer)
{
    mcanMapCanvas->setExtent(pmlNewLayer->extent());
    mliLayersList.append(pmlNewLayer);
    mliVisibleLayers.append(pmlNewLayer);
    //mcanMapCanvas->setLayers(mliVisibleLayers);
    // 获取工程中所有图层
    QList<QgsMapLayer *> layers = mppjProject->mapLayers().values();
    mcanMapCanvas->setLayers(layers);
    mcanMapCanvas->refresh();
    mppjProject->addMapLayer(pmlNewLayer);
    updateLayerList();
}
// 修改图层颜色
void MainWidget::setLayerColor(QgsVectorLayer *pmlChoose, QColor qcChoose)
{
    // 设置矢量要素的颜色
    QgsSymbol *symbol = QgsSymbol::defaultSymbol(pmlChoose->geometryType());
    if (symbol)
    {
        symbol->setColor(qcChoose);
        QgsSingleSymbolRenderer *renderer = new QgsSingleSymbolRenderer(symbol);
        pmlChoose->setRenderer(renderer);
    }
}

// 设置图层样式
void MainWidget::slotApplySymbol(QString strLayer, QgsSymbol *psSymbol)
{
    // 根据名称获取图层
    QgsVectorLayer *pvlLayer = nullptr;
    QList<QgsMapLayer *> layers = mppjProject->mapLayersByName(strLayer);
    if(layers.size() > 0)
	{
		pvlLayer = dynamic_cast<QgsVectorLayer *>(layers.at(0));
	}
    if(!pvlLayer)
	{
        QMessageBox::critical(this, "error", QString("图层不存在: \n") + strLayer);
		return;
	}
	// 创建符号渲染器
	QgsSingleSymbolRenderer *renderer = new QgsSingleSymbolRenderer(psSymbol);
	pvlLayer->setRenderer(renderer);
	pvlLayer->triggerRepaint();
}