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
	ui.gridLayout->addWidget(mTableView);
	delete ui.tableView;
	ui.ctrl_btnAddAttri->setEnabled(false);
	ui.ctrl_btnAddFeature->setEnabled(false);
	ui.ctrl_btnDelAttri->setEnabled(false);
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
		
		mTableView->setEditMode(false);
	}
}