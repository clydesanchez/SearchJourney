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
	StyleManager(QString strLayerName, Qgis::GeometryType layerType, MainWidget* widMain, QWidget *parent = nullptr);
	~StyleManager();

	void refreshStlView();
	void markerViewer(QListView* listView);
	void lineViewer(QListView* listView);
	void fillViewer(QListView* listView);
	/*void colorRmapViewer();*/
	void onTabChanged(int index);
	void onActionIptStl();
	void onActionStlExport(QListView* listView);
	void onActionStlApply(QListView* listView);

signals:
	void signalApplyStlSymbol(QString strLayerName, QgsSymbol* psSymbol);

private:
	Ui::StyleManagerClass ui;
	QList<CustomMarkerSymbol*> mMarkerSymbols; // 存储读取到的符号信息
	QList<CustomLineSymbol*> mLineSymbols;
	QList<CustomFillSymbol*> mFillSymbols;
	QString listViewMode;
	QString mStlLayerName;
	Qgis::GeometryType mStlLayerType;
};

