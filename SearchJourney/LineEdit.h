#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QgsMapToolEdit.h>
#include <QgsVectorLayer.h>
#include <QgsPointXY.h>
#include <QgsGeometry.h>
#include <QgsFeature.h>
#include <QgsRubberBand.h>
#include <QList>

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

signals:
    /**
     * @brief 分割线绘制完成信号
     * @param startPoint 分割线起点
     * @param endPoint 分割线终点
     */
    void lineCutSignal(const QgsPointXY& startPoint, const QgsPointXY& endPoint);

protected:
    void canvasPressEvent(QgsMapMouseEvent* e) override;
    void canvasMoveEvent(QgsMapMouseEvent* e) override;
    //void canvasReleaseEvent(QgsMapMouseEvent* e) override;

private:
    QgsVectorLayer* mLayer;                 // 当前操作的图层
    QgsRubberBand* mTempRubberBand;         // 用于动态绘制分割线
    QList<QgsFeature> mSelectedFeatures;   // 当前选中的要素
    bool mIsDrawing;                        // 是否正在绘制
    QgsPointXY mStartPoint;                 // 分割线起点
    QgsPointXY mEndPoint;                   // 分割线终点

    // 光滑算法函数
    void applyBezierSmoothing(const QList<QgsFeature>& selectedFeatures);
    void applySplineSmoothing(const QList<QgsFeature>& selectedFeatures);
};


QgsVectorLayer* polygonToLines(QgsVectorLayer* polygonLayer, const QString& outputLineLayerPath);

#endif // LINEEDIT_H
