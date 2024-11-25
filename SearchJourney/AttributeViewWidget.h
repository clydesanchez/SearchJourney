#pragma once

#include <QWidget>
#include "ui_AttributeViewWidget.h"
#include "CustomAttributeTableView.h"
#include <qgsvectorlayer.h>
#include <qgsmapcanvas.h>
class AttributeViewWidget : public QWidget
{
	Q_OBJECT

public:
	AttributeViewWidget(QgsVectorLayer* curVecLayer,QgsMapCanvas* mapcanvas,QWidget *parent = nullptr);
	~AttributeViewWidget();
	void setModel(QgsAttributeTableFilterModel* attriModel);
public slots:
	void getIdx(QModelIndex pos);
	void WriteData(QString val);
	void addFeature(); // 添加要素
	void deleteFeature(); // 删除要素
	void addAttribute(); // 添加属性
	void deleteAttribute(); // 删除属性
	void changeEditMode();
private:
	Ui::AttributeViewWidgetClass ui;

	CustomAttributeTableView* mTableView;
	QgsVectorLayer* mCurVecLayer;
	QModelIndex mClickedIndex;
};
