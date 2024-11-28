#include "MainWidget.h"
#include "SelectFeatureTool.h"
#include "MoveFeatureTool.h"
#include <QMessageBox>
#include <qgsvectorlayer.h>
#include <qformlayout.h>
#include <qlineedit.h>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QSpinBox>

void MainWidget::deleteFeature(const QList<QgsFeature>& selectedFeatures) {
	if (!selectedFeatures.isEmpty())
	{
		QList<QgsMapLayer*> selectedLayers = mcanMapCanvas->layers();
		if (selectedLayers.isEmpty())
		{
			return;
		}
		QgsMapLayer* currentLayer = selectedLayers[mnActiveLayerIndex];
		QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(currentLayer);

		// 弹出对话框提示用户确认操作
		int ret = QMessageBox::question(
			this,
			"确认删除",
			QString("当前有 %1 个要素被选中，是否确定删除？").arg(selectedFeatures.size()),
			QMessageBox::Yes | QMessageBox::No
		);

		// 如果用户选择 No，则取消操作
		if (ret != QMessageBox::Yes) {
			return;
		}

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

void MainWidget::copyFeature(const QList<QgsFeature>& selectedFeatures) {
	// 如果没有选中的要素，直接返回
	if (selectedFeatures.isEmpty()) {
		QMessageBox::information(this, "提示", "没有选中的要素，无法进行复制操作！");
		return;
	}

	// 弹出对话框提示用户确认操作
	int ret = QMessageBox::question(
		this,
		"确认复制",
		QString("当前有 %1 个要素被选中，是否确定复制？").arg(selectedFeatures.size()),
		QMessageBox::Yes | QMessageBox::No
	);

	// 如果用户选择 No，则取消操作
	if (ret != QMessageBox::Yes) {
		return;
	}

	// 获取目标图层（假设已经在某处设置了目标图层）
	QList<QgsMapLayer*> selectedLayers = mcanMapCanvas->layers();
	if (selectedLayers.isEmpty())
	{
		return;
	}
	QgsMapLayer* currentLayer = selectedLayers[mnActiveLayerIndex];
	QgsVectorLayer* vectorLayer = dynamic_cast<QgsVectorLayer*>(currentLayer);
	if (!vectorLayer) {
		QMessageBox::warning(this, "错误", "目标图层不存在！");
		return;
	}

	// 检查目标图层是否可编辑
	if (!vectorLayer->isEditable()) {
		if (!vectorLayer->startEditing()) {
			QMessageBox::warning(this, "错误", "无法进入编辑模式！");
			return;
		}
	}

	QDialog* pDl = new QDialog();
	QVBoxLayout* pVbl = new QVBoxLayout();
	QHBoxLayout* pHb1 = new QHBoxLayout();
	QHBoxLayout* pHb2 = new QHBoxLayout();
	QHBoxLayout* pHb3 = new QHBoxLayout();
	QHBoxLayout* pHb4 = new QHBoxLayout();
	QHBoxLayout* pHb5 = new QHBoxLayout();
	QLabel* pL1 = new QLabel();
	QLabel* pL2 = new QLabel();
	QLabel* pL3 = new QLabel();
	QLabel* pL4 = new QLabel();
	QSpinBox* pSb1 = new QSpinBox();
	QSpinBox* pSb2 = new QSpinBox();
	QLineEdit* pLe1 = new QLineEdit();
	QLineEdit* pLe2 = new QLineEdit();
	QPushButton* pPb1 = new QPushButton();
	QPushButton* pPb2 = new QPushButton();
	pL1->setText(QStringLiteral("请输入阵列行数"));
	pL2->setText(QStringLiteral("请输入阵列列数"));
	pL3->setText(QStringLiteral("请输入横向复制间距"));
	pL4->setText(QStringLiteral("请输入纵向复制间距"));
	pSb1->setMinimum(1);
	pSb2->setMinimum(1);
	pPb1->setText(QStringLiteral("确定"));
	pPb2->setText(QStringLiteral("取消"));
	pHb1->addWidget(pL1);
	pHb1->addWidget(pSb1);
	pHb2->addWidget(pL2);
	pHb2->addWidget(pSb2);
	pHb3->addWidget(pL3);
	pHb3->addWidget(pLe1);
	pHb4->addWidget(pL4);
	pHb4->addWidget(pLe2);
	pHb5->addWidget(pPb1);
	pHb5->addWidget(pPb2);
	pVbl->addLayout(pHb1);
	pVbl->addLayout(pHb2);
	pVbl->addLayout(pHb3);
	pVbl->addLayout(pHb4);
	pVbl->addLayout(pHb5);
	pDl->setLayout(pVbl);

	connect(pPb1, &QPushButton::clicked, this, [=]() {
		int nRows = pSb1->value();
		int nCols = pSb2->value();
		bool ok1, ok2;
		double dX = pLe1->text().toDouble(&ok1);
		double dY = pLe2->text().toDouble(&ok2);

		if (!ok1 && &ok2) {
			QMessageBox::warning(this, "错误", "请输入正确的复制间距");
			return;
		}
		// 遍历选中的要素并执行阵列复制
		for (const QgsFeature& feature : selectedFeatures) {
			for (int row = 0; row < nRows; ++row) {
				for (int col = 0; col < nCols; ++col) {
					if (row == 0 && col == 0) {
						continue; // 跳过原始要素
					}

					QgsFeature newFeature(feature); // 创建一个副本
					newFeature.setId(QgsFeatureId()); // 重置 ID，避免冲突

					// 偏移几何位置
					QgsGeometry geom = newFeature.geometry();
					if (geom.isNull()) {
						continue;
					}

					// 计算偏移量
					double offsetX = dX * col;
					double offsetY = dY * row;

					// 应用偏移
					geom.translate(offsetX, offsetY);
					newFeature.setGeometry(geom);

					// 将要素添加到目标图层
					if (!vectorLayer->addFeature(newFeature)) {
						QMessageBox::warning(this, "错误", "添加要素失败！");
						vectorLayer->rollBack();
						return;
					}
				}
			}
		}
		
		pDl->accept();
		});

	connect(pPb2, &QPushButton::clicked, this, [=]() {
		pDl->accept();
		});

	pDl->exec();

	//// 遍历选中的要素并添加到目标图层
	//for (const QgsFeature& feature : selectedFeatures) {
	//	QgsFeature newFeature(feature); // 创建一个副本
	//	newFeature.setId(QgsFeatureId()); // 重置 ID，避免冲突

	//	// 将要素添加到目标图层
	//	if (!vectorLayer->dataProvider()->addFeature(newFeature)) {
	//		QMessageBox::warning(this, "错误", "添加要素失败！");
	//		vectorLayer->rollBack();
	//		return;
	//	}
	//}

	// 提交事务并刷新目标图层
	if (!vectorLayer->commitChanges()) {
		QMessageBox::warning(this, "错误", "提交事务失败！");
		vectorLayer->rollBack();
	}
	else {
		vectorLayer->triggerRepaint();
		QMessageBox::information(this, "成功", "要素复制完成！");
	}
}
