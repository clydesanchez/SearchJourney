#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QgsMapToolEdit.h>
#include <QgsVectorLayer.h>
#include <QgsPointXY.h>
#include <QgsGeometry.h>
#include <QgsFeature.h>
#include <QList>
#include <QPolygonF>

class LineEdit : public QgsMapToolEdit
{
    Q_OBJECT
public:
    LineEdit(QgsMapCanvas* canvas, QgsVectorLayer* layer);
    ~LineEdit();

    // 剪断线功能
    void cutLine(const QList<QgsFeature>& selectedFeatures, const QgsPointXY& startPoint, const QgsPointXY& endPoint);
    // 光滑线功能
    void smoothLine(const QList<QgsFeature>& selectedFeatures);

protected:
    void canvasPressEvent(QgsMapMouseEvent* e) override;
    void canvasMoveEvent(QgsMapMouseEvent* e) override;
    void canvasReleaseEvent(QgsMapMouseEvent* e) override;

private:
    QgsVectorLayer* mLayer;
    bool mIsDrawing;
    QgsPointXY mStartPoint;
    QgsPointXY mEndPoint;

    // 光滑算法函数
    void applyBezierSmoothing(const QList<QgsFeature>& selectedFeatures);
    void applySplineSmoothing(const QList<QgsFeature>& selectedFeatures);
};

#endif // LINEEDIT_H
