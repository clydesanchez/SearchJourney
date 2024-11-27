#include "MainWidget.h"
#include "SelectFeatureTool.h"
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
		vectorLayer->commitChanges();  // 提交更改
	}
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
		vectorLayer->commitChanges();
		QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("修改结果已成功提交"));
	}
	else {
		vectorLayer->rollBack();
	}
}