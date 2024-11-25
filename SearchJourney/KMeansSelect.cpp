/*
FileName: KMeansSelect.cpp
Author:RYB
Date:2024.7.26
Description:
	KMeans聚类分析对话框的实现
Function Lists:
	1. KMeansSelect(QWidget *parent = nullptr) 构造函数
	2. void setMapLayers(QList<QgsMapLayer *> liMapLayers) 设置图层
	3. void addLayerItems() 在ComboBox中添加图层
	4. QgsVectorLayer *getSelectLayer() const 获取选择的图层
	5. int getClusterNum() const 获取聚类数
	6. void on_ctrlBtnConfirm_clicked() 确认按钮
	7. void on_ctrlBtnCancel_clicked() 取消按钮
*/

#include "SelectDialog.h"
#include <QMessageBox>
#include <QgsMapLayer.h>
#include <qprogressbar.h>
KMeansSelect::KMeansSelect(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("K-Means聚类分析");
	ui.ctrlLabelX->setText("分析图层");
	ui.ctrlLabelY->setText("聚类数");
	ui.ctrlBtnConfirm->setText("运行");
	ui.ctrlComboBoxX->setMinimumWidth(130);
	ui.ctrlComboBoxX->clear();
	ui.ctrlComboBoxY->clear();
	// 将ui.ctrlComboBoxY替换为spinBox
	ui.ctrlSpinBox->setGeometry(ui.ctrlComboBoxY->geometry());
	ui.ctrlComboBoxY->hide();
	ui.ctrlSpinBox->show();
	ui.ctrlComboBoxCRS->show();
	ui.ctrlLabelCRS->show();
}

void KMeansSelect::setMapLayers(QList<QgsMapLayer *> liMapLayers)
{
	this->mliMapLayers = liMapLayers;
	addLayerItems();
}

void KMeansSelect::addLayerItems()
{
	for (int nIndex = 0; nIndex < mliMapLayers.count(); nIndex++)
	{
		// 判断是否为矢量图层
		if (mliMapLayers[nIndex]->type() != Qgis::LayerType::Vector)
		{
			continue;
		}
		ui.ctrlComboBoxX->addItem(mliMapLayers[nIndex]->name());
	}
}
QgsVectorLayer *KMeansSelect::getSelectLayer() const
{
	return mpvlSelectLayer;
}
int KMeansSelect::getClusterNum() const
{
	return mnClusterNum;
}
void KMeansSelect::on_ctrlBtnConfirm_clicked()
{
	QString qstrCurrentLayer = ui.ctrlComboBoxX->currentText();
	if (qstrCurrentLayer.isEmpty())
	{
		QMessageBox::critical(this, "error", "请选择图层");
		return;
	}
	// 根据图层名，找到对应矢量图层
	for (QgsMapLayer *layer : mliMapLayers)
	{
		if (layer->name() == qstrCurrentLayer)
		{
			mpvlSelectLayer = dynamic_cast<QgsVectorLayer *>(layer);
			mnClusterNum = ui.ctrlSpinBox->value();
			break;
		}
	}
	if (!mpvlSelectLayer)
	{
		QMessageBox::critical(this, "error", "未找到对应图层");
		return;
	}
	if (mpvlSelectLayer->geometryType() != Qgis::GeometryType::Point)
	{
		QMessageBox::critical(this, "error", "请选择点要素图层");
		return;
	}
	this->accept();
	//this->close();
}
void KMeansSelect::on_ctrlBtnCancel_clicked()
{
	this->close();
}