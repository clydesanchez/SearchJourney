#pragma once

#include "MainWidget.h"
#include <QMainWindow>
#include "ui_StyleManager.h"
#include <qgssymbol.h>
#include <qgsmarkersymbollayer.h>
#include <qgsmarkersymbol.h>
#include <qgslinesymbol.h>
#include <qgsfillsymbol.h>
#include <qgssymbol.h>
#include <QStandardItemModel>
#include <QIcon>
#include <QPixmap>
#include <QListView>
#include "CustomSymbol.h"

class StyleManager : public QMainWindow
{
	Q_OBJECT

public:
	StyleManager(QString strLayerName, Qgis::GeometryType layerType, MainWidget* widMain, QgsVectorLayer* veclayer, QWidget *parent = nullptr);
	~StyleManager();

	// 刷新符号库
	void refreshStlView();
	// 符号显示
	void markerViewer(QListView* listView);
	void lineViewer(QListView* listView);
	void fillViewer(QListView* listView);
	// 符号库切换
	void onTabChanged(int index);
	// 符号导入
	void onActionIptStl();
	// 符号导出
	void onActionStlExport(QListView* listView);
    // 符号应用
	void onActionStlApply(QListView* listView);

signals:
	void signalApplyStlSymbol(QString strLayerName, QgsSymbol* psSymbol);

private:
	Ui::StyleManagerClass ui;
	QList<CustomMarkerSymbol*> mMarkerSymbols; 
	QList<CustomLineSymbol*> mLineSymbols;
	QList<CustomFillSymbol*> mFillSymbols;
	QString mListViewMode;
	QString mStlLayerName;
	Qgis::GeometryType mStlLayerType;
	QgsVectorLayer* mVeclayer;
};

