/*
FileName: SelectDialog.h
Author:RYB
Date:2024.7.25
Description:
	声明导入分隔符文本图层对话框
	声明kmeans聚类分析对话框
	声明空间连接对话框
	声明栅格统计对话框
	声明文件保存对话框
*/

#ifndef SELECTDIALOG_H
#define SELECTDIALOG_H

#include <QDialog>
#include "ui_TextSelect.h"
#include <Qgsvectorlayer.h>
#include <Qgsrasterlayer.h>
#include <qprogressbar.h>
// 导入分隔符文本图层 选择字段窗口
class TextSelect : public QDialog
{
	Q_OBJECT

public:
	TextSelect(QWidget *parent = nullptr);
	~TextSelect();

public:
	void addItems(QgsVectorLayer *pvlInput);
	QString getNameX() const;
	QString getNameY() const;
	QString getNameCRS() const;
public slots:
	void on_ctrlBtnConfirm_clicked();
	void on_ctrlBtnCancel_clicked();

private:
	QString mqstrNameX;
	QString mqstrNameY;
	QString mqstrNameCRS;

private:
	Ui::TextSelectClass ui;
};
// kmeans聚类分析 选择图层窗口
class KMeansSelect : public QDialog
{
	Q_OBJECT

public:
	KMeansSelect(QWidget *parent = nullptr);
	~KMeansSelect() = default;

private:
	Ui::TextSelectClass ui;

public:
	void addLayerItems();
	void setMapLayers(QList<QgsMapLayer *> liMapLayers);
	QgsVectorLayer *getSelectLayer() const;
	int getClusterNum() const;
private:
	QProgressBar* mProgressBar; // 进度条
	int mnClusterNum = 0;
	QList<QgsMapLayer *> mliMapLayers;
	QgsVectorLayer *mpvlSelectLayer = nullptr;
public slots:
	void on_ctrlBtnConfirm_clicked();
	void on_ctrlBtnCancel_clicked();
};

// 空间连接 选择图层窗口
class ConnectSelect : public QDialog
{
	Q_OBJECT
public:
	ConnectSelect(QWidget *parent = nullptr);
	~ConnectSelect() = default;

private:
	Ui::TextSelectClass ui;

public:
	void addLayerItems();
	void setMapLayers(QList<QgsMapLayer *> liMapLayers);
	QgsVectorLayer *getSelectLayerResult() const;
	QgsVectorLayer *getSelectLayerCompare() const;
	QString getConnectType() const;

private:
	QgsVectorLayer *mpvlSelectLayerResult = nullptr;
	QgsVectorLayer *mpvlSelectLayerCompare = nullptr;
	QList<QgsMapLayer *> mliMapLayers;
	QString mqstrConnectType = "";
public slots:
	void on_ctrlBtnConfirm_clicked();
	void on_ctrlBtnCancel_clicked();
};
// 栅格统计 选择图层窗口
class RasterStatisticsSelect : public QDialog
{
	Q_OBJECT
public:
	RasterStatisticsSelect(QWidget *parent = nullptr);
	~RasterStatisticsSelect() = default;

private:
	Ui::TextSelectClass ui;

public:
	void addLayerItems();
	void setMapLayers(QList<QgsMapLayer *> liMapLayers);
	QgsRasterLayer *getSelectLayer() const;
	int getBandID() const;

private:
	int mnBandID = 0;
	QList<QgsMapLayer *> mliMapLayers;
	QgsRasterLayer *mpvlSelectLayer = nullptr;
public slots:
	void on_ctrlBtnConfirm_clicked();
	void on_ctrlBtnCancel_clicked();
	void on_ctrlComboBoxX_currentIndexChanged(int nIndex);
};
// 文件保存对话框
class FileSaveSelect : public QDialog
{
	Q_OBJECT
public:
	FileSaveSelect(QWidget *parent = nullptr);
	~FileSaveSelect() = default;

private:
	Ui::TextSelectClass ui;

public:
	void addLayerItems();
	void setMapLayers(QList<QgsMapLayer *> liMapLayers);
	QString getFilePath() const;
	QgsVectorLayer *getSelectLayer() const;

private:
	QLineEdit *mpctrlFilePath = nullptr;

private:
	QString mqstrFilePath;
	QList<QgsMapLayer *> mliMapLayers;
	QgsVectorLayer *mpvlSelectLayer = nullptr;
public slots:
	void on_ctrlBtnConfirm_clicked();
	void on_ctrlBtnCancel_clicked();
	void on_toolbtn_pressed();
};
#endif