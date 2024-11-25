/*
FileName: ConnectSelect.cpp
Author:RYB
Date:2024.7.27
Description:
	空间连接对话框的实现
Function Lists:
	1. ConnectSelect(QWidget *parent = nullptr) 构造函数
	2. ~ConnectSelect() 析构函数
	3. void setMapLayers(QList<QgsMapLayer *> liMapLayers) 设置图层
	4. void addLayerItems() 在ComboBox中添加图层
	5. QgsVectorLayer *getSelectLayerResult() const 获取选择的图层
	6. QgsVectorLayer *getSelectLayerCompare() const 获取选择的图层
	7. QString getConnectType() const 获取连接类型
	8. void on_ctrlBtnConfirm_clicked() 确认按钮
	9. void on_ctrlBtnCancel_clicked() 取消按钮
*/

#include "SelectDialog.h"
#include <QMessageBox>
#include <QgsMapLayer.h>

ConnectSelect::ConnectSelect(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("空间连接");
	ui.ctrlLabelX->setText("连接要素到：");
	ui.ctrlLabelY->setText("对比图层：");
	ui.ctrlLabelCRS->setText("连接类型：");
	ui.ctrlBtnConfirm->setText("运行");
	ui.ctrlComboBoxX->clear();
	ui.ctrlComboBoxY->clear();
	ui.ctrlComboBoxCRS->clear();

	ui.ctrlSpinBox->hide();
	ui.ctrlComboBoxCRS->addItem("包含");
	ui.ctrlComboBoxCRS->addItem("相交");
	ui.ctrlComboBoxCRS->addItem("相接");
	ui.ctrlComboBoxCRS->addItem("相等");
	ui.ctrlComboBoxCRS->addItem("交叉");
	ui.ctrlComboBoxCRS->addItem("重叠");

	ui.ctrlLabelCRS->show();
	ui.ctrlComboBoxX->show();
	ui.ctrlComboBoxY->show();
}
// 设置图层
void ConnectSelect::setMapLayers(QList<QgsMapLayer *> liMapLayers)
{
	this->mliMapLayers = liMapLayers;
	addLayerItems();
}
// 在ComboBox中添加图层
void ConnectSelect::addLayerItems()
{
	for (int nIndex = 0; nIndex < mliMapLayers.count(); nIndex++)
	{
		// 判断是否为矢量图层
		if (mliMapLayers[nIndex]->type() != Qgis::LayerType::Vector)
		{
			continue;
		}
		// 写入到ComboBox中
		ui.ctrlComboBoxX->addItem(mliMapLayers[nIndex]->name());
		ui.ctrlComboBoxY->addItem(mliMapLayers[nIndex]->name());
	}
}
// 获取选择的图层
QgsVectorLayer* ConnectSelect::getSelectLayerResult() const
{
	return mpvlSelectLayerResult;
}
// 获取选择的图层
QgsVectorLayer* ConnectSelect::getSelectLayerCompare() const
{
	return mpvlSelectLayerCompare;
}
// 获取连接类型
QString ConnectSelect::getConnectType() const
{
	return mqstrConnectType;
}
// 确认按钮
void ConnectSelect::on_ctrlBtnConfirm_clicked()
{
	QString qstrCurrentLayerResult = ui.ctrlComboBoxX->currentText();
	QString qstrCurrentLayerCompare = ui.ctrlComboBoxY->currentText();
	mqstrConnectType = ui.ctrlComboBoxCRS->currentText();
	if (qstrCurrentLayerResult.isEmpty() || qstrCurrentLayerCompare.isEmpty())
	{
		QMessageBox::critical(this, "error", "请选择图层");
		return;
	}
	// 根据图层名，找到对应矢量图层
	for (QgsMapLayer *layer : mliMapLayers)
	{
		if (layer->name() == qstrCurrentLayerResult)
		{
			mpvlSelectLayerResult = dynamic_cast<QgsVectorLayer *>(layer);
		}
		if (layer->name() == qstrCurrentLayerCompare)
		{
			mpvlSelectLayerCompare = dynamic_cast<QgsVectorLayer *>(layer);
		}
	}
	this->close();
}
// 取消按钮
void ConnectSelect::on_ctrlBtnCancel_clicked()
{
	this->close();
}