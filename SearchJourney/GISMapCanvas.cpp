/*
FileName: GISMapCanvas.cpp
Author:RYB
Date:2024.7.25
Description:
	地图画布类的实现
Function Lists:
	1. GISMapCanvas(QWidget *parent = nullptr) 构造函数
	2. ~GISMapCanvas() 析构函数
	3. void mouseMoveEvent(QMouseEvent *event) 鼠标移动事件
	4. void setStatusBar(QStatusBar *qsbStatusBar) 设置状态栏
*/

#include "GISMapCanvas.h"
#include <QLabel>
#include <QMessageBox>
GISMapCanvas::GISMapCanvas(QWidget *parent = (QWidget *)nullptr)
	: QgsMapCanvas(parent)
{
	QgsMapCanvas::QgsMapCanvas(parent);
	this->setAcceptDrops(true);
}

// 复制构造函数
GISMapCanvas::GISMapCanvas(GISMapCanvas* srcCanvas)
	: QgsMapCanvas(nullptr)
{
	QgsMapCanvas::QgsMapCanvas(nullptr);
	this->setLayers(srcCanvas->layers());
	this->setExtent(srcCanvas->extent());
	this->refresh();
}

GISMapCanvas::~GISMapCanvas()
{
	// QgsMapCanvas::~QgsMapCanvas();
}

void GISMapCanvas::mouseMoveEvent(QMouseEvent *event)
{
	// 调用基类函数
	QgsMapCanvas::mouseMoveEvent(event);
	if (qsbMainWin != nullptr) {
		// 读取鼠标位置处的坐标
		QgsPointXY point = this->getCoordinateTransform()->toMapCoordinates(event->pos().x(), event->pos().y());
		// 保留7位小数
		QString qstrTransX = QString::number(point.x(), 'f', 7);
		QString qstrTransY = QString::number(point.y(), 'f', 7);
		// 显示在状态栏
		qsbMainWin->showMessage(QString("X: %1, Y: %2").arg(qstrTransX).arg(qstrTransY));
		if (event->buttons() == Qt::LeftButton)
		{

			emit moved(event->pos());
		}
	}
}

void GISMapCanvas::setStatusBar(QStatusBar *qsbStatusBar)
{
	qsbMainWin = qsbStatusBar;
}

void GISMapCanvas::mousePressEvent(QMouseEvent* event)
{
	// 调用基类函数
	QgsMapCanvas::mousePressEvent(event);
	if (event->button() == Qt::LeftButton) {
		emit pressed(event->pos());
	}
}

void GISMapCanvas::mouseReleaseEvent(QMouseEvent* event)
{
	// 调用基类函数
	QgsMapCanvas::mouseReleaseEvent(event);
	if (event->button() == Qt::LeftButton) {
		emit released(event->pos());

	}
}




void GISMapCanvas::dropEvent(QDropEvent *event)
{
	// 调用基类函数
	QgsMapCanvas::dropEvent(event);
}

void GISMapCanvas::dragEnterEvent(QDragEnterEvent *event)
{
	// 调用基类函数
	QgsMapCanvas::dragEnterEvent(event);

	event->acceptProposedAction();
}

void GISMapCanvas::dragMoveEvent(QDragMoveEvent *event)
{
	// 调用基类函数
	QgsMapCanvas::dragMoveEvent(event);

}
