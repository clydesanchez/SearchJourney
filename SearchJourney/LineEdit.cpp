#include "LineEdit.h"
#include <QgsMapCanvas.h>
#include <QgsGeometry.h>
#include <QgsFeature.h>
#include <QgsMapMouseEvent.h>
#include <QgsPointXY.h>
#include <QgsSpatialIndex.h>
#include <QgsPointLocator.h>
#include <QPainterPath>
#include <QVector>
#include <QComboBox>
#include <QPushButton>
#include <QDialog>
#include <QVBoxLayout>

// 构造函数
LineEdit::LineEdit(QgsMapCanvas* canvas, QgsVectorLayer* layer)
    : QgsMapToolEdit(canvas), mLayer(layer), mIsDrawing(false)
{
}

// 析构函数
LineEdit::~LineEdit() {}

// 光滑线功能
void LineEdit::smoothLine(const QList<QgsFeature>& selectedFeatures)
{
    // 创建光滑算法选择对话框
    QDialog dialog;
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 创建下拉列表
    QComboBox* comboBox = new QComboBox(&dialog);
    comboBox->addItem("Bezier");  // 贝塞尔曲线
    comboBox->addItem("Spline");  // 样条曲线
    layout->addWidget(comboBox);

    // 创建确定按钮
    QPushButton* okButton = new QPushButton("OK", &dialog);
    layout->addWidget(okButton);

    // 连接按钮点击事件，关闭对话框
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    // 显示对话框并等待用户选择
    if (dialog.exec() == QDialog::Accepted) {
        QString method = comboBox->currentText();  // 获取选择的光滑算法
        if (method == "Bezier") {
            applyBezierSmoothing(selectedFeatures);  // 应用贝塞尔曲线光滑
        }
        else if (method == "Spline") {
            applySplineSmoothing(selectedFeatures);  // 应用样条曲线光滑
        }
    }
}

// 贝塞尔曲线光滑算法
void LineEdit::applyBezierSmoothing(const QList<QgsFeature>& selectedFeatures)
{
    QVector<QPointF> points;

    // 获取所有选中图元的点
    for (const QgsFeature& feature : selectedFeatures)
    {
        QgsGeometry geom = feature.geometry();
        if (geom.isMultipart())
        {
            const QVector<QgsPointXY>& multiPoints = geom.asMultiPoint();  // 获取多点几何
            for (const QgsPointXY& pt : multiPoints)
            {
                points.append(QPointF(pt.x(), pt.y()));
            }
        }
        else
        {
            const QgsPointXY& pt = geom.asPoint();
            points.append(QPointF(pt.x(), pt.y()));
        }
    }

    // 如果有足够的点进行贝塞尔曲线平滑
    if (points.size() >= 4)
    {
        // 使用 QPainterPath 进行贝塞尔光滑
        QPainterPath path;
        path.moveTo(points[0]);

        for (int i = 1; i < points.size() - 2; i += 3)
        {
            path.cubicTo(points[i], points[i + 1], points[i + 2]);
        }

        // 将 QPainterPath 转换为 QgsPolylineXY
        QPolygonF polygon = path.toSubpathPolygons().first();  // 获取路径的第一个子路径
        QVector<QgsPointXY> smoothedPoints;
        for (const QPointF& pt : polygon)
        {
            smoothedPoints.append(QgsPointXY(pt.x(), pt.y()));  // 转换为 QgsPointXY
        }

        // 创建新的几何对象
        QgsGeometry smoothedGeometry = QgsGeometry::fromPolylineXY(smoothedPoints);

        // 创建新的要素
        QgsFeature smoothedFeature;
        smoothedFeature.setGeometry(smoothedGeometry);

        // 将光滑后的要素添加到图层中
        mLayer->dataProvider()->addFeature(smoothedFeature);
        mLayer->triggerRepaint();  // 刷新图层
    }
}

// 样条曲线光滑算法
void LineEdit::applySplineSmoothing(const QList<QgsFeature>& selectedFeatures)
{
    QVector<QPointF> points;

    // 获取所有选中图元的点
    for (const QgsFeature& feature : selectedFeatures)
    {
        QgsGeometry geom = feature.geometry();
        if (geom.isMultipart())
        {
            const QVector<QgsPointXY>& multiPoints = geom.asMultiPoint();  // 获取多点几何
            for (const QgsPointXY& pt : multiPoints)
            {
                points.append(QPointF(pt.x(), pt.y()));
            }
        }
        else
        {
            const QgsPointXY& pt = geom.asPoint();
            points.append(QPointF(pt.x(), pt.y()));
        }
    }

    // 使用 Catmull-Rom 样条插值光滑曲线
    QVector<QPointF> smoothedPoints;
    int numPoints = points.size();
    if (numPoints < 4)
        return;  // 样条需要至少4个点

    // 对于每4个点，使用 Catmull-Rom 样条插值
    for (int i = 1; i < numPoints - 2; ++i)
    {
        QPointF p0 = points[i - 1];
        QPointF p1 = points[i];
        QPointF p2 = points[i + 1];
        QPointF p3 = points[i + 2];

        // 使用 Catmull-Rom 插值计算新的点
        for (float t = 0.0f; t <= 1.0f; t += 0.1f)
        {
            float tt = t * t;
            float ttt = tt * t;
            QPointF smoothedPoint =
                0.5f * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * tt + (-p0 + 3 * p1 - 3 * p2 + p3) * ttt);
            smoothedPoints.append(smoothedPoint);
        }
    }

    // 创建新的几何对象
    QVector<QgsPointXY> smoothedGeometryPoints;
    for (const QPointF& pt : smoothedPoints)
    {
        smoothedGeometryPoints.append(QgsPointXY(pt.x(), pt.y()));
    }

    QgsGeometry smoothedGeometry = QgsGeometry::fromPolylineXY(smoothedGeometryPoints);

    // 创建新的要素
    QgsFeature smoothedFeature;
    smoothedFeature.setGeometry(smoothedGeometry);

    // 将光滑后的要素添加到图层中
    mLayer->dataProvider()->addFeature(smoothedFeature);
    mLayer->triggerRepaint();  // 刷新图层
}


// 剪断线功能
void LineEdit::cutLine(const QList<QgsFeature>& selectedFeatures, const QgsPointXY& startPoint, const QgsPointXY& endPoint)
{
    // 获取当前选中的图元
    for (const QgsFeature& feature : selectedFeatures)
    {
        QgsGeometry geometry = feature.geometry();
        QgsGeometry cutGeometry = geometry.intersection(QgsGeometry::fromPolylineXY({ startPoint, endPoint }));

        if (cutGeometry.isGeosValid() && !cutGeometry.isEmpty())
        {
            // 分割线：假设交点分割的线是一个新的几何对象
            QgsFeature newFeature;
            newFeature.setGeometry(cutGeometry);
            mLayer->dataProvider()->addFeature(newFeature);  // 将新图元添加到图层
        }
    }
}

// 鼠标按下事件：开始绘制剪断线
void LineEdit::canvasPressEvent(QgsMapMouseEvent* e)
{
    if (!mIsDrawing)
    {
        mStartPoint = e->mapPoint();
        mIsDrawing = true;
    }
}

// 鼠标移动事件：绘制剪断线的临时线
void LineEdit::canvasMoveEvent(QgsMapMouseEvent* e)
{
    if (mIsDrawing)
    {
        mEndPoint = e->mapPoint();

        // 创建 QPainter 对象，传入 mCanvas（QgsMapCanvas 继承自 QPaintDevice）
        QPainter painter(mCanvas);  // 直接使用 mCanvas，QgsMapCanvas 是 QPaintDevice 的子类

        // 设置绘制颜色和线宽
        painter.setPen(QPen(Qt::red, 2));

        // 绘制从起点到当前鼠标位置的临时剪断线
        painter.drawLine(mStartPoint.x(), mStartPoint.y(), mEndPoint.x(), mEndPoint.y());

        // 刷新画布以显示剪断线的临时预览
        mCanvas->refresh();
    }
}



// 鼠标释放事件：完成绘制剪断线
void LineEdit::canvasReleaseEvent(QgsMapMouseEvent* e)
{
    if (mIsDrawing)
    {
        mEndPoint = e->mapPoint();
        cutLine(QList<QgsFeature>(), mStartPoint, mEndPoint);  // 调用剪断线的函数
        mIsDrawing = false;
    }
}
