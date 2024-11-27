#ifndef POINTEDIT_H
#define POINTEDIT_H

#include <QgsMapToolEdit.h>
#include <QgsPointXY.h>
#include <QgsVectorLayer.h>
#include <qstring.h>
#include <QUndoStack>
#include"GeometryEditTool.h"

class PointEdit : public QgsMapToolEdit
{
	Q_OBJECT
public:
	PointEdit(QgsMapCanvas* canvas);
	PointEdit(QgsMapCanvas* canvas, QgsVectorLayer* layer);
	~PointEdit();
	void addPoint(QgsMapMouseEvent* e);					// 添加点功能（地图点或注记）
	void addAnnotation(const QgsPointXY& position);		// 添加注记
	void setVectorLayer(QgsVectorLayer* layer);	// 设置外部传入的矢量图层
	void setGeometryEditTool(GeometryEditTool* tool);	// 设置几何体重做编辑工具

private:
	QString mPointType;									// 点的类型（地图点、注记等）
	void showAddPointDialog();							// 显示添加点类型选择对话框
	QgsVectorLayer* mVectorLayer;						// 外部传入的矢量图层
	GeometryEditTool* mGeometryEditTool;					// 几何体重做编辑工具

protected:
	void canvasPressEvent(QgsMapMouseEvent* e) override;	// 鼠标按下事件

signals:
	void pointAdded(QgsPointXY, QString);				// 发出信号：点位置和类型
	//void annotationAdded(QgsPointXY);					// 发出信号：注记位置
};

#endif // POINTEDIT-H

