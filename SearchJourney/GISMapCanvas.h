/*
FileName: GISMapCanvas.h
Author:RYB
Date:2024.7.25
Description:
	地图画布类的声明
*/

#ifndef GISMAPCANVAS_H
#define GISMAPCANVAS_H

#include <qgsmapcanvas.h>
#include <QWidget>
#include <QPointF>
#include <QStatusBar>
#include <QMenu>
class GISMapCanvas : public QgsMapCanvas
{
	Q_OBJECT

public:
	GISMapCanvas(QWidget *parent);
	~GISMapCanvas();

public:
	void mouseMoveEvent(QMouseEvent *event) override;
	void setStatusBar(QStatusBar *qsbStatusBar);
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void dropEvent(QDropEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	
private:
	QStatusBar *qsbMainWin = nullptr; // 主窗口状态栏
signals:
	void pressed(const QPoint& pos);
	void released(const QPoint& pos);
	void moved(const QPoint& pos);
};
#endif