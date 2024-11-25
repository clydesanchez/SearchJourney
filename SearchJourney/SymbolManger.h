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
#include "MainWidget.h"
class SvgTableModel : public QAbstractTableModel {
	Q_OBJECT

public:
	SvgTableModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {};
	// 设置模型数据
	void setSvgFiles(const QStringList& filePaths);
	// 返回行数
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	// 返回列数
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	// 返回每个单元格的数据
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole)const override;
	// 设置每个单元格的标头
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
private:
	QStringList mSvgFiles;
public:
	// 单元格点击事件
	void onCellClicked(const QModelIndex& index);
signals:
	void signalCellClicked(const QString& filePath);
};

class SymbolManger : public QWidget
{
	Q_OBJECT

public:
	SymbolManger(QString strLayerName, Qgis::GeometryType layerType, MainWidget* widMain, QgsSymbolList Srcsymbol,QWidget *parent = nullptr);
	~SymbolManger();
private:
	QFileSystemModel* mfsmModel;
	SvgTableModel* mSvgTableModel;

	QString mstrLayerName;
	Qgis::GeometryType mLayerType;

	QColor mFillColor = Qt::red;
	QColor mStrokeColor = Qt::black;
	double mStrokeWidth=0.3;
	int mPenStyle = Qt::SolidLine;
private:
	void previewSymbol();
	void setSymbolByLayerType(Qgis::GeometryType layerType, QgsSymbol* psSymbol, QString svgPath);
signals:
	void signalApplySymbol(QString strLayerName,QgsSymbol* psSymbol);
private slots:
	void onDirectoryClicked(const QModelIndex& index);// 点击文件夹时的槽函数
public slots:
	void slotCellClicked(const QString& filePath);// 单元格点击事件
	void getSelectFilleColor(QColor fillcolor);// 获取填充颜色
	void getSelectStrokeColor(QColor StrokeColor);// 获取边界颜色
	void getSelectStrokeWidth(double strokewidth);// 获取边界宽度
	void getSelecctPenStyle(int penstyle);// 获取边界样式
	// 应用按钮
	void onConfirmBtnClicked();
private:
	Ui::SymbolMangerClass ui;
	QgsColorButton* mctrlFillColorBtn;
	QgsColorButton* mctrlStrokeColorBtn;
};
#endif