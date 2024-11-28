#include "MainWidget.h"
#include <QInputDialog>
#include <QMessagebox>
#include <qgssymbol.h>
#include <qgsvectorlayer.h>
#include <qgssinglesymbolrenderer.h>
#include <qgsvertexid.h>
#include <qgsvertexmarker.h>

// 获取点击图层要素
void MainWidget::onMapCanvasClicked(const QPoint& point)
{
    mbDragging = true;
    mLastMousePos = mcanMapCanvas->getCoordinateTransform()->toMapCoordinates(point);
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
    if (fit.nextFeature(feature)) {
        mpfSelectFeature.push_back(feature);
        mnSelectVertexIndex = -1; // 初始化为未选中顶点

        // 判断是否选中了顶点
        QgsGeometry geometry = feature.geometry();
        if (geometry.type() == Qgis::GeometryType::Polygon) {
            for (const auto& polygon : geometry.asMultiPolygon()) {
                for (const auto& vertex : polygon) {
                    for (int i = 0; i < vertex.size(); ++i) {
                        double distance = vertex[i].distance(mapPoint);
                        if (distance < searchRadius) {
                            mnSelectVertexIndex = i; // 选中具体顶点
                            break;
                        }
                    }
                }
            }
        }
        else if (geometry.type() == Qgis::GeometryType::Line) {
            for (const auto& line : geometry.asMultiPolyline()) {
                for (int i = 0; i < line.size(); ++i) {
                    double distance = line[i].distance(mapPoint);
                    if (distance < searchRadius) {
                        mnSelectVertexIndex = i; // 选中具体顶点
                        break;
                    }
                }
            }
        }
        else if (geometry.type() == Qgis::GeometryType::Point) {
            double distance = geometry.asPoint().distance(mapPoint);
            if (distance < searchRadius) {
                mnSelectVertexIndex = 0; // 选中顶点
            }
        }
        // 修改选中要素的符号
        QgsMultiPointXY points;
        QgsSymbol* symbol = QgsSymbol::defaultSymbol(vectorLayer->geometryType());
        // 修改选中要素中所有节点的符号
        if (mpfSelectFeature[0].geometry().type() == Qgis::GeometryType::Polygon) {
            for (const auto& polygon : mpfSelectFeature[0].geometry().asMultiPolygon()) {
                for (const auto& vertex : polygon) {
                    for (int i = 0; i < vertex.size(); ++i) {
                        points.append(vertex[i]);
                    }
                }
            }
        }
        else if (mpfSelectFeature[0].geometry().type() == Qgis::GeometryType::Line) {
            for (const auto& line : mpfSelectFeature[0].geometry().asMultiPolyline()) {
                for (const auto& vertex : line) {
                    points.append(vertex);
                }
            }
        }
        else if (mpfSelectFeature[0].geometry().type() == Qgis::GeometryType::Point) {
            points.append(mpfSelectFeature[0].geometry().asPoint());
        }
        // 删除原有符号
        for (int i = 0; i < mvVertices.size() && !mvVertices.isEmpty(); i++) {
            delete mvVertices[i];
        }
        mvVertices.clear();
        // 添加符号
        for (int i = 0; i < points.size() && mnActiveLayerIndex != -1; i++) {
            mvVertices.append(new QgsVertexMarker(mcanMapCanvas));
            mvVertices[i]->setCenter(points[i]);
            mvVertices[i]->setColor(QColor(255, 0, 0));
            mvVertices[i]->setFillColor(QColor(0, 191, 255));
            mvVertices[i]->setIconType(QgsVertexMarker::ICON_RHOMBUS);
            mvVertices[i]->setPenWidth(1);
            mvVertices[i]->show();
        }
        // 刷新地图
        mcanMapCanvas->refresh();
    }
    else {
        // 删除原有符号
        for (int i = 0; i < mvVertices.size() && !mvVertices.isEmpty(); i++) {
            delete mvVertices[i];
        }
        mvVertices.clear();
    }
}

void MainWidget::onMapCanvasReleased(const QPoint& point)
{
	// 将屏幕坐标转换为地图坐标
	QgsPointXY mapPoint = mcanMapCanvas->getCoordinateTransform()->toMapCoordinates(point);
    mbDragging = false;
    if (mpfSelectFeature.size() == 1) {
        mpfSelectFeature.clear();
    }
}

void MainWidget::onMapCanvasMoved(const QPoint& point) {
    if (mbDragging && mnActiveLayerIndex != -1)
    {
        // 将屏幕坐标转换为地图坐标
        QgsPointXY currentMousePos = mcanMapCanvas->getCoordinateTransform()->toMapCoordinates(point);
        QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(mcanMapCanvas->layers()[mnActiveLayerIndex]);

        if (mnSelectVertexIndex == -1) // 整体平移逻辑
        {
            // 计算偏移量
            double deltaX = currentMousePos.x() - mLastMousePos.x();
            double deltaY = currentMousePos.y() - mLastMousePos.y();

            // 遍历所有选中的要素并平移
            for (QgsFeature& feature : mpfSelectFeature) {
                QgsGeometry geometry = feature.geometry();
                geometry.translate(deltaX, deltaY);
                feature.setGeometry(geometry);

                vectorLayer->startEditing();
                vectorLayer->updateFeature(feature);
            }

            // 更新顶点标记位置
            for (int i = 0; i < mvVertices.size(); ++i)
            {
                QgsPointXY updatedPoint = mvVertices[i]->center();
                updatedPoint.setX(updatedPoint.x() + deltaX);
                updatedPoint.setY(updatedPoint.y() + deltaY);
                mvVertices[i]->setCenter(updatedPoint);
            }

            //vectorLayer->commitChanges();
            vectorLayer->triggerRepaint();
            mcanMapCanvas->refresh();
        }
        else // 单点拖动逻辑
        {
            if (mpfSelectFeature.size() > 1 && vectorLayer->geometryType() != Qgis::GeometryType::Point) {
                return;
            }
            // 遍历所有选中的要素并修改相应顶点
            for (QgsFeature& feature : mpfSelectFeature) {
                QgsGeometry newGeometry = feature.geometry();

                newGeometry.moveVertex(currentMousePos.x(), currentMousePos.y(), mnSelectVertexIndex);
                feature.setGeometry(newGeometry);

                vectorLayer->startEditing();
                vectorLayer->updateFeature(feature);
            }

            mvVertices[mnSelectVertexIndex]->setCenter(currentMousePos);

            //vectorLayer->commitChanges();
            vectorLayer->triggerRepaint();
            mcanMapCanvas->refresh();
        }

        mLastMousePos = currentMousePos; // 更新最后一次鼠标位置
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
            ui.ctrlDeleteAction->setEnabled(true);
            ui.ctrlEditAttriAction->setEnabled(true);
            ui.ctrlMoveAction->setEnabled(true);
			break;
		}
        if (chooseLayer == "停用编辑") {
            //QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(mcanMapCanvas->layers()[mnActiveLayerIndex]);
            //vectorLayer->commitChanges();
            mcanMapCanvas->unsetMapTool(mcanMapCanvas->mapTool());
            ui.ctrlDeleteAction->setEnabled(false);
            ui.ctrlEditAttriAction->setEnabled(false);
            ui.ctrlMoveAction->setEnabled(false);
            mpfSelectFeature.clear();
            // 删除原有符号
            for (int i = 0; i < mvVertices.size() && !mvVertices.isEmpty(); i++) {
                delete mvVertices[i];
            }
            mvVertices.clear();
            mnActiveLayerIndex = -1;
            mnSelectVertexIndex = -1;
            mbDragging = false;
            ui.ctrlEditableAction->setText("启用编辑");
            break;
        }
	}

    qDebug() << chooseLayer;

}
