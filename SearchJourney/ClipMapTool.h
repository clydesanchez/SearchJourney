#pragma once
#pragma once
#include <QObject>
#include <QList>
#include <QScopedPointer>
#include <qmath.h>
#include <QgsPolygon.h>
#include <qgsmapcanvas.h>
#include <qgsmaptool.h>
#include <qgsrubberband.h>
#include <qgsmapmouseevent.h>
#include <qgsvectorlayer.h>
//#include <>
#include <qgsmaptoolidentify.h>

class RectDrawingTool : public QgsMapTool
{
    Q_OBJECT
public:
    RectDrawingTool(QgsMapCanvas* mapCanvas, QgsVectorLayer* targetLayer);

    QgsGeometry getGeometry() const;

protected:
    // 重写鼠标移动
    void canvasMoveEvent(QgsMapMouseEvent* e) override;
    // 重写鼠标按下
    void canvasPressEvent(QgsMapMouseEvent* e) override;
    // 重写鼠标抬起
    void canvasReleaseEvent(QgsMapMouseEvent* e) override;

    void initRubberBand();

private:
    bool mDrawActive = false;

    // 是否正在选择
    QScopedPointer<QgsRubberBand> mSelectionRubberBand;
    QColor mFillColor = QColor(254, 178, 76, 63);
    QColor mStrokeColor = QColor(254, 58, 29, 100);
    QPoint mInitDragPos;

    // 选择的Geometry
    QgsGeometry mClipGeometry;
    QgsVectorLayer* mpTargetLayer;
};




class CircleDrawingTool : public QgsMapTool
{
    Q_OBJECT
public:
    CircleDrawingTool(QgsMapCanvas* mapCanvas, QgsVectorLayer* targetLayer);

    QgsGeometry getGeometry() const;

protected:
    // 重写鼠标移动
    void canvasMoveEvent(QgsMapMouseEvent* e) override;
    // 重写鼠标按下
    void canvasPressEvent(QgsMapMouseEvent* e) override;
    // 重写鼠标抬起
    //void canvasReleaseEvent(QgsMapMouseEvent* e) override;

    void initRubberBand();
    QgsGeometry createCircleGeometry(const QPointF& center, double radius);

    void canvasReleaseEvent(QgsMapMouseEvent* e);

private:
    bool mDrawActive = false;

    // 是否正在选择
    QScopedPointer<QgsRubberBand> mSelectionRubberBand;
    QColor mFillColor = QColor(254, 178, 76, 63);
    QColor mStrokeColor = QColor(254, 58, 29, 100);
    QPoint mInitDragPos;

    QgsPointXY mCenterPoint;

    // 选择的Geometry
    QgsGeometry mClipGeometry;
    QgsVectorLayer* mpTargetLayer;
};





class PolygonDrawingTool : public QgsMapTool
{
    Q_OBJECT
public:
    PolygonDrawingTool(QgsMapCanvas* mapCanvas, QgsVectorLayer* targetLayer);

    QgsGeometry getGeometry() const;

protected:
    // 重写鼠标移动
    void canvasMoveEvent(QgsMapMouseEvent* e) override;
    // 重写鼠标按下
    void canvasPressEvent(QgsMapMouseEvent* e) override;

    void initRubberBand();

signals:
    // 当绘制完成后，发出信号
    void sigPolygonDrawn(const QgsGeometry& geometry);

private:
    bool mDrawActive = false;

    QVector<QgsPointXY> mVertices;  // 存储多边形的顶点

    // 是否正在选择
    QScopedPointer<QgsRubberBand> mSelectionRubberBand;
    QColor mFillColor = QColor(254, 178, 76, 63);
    QColor mStrokeColor = QColor(254, 58, 29, 100);
    QPoint mInitDragPos;

    // 选择的Geometry
    QgsGeometry mClipGeometry;
    QgsVectorLayer* mpTargetLayer;
};