/*
FileName: FileSaveSelect.cpp
Author:RYB
Date:2024.7.28
Description:
	保存文件对话框的实现
Function Lists:
	1. FileSaveSelect(QWidget *parent = nullptr) 构造函数
	2. void setMapLayers(QList<QgsMapLayer *> liMapLayers) 设置图层
	3. void addLayerItems() 在ComboBox中添加图层
	4. QString getFilePath() const 获取文件路径
	5. QgsVectorLayer *getSelectLayer() const 获取选择的图层
	6. void on_ctrlBtnConfirm_clicked() 确认按钮
	7. void on_ctrlBtnCancel_clicked() 取消按钮
	8. void on_toolbtn_pressed() 选择文件路径
*/

#include "SelectDialog.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QlineEdit>
#include <QToolButton>
FileSaveSelect::FileSaveSelect(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	this->setWindowTitle("保存文件");
	ui.ctrlLabelX->setText("保存图层");
	ui.ctrlLabelY->setText("保存路径");
	ui.ctrlBtnConfirm->setText("保存");
	ui.ctrlComboBoxX->clear();
	ui.ctrlComboBoxY->clear();

	ui.ctrlSpinBox->hide();
	ui.ctrlComboBoxCRS->hide();
	ui.ctrlLabelCRS->hide();
	ui.ctrlComboBoxY->hide();

	ui.ctrlComboBoxX->show();
	// 新增文件路径显示控件(lineedit + toolButton)
	mpctrlFilePath = new QLineEdit(this);
	mpctrlFilePath->setGeometry(ui.ctrlComboBoxY->geometry());
	QToolButton *pctrlFilePath = new QToolButton(this);
	pctrlFilePath->setText("...");
	pctrlFilePath->setGeometry(ui.ctrlComboBoxY->geometry().adjusted(ui.ctrlComboBoxY->width() - 20, 0, 0, 0));
	mpctrlFilePath->show();
	pctrlFilePath->show();
	connect(pctrlFilePath, &QToolButton::pressed, this, &FileSaveSelect::on_toolbtn_pressed);
}

void FileSaveSelect::setMapLayers(QList<QgsMapLayer*> liMapLayers)
{
	this->mliMapLayers = liMapLayers;
	addLayerItems();
}
void FileSaveSelect::addLayerItems()
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

QString FileSaveSelect::getFilePath() const
{
	return mpctrlFilePath->text();
}

QgsVectorLayer* FileSaveSelect::getSelectLayer() const
{
	return mpvlSelectLayer;
}

void FileSaveSelect::on_ctrlBtnConfirm_clicked()
{
	mqstrFilePath = ui.ctrlComboBoxY->currentText();
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
			mpvlSelectLayer = dynamic_cast<QgsVectorLayer*>(layer);
			break;
		}
	}
	this->close();
}

void FileSaveSelect::on_ctrlBtnCancel_clicked()
{
	this->close();
}

void FileSaveSelect::on_toolbtn_pressed()
{
	mqstrFilePath = QFileDialog::getSaveFileName(this, tr("保存图层到"), "", tr("Shapefile (*.shp)"));
	mpctrlFilePath->setText(mqstrFilePath);
}