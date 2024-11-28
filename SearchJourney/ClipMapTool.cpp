#include "qgsapplication.h"
#include "qgsvectorlayer.h"
#include "ClipMapTool.h"

RectDrawingTool::RectDrawingTool(QgsMapCanvas* mapCanvas, QgsVectorLayer* targetLayer)
    : QgsMapTool(mapCanvas), mpTargetLayer(targetLayer)
{
}

void RectDrawingTool::canvasMoveEvent(QgsMapMouseEvent* e)
{
    if (e->buttons() != Qt::LeftButton)
        return;
    QRect rect;
    if (!mDrawActive)
    {
        mDrawActive = true;
        rect = QRect(e->pos(), e->pos());
    }
    else
    {
        rect = QRect(e->pos(), mInitDragPos);
    }
    if (mSelectionRubberBand)
        mSelectionRubberBand->setToCanvasRectangle(rect);
}

void RectDrawingTool::canvasPressEvent(QgsMapMouseEvent* e)
{
    if (!mSelectionRubberBand)
        initRubberBand();
    mInitDragPos = e->pos();
}

void RectDrawingTool::canvasReleaseEvent(QgsMapMouseEvent* e)
{
    QPoint point = e->pos() - mInitDragPos;
    //矩形
    if (mSelectionRubberBand && mDrawActive)
    {
        mClipGeometry = mSelectionRubberBand->asGeometry();
        mSelectionRubberBand.reset();
    }
    mDrawActive = false;
}

void RectDrawingTool::initRubberBand()
{
    mSelectionRubberBand.reset(new QgsRubberBand(mCanvas, Qgis::GeometryType::Polygon));
    mSelectionRubberBand->setFillColor(mFillColor);
    mSelectionRubberBand->setStrokeColor(mStrokeColor);
}

QgsGeometry RectDrawingTool::getGeometry() const {
    return mClipGeometry;
}






//-----------------------圆形-----------------------------------------------------------------
CircleDrawingTool::CircleDrawingTool(QgsMapCanvas* mapCanvas, QgsVectorLayer* targetLayer)
    : QgsMapTool(mapCanvas), mpTargetLayer(targetLayer)
{
}

void CircleDrawingTool::canvasMoveEvent(QgsMapMouseEvent* e)
{
    if (e->buttons() != Qt::LeftButton)
        return;
    QgsGeometry clipGeometry;
    if (!mDrawActive)
    {
        mDrawActive = true;
        mCenterPoint = toMapCoordinates(e->pos());
        clipGeometry = createCircleGeometry(mCenterPoint.toQPointF(), 0.1); // 更新圆形几何
    }
    else
    {
        // 如果正在绘制，动态更新半径并绘制圆形
        QgsPointXY edge = toMapCoordinates(e->pos());
        double radius = mCenterPoint.distance(edge); // 动态计算半径
        clipGeometry = createCircleGeometry(mCenterPoint.toQPointF(), radius); // 更新圆形几何
    }
    if (mSelectionRubberBand)
        mSelectionRubberBand->setToGeometry(clipGeometry);
}

void CircleDrawingTool::canvasPressEvent(QgsMapMouseEvent* e)
{
    if (!mSelectionRubberBand)
        initRubberBand();
    mInitDragPos = e->pos();
}

void CircleDrawingTool::canvasReleaseEvent(QgsMapMouseEvent* e)
{
    QPoint point = e->pos() - mInitDragPos;
    if (mSelectionRubberBand && mDrawActive)
    {
        mClipGeometry = mSelectionRubberBand->asGeometry();
        mSelectionRubberBand.reset();
    }
    mDrawActive = false;
}

QgsGeometry CircleDrawingTool::createCircleGeometry(const QPointF& center, double radius)
{
    const int segments = 60; // 圆的边数（近似圆）
    QVector<QgsPointXY> points;
    for (int i = 0; i < segments; ++i)
    {
        double angle = 2 * M_PI * i / segments; // 每段的角度
        double x = center.x() + radius * std::cos(angle);
        double y = center.y() + radius * std::sin(angle);
        points.append(QgsPointXY(x, y));
    }
    points.append(points.first()); // 闭合圆形

    QgsPolylineXY polyline = points;
    QgsPolygonXY polygon = { polyline };
    return QgsGeometry::fromPolygonXY(polygon);

}

void CircleDrawingTool::initRubberBand()
{
    qDebug() << "initRubberBand";
    mSelectionRubberBand.reset(new QgsRubberBand(mCanvas, Qgis::GeometryType::Polygon));
    mSelectionRubberBand->setFillColor(mFillColor);
    mSelectionRubberBand->setStrokeColor(mStrokeColor);
}

QgsGeometry CircleDrawingTool::getGeometry() const {
    return mClipGeometry;
}





//------------------------------多边形--------------------------------------------
PolygonDrawingTool::PolygonDrawingTool(QgsMapCanvas* mapCanvas, QgsVectorLayer* targetLayer)
    : QgsMapTool(mapCanvas), mpTargetLayer(targetLayer)
{
    initRubberBand();
}

void PolygonDrawingTool::canvasPressEvent(QgsMapMouseEvent* e)
{
    QgsGeometry clipGeometry;
    if (e->button() == Qt::LeftButton)  // 左键点击添加顶点
    {
        QgsPointXY point = toMapCoordinates(e->pos());  // 获取点击位置的地图坐标
        mVertices.append(point);  // 添加到顶点列表

        if (mVertices.size() > 1)
        {
            // 创建多边形的线段
            clipGeometry = QgsGeometry::fromPolygonXY({ mVertices });
            mSelectionRubberBand->setToGeometry(clipGeometry, nullptr);  // 更新RubberBand
        }
    }
    else if (e->button() == Qt::RightButton)  // 右键点击结束绘制
    {
        mDrawActive = false;  // 停止绘制
        if (mVertices.size() > 2)
        {
            // 完成多边形绘制，发出信号
            clipGeometry = QgsGeometry::fromPolygonXY({ mVertices });
            mClipGeometry = clipGeometry;
            emit sigPolygonDrawn(clipGeometry);
        }
        mVertices.clear();  // 清空顶点
        mSelectionRubberBand.reset();  // 清空RubberBand
    }
}

void PolygonDrawingTool::canvasMoveEvent(QgsMapMouseEvent* e)
{
    if (!mDrawActive)
        return;

    if (mVertices.isEmpty())
        return;

    QgsPointXY lastPoint = mVertices.last();
    QgsPointXY currentPoint = toMapCoordinates(e->pos());  // 当前鼠标位置

    // 动态更新RubberBand
    QVector<QgsPointXY> tempVertices = mVertices;
    tempVertices.append(currentPoint);

    QgsGeometry tempGeometry = QgsGeometry::fromPolygonXY({ tempVertices });
    mSelectionRubberBand->setToGeometry(tempGeometry, nullptr);
}

void PolygonDrawingTool::initRubberBand()
{
    mSelectionRubberBand.reset(new QgsRubberBand(mCanvas, Qgis::GeometryType::Polygon));
    mSelectionRubberBand->setColor(mStrokeColor);
    mSelectionRubberBand->setFillColor(mFillColor);
    mSelectionRubberBand->setWidth(2);
}

QgsGeometry PolygonDrawingTool::getGeometry() const
{
    return mClipGeometry;
}