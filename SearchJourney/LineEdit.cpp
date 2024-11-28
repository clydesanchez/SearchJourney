#include "LineEdit.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QPainterPath>
#include <QgsMapMouseEvent.h>
#include <QgsGeometry.h>
#include <QgsAbstractGeometry.h>
#include <QgsPoint.h> 
#include <ogrsf_frmts.h> // GDAL 几何操作的头文件
#include <QgsvectorLayer.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsVectorFileWriter.h>
#include <QgsProject.h>


LineEdit::LineEdit(QgsMapCanvas* canvas, QgsVectorLayer* layer)
    : QgsMapToolEdit(canvas), mLayer(layer), mTempRubberBand(nullptr), mIsDrawing(false)
{
    // 初始化 RubberBand 用于动态绘制分割线
    mTempRubberBand = new QgsRubberBand(canvas, Qgis::GeometryType::Line);
    mTempRubberBand->setColor(Qt::red);  // 红色分割线
    mTempRubberBand->setWidth(2);        // 线宽
}

LineEdit::~LineEdit()
{
}

OGRGeometry* createSplitLine(const QgsPointXY& startPoint, const QgsPointXY& endPoint)
{
    // 使用 OGR API 创建裁剪线
    OGRGeometry* splitLine = nullptr;
    OGRLineString* lineString = new OGRLineString();

    // 添加点到裁剪线
    lineString->addPoint(startPoint.x(), startPoint.y());
    lineString->addPoint(endPoint.x(), endPoint.y());

    // 验证几何有效性
    if (!lineString->IsValid())
    {
        qDebug() << "Split line geometry is invalid. Attempting to make it valid.";
        OGRGeometry* validLine = lineString->MakeValid();
        OGRGeometryFactory::destroyGeometry(lineString);
        lineString = dynamic_cast<OGRLineString*>(validLine);
    }

    // 检查修复后的几何是否有效
    if (!lineString || !lineString->IsValid())
    {
        qDebug() << "Failed to create a valid split line geometry.";
        OGRGeometryFactory::destroyGeometry(lineString);
        return nullptr;
    }

    splitLine = lineString;
    return splitLine;
}

// 转换函数
QgsPolylineXY toPolylineXY(const QgsPointSequence& pointSequence)
{
    QgsPolylineXY polyline;
    for (const QgsPoint& point : pointSequence)
    {
        polyline.push_back(QgsPointXY(point)); // 转换为 QgsPointXY
    }
    return polyline;
}

// 剪断线功能
void LineEdit::cutLine(const QList<QgsFeature>& selectedFeatures, const QgsPointXY& startPoint, const QgsPointXY& endPoint)
{
    // 创建裁剪线 WKT
     // 创建裁剪线
    OGRGeometry* splitLine = createSplitLine(startPoint, endPoint);
    if (!splitLine)
    {
        qDebug() << "Failed to create split line.";
        return;
    }
    for (const QgsFeature& feature : selectedFeatures)
    {
        QgsGeometry featureGeometry = feature.geometry();
        if (featureGeometry.isEmpty())
        {
            qDebug() << "Skipping empty feature geometry.";
            continue;
        }

        // 转换目标几何到 GDAL
        OGRGeometry* targetGeometry = nullptr;
        char* featureWkt = const_cast<char*>(featureGeometry.asWkt().toUtf8().constData());
        if (OGRGeometryFactory::createFromWkt(featureWkt, nullptr, &targetGeometry) != OGRERR_NONE || !targetGeometry)
        {
            qDebug() << "Failed to create GDAL geometry from WKT.";
            continue;
        }

        if (!targetGeometry->Intersects(splitLine))
        {
            qDebug() << "No intersection with split line.";
            OGRGeometryFactory::destroyGeometry(targetGeometry);
            continue;
        }

        // 裁剪逻辑
        OGRGeometry* intersection = targetGeometry->Intersection(splitLine);
        if (intersection && !intersection->IsEmpty())
        {
            OGRGeometry* part1 = targetGeometry->Difference(intersection);
            OGRGeometry* part2 = targetGeometry->Difference(part1);

            if (part1 && part2)
            {
                char* wkt1 = nullptr;
                char* wkt2 = nullptr;

                part1->exportToWkt(&wkt1);
                part2->exportToWkt(&wkt2);

                QgsGeometry qgisPart1 = QgsGeometry::fromWkt(QString(wkt1));
                QgsGeometry qgisPart2 = QgsGeometry::fromWkt(QString(wkt2));

                // 更新原有要素
                QgsFeature updatedFeature = feature;
                updatedFeature.setGeometry(qgisPart1);
                mLayer->changeGeometry(feature.id(), qgisPart1);

                // 添加新要素
                QgsFeature newFeature;
                newFeature.setFields(mLayer->fields());
                newFeature.setAttributes(feature.attributes());
                newFeature.setGeometry(qgisPart2);
                mLayer->dataProvider()->addFeature(newFeature);

                CPLFree(wkt1);
                CPLFree(wkt2);
            }

            OGRGeometryFactory::destroyGeometry(part1);
            OGRGeometryFactory::destroyGeometry(part2);
        }

        OGRGeometryFactory::destroyGeometry(intersection);
        OGRGeometryFactory::destroyGeometry(targetGeometry);
    }

    // 清理裁剪线
    OGRGeometryFactory::destroyGeometry(splitLine);

    // 刷新图层
    mLayer->triggerRepaint();
    qDebug() << "Cut operation completed.";
}


// 光滑线功能
void LineEdit::smoothLine(const QList<QgsFeature>& selectedFeatures)
{
    QDialog dialog;
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QComboBox* comboBox = new QComboBox(&dialog);
    comboBox->addItem("Bezier");
    comboBox->addItem("Spline");
    layout->addWidget(comboBox);

    QPushButton* okButton = new QPushButton("OK", &dialog);
    layout->addWidget(okButton);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted)
    {
        QString method = comboBox->currentText();
        if (method == "Bezier")
            applyBezierSmoothing(selectedFeatures);
        else if (method == "Spline")
            applySplineSmoothing(selectedFeatures);
    }
}

// 贝塞尔光滑算法
void LineEdit::applyBezierSmoothing(const QList<QgsFeature>& selectedFeatures)
{
    for (const QgsFeature& feature : selectedFeatures)
    {
        QVector<QgsPointXY> points = feature.geometry().asPolyline();
        if (points.size() < 4) continue;

        QPainterPath path;
        path.moveTo(points[0].x(), points[0].y());
        for (int i = 1; i < points.size() - 2; i += 3)
        {
            path.cubicTo(points[i].toQPointF(), points[i + 1].toQPointF(), points[i + 2].toQPointF());
        }

        QVector<QgsPointXY> smoothedPoints;
        for (const QPointF& pt : path.toFillPolygon())
            smoothedPoints.append(QgsPointXY(pt.x(), pt.y()));

        QgsFeature newFeature;
        newFeature.setGeometry(QgsGeometry::fromPolylineXY(smoothedPoints));
        mLayer->dataProvider()->addFeature(newFeature);
    }
    mLayer->triggerRepaint();
}

// 样条光滑算法
void LineEdit::applySplineSmoothing(const QList<QgsFeature>& selectedFeatures)
{
    for (const QgsFeature& feature : selectedFeatures)
    {
        QVector<QgsPointXY> points = feature.geometry().asPolyline();
        if (points.size() < 4)
        {
            qDebug() << "Not enough points for spline smoothing.";
            continue;
        }

        QVector<QgsPointXY> smoothedPoints;
        for (int i = 1; i < points.size() - 2; ++i)
        {
            QgsPointXY p0 = points[i - 1];
            QgsPointXY p1 = points[i];
            QgsPointXY p2 = points[i + 1];
            QgsPointXY p3 = points[i + 2];

            // Catmull-Rom 插值
            for (float t = 0.0f; t <= 1.0f; t += 0.1f)
            {
                float t2 = t * t;
                float t3 = t2 * t;

                double x = 0.5 * ((2 * p1.x()) +
                    (-p0.x() + p2.x()) * t +
                    (2 * p0.x() - 5 * p1.x() + 4 * p2.x() - p3.x()) * t2 +
                    (-p0.x() + 3 * p1.x() - 3 * p2.x() + p3.x()) * t3);

                double y = 0.5 * ((2 * p1.y()) +
                    (-p0.y() + p2.y()) * t +
                    (2 * p0.y() - 5 * p1.y() + 4 * p2.y() - p3.y()) * t2 +
                    (-p0.y() + 3 * p1.y() - 3 * p2.y() + p3.y()) * t3);

                smoothedPoints.append(QgsPointXY(x, y));
            }
        }

        // 创建新的光滑几何
        QgsGeometry smoothedGeometry = QgsGeometry::fromPolylineXY(smoothedPoints);

        // 创建新的要素
        QgsFeature newFeature;
        newFeature.setGeometry(smoothedGeometry);
        newFeature.setFields(mLayer->fields());
        newFeature.setAttributes(feature.attributes());
        mLayer->dataProvider()->addFeature(newFeature);
    }

    mLayer->triggerRepaint();
}


// 动态绘制分割线
void LineEdit::canvasPressEvent(QgsMapMouseEvent* e)
{
    if (!mIsDrawing)
    {
        mStartPoint = e->mapPoint();
        mTempRubberBand->addPoint(mStartPoint);
        mIsDrawing = true;
    }
    else
    {
        mEndPoint = e->mapPoint();
        mTempRubberBand->addPoint(mEndPoint);

        emit lineCutSignal(mStartPoint, mEndPoint);
        cutLine(mSelectedFeatures, mStartPoint, mEndPoint);

        mTempRubberBand->reset(Qgis::GeometryType::Line);
        mIsDrawing = false;
    }
}

void LineEdit::canvasMoveEvent(QgsMapMouseEvent* e)
{
    if (mIsDrawing)
    {
        mTempRubberBand->removeLastPoint();
        mTempRubberBand->addPoint(e->mapPoint());
    }
}


//区边界转线
QgsVectorLayer* polygonToLines(QgsVectorLayer* polygonLayer, const QString& outputLineLayerPath)
{
    if (!polygonLayer || polygonLayer->geometryType() != Qgis::GeometryType::Polygon)
    {
        qDebug() << "Invalid polygon layer.";
        return NULL;
    }

    // 创建输出图层
    QgsFields fields = polygonLayer->fields();
    QString crsWkt = polygonLayer->crs().toWkt();
    QgsVectorLayer* lineLayer = new QgsVectorLayer(QString("LineString?crs=%1").arg(crsWkt),
        "ConvertedLines", "memory");
    QgsVectorDataProvider* lineProvider = lineLayer->dataProvider();
    lineProvider->addAttributes(fields.toList());
    lineLayer->updateFields();

    // 遍历多边形图层中的要素
    QgsFeature polygonFeature;
    QgsFeatureList lineFeatures;

    QgsFeatureIterator it = polygonLayer->getFeatures();
    while (it.nextFeature(polygonFeature))
    {
        QgsGeometry polygonGeometry = polygonFeature.geometry();
        if (polygonGeometry.isEmpty())
        {
            qDebug() << "Skipping empty polygon feature.";
            continue;
        }

        // 获取边界
        QgsGeometry boundaryGeometry = polygonGeometry.convertToType(Qgis::GeometryType::Line, true);

        if (boundaryGeometry.isEmpty())
        {
            qDebug() << "Boundary geometry is empty.";
            continue;
        }

        // 判断边界几何的类型
        Qgis::WkbType wkbType = boundaryGeometry.wkbType();
        if (wkbType == Qgis::WkbType::LineString)
        {
            // 单一 LineString
            QgsFeature lineFeature(fields);
            lineFeature.setGeometry(boundaryGeometry);
            lineFeature.setAttributes(polygonFeature.attributes());
            lineFeatures.append(lineFeature);
        }
        else if (wkbType == Qgis::WkbType::MultiLineString)
        {
            // 多部分 LineString
            const auto parts = boundaryGeometry.asMultiPolyline();
            for (const QgsPolylineXY& part : parts)
            {
                QgsFeature lineFeature(fields);
                lineFeature.setGeometry(QgsGeometry::fromPolylineXY(part));
                lineFeature.setAttributes(polygonFeature.attributes());
                lineFeatures.append(lineFeature);
            }
        }
        else
        {
            qDebug() << "Unsupported geometry type in boundary.";
        }
    }

    // 将线要素添加到线图层
    lineProvider->addFeatures(lineFeatures);

    // 保存到文件
    QgsVectorFileWriter::writeAsVectorFormatV2(
        lineLayer,
        outputLineLayerPath,
        QgsCoordinateTransformContext(),
        QgsVectorFileWriter::SaveVectorOptions()
    );

    // 将线图层添加到地图工程
    //QgsProject::instance()->addMapLayer(lineLayer);

    qDebug() << "Polygon boundaries successfully converted to lines.";
	return lineLayer;
}