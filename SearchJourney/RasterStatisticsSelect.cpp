/*
FileName: RasterStatisticsSelect.cpp
Author:RYB
Date:2024.7.28
Description:
	栅格统计分析对话框的实现
Function Lists:
	1. RasterStatisticsSelect(QWidget *parent = nullptr) 构造函数
	2. ~RasterStatisticsSelect() 析构函数
	3. void setMapLayers(QList<QgsMapLayer *> liMapLayers) 设置图层
	4. void addLayerItems() 在ComboBox中添加图层
	5. void on_ctrlComboBoxX_currentIndexChanged(int index) 更换图层后，更新波段数选择框
	6. QgsRasterLayer *getSelectLayer() const 获取选择的图层
	7. int getBandID() const 获取选择的波段
	8. void on_ctrlBtnCancel_clicked() 取消按钮
	9. void on_ctrlBtnConfirm_clicked() 确认按钮
*/

#include "SelectDialog.h"
#include <QMessageBox>

RasterStatisticsSelect::RasterStatisticsSelect(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("栅格统计分析");
	ui.ctrlLabelX->setText("统计图层：");
	ui.ctrlLabelY->setText("选择波段：");
	ui.ctrlBtnConfirm->setText("运行");
	ui.ctrlComboBoxX->clear();
	ui.ctrlComboBoxY->clear();

	ui.ctrlSpinBox->hide();
	ui.ctrlComboBoxCRS->hide();
	ui.ctrlLabelCRS->hide();

	ui.ctrlComboBoxY->show();
	ui.ctrlComboBoxX->show();
}
// 设置图层
void RasterStatisticsSelect::setMapLayers(QList<QgsMapLayer *> liMapLayers)
{
	this->mliMapLayers = liMapLayers;
	addLayerItems();
}
// 在ComboBox中添加图层
void RasterStatisticsSelect::addLayerItems()
{
	for (int nIndex = 0; nIndex < mliMapLayers.count(); nIndex++)
	{
		// 判断是否为栅格图层
		if (mliMapLayers[nIndex]->type() != Qgis::LayerType::Raster)
		{
			continue;
		}
		ui.ctrlComboBoxX->addItem(mliMapLayers[nIndex]->name());
		// 更新波段数选择框
		int nBandCount = ((QgsRasterLayer *)mliMapLayers[nIndex])->bandCount();
		ui.ctrlComboBoxY->clear();
		for (int nIdx = 1; nIdx <= nBandCount; nIdx++)
		{
			ui.ctrlComboBoxY->addItem(QString::number(nIdx));
		}
	}
}
// 更换图层后，更新波段数选择框
void RasterStatisticsSelect::on_ctrlComboBoxX_currentIndexChanged(int index)
{
	if (index < 0)
	{
		return;
	}
	QgsRasterLayer *pvlLayer = dynamic_cast<QgsRasterLayer *>(mliMapLayers[index]);
	if (pvlLayer == nullptr)
	{
		return;
	}
	ui.ctrlComboBoxY->clear();
	for (int nIndex = 1; nIndex <= pvlLayer->bandCount(); nIndex++)
	{
		ui.ctrlComboBoxY->addItem(QString::number(nIndex));
	}
}
// 获取选择的图层
QgsRasterLayer *RasterStatisticsSelect::getSelectLayer() const
{
	return mpvlSelectLayer;
}
// 获取选择的波段
int RasterStatisticsSelect::getBandID() const
{
	return mnBandID;
}

void RasterStatisticsSelect::on_ctrlBtnCancel_clicked()
{
	this->close();
}
void RasterStatisticsSelect::on_ctrlBtnConfirm_clicked()
{
	QString qstrCurrentLayer = ui.ctrlComboBoxX->currentText();
	if (qstrCurrentLayer.isEmpty())
	{
		QMessageBox::critical(this, "error", "请选择图层");
		return;
	}
	// 根据图层名，找到对应栅格图层
	for (QgsMapLayer *layer : mliMapLayers)
	{
		if (layer->name() == qstrCurrentLayer)
		{
			mpvlSelectLayer = dynamic_cast<QgsRasterLayer *>(layer);
			mnBandID = ui.ctrlComboBoxY->currentText().toInt();
			break;
		}
	}
	this->close();
}