/*
FileName: TextSelect.cpp
Author:RYB
Date:2024.7.25
Description:
	导入分隔符文本图层对话框的实现
Function Lists:
	1. TextSelect(QWidget *parent = nullptr) 构造函数
	2. ~TextSelect() 析构函数
	3. void addItems(QgsVectorLayer *pvlInput) 添加字段
	4. QString getNameX() const 获取X字段
	5. QString getNameY() const 获取Y字段
	6. QString getNameCRS() const 获取CRS字段
	7. void on_ctrlBtnConfirm_clicked() 确认按钮
	8. void on_ctrlBtnCancel_clicked() 取消按钮
*/

#include "SelectDialog.h"
#include <QMessageBox>
TextSelect::TextSelect(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("导入分隔符文本图层");
	ui.ctrlLabelX->setText("X字段");
	ui.ctrlLabelY->setText("Y字段");
	ui.ctrlComboBoxX->clear();
	ui.ctrlComboBoxY->clear();
	ui.ctrlSpinBox->hide();
	ui.ctrlComboBoxY->show();
	ui.ctrlComboBoxX->show();
	ui.ctrlComboBoxCRS->show();
	ui.ctrlLabelCRS->show();
}

TextSelect::~TextSelect()
{
}
QString TextSelect::getNameX() const
{
	return mqstrNameX;
}
QString TextSelect::getNameY() const
{
	return mqstrNameY;
}
QString TextSelect::getNameCRS() const
{
	return mqstrNameCRS;
}
void TextSelect::addItems(QgsVectorLayer *pvlInput)
{
	for (int nIndex = 0; nIndex < pvlInput->attributeList().count(); nIndex++)
	{
		ui.ctrlComboBoxX->addItem(pvlInput->attributeDisplayName(nIndex));
		ui.ctrlComboBoxY->addItem(pvlInput->attributeDisplayName(nIndex));
	}
}

void TextSelect::on_ctrlBtnConfirm_clicked()
{
	mqstrNameX = ui.ctrlComboBoxX->currentText();
	mqstrNameY = ui.ctrlComboBoxY->currentText();
	mqstrNameCRS = ui.ctrlComboBoxCRS->currentText();
	if (mqstrNameX.isEmpty() || mqstrNameY.isEmpty() || mqstrNameCRS.isEmpty())
	{
		QMessageBox::critical(this, "error", "请选择经纬度字段");
		return;
	}
	this->close();
}
void TextSelect::on_ctrlBtnCancel_clicked()
{
	this->close();
}