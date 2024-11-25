#include "MainWidget.h"
#include <QgsvectorLayer.h>
#include <QInputDialog>
#include <QMessagebox>
#include <QgsSymbol.h>
#include <QgsSingleSymbolRenderer.h>
#include <qgsvertexid.h>
#include <qgsvertexmarker.h>
// 获取点击图层要素
void MainWidget::onMapCanvasClicked(const QPoint& point)
{

    mbDragging = true;
    // 将屏幕坐标转换为地图坐标
    QgsPointXY mapPoint = mcanMapCanvas->getCoordinateTransform()->toMapCoordinates(point);

    // 获取当前的矢量图层
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    // TODO：目前只支持一个图层，创建激活图层
    if (mnActiveLayerIndex < 0) {
        return;
    }
    if (layers.size() < mnActiveLayerIndex) {
        QMessageBox::information(this, "提示", "该图层不存在");
        return;
    }
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    if (!vectorLayer)
    {
        return;
    }

    // 构建查询矩形区域
    double searchRadius = mcanMapCanvas->mapUnitsPerPixel() * 5;  // 调整搜索半径
    QgsRectangle searchRect(mapPoint.x() - searchRadius, mapPoint.y() - searchRadius,
        mapPoint.x() + searchRadius, mapPoint.y() + searchRadius);

    // 查询要素
    QgsFeature feature;
    QgsFeatureIterator fit = vectorLayer->getFeatures(QgsFeatureRequest().setFilterRect(searchRect));
    if (fit.nextFeature(feature))
    {
        // 获取要素属性
        QgsFields fields = feature.fields();
        mpfSelectFeature = feature;
        for (int i = 0; i < fields.size(); i++)
        {
            QgsField field = fields.at(i);
            qDebug() << field.name() << ":" << mpfSelectFeature.attribute(i);
        }
        // 获取要素几何
        QgsGeometry geometry = mpfSelectFeature.geometry();
        if (!geometry.isEmpty())
        {
            QgsPointXY point = geometry.asPoint();
            qDebug() << "x:" << point.x() << "y:" << point.y();
        }
        // 刷新地图
        mcanMapCanvas->refresh();
        // 设置要素几何

    }
    // 查询节点
    // 如果为多多边形
    qDebug() << mpfSelectFeature.geometry().type();
    mnSelectVertexIndex = -1;
    if (mpfSelectFeature.geometry().type() == Qgis::GeometryType::Polygon) {
        for (const auto& polygon : mpfSelectFeature.geometry().asMultiPolygon()) {
            for (const auto& vertex : polygon) {
                for (int i = 0; i < vertex.size(); ++i) {
                    double distance = vertex[i].distance(mapPoint);
                    if (distance < searchRadius) {
                        mnSelectVertexIndex = i;
                        break;
                    }
                }
            }
        }
    }
    // 如果为多线
    else if (mpfSelectFeature.geometry().type() == Qgis::GeometryType::Line) {
        for (const auto& line : mpfSelectFeature.geometry().asMultiPolyline()) {
            int i = 0;
            for (const auto& vertex : line) {
                double distance = vertex.distance(mapPoint);
                if (distance < searchRadius) {
                    mnSelectVertexIndex = i;
                    break;
                }
                i++;
            }
        }
    }
    // 如果为点
    else if (mpfSelectFeature.geometry().type() == Qgis::GeometryType::Point) {
        double distance = mpfSelectFeature.geometry().asPoint().distance(mapPoint);
        if (distance < searchRadius) {
            mnSelectVertexIndex = 0;
        }
    }
    // 修改选中要素的符号
    QgsMultiPointXY points;
    QgsSymbol* symbol = QgsSymbol::defaultSymbol(vectorLayer->geometryType());
    // 修改选中要素中所有节点的符号
    if (mpfSelectFeature.geometry().type() == Qgis::GeometryType::Polygon) {
        for (const auto& polygon : mpfSelectFeature.geometry().asMultiPolygon()) {
            for (const auto& vertex : polygon) {
                for (int i = 0; i < vertex.size(); ++i) {
                    points.append(vertex[i]);
                }
            }
        }
    }
    else if (mpfSelectFeature.geometry().type() == Qgis::GeometryType::Line) {
        for (const auto& line : mpfSelectFeature.geometry().asMultiPolyline()) {
            for (const auto& vertex : line) {
                points.append(vertex);
            }
        }
    }
    else if (mpfSelectFeature.geometry().type() == Qgis::GeometryType::Point) {
        points.append(mpfSelectFeature.geometry().asPoint());
    }
    // 删除原有符号
    for (int i = 0; i < mvVertices.size()&& !mvVertices.isEmpty(); i++) {
        delete mvVertices[i];
    }
    mvVertices.clear();
    // 添加符号
    for (int i = 0; i < points.size()&& mnActiveLayerIndex != -1; i++) {
        mvVertices.append(new QgsVertexMarker(mcanMapCanvas));
        mvVertices[i]->setCenter(points[i]);
        mvVertices[i]->setColor(QColor(255, 0, 0));
        mvVertices[i]->setFillColor(QColor(0, 191, 255));
        mvVertices[i]->setIconType(QgsVertexMarker::ICON_RHOMBUS);
        mvVertices[i]->setPenWidth(1);
        mvVertices[i]->show();
    }
    mcanMapCanvas->refresh();
}

void MainWidget::onMapCanvasReleased(const QPoint& point)
{
	// 将屏幕坐标转换为地图坐标
	QgsPointXY mapPoint = mcanMapCanvas->getCoordinateTransform()->toMapCoordinates(point);
    mbDragging = false;
}

void MainWidget::onMapCanvasMoved(const QPoint& point) {
    if(mbDragging&&mnSelectVertexIndex>-1&&mnActiveLayerIndex>-1)
	{
        // 将屏幕坐标转换为地图坐标
        QgsPointXY mapPoint = mcanMapCanvas->getCoordinateTransform()->toMapCoordinates(point);
        // 获取当前的矢量图层
        QgsGeometry newGeometry = mpfSelectFeature.geometry();
        // 修改要素顶点坐标
        newGeometry.moveVertex(mapPoint.x(), mapPoint.y(), mnSelectVertexIndex);
        mpfSelectFeature.setGeometry(newGeometry);
        QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(mcanMapCanvas->layers()[mnActiveLayerIndex]);
        // 写入数据
        vectorLayer->startEditing();
        vectorLayer->updateFeature(mpfSelectFeature);
        vectorLayer->commitChanges();
        // 修改标记坐标
        mvVertices[mnSelectVertexIndex]->setCenter(mapPoint);
        //qDebug() << "idx:" << mnSelectVertexIndex;
        //qDebug() << "x:" << mpfSelectFeature.geometry().asPoint().x() << "y:" << mpfSelectFeature.geometry().asPoint().y();
        vectorLayer->triggerRepaint();
        mcanMapCanvas->refresh();
	}

}
// 获取激活图层
void MainWidget::on_ctrlEditableAction_triggered() {
    // 弹出图层选择对话框，选择一个矢量图层
    QgsVectorLayer* vectorLayer = nullptr;
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QStringList layerNames;
    layerNames.append("停用编辑");
    for(QgsMapLayer* layer : layers)
	{
		if (layer->type() == Qgis::LayerType::Vector)
		{
			layerNames.append(layer->name());
		}
	}
    if (layerNames.count() < 1) {
        QMessageBox::information(this, "提示", "请先添加矢量图层");
		return;
	
    }
    QString chooseLayer = QInputDialog::getItem(this, "选择图层", "选择一个矢量图层", layerNames, 0, false);
    // 根据名称获取图层索引
    for(int i = 0; i < layers.size(); i++)
	{
		if (layers[i]->name() == chooseLayer)
		{
			mnActiveLayerIndex = i;
            ui.ctrlEditableAction->setText(QString("当前编辑图层：%1").arg(chooseLayer));
			break;
		}
        if (chooseLayer == "停用编辑") {
            mnActiveLayerIndex = -1;
            // 删除原有符号
            for (int i = 0; i < mvVertices.size() && !mvVertices.isEmpty(); i++) {
                delete mvVertices[i];
            }
            mvVertices.clear();
            ui.ctrlEditableAction->setText("启用编辑");
            break;
        }
	}

    qDebug() << chooseLayer;

}
