#include "MainWidget.h"
#include "SelectFeatureTool.h"
#include "MoveFeatureTool.h"
#include <QMessageBox>
#include <qgsvectorlayer.h>
#include <qformlayout.h>
#include <qlineedit.h>
#include <QDialogButtonBox>
#include <QInputDialog>

void MainWidget::deleteFeature(const QList<QgsFeature>& selectedFeatures) {
	if (!selectedFeatures.isEmpty())
	{
		// 删除选中的图元
		QList<QgsMapLayer*> selectedLayers = mcanMapCanvas->layers();

		if (selectedLayers.isEmpty())
		{
			return;
		}

		QgsMapLayer* currentLayer = selectedLayers[mnActiveLayerIndex];

		QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(currentLayer);
		vectorLayer->startEditing();
		vectorLayer->deleteSelectedFeatures();  // 删除当前选中的图元
		//vectorLayer->commitChanges();  // 提交更改
	}
}

void MainWidget::moveFeature(const QList<QgsFeature>& selectedFeatures) {
	if (selectedFeatures.isEmpty()) {
		return;
	}
	mpfSelectFeature = selectedFeatures;

	QList<QgsMapLayer*> selectedLayers = mcanMapCanvas->layers();
	if (selectedLayers.isEmpty())
	{
		return;
	}
	QgsMapLayer* currentLayer = selectedLayers[mnActiveLayerIndex];
	QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(currentLayer);

	// 开始编辑图层
	vectorLayer->startEditing();

	// 清空当前显示的所有顶点符号
	for (int i = 0; i < mvVertices.size() && !mvVertices.isEmpty(); i++) {
		delete mvVertices[i];
	}
	mvVertices.clear();

	// 遍历所有选中的要素并使它们可编辑
	for (const QgsFeature& feature : mpfSelectFeature) {
		// 获取要素几何
		QgsGeometry geom = feature.geometry();
		if (geom.isEmpty()) {
			continue;
		}

		// 创建一个点集合，用于显示要素的各个节点
		QgsMultiPointXY points;

		// 如果是多边形
		if (geom.type() == Qgis::GeometryType::Polygon) {
			for (const auto& polygon : geom.asMultiPolygon()) {
				for (const auto& vertex : polygon) {
					for (const auto& point : vertex) {
						points.append(point);
					}
				}
			}
		}
		// 如果是多线
		else if (geom.type() == Qgis::GeometryType::Line) {
			for (const auto& line : geom.asMultiPolyline()) {
				for (const auto& point : line) {
					points.append(point);
				}
			}
		}
		// 如果是点
		else if (geom.type() == Qgis::GeometryType::Point) {
			points.append(geom.asPoint());
		}

		// 添加每个节点的符号显示
		for (int i = 0; i < points.size(); i++) {
			QgsVertexMarker* vertexMarker = new QgsVertexMarker(mcanMapCanvas);
			vertexMarker->setCenter(points[i]);
			vertexMarker->setColor(QColor(255, 0, 0));        // 设置符号颜色
			vertexMarker->setFillColor(QColor(0, 191, 255));  // 设置填充颜色
			vertexMarker->setIconType(QgsVertexMarker::ICON_RHOMBUS); // 设置符号形状
			vertexMarker->setPenWidth(1);                      // 设置边框宽度
			vertexMarker->show();
			mvVertices.append(vertexMarker);
		}
	}

	// 刷新地图
	mcanMapCanvas->refresh();
	mnSelectVertexIndex = -1;
}

void MainWidget::editAttribute(const QList<QgsFeature>& selectedFeatures) {
	if (selectedFeatures.isEmpty())
		return;

	QList<QgsMapLayer*> selectedLayers = mcanMapCanvas->layers();

	if (selectedLayers.isEmpty())
	{
		return;
	}

	QgsMapLayer* currentLayer = selectedLayers[mnActiveLayerIndex];

	QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(currentLayer);
	if (!vectorLayer)
	{
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("图层错误"));
		return;
	}
	vectorLayer->startEditing();

	// 只编辑第一个选中的图元
	QgsFeature feature = selectedFeatures.first();

	// 创建对话框
	QDialog dialog(this);
	dialog.setWindowTitle(QStringLiteral("修改属性"));
	QVBoxLayout* layout = new QVBoxLayout(&dialog);

	// 使用 QFormLayout 展示属性字段和输入框
	QFormLayout* formLayout = new QFormLayout;
	QMap<QString, QLineEdit*> attributeEdits; // 用于存储属性名和对应的编辑框

	const QgsFields& fields = feature.fields();
	for (const QgsField& field : fields) {
		QString fieldName = field.name();
		QVariant value = feature.attribute(fieldName);

		// 创建 QLineEdit，用于显示和编辑属性值
		QLineEdit* edit = new QLineEdit(value.toString(), &dialog);
		formLayout->addRow(fieldName, edit);
		attributeEdits.insert(fieldName, edit); // 存储编辑框
	}

	layout->addLayout(formLayout);

	// 添加确认和取消按钮
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttonBox);

	connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	// 显示对话框并等待用户输入
	if (dialog.exec() == QDialog::Accepted) {
		// 用户点击确定后，更新属性
		for (auto it = attributeEdits.cbegin(); it != attributeEdits.cend(); ++it) {
			QString fieldName = it.key();
			QString newValue = it.value()->text();
			feature.setAttribute(fieldName, newValue);
		}

		// 将修改提交到图层
		vectorLayer->updateFeature(feature);
		//vectorLayer->commitChanges();
		QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("修改结果已成功提交"));
	}
	else {
		vectorLayer->rollBack();
	}
}
