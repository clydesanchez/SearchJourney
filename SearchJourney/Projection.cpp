#include "MainWidget.h"
#include <QMessagebox>
#include <QgsvectorLayer.h>
#include <QgsRasterLayer.h>
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
QgsVectorLayer* MainWidget::TransformCRS_Vec(QgsVectorLayer* veclayer, int newCRScode)
{
	// 新建一个内存图层
	QString newLayerName = veclayer->name() + "_" + QString::number(newCRScode);
	// 获取当前图层类型
	QString strLayerType;
	switch (veclayer->geometryType())
	{
		case Qgis::GeometryType::Point:
			strLayerType = "Point";
			break;
		case Qgis::GeometryType::Line:
			strLayerType = "LineString";
			break;
		case Qgis::GeometryType::Polygon:
			strLayerType = "Polygon";
			break;
		default:
			break;
	}
	QString qstrLayerUri = QString("%1?crs=epsg:%2:float&index=yes").arg(strLayerType).arg(newCRScode);
	qDebug() << qstrLayerUri;
	QgsVectorLayer::LayerOptions options;
	options.fallbackCrs = QgsCoordinateReferenceSystem::fromEpsgId(newCRScode);
	QgsVectorLayer* pvNewLayer = new QgsVectorLayer(qstrLayerUri, newLayerName, "memory");
	// 获取原始图层坐标系
	QgsCoordinateReferenceSystem srcCRS = veclayer->crs();
	QString SrcCRScode = srcCRS.authid();
	// 获取当前工程坐标系
	QgsCoordinateReferenceSystem newCRS = QgsCoordinateReferenceSystem::fromEpsgId(newCRScode);
	QgsCoordinateTransform transform(srcCRS, newCRS, QgsProject::instance());
	qDebug() << transform.destinationCrs().authid();
	// 投影坐标区域
	if (newCRScode == SrcCRScode)
	{
		return nullptr;
	}
	else if (!transform.isValid()) {
		QMessageBox::critical(nullptr, "错误", "不存在该转换");
		return nullptr;
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
			QgsFeature* newFeature = new QgsFeature();
			// 获取几何对象
			QgsGeometry geometry = feature.geometry();
			// 获取几何类型
			Qgis::GeometryType geometryType = geometry.type();
			// 判断几何类型
			if (geometryType == Qgis::GeometryType::Point)
			{
				QgsMultiPointXY vecpoints = geometry.asMultiPoint();
				QgsMultiPointXY newPoints;
				for (QgsPointXY point : vecpoints)
				{
					QgsPointXY newCRSPoint;
					newCRSPoint = transform.transform(point);
					newPoints.append(newCRSPoint);
					//qDebug() << "转换前坐标：" << point.x() << "," << point.y() << "转换后坐标：" << newCRSPoint.x() << "," << newCRSPoint.y() << "\n";
				}
				QgsGeometry newGeometry = QgsGeometry::fromMultiPointXY(newPoints);
				// 如果为单点
				if (vecpoints.size() == 0) {
					QgsPointXY newCRSPoint = geometry.asPoint();
					newCRSPoint = transform.transform(newCRSPoint);
					newGeometry = QgsGeometry::fromPointXY(newCRSPoint);
				}
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
						QgsPointXY newCRSPoint;
						// 转换坐标
						newCRSPoint = transform.transform(point);
						// 添加到新的线中
						newLine.append(newCRSPoint);
						//qDebug() << "转换前坐标：" << point.x() << "," << point.y() << "转换后坐标：" << newCRSPoint.x() << "," << newCRSPoint.y() << "\n";

					}
					// 添加新的线到新的线对象中
					newLines.append(newLine);
				}
				// 创建新的几何对象
				QgsGeometry newGeometry = QgsGeometry::fromMultiPolylineXY(newLines);
				// 如果为单线
				if(newLines.size()==0)
				{
					QgsPolylineXY srcLine = geometry.asPolyline();
					QgsPolylineXY newLine;
					for (QgsPointXY point: srcLine) {
						QgsPointXY newCRSPoint;
						newCRSPoint = transform.transform(point);
						//qDebug() << "转换前坐标：" << point.x() << "," << point.y() << "转换后坐标：" << newCRSPoint.x() << "," << newCRSPoint.y() << "\n";
						newLine.append(newCRSPoint);
					}
					newGeometry = QgsGeometry::fromPolylineXY(newLine);
				}
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
							QgsPointXY newCRSPoint;
							// 转换坐标
							newCRSPoint = transform.transform(point);
							// 添加到新的环中
							newRing.append(newCRSPoint);
							//qDebug() << "转换前坐标：" << point.x() << "," << point.y() << "转换后坐标：" << newCRSPoint.x() << "," << newCRSPoint.y() << "\n";

						}
						// 添加新的环到新的面对象中
						newpoly.append(newRing);
					}
					// 添加新的环到新的面对象中
					newPolygons.append(newpoly);
				}
				// 创建新的几何对象
				QgsGeometry newGeometry = QgsGeometry::fromMultiPolygonXY(newPolygons);
				// 如果为单面
				if(newPolygons.size()==0)
				{
					QgsPolygonXY srcPolygon = geometry.asPolygon();
					QgsPolygonXY newPolygon;
					for (QgsPolylineXY ring : srcPolygon) {
						QgsPolylineXY newRing;
						for (QgsPointXY point : ring) {
							QgsPointXY newCRSPoint = transform.transform(point);
							newRing.append(newCRSPoint);
						}
						newPolygon.append(newRing);
					}
					newGeometry = QgsGeometry::fromPolygonXY(newPolygon);
				}
				// 设置新的几何对象
				feature.setGeometry(newGeometry);
				// 添加到要素列表
				featureList.append(feature);
			}
		}
		pvNewLayer->startEditing();
		// 将要素列表添加到图层
		QgsFields fields = veclayer->dataProvider()->fields();
		pvNewLayer->dataProvider()->addAttributes(fields.toList());
		// 更新图层crs代码
		pvNewLayer->setCrs(QgsCoordinateReferenceSystem::fromEpsgId(newCRScode));
		pvNewLayer->commitChanges();
		pvNewLayer->dataProvider()->addFeatures(featureList);
		qDebug() << "要素数：" << pvNewLayer->featureCount() << "\n";
		QMessageBox::information(nullptr, "提示", "坐标系转换成功");
	}
	return pvNewLayer;
}


QgsRasterLayer* MainWidget::TransformCRS_Ras(QgsRasterLayer* raslayer, int newCRScode) {
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
	projector->setCrs(srcCRS, newCRS);

	// 设置新的栅格参数
	QgsRasterDataProvider* provider = raslayer->dataProvider();
	QgsRasterPipe* pipe = new QgsRasterPipe(*raslayer->pipe());
	// 索引3为投影器
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
	QgsRasterLayer* newRasterLayer = new QgsRasterLayer(outputFilePath, raslayer->name() + "_" + newCRScode);
	QgsProject::instance()->addMapLayer(newRasterLayer);
	return newRasterLayer;
}