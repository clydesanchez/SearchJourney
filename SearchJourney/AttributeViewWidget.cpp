#include "AttributeViewWidget.h"
#include <qgsvectorlayercache.h>
#include <Qgsfeature.h>
#include <QInputDialog>
#include <QMessageBox>
#include <QgsAttributeTableFilterModel.h>
AttributeViewWidget::AttributeViewWidget(QgsVectorLayer* curVecLayer, QgsMapCanvas* mapcanvas, QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	// 自定义的tableview替换到原有的tableview的布局中
	mTableView = new CustomAttributeTableView(this);
	ui.verticalLayout_2->addWidget(mTableView);
	delete ui.tableView;
	ui.ctrl_btnAddAttri->setEnabled(false);
	ui.ctrl_btnAddFeature->setEnabled(false);
	ui.ctrl_btnDelAttri->setEnabled(false);
	ui.ctrl_btnModifyAttri->setEnabled(false);
	ui.ctrl_btnDelFeature->setEnabled(false);
	// 设置model
	mCurVecLayer = curVecLayer;
	QgsVectorLayerCache* cache = new QgsVectorLayerCache(mCurVecLayer, 1000);
	QgsAttributeTableModel* attriTableModel = new QgsAttributeTableModel(cache);
	QgsAttributeTableFilterModel* attriModel = new QgsAttributeTableFilterModel(mapcanvas, attriTableModel);
	attriTableModel->loadLayer();
	mTableView->setModel(attriModel);
	// 接收输入数据
	connect(mTableView, &CustomAttributeTableView::idxSender, this, &AttributeViewWidget::getIdx);
	connect(mTableView, &CustomAttributeTableView::inputData, this, &AttributeViewWidget::WriteData);
	connect(ui.ctrl_btnEdit, &QPushButton::clicked, this, &AttributeViewWidget::changeEditMode);
	connect(ui.ctrl_btnAddAttri, &QPushButton::clicked, this, &AttributeViewWidget::addAttribute);
	connect(ui.ctrl_btnAddFeature, &QPushButton::clicked, this, &AttributeViewWidget::addFeature);
	connect(ui.ctrl_btnDelAttri, &QPushButton::clicked, this, &AttributeViewWidget::deleteAttribute);
	connect(ui.ctrl_btnDelFeature, &QPushButton::clicked, this, &AttributeViewWidget::deleteFeature);
	connect(ui.ctrl_btnModifyAttri, &QPushButton::clicked, this, &AttributeViewWidget::modifyAttribute);
}

AttributeViewWidget::~AttributeViewWidget()
{}

void AttributeViewWidget::setModel(QgsAttributeTableFilterModel* attriModel)
{
	mTableView->setModel(attriModel);
}

void AttributeViewWidget::getIdx(QModelIndex pos)
{
	mClickedIndex = pos;
}

void AttributeViewWidget::WriteData( QString val)
{
	qDebug()<<mClickedIndex.row()<<" " << mClickedIndex.column()<<" " << val;
	// 如果编辑的字段为int或者double类型，需要判断输入的是否为数字
	if (mCurVecLayer->fields().at(mClickedIndex.column()).type() == QVariant::Int || mCurVecLayer->fields().at(mClickedIndex.column()).type() == QVariant::Double)
	{
		bool ok;
		if (mCurVecLayer->fields().at(mClickedIndex.column()).type() == QVariant::Int)
		{
			val.toInt(&ok);
		}
		else
		{
			val.toDouble(&ok);
		}
		if (!ok)
		{
			QMessageBox::warning(this, "警告", "请输入数字");
			return;
		}
	}
	// 将数据写入到图层中
	mCurVecLayer->startEditing();
	mCurVecLayer->changeAttributeValue(mClickedIndex.row(), mClickedIndex.column(), val);
	mCurVecLayer->commitChanges();
	// 更新数据
	mCurVecLayer->updateExtents();
	mCurVecLayer->triggerRepaint();
	// 更新表格
	mTableView->update();
}

void AttributeViewWidget::addFeature()
{
	mCurVecLayer->startEditing();
	// 在末尾新增一个要素
	QgsFeature feature;
	//feature.setGeometry(QgsGeometry::fromPointXY(QgsPointXY(0, 0)));
	QgsAttributes attrilist;
	for (int i = 0; i < mCurVecLayer->attributeList().count(); i++)
	{
		attrilist.append("");
	}
	feature.setAttributes(attrilist);
	mCurVecLayer->addFeature(feature);
	mCurVecLayer->commitChanges();
	// 更新数据
	mCurVecLayer->updateExtents();
	mCurVecLayer->triggerRepaint();
	// 更新表格
	mTableView->update();
	qDebug() << mCurVecLayer->featureCount();
}

void AttributeViewWidget::deleteFeature()
{
	if(!mClickedIndex.isValid())
	{
		QMessageBox::warning(this, "警告", "请先选择要删除的要素的任一单元格");
		return;
	}
	mCurVecLayer->startEditing();
	mCurVecLayer->deleteFeature(mClickedIndex.row());
	mCurVecLayer->commitChanges();
	QMessageBox::information(this, "提示", "删除成功");
	// 更新数据
	mCurVecLayer->updateExtents();
	mCurVecLayer->triggerRepaint();
	// 更新表格
	mTableView->update();
	qDebug() << mCurVecLayer->featureCount();
}

void AttributeViewWidget::addAttribute()
{
	mCurVecLayer->startEditing();
	// 在末尾新增一个属性
	// 弹出窗口输入属性名
	QString attriName = QInputDialog::getText(this, "添加字段", "请输入字段名");
	// 弹出窗口输入属性类型
	QString attriType = QInputDialog::getItem(this, "添加字段", "请选择字段类型", QStringList() << "String" << "Int" << "Double" << "Date"<<"Bool", 0, false);
	// 根据选择新建字段
	if (attriType == "Int") {
		mCurVecLayer->addAttribute(QgsField(attriName, QVariant::Int));
	}
	else if (attriType == "Double") {
		mCurVecLayer->addAttribute(QgsField(attriName, QVariant::Double));
	}
	else if (attriType == "Date") {
		mCurVecLayer->addAttribute(QgsField(attriName, QVariant::Date));
	}
	else if (attriType == "Bool") {
		mCurVecLayer->addAttribute(QgsField(attriName, QVariant::Bool));
	}
	else {
		mCurVecLayer->addAttribute(QgsField(attriName, QVariant::String));
	}
	mCurVecLayer->commitChanges();
	QMessageBox::information(this, "提示", "添加成功");
	// 更新数据
	mCurVecLayer->updateExtents();
	mCurVecLayer->triggerRepaint();
	// 更新表格
	mTableView->update();
	qDebug() << mCurVecLayer->attributeList().count();
}

void AttributeViewWidget::deleteAttribute()
{
	if (!mClickedIndex.isValid())
	{
		QMessageBox::warning(this, "警告", "请先选择要删除的字段的任一单元格");
		return;
	}
	mCurVecLayer->startEditing();
	// 删除属性
	mCurVecLayer->deleteAttribute(mClickedIndex.column());
	mCurVecLayer->commitChanges();
	QMessageBox::information(this, "提示", "删除成功");
	// 更新数据
	mCurVecLayer->updateExtents();
	mCurVecLayer->triggerRepaint();
	// 更新表格
	mTableView->update();
	qDebug() << mCurVecLayer->attributeList().count();
}

void AttributeViewWidget::changeEditMode()
{
	if(ui.ctrl_btnEdit->text()=="启用编辑")
	{
		//mCurVecLayer->startEditing();
		ui.ctrl_btnEdit->setText("停止编辑");

		ui.ctrl_btnAddAttri->setEnabled(true);
		ui.ctrl_btnAddFeature->setEnabled(true);
		ui.ctrl_btnDelAttri->setEnabled(true);
		ui.ctrl_btnDelFeature->setEnabled(true);
		ui.ctrl_btnModifyAttri->setEnabled(true);

		mTableView->setEditMode(true);
	}
	else
	{
		//mCurVecLayer->commitChanges();
		ui.ctrl_btnEdit->setText("启用编辑");

		ui.ctrl_btnAddAttri->setEnabled(false);
		ui.ctrl_btnAddFeature->setEnabled(false);
		ui.ctrl_btnDelAttri->setEnabled(false);
		ui.ctrl_btnDelFeature->setEnabled(false);
		ui.ctrl_btnModifyAttri->setEnabled(false);
		
		mTableView->setEditMode(false);
	}
}

void AttributeViewWidget::modifyAttribute() {
	QgsVectorLayer* pLayer = mCurVecLayer;
	pLayer->startEditing();
	if (!pLayer || !pLayer->isEditable()) {
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请选择一个有效的图层"));
		return;
	}

	const QgsFields& fields = pLayer->fields();
	if (fields.isEmpty()) {
		QMessageBox::information(this, QStringLiteral("错误"), QStringLiteral("字段为空"));
		return;
	}

	QStringList fieldNames;
	for (const QgsField& field : fields) {
		fieldNames << field.name();
	}

	bool ok;
	QString fieldNameToModify = QInputDialog::getItem(this, QStringLiteral("请选择需要修改的字段"),
		QStringLiteral("选择字段"), fieldNames, 0, false, &ok);
	if (!ok || fieldNameToModify.isEmpty()) {
		return;
	}

	int fieldIndex = fields.indexOf(fieldNameToModify);
	if (fieldIndex == -1) {
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("字段无效"));
		return;
	}

	QStringList fieldTypes = { QStringLiteral("字符型"), QStringLiteral("整型"), QStringLiteral("浮点型") };

	QDialog* pModify = new QDialog();
	pModify->setWindowModality(Qt::ApplicationModal);
	pModify->setWindowTitle(QStringLiteral("修改字段"));
	// 字段名相关控件
	QHBoxLayout* hbl = new QHBoxLayout();
	QLabel* pFNLabel = new QLabel();
	QLineEdit* pFNLineEdit = new QLineEdit();
	pFNLabel->setText(QStringLiteral("字段名称"));
	pFNLineEdit->setText(fieldNameToModify);
	hbl->addWidget(pFNLabel);
	hbl->addWidget(pFNLineEdit);
	// 字段类型相关控件
	QHBoxLayout* hbl2 = new QHBoxLayout();
	QLabel* pFTLabel = new QLabel();
	QComboBox* pFTComboBox = new QComboBox();
	pFTLabel->setText(QStringLiteral("字段类型"));
	pFTComboBox->addItems(fieldTypes);
	hbl2->addWidget(pFTLabel);
	hbl2->addWidget(pFTComboBox);
	// 字段长度相关控件
	QHBoxLayout* hbl3 = new QHBoxLayout();
	QLabel* pFLLabel = new QLabel();
	QSpinBox* pFLSpinBox = new QSpinBox();
	pFLLabel->setText(QStringLiteral("字段长度"));
	hbl3->addWidget(pFLLabel);
	hbl3->addWidget(pFLSpinBox);
	// 确定取消相关控件
	QHBoxLayout* hbl4 = new QHBoxLayout();
	QPushButton* pYes = new QPushButton();
	QPushButton* pNo = new QPushButton();
	pYes->setText(QStringLiteral("确定"));
	pNo->setText(QStringLiteral("取消"));
	hbl4->addWidget(pYes);
	hbl4->addWidget(pNo);
	// 窗口设置
	QVBoxLayout* vbl = new QVBoxLayout();
	vbl->addLayout(hbl);
	vbl->addLayout(hbl2);
	vbl->addLayout(hbl3);
	vbl->addLayout(hbl4);
	pModify->setLayout(vbl);

	connect(pYes, &QPushButton::clicked, pModify, [=]() {
		QString newFieldName = pFNLineEdit->text();
		QString fieldTypeStr = pFTComboBox->currentText();
		int newFieldLength = 0;
		
		QVariant::Type newFieldType;

		if (fieldTypeStr == QStringLiteral("字符型")) {
			newFieldType = QVariant::String;
			newFieldLength = pFLSpinBox->value();
			if (newFieldLength > 255) {
				QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("字符长度超出限制"));
			}
		}
		else if (fieldTypeStr == QStringLiteral("整型")) {
			newFieldType = QVariant::Int;
		}
		else if (fieldTypeStr == QStringLiteral("浮点型")) {
			newFieldType = QVariant::Double;
		}
		else {
			QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请选择有效的字段"));
			return;
		}

		if (!pLayer->dataProvider()->deleteAttributes({ fieldIndex })) {
			QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("字段修改错误"));
			pLayer->rollBack();
			return;
		}
		pLayer->updateFields();

		QgsField newField(newFieldName, newFieldType, "", newFieldLength);
		if (!pLayer->dataProvider()->addAttributes({ newField })) {
			QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("字段修改错误"));
			pLayer->rollBack();
			return;
		}

		if (!pLayer->commitChanges()) {
			QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("提交修改错误"));
			return;
		}

		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("字段修改成功"));

		
		pLayer->updateFields();
		// 更新数据
		mCurVecLayer->updateExtents();
		mCurVecLayer->triggerRepaint();
		// 更新表格
		mTableView->update();
		pModify->accept();
		});

	pModify->exec();
}