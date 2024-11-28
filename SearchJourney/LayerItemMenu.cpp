#include "LayerItemMenu.h"
#include <QMenu>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h>
#include <QgsLayertree.h>
#include <QgsLayerTreeViewDefaultActions.h>
#include <qgsattributetableview.h>
#include <qgsattributetablemodel.h>
#include <qgsattributetablefiltermodel.h>
#include <qgsvectorlayercache.h>
#include <qmessagebox.h>
#include <qgsstylemanagerdialog.h>
#include <qgssymbolselectordialog.h>
#include <qgssymbol.h>
#include "SymbolManger.h"
#include "CustomAttributeTableView.h"
#include <QgsAttributeTableModel.h>
#include "AttributeViewWidget.h"
#include <qgsfeature.h>
#include <qgsattributedialog.h>
#include <qgsattributeeditorcontext.h>
#include <QInputDialog>
// 栅格样式新增
#include"RasterStyle.h"
#include "StyleManager.h"
#include <qgsrasterdataprovider.h>
#include <QMessageBox>
#include <qgssinglebandpseudocolorrenderer.h>

LayerItemMenu::LayerItemMenu(QgsLayerTreeView*view, QgsMapCanvas *canvas, MainWidget* widMain,QgsProject* prjSrc)
	: QgsLayerTreeViewMenuProvider()
{
	mcanMapCanvas = canvas;
	mctrlLayerItem = new QgsLayerTreeView(view);
	mctrlLayerItem = view;
	mwidMain = widMain;
	mprjProject = prjSrc;
}

//LayerItemMenu::~LayerItemMenu()
//{
//	delete mctrlLayerItem;
//}

QMenu* LayerItemMenu::createContextMenu()
{
	QMenu* menu = new QMenu(mctrlLayerItem);
	QModelIndex nIndex = mctrlLayerItem->currentIndex();
	QgsLayerTreeNode* node = mctrlLayerItem->index2node(nIndex);


	if (QgsLayerTree::isLayer(node)) {
		QgsMapLayer* layer = QgsLayerTree::toLayer(node)->layer();
		if (layer&& layer->type() == Qgis::LayerType::Vector) {
			QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
			Qgis::GeometryType layerType = vectorLayer->geometryType();


			// 创建一个 QgsMapSettings 对象
			QgsMapSettings mapSettings;
			mapSettings.setLayers(QList<QgsMapLayer*>() << vectorLayer); // 设置图层
			QgsRenderContext renderContext = QgsRenderContext::fromMapSettings(mapSettings);
			// 获取当前图层的符号
			QgsSymbolList symbollist = vectorLayer->renderer()->symbols(renderContext);

			menu->addAction(actionZoomToLayer(mcanMapCanvas,menu));//缩放到图层
			menu->addAction(actionRemoveLayer(layer->name()));	//移除图层
			menu->addAction(actionSymbolManger(layer->name(), layerType,symbollist));	//符号管理
			menu->addAction(actionShowProperties(layer->name(),vectorLayer));	//属性表
			menu->addAction(actionCrsTransform_vec(layer->name(), vectorLayer));	//坐标转换
            menu->addAction(actionStyleManager(layer->name(), layerType));		//符号库
			//menu->addAction(mctrlLayerItem->defaultActions()->actionZoomToLayer(mcanMapCanvas, menu));//缩放到图层
			//menu->addAction(mctrlLayerItem->defaultActions()->actionRemoveGroupOrLayer(menu));	//删除
			//menu->addAction(mctrlLayerItem->defaultActions()->actionRenameGroupOrLayer(menu)); //重命名
		}
		else if (layer) {
			QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer*>(layer);
			menu->addAction(actionZoomToLayer(mcanMapCanvas, menu));//缩放到图层
			menu->addAction(actionRemoveLayer(layer->name()));	//移除图层
			menu->addAction(actionSymbolManger_ras(layer->name(), rasterLayer));	//符号管理
			menu->addAction(actionCrsTransform_ras(layer->name(), rasterLayer));	//坐标转换
			//menu->addAction(actionStyleManager());		//符号库
			//menu->addAction(actionCrsTransform(layer->name(), vectorLayer));		

		}
	}
	return menu;
}
QAction* LayerItemMenu::actionZoomToLayer(QgsMapCanvas* canvas, QMenu* menu)
{
    QAction* action = new QAction("缩放到图层");
    QgsLayerTreeNode* node = mctrlLayerItem->index2node(mctrlLayerItem->currentIndex());
    QgsMapLayer* layer = QgsLayerTree::toLayer(node)->layer();
    QObject::connect(action, &QAction::triggered, [canvas, layer]() {
        QgsRectangle extent = layer->extent();
        canvas->setExtent(extent);
        canvas->refresh();
    });
    return action;
}

QAction* LayerItemMenu::actionRemoveLayer(QString strLayerName)
{
	QAction* action = new QAction("移除图层");
	QgsProject* prjSrc = mprjProject;
	QgsMapCanvas* canvas = mcanMapCanvas;
	QObject::connect(action, &QAction::triggered, [strLayerName, prjSrc,canvas]() {
		// 根据名称获取图层
		QgsVectorLayer* pvlLayer = nullptr;
		QgsRasterLayer* prlLayer = nullptr;
		QList<QgsMapLayer*> layers = prjSrc->mapLayersByName(strLayerName);
		if (layers.size() > 0)
		{
			pvlLayer = dynamic_cast<QgsVectorLayer*>(layers.at(0));
			prlLayer = dynamic_cast<QgsRasterLayer*>(layers.at(0));
		}
		if (!pvlLayer&&!prlLayer)
		{
			QMessageBox::critical(nullptr, "error", QString("图层不存在: \n") + strLayerName);
			return;
		}
		// 删除图层
		prjSrc->removeMapLayer(pvlLayer);
		prjSrc->removeMapLayer(prlLayer);
		// 更新画布
		canvas->refresh();
		});
	return action;
}

QAction* LayerItemMenu::actionSymbolManger(QString strLayerName, Qgis::GeometryType layerType, QgsSymbolList Srcsymbol)
{
	QAction* action = new QAction("符号管理");
	MainWidget* widMain = mwidMain;
	QObject::connect(action, &QAction::triggered, [strLayerName, layerType,widMain,Srcsymbol]() {
		// 弹出新的符号管理窗口
		SymbolManger* symbolManger = new SymbolManger(strLayerName,layerType,widMain,Srcsymbol);
		symbolManger->show();
		//QgsStyleManagerDialog* styleManager = new QgsStyleManagerDialog();
		//styleManager->show();
		});
	return action;
}
// 连接栅格图层符号管理
QAction* LayerItemMenu::actionSymbolManger_ras(QString strLayerName, QgsRasterLayer* rasLayer) {
	QAction* action = new QAction("符号管理");
	MainWidget* widMain = mwidMain;
	QObject::connect(action, &QAction::triggered, [strLayerName, rasLayer, widMain]() {
		// 弹出新的符号管理窗口
		RasterStyle* rasterSymbolManager = new RasterStyle(strLayerName, rasLayer, widMain);
		rasterSymbolManager->show();
		//QgsStyleManagerDialog* styleManager = new QgsStyleManagerDialog();
		//styleManager->show();
		});
	return action;
}

QAction* LayerItemMenu::actionShowProperties(QString strLayerName, QgsVectorLayer* vectorlayer)
{


	QAction* action = new QAction("属性表");
	QgsMapCanvas* canvas = mcanMapCanvas;
	QgsProject* prjSrc  = mprjProject;
	QObject::connect(action, &QAction::triggered, [strLayerName, prjSrc,canvas,vectorlayer]() {
		// 根据名称获取图层
		
		QgsVectorLayer* pvlLayer = nullptr;
		QList<QgsMapLayer*> layers = prjSrc->mapLayersByName(strLayerName);
		if (layers.size() > 0)
		{
			pvlLayer = dynamic_cast<QgsVectorLayer*>(layers.at(0));
		}
		if (!pvlLayer)
		{
			QMessageBox::critical(nullptr, "error", QString("图层不存在: \n") + strLayerName);
			return;
		}
		// 弹出新的属性表窗口
		if (pvlLayer) {
			// 设置属性表模型
			AttributeViewWidget* attriView = new AttributeViewWidget(pvlLayer,canvas);
			// 设置属性表窗口大小
			attriView->setWindowTitle(strLayerName + "属性表");
			attriView->show();
		}
		});
	return action;
}

void TransformCRS_Vec(QgsVectorLayer* veclayer, int newCRScode);
QAction* LayerItemMenu::actionCrsTransform_vec(QString strLayerName, QgsVectorLayer* veclayer)
{
	QAction* action = new QAction("坐标转换");
	QgsMapCanvas* canvas = mcanMapCanvas;
	QgsProject* prjSrc = mprjProject;
	QObject::connect(action, &QAction::triggered, [strLayerName, prjSrc, canvas, veclayer]() {
			int SrcCRScode = veclayer->crs().postgisSrid();
			int newCRScode = QInputDialog::getInt(nullptr, "坐标转换", "请输入新的坐标系代码", SrcCRScode);
			if(newCRScode == SrcCRScode)
			{
				return;
			}
			// 转换坐标系
			TransformCRS_Vec(veclayer, newCRScode);
			// 刷新画布
			veclayer->triggerRepaint();
			// 缩放到图层
			canvas->setExtent(veclayer->extent());
			canvas->refresh();
		});
	return action;
}

QgsRasterLayer* TransformCRS_Ras(QgsRasterLayer* raslayer, int newCRScode);
QAction* LayerItemMenu::actionCrsTransform_ras(QString strLayerName, QgsRasterLayer* rasLayer)
{
	QAction* action = new QAction("坐标转换");
	QgsMapCanvas* canvas = mcanMapCanvas;
	QgsProject* prjSrc = mprjProject;
	QObject::connect(action, &QAction::triggered, [strLayerName, prjSrc, canvas, rasLayer]() {
		int SrcCRScode = rasLayer->crs().postgisSrid();
		int newCRScode = QInputDialog::getInt(nullptr, "坐标转换", "请输入新的坐标系代码", SrcCRScode);
		if (newCRScode == SrcCRScode)
		{
			return;
		}
		// 转换坐标系
		QgsRasterLayer* resultLayer = TransformCRS_Ras(rasLayer, newCRScode);
		if(resultLayer==nullptr)
		{
			return;
		}
		// 添加到画布
		prjSrc->addMapLayer(resultLayer);
		resultLayer->triggerRepaint();
		// 刷新画布
		canvas->setExtent(resultLayer->extent());
		canvas->refresh();
		// 刷新画布
		//rasLayer->triggerRepaint();
		// 缩放到图层
		//canvas->setExtent(rasLayer->extent());
		//canvas->refresh();
		});
	return action;
}

QAction* LayerItemMenu::actionStyleManager(QString strLayerName, Qgis::GeometryType layerType) {
	QAction* action = new QAction("符号库");
	MainWidget* widMain = mwidMain;
	QObject::connect(action, &QAction::triggered, [strLayerName, layerType, widMain]() {
		// 弹出新的符号库窗口
		StyleManager* styleManager = new StyleManager(strLayerName, layerType, widMain);
		styleManager->show();
		});
	return action;
}



#include <QgsCoordinateTransform.h>
#include <qgsMultiPolygon.h>
#include <qgsMultiLineString.h>
#include <qgsmultipoint.h>
#include <QgsPointXY.h>
#include <qgsrasterprojector.h>
#include <qgsrasterfilewriter.h>
#include <qgsrasterdataprovider.h>
#include <qgsproject.h>
#include <QFileDialog>
#include <QgsRasterPipe.h>
// 坐标转换
void TransformCRS_Vec(QgsVectorLayer* veclayer, int newCRScode)
{
    // 获取当前工程坐标系
    int SrcCRScode = veclayer->crs().postgisSrid();
	QgsCoordinateReferenceSystem utmCRS = QgsCoordinateReferenceSystem::fromEpsgId(SrcCRScode);
	QgsCoordinateReferenceSystem wgs84CRS = QgsCoordinateReferenceSystem::fromEpsgId(newCRScode);
	QgsCoordinateTransform transform(utmCRS, wgs84CRS, QgsProject::instance());
	qDebug()<< transform.destinationCrs().authid();
    // 投影坐标区域
    if (newCRScode == SrcCRScode)
    {
        return;
    }
	else if (!transform.isValid()) {
		QMessageBox::critical(nullptr, "错误", "不存在该转换");
		return;
	}
    else
    {
        // 获取当前图层的要素
        QgsFeatureIterator iter = veclayer->getFeatures();
        // 创建要素列表
        QgsFeatureList featureList;
        // 遍历要素
        QgsFeature feature;
        while (iter.nextFeature(feature))
        {
            // 获取几何对象
            QgsGeometry geometry = feature.geometry();
            // 获取几何类型
            Qgis::GeometryType geometryType = geometry.type();
            // 判断几何类型
            if (geometryType == Qgis::GeometryType::Point)
            {
				QgsMultiPointXY vecpoints = geometry.asMultiPoint();
				//qDebug() << "点对象数：" << vecpoints.size() << "\n";
				QgsMultiPointXY newPoints;
				for (QgsPointXY point : vecpoints)
				{
					QgsPointXY wgs84Point;
					wgs84Point = transform.transform(point);
					newPoints.append(wgs84Point);
					//qDebug() << "转换前坐标：" << point.x() << "," << point.y() << "转换后坐标：" << wgs84Point.x() << "," << wgs84Point.y() << "\n";
				}
				QgsGeometry newGeometry = QgsGeometry::fromMultiPointXY(newPoints);
				feature.setGeometry(newGeometry);
				featureList.append(feature);
			}
			else if (geometryType == Qgis::GeometryType::Line)
			{
				// 获取多线对象
				QgsMultiPolylineXY veclines = geometry.asMultiPolyline();
				//qDebug() << "线对象数：" << veclines.size() << "\n";
				// 创建新的多线对象
				QgsMultiPolylineXY newLines;
				// 遍历多线对象
				for (QgsPolylineXY line : veclines)
				{
					// 创建新的线对象
					QgsPolylineXY newLine;
					// 遍历线中的点
					for (QgsPointXY point : line)
					{
						// 转换坐标
						QgsPointXY wgs84Point;
						// 转换坐标
						wgs84Point = transform.transform(point);
						// 添加到新的线中
						newLine.append(wgs84Point);
						//qDebug() << "转换前坐标：" << point.x() << "," << point.y() << "转换后坐标：" << wgs84Point.x() << "," << wgs84Point.y() << "\n";

					}
					// 添加新的线到新的线对象中
					newLines.append(newLine);
				}
				// 创建新的几何对象
				QgsGeometry newGeometry = QgsGeometry::fromMultiPolylineXY(newLines);
				// 设置新的几何对象
				feature.setGeometry(newGeometry);
				// 添加到要素列表
				featureList.append(feature);
			}
			// 转换面要素坐标系
			else if (geometryType == Qgis::GeometryType::Polygon)
			{
				// 获取多面对象
				QgsMultiPolygonXY vecpolygons = geometry.asMultiPolygon();
				//qDebug() << "面对象环数：" << vecpolygons.size() << "\n";
				// 创建新的多面对象
				QgsMultiPolygonXY newPolygons;
				// 遍历多面对象的面
				for (QgsPolygonXY polygon : vecpolygons)
				{
					// 创建新的面对象
					QgsPolygonXY newpoly;
					// 遍历面中的点
					for (QgsPolylineXY ring : polygon) {
						QgsPolylineXY newRing;
						for (QgsPointXY point : ring)
						{
							// 转换坐标
							QgsPointXY wgs84Point;
							// 转换坐标
							wgs84Point = transform.transform(point);
							// 添加到新的环中
							newRing.append(wgs84Point);
							//qDebug() << "转换前坐标：" << point.x() << "," << point.y() << "转换后坐标：" << wgs84Point.x() << "," << wgs84Point.y() << "\n";

						}
						// 添加新的环到新的面对象中
						newpoly.append(newRing);
					}
					// 添加新的环到新的面对象中
					newPolygons.append(newpoly);
				}
				// 创建新的几何对象
				QgsGeometry newGeometry = QgsGeometry::fromMultiPolygonXY(newPolygons);
				// 设置新的几何对象
				feature.setGeometry(newGeometry);
				// 添加到要素列表
				featureList.append(feature);
            }
        }
		veclayer->startEditing();
		// 清空图层原有要素
		veclayer->dataProvider()->deleteFeatures(veclayer->allFeatureIds());
        // 添加要素到图层
        veclayer->dataProvider()->addFeatures(featureList);
		// 更新图层crs代码
		veclayer->setCrs(QgsCoordinateReferenceSystem::fromEpsgId(newCRScode));

		veclayer->commitChanges();
		QMessageBox::information(nullptr, "提示", "坐标系转换成功");
        // 更新图层范围
        veclayer->updateExtents();
    }
}


QgsRasterLayer* TransformCRS_Ras(QgsRasterLayer* raslayer, int newCRScode) {
    // 获取当前工程坐标系
    int SrcCRScode = raslayer->crs().postgisSrid();
    // 创建坐标转换对象
    QgsCoordinateReferenceSystem srcCRS = QgsCoordinateReferenceSystem::fromEpsgId(SrcCRScode);
    QgsCoordinateReferenceSystem newCRS = QgsCoordinateReferenceSystem::fromEpsgId(newCRScode);
    // 创建坐标转换对象
    QgsCoordinateTransform transform(srcCRS, newCRS, QgsProject::instance());
    // 判断坐标转换是否有效
    if (!transform.isValid()) {
        QMessageBox::critical(nullptr, "错误", "不存在该转换");
        return nullptr;
    }
    QString outputFilePath = QFileDialog::getSaveFileName(nullptr, "保存文件", "", "GeoTIFF文件(*.tif)");
    // 创建栅格投影器
    QgsRasterProjector* projector = new QgsRasterProjector();
    projector->setCrs(srcCRS,newCRS);

    // 设置新的栅格参数
    QgsRasterDataProvider* provider = raslayer->dataProvider();
    QgsRasterPipe* pipe = new QgsRasterPipe(*raslayer->pipe());

	pipe->replace(3, projector);  // 替换投影器
	// 重新计算栅格数据范围
	QgsRectangle extent = raslayer->extent();
	QgsRectangle newExtent = transform.transformBoundingBox(extent);

    // 设置栅格文件写入器
    QgsRasterFileWriter writer(outputFilePath);
	// 写入转换后的栅格
    if (writer.writeRaster(pipe, provider->xSize(), provider->ySize(), newExtent, newCRS) != Qgis::RasterFileWriterResult::Success) {
        QMessageBox::critical(nullptr, "错误", "栅格坐标系转换失败！");
    }
    else {
        QMessageBox::information(nullptr, "提示", "栅格坐标系转换成功！");
    }
	QgsRasterLayer* newRasterLayer = new QgsRasterLayer(outputFilePath, raslayer->name()+"_"+newCRScode);
	QgsProject::instance()->addMapLayer(newRasterLayer);
	return newRasterLayer;
}