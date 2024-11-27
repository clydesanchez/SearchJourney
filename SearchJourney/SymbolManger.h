#pragma once
#ifndef SYMBOLMANGER_H
#define SYMBOLMANGER_H

#include <QWidget>
#include "ui_SymbolManger.h"
#include <QFileSystemModel>
#include <QAbstractTableModel>
#include <qgsvectorlayer.h>
#include <qgscolorbutton.h>
#include <qgscolordialog.h>
#include <qgssymbolselectordialog.h>
#include <QDockWidget>
#include "MainWidget.h"
// 符号管理窗口
class SymbolManger : public QDockWidget
{
	Q_OBJECT

public:
	SymbolManger(QgsVectorLayer* pvLayer, MainWidget* widMain, QgsSymbolList Srcsymbol,QWidget *parent = nullptr);
	~SymbolManger();
private:
	QFileSystemModel* mfsmModel;// 文件系统模型
	
	QgsVectorLayer* mpvLayer;// 图层
	QString mstrLayerName;// 图层名称
	Qgis::GeometryType mLayerType;// 图层类型

	QColor mFillColor = Qt::red;// 填充颜色
	QColor mStrokeColor = Qt::black;// 边界颜色
	double mStrokeWidth=0.3;// 边界宽度
	int mPenStyle = Qt::SolidLine;// 边界样式
	double mOpacity = 1.0;// 透明度
private:
	// 预览符号
	void previewSymbol();
	// 根据图层类型设置svg符号
	void setSymbolByLayerType(Qgis::GeometryType layerType, QgsSymbol* psSymbol, QString svgPath);
	// 窗口大小改变事件
	void resizeEvent(QResizeEvent* event) override;
signals:
	void signalApplySymbol(QString strLayerName,QgsSymbol* psSymbol);
	//void signalApplyMark(QString strLayerName,QgsVectorLayerSimpleLabeling* pMark);
	void signalApplyMark(QString strLayerName, QgsPalLayerSettings settings);
private slots:
	void onDirectoryClicked(const QModelIndex& index);// 点击文件夹时的槽函数
public slots:
	void slotCellClicked(const QString& filePath);// 单元格点击事件
	void getSelectFilleColor(QColor fillcolor);// 获取填充颜色
	void getSelectStrokeColor(QColor StrokeColor);// 获取边界颜色
	void getSelectStrokeWidth(double strokewidth);// 获取边界宽度
	void getSelecctPenStyle(int penstyle);// 获取边界样式
	void getSelectOpacity(double opacity);// 获取透明度

	// 应用按钮
	void onConfirmBtnClicked();
	void onConfirmBtnClicked_Mark();
	// 响应svg符号选择
	void onSymbolSelect(QString strSvgPath);
	// 更换符号类型
	void onSymbolTypeChanged(int nIndex);
private:
	Ui::SymbolMangerClass ui;
	QgsColorButton* mctrlFillColorBtn;
	QgsColorButton* mctrlStrokeColorBtn;
	QgsColorButton* mctrlStrokeColorBtn_svg;
	QgsColorButton* mctrlFontColor;
};
#endif