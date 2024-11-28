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
#include <QgsvectorLayer.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsVectorFileWriter.h>
#include <QgsProject.h>
#include <QgsVectorLayerEditUtils.h>
#include <qinputdialog.h>
#include <qobject.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <cmath>
#include <vector>

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
//光滑线
void smoothLines(QgsVectorLayer* lineLayer, const QList<QgsFeature>& selectedFeatures)
{
    if (!lineLayer || selectedFeatures.isEmpty())
    {
        qDebug() << "Invalid layer or no features selected.";
        return;
    }

    // 创建弹窗
    QDialog dialog;
    dialog.setWindowTitle("选择光滑算法");
    dialog.resize(300, 250);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 创建算法选择框
    QComboBox* comboBox = new QComboBox(&dialog);
    comboBox->addItem("贝塞尔光滑");
    comboBox->addItem("样条光滑");
    comboBox->addItem("高斯光滑"); // 新算法
    layout->addWidget(new QLabel("选择光滑算法:", &dialog));
    layout->addWidget(comboBox);

    // 参数输入区
    QLabel* paramLabel1 = new QLabel("细分数量:", &dialog);
    QSpinBox* paramInput1 = new QSpinBox(&dialog);
    paramInput1->setRange(1, 100);
    paramInput1->setValue(20); // 贝塞尔默认细分数量
    paramInput1->setVisible(true);

    QLabel* paramLabel2 = new QLabel("张力系数:", &dialog);
    QDoubleSpinBox* paramInput2 = new QDoubleSpinBox(&dialog);
    paramInput2->setRange(0.0, 1.0);
    paramInput2->setDecimals(2);
    paramInput2->setSingleStep(0.1);
    paramInput2->setValue(0.2); // 样条默认张力
    paramInput2->setVisible(false);

    QLabel* paramLabel3 = new QLabel("插值点数:", &dialog);
    QSpinBox* paramInput3 = new QSpinBox(&dialog);
    paramInput3->setRange(1, 50);
    paramInput3->setValue(10); // 样条默认插值点数
    paramInput3->setVisible(false);

    QLabel* paramLabel4 = new QLabel("窗口大小:", &dialog); // 高斯平滑参数
    QSpinBox* paramInput4 = new QSpinBox(&dialog);
    paramInput4->setRange(1, 20);
    paramInput4->setValue(5); // 默认窗口大小
    paramInput4->setVisible(false);

    QLabel* paramLabel5 = new QLabel("标准差 (Sigma):", &dialog); // 高斯平滑参数
    QDoubleSpinBox* paramInput5 = new QDoubleSpinBox(&dialog);
    paramInput5->setRange(0.1, 10.0); // 标准差范围
    paramInput5->setValue(1.0);       // 默认标准差
    paramInput5->setVisible(false);

    layout->addWidget(paramLabel1);
    layout->addWidget(paramInput1);
    layout->addWidget(paramLabel2);
    layout->addWidget(paramInput2);
    layout->addWidget(paramLabel3);
    layout->addWidget(paramInput3);
    layout->addWidget(paramLabel4);
    layout->addWidget(paramInput4);
    layout->addWidget(paramLabel5);
    layout->addWidget(paramInput5);

    // 确认按钮
    QPushButton* okButton = new QPushButton("确定", &dialog);
    layout->addWidget(okButton);

    // 动态调整界面参数设置
    QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
        if (comboBox->itemText(index) == "贝塞尔光滑")
        {
            paramLabel1->setVisible(true);
            paramInput1->setVisible(true);

            paramLabel2->setVisible(false);
            paramInput2->setVisible(false);
            paramLabel3->setVisible(false);
            paramInput3->setVisible(false);
            paramLabel4->setVisible(false);
            paramInput4->setVisible(false);
            paramLabel5->setVisible(false);
            paramInput5->setVisible(false);
        }
        else if (comboBox->itemText(index) == "样条光滑")
        {
            paramLabel1->setVisible(false);
            paramInput1->setVisible(false);

            paramLabel2->setVisible(true);
            paramInput2->setVisible(true);
            paramLabel3->setVisible(true);
            paramInput3->setVisible(true);
            paramLabel4->setVisible(false);
            paramInput4->setVisible(false);
            paramLabel5->setVisible(false);
            paramInput5->setVisible(false);
        }
        else if (comboBox->itemText(index) == "高斯光滑")
        {
            paramLabel1->setVisible(false);
            paramInput1->setVisible(false);

            paramLabel2->setVisible(false);
            paramInput2->setVisible(false);
            paramLabel3->setVisible(false);
            paramInput3->setVisible(false);

            paramLabel4->setVisible(true);
            paramInput4->setVisible(true);
            paramLabel5->setVisible(true);
            paramInput5->setVisible(true);
        }
        });

    // 点击确认按钮，调用对应算法
    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    // 显示弹窗
    if (dialog.exec() == QDialog::Accepted)
    {
        QString method = comboBox->currentText();
        if (method == "贝塞尔光滑")
        {
            int subdivision = paramInput1->value();
            applyBezierSmoothing(lineLayer, selectedFeatures, subdivision);
        }
        else if (method == "样条光滑")
        {
            double tension = paramInput2->value();
            int interpolationPoints = paramInput3->value();
            applySplineSmoothing(lineLayer, selectedFeatures, tension, interpolationPoints);
        }
        else if (method == "高斯光滑")
        {
            int kernelSize = paramInput4->value();
            double sigma = paramInput5->value();
            applyGaussianSmoothing(lineLayer, selectedFeatures, kernelSize, sigma);
        }
        else
        {
            qDebug() << "Unsupported smoothing method.";
        }
    }
}

// 平滑线
void applyBezierSmoothing(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, int subdivision )
{
    // 检查图层是否有效
    if (!layer)
    {
        qDebug() << "Invalid vector layer.";
        return;
    }

    double offsetX = 0.0; // X轴偏移量，可根据需求调整
    double offsetY = 0.0; // Y轴偏移量，可根据需求调整

    // 检查并开启图层的编辑模式，确保可以对几何进行修改
    if (!layer->isEditable())
    {
        layer->startEditing(); // 开启编辑模式
    }

    // 遍历所有选中的要素进行处理
    for (const QgsFeature& feature : selectedFeatures)
    {
        QVector<QgsPointXY> points; // 存储当前要素的点集合

        // 获取几何类型并提取点集合
        Qgis::WkbType geomType = feature.geometry().wkbType();
        if (geomType == Qgis::WkbType::LineString)
        {
            // 如果是单线段，则提取为折线
            points = feature.geometry().asPolyline();
        }
        else if (geomType == Qgis::WkbType::MultiLineString)
        {
            // 如果是多线段，则提取每条线段并合并到一个点集合中
            const auto multiPolyline = feature.geometry().asMultiPolyline();
            for (const auto& line : multiPolyline)
            {
                points.append(line);
            }
        }
        else
        {
            // 如果几何类型不支持，跳过处理
            qDebug() << "Unsupported geometry type. Skipping.";
            continue;
        }

        qDebug() << "Number of points: " << points.size();

        // 如果点数量不足以生成贝塞尔曲线，跳过处理
        if (points.size() < 4)
        {
            qDebug() << "Not enough points for Bezier smoothing. Keeping original line.";
            continue;
        }

        QVector<QgsPointXY> smoothedPoints; // 存储平滑后的点集合
        smoothedPoints.append(points[0]);   // 添加第一个点，作为起点保留

        // 遍历点集合，按段生成贝塞尔曲线并细分为更多点
        for (int i = 0; i < points.size() - 3; i += 3)
        {
            // 提取当前段的4个控制点
            QgsPointXY p0 = points[i];
            QgsPointXY p1 = points[i + 1];
            QgsPointXY p2 = points[i + 2];
            QgsPointXY p3 = points[i + 3];

            // 定义细分数量，用于增加平滑曲线上的点
           //const int subdivision = 20; // 每段细分成20个点，可根据需求调整

            // 在曲线上进行细分插值
            for (int j = 1; j <= subdivision; ++j)
            {
                double t = j / static_cast<double>(subdivision); // 当前插值参数，范围[0, 1]
                double oneMinusT = 1.0 - t; // 插值反向参数
                // 计算贝塞尔曲线的插值点坐标
                double x = oneMinusT * oneMinusT * oneMinusT * p0.x() +
                    3 * oneMinusT * oneMinusT * t * p1.x() +
                    3 * oneMinusT * t * t * p2.x() +
                    t * t * t * p3.x();
				// 计算贝塞尔曲线的插值点坐标
                double y = oneMinusT * oneMinusT * oneMinusT * p0.y() +
                    3 * oneMinusT * oneMinusT * t * p1.y() +
                    3 * oneMinusT * t * t * p2.y() +
                    t * t * t * p3.y();

                // 将插值点加入平滑点集合，同时添加偏移
                smoothedPoints.append(QgsPointXY(x + offsetX, y + offsetY));
            }
        }

        smoothedPoints.append(points.last()); // 添加最后一个点，作为终点保留

        // 创建新的几何对象，使用平滑后的点集合
        QgsGeometry newGeometry = QgsGeometry::fromPolylineXY(smoothedPoints);

        // 替换当前要素的几何，支持撤销操作
        if (!layer->changeGeometry(feature.id(), newGeometry))
        {
            qDebug() << "Failed to update geometry for feature ID: " << feature.id();
            continue; // 如果替换失败，跳过该要素
        }
    }

    // 刷新图层以反映修改
    layer->triggerRepaint();
    qDebug() << "Bezier smoothing applied successfully.";
}

// 应用样条平滑
void applySplineSmoothing(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, double tension, int interpolationPoints)
{
	//double tension = 0.2;       // 张力系数
	//int interpolationPoints = 10;       // 插值点数
    if (!layer)
    {
        qDebug() << "Invalid vector layer.";
        return;
    }
    // 开启编辑模式
    if (!layer->isEditable())
    {
        if (!layer->startEditing())
        {
            qDebug() << "Failed to start editing the layer.";
            return;
        }
    }

    for (const QgsFeature& feature : selectedFeatures)
    {
        QVector<QgsPointXY> points;

        // 检查几何类型并提取点
        Qgis::WkbType geomType = feature.geometry().wkbType();
        if (geomType == Qgis::WkbType::LineString)
        {
            points = feature.geometry().asPolyline();
        }
        else if (geomType == Qgis::WkbType::MultiLineString)
        {
            const auto multiPolyline = feature.geometry().asMultiPolyline();
            for (const auto& line : multiPolyline)
            {
                points.append(line);
            }
        }
        else
        {
            qDebug() << "Unsupported geometry type. Skipping.";
            continue;
        }

        qDebug() << "Number of points: " << points.size();

        // 如果点不足，直接跳过
        if (points.size() < 3)
        {
            qDebug() << "Not enough points for spline smoothing. Keeping original line.";
            continue;
        }

        QVector<QgsPointXY> smoothedPoints;

        // 添加首点
        smoothedPoints.append(points[0]);
        // 将点从地理坐标投影到平面坐标
        QgsCoordinateReferenceSystem crsGeographic("EPSG:4326");  // 地理坐标系（WGS 84）
        QgsCoordinateReferenceSystem crsProjected("EPSG:3857");   // 投影坐标系（Web Mercator）
        QgsCoordinateTransform transform(crsGeographic, crsProjected, QgsProject::instance());

        // 投影转换原始点
        for (int i = 0; i < points.size(); ++i) {
            points[i] = transform.transform(points[i]);
        }
        // Catmull-Rom 样条光滑
        double tension = 1;  // 减小张力系数
        int interpolationPoints = 10;  // 减少插值点

        for (int i = 0; i < points.size() - 1; ++i)
        {
            //打印原始点的坐标
			qDebug() << "原始点："<<"x:" << points[i].x() << "y:" << points[i].y();
            
            // 边界点改进：加入外推点
            QgsPointXY p0 = (i > 0) ? points[i - 1] : QgsPointXY(2 * points[i].x() - points[i + 1].x(), 2 * points[i].y() - points[i + 1].y());
            QgsPointXY p1 = points[i];
            QgsPointXY p2 = points[i + 1];
            QgsPointXY p3 = (i + 2 < points.size()) ? points[i + 2] : QgsPointXY(2 * points[i + 1].x() - points[i].x(), 2 * points[i + 1].y() - points[i].y());
            qDebug() << "p0: " << p0.x() << "," << p0.y();
            qDebug() << "p1: " << p1.x() << "," << p1.y();
            qDebug() << "p2: " << p2.x() << "," << p2.y();
            qDebug() << "p3: " << p3.x() << "," << p3.y();
            // 确保所有点在统一坐标系
            QgsCoordinateTransform transform(layer->crs(), QgsProject::instance()->crs(), QgsProject::instance());
            for (auto& point : points) {
                point = transform.transform(point);
            }
            // 插值点生成
            for (int j = 0; j <= interpolationPoints; ++j) {
                double t = static_cast<double>(j) / interpolationPoints;
                double tt = t * t;
                double ttt = tt * t;

                double x =
                     ((-tension * p0.x() + (2 - tension) * p1.x() + (tension - 2) * p2.x() + tension * p3.x()) * ttt +
                        ((2 * tension) * p0.x() + (-3 + tension) * p1.x() + (3 - 2 * tension) * p2.x() + (-tension) * p3.x()) * tt +
                        ((-tension) * p0.x() + tension * p2.x()) * t +
                        (p1.x()));
                double y =
                    ((-tension * p0.y() + (2 - tension) * p1.y() + (tension - 2) * p2.y() + tension * p3.y()) * ttt +
                        ((2 * tension) * p0.y() + (-3 + tension) * p1.y() + (3 - 2 * tension) * p2.y() + (-tension) * p3.y()) * tt +
                        ((-tension) * p0.y() + tension * p2.y()) * t +
                        (p1.y()));

                smoothedPoints.append(QgsPointXY(x, y));
            }
        }
        // 添加最后一个点
        smoothedPoints.append(points.last());
        // 投影转换回地理坐标
        QgsCoordinateTransform reverseTransform(crsProjected, crsGeographic, QgsProject::instance());
        for (int i = 0; i < smoothedPoints.size(); ++i) {
            smoothedPoints[i] = reverseTransform.transform(smoothedPoints[i]);
        }
        // 创建新的几何
        QgsGeometry newGeometry = QgsGeometry::fromPolylineXY(smoothedPoints);

        // 替换原几何
        if (!layer->changeGeometry(feature.id(), newGeometry))
        {
            qDebug() << "Failed to update geometry for feature ID: " << feature.id();
            continue;
        }
    }

    // 刷新图层
    layer->triggerRepaint();

    qDebug() << "Spline smoothing applied successfully.";
}

void applyGaussianSmoothing(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, int kernelSize, double sigma)
{
    if (!layer || selectedFeatures.isEmpty() || kernelSize <= 0 || sigma <= 0)
    {
        qDebug() << "无效的图层或选择的要素，或者无效的核大小或标准差。";
        return;
    }

    // 处理每个要素
    for (const QgsFeature& feature : selectedFeatures)
    {
		QVector<QgsPointXY> points;  // 存储要素的点

        // 获取几何类型并提取点
        Qgis::WkbType geomType = feature.geometry().wkbType();
        qDebug() << "几何类型：" << geomType;

        // 处理 LineString 类型
        if (geomType == Qgis::WkbType::LineString)
        {
            points = feature.geometry().asPolyline();
        }
        // 处理 MultiLineString 类型
        else if (geomType == Qgis::WkbType::MultiLineString)
        {
            const auto multiPolyline = feature.geometry().asMultiPolyline();
            for (const auto& line : multiPolyline)
            {
                points.append(line);  // 将多个线段的点合并
            }
        }
        else
        {
            qDebug() << "不支持的几何类型，跳过此要素。";
            continue;  // 如果是其他几何类型，跳过该要素
        }

        qDebug() << "提取的点的数量：" << points.size();

        // 如果点数不足，跳过该要素
        if (points.size() < kernelSize)
        {
            qDebug() << "点数不足，跳过该要素的高斯平滑。";
            continue; // 如果点数不足，跳过平滑处理
        }

        // 计算高斯核
        std::vector<double> kernel(kernelSize);
        double sum = 0.0;
        int radius = kernelSize / 2;

        // 构建高斯核
        for (int i = 0; i < kernelSize; ++i)
        {
            double x = i - radius;
            kernel[i] = std::exp(-0.5 * (x * x) / (sigma * sigma));  // 使用高斯公式
            sum += kernel[i];  // 计算高斯核的总和，用于归一化
        }

        // 归一化高斯核，使得权重之和为 1
        for (int i = 0; i < kernelSize; ++i)
        {
            kernel[i] /= sum;
        }

        qDebug() << "高斯核（Kernel）:";
        for (int i = 0; i < kernelSize; ++i)
        {
            qDebug() << "kernel[" << i << "] = " << kernel[i];
        }

        // 输出点的数量
        qDebug() << "点的数量: " << points.size();


        // 创建一个用于存放平滑点的 QVector
        QVector<QgsPointXY> smoothedPoints;

        // 对每个点应用高斯平滑
        for (int i = 0; i < points.size(); ++i)
        {
            double sumX = 0.0, sumY = 0.0;

            // 处理边界点：确保首尾点有足够的邻域点
            for (int j = i - radius; j <= i + radius; ++j)
            {
                // 检查 j 是否在有效范围内
                if (j < 0) j = 0;  // 对第一个点进行外推处理
                if (j >= points.size()) j = points.size() - 1;  // 对最后一个点进行外推处理

                // 计算偏移量，确保索引正确
                int offset = j - i + radius;

                // 计算加权平均
                sumX += points[j].x() * kernel[offset];  // X 坐标的加权平均
                sumY += points[j].y() * kernel[offset];  // Y 坐标的加权平均
            }
			qDebug() << "count:" << i << "sumX:" << sumX << "sumY:" << sumY;
            // 输出每个平滑点的坐标
            qDebug() << "平滑后的点： x:" << sumX << "y:" << sumY;

            // 记录平滑后的点
            smoothedPoints.append(QgsPointXY(sumX, sumY));  // 将平滑后的点添加到列表中
        }
        // 替换几何体
        QgsGeometry newGeometry = QgsGeometry::fromPolylineXY(smoothedPoints);
        if (!layer->changeGeometry(feature.id(), newGeometry))
        {
            qDebug() << "无法更新要素 ID:" << feature.id();
        }
    }

    // 刷新图层，应用更改
    layer->triggerRepaint();
    qDebug() << "高斯平滑成功应用。";
}
//抽稀线
void thiningLines(QgsVectorLayer* lineLayer, const QList<QgsFeature>& selectedFeatures)
{
    if (!lineLayer || selectedFeatures.isEmpty())
    {
        qDebug() << "Invalid layer or no features selected.";
        return;
    }

    // 创建弹窗
    QDialog dialog;
    dialog.resize(300, 200);
    dialog.setWindowTitle("Select Simplification Method");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 创建第一个下拉框（选择算法）
    QComboBox* methodComboBox = new QComboBox(&dialog);
    methodComboBox->addItem("贝塞尔抽稀");
    methodComboBox->addItem("固定间隔抽稀");
    methodComboBox->addItem("道格拉斯算法");
    layout->addWidget(new QLabel("选择抽稀算法:", &dialog));
    layout->addWidget(methodComboBox);

    // 参数输入框
    QLabel* paramLabel = new QLabel("算法参数:", &dialog);
    layout->addWidget(paramLabel);

    QLineEdit* paramInput = new QLineEdit(&dialog);
    paramInput->setPlaceholderText("请输入算法参数");
    layout->addWidget(paramInput);

    // 确认按钮
    QPushButton* okButton = new QPushButton("确定", &dialog);
    layout->addWidget(okButton);

    // 动态更新参数提示
    QObject::connect(methodComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
        QString method = methodComboBox->itemText(index);
        if (method == "贝塞尔抽稀")
        {
            paramLabel->setText("细分数量 (整数):");
            paramInput->setPlaceholderText("例如：20");
        }
        else if (method == "固定间隔抽稀")
        {
            paramLabel->setText("间隔 (整数):");
            paramInput->setPlaceholderText("例如：10");
        }
        else if (method == "道格拉斯算法")
        {
            paramLabel->setText("阈值 (小数):");
            paramInput->setPlaceholderText("例如：5.0");
        }
        });
    // 点击确认按钮后获取输入
    QObject::connect(okButton, &QPushButton::clicked, [&dialog]() { dialog.accept(); });

    // 弹窗显示
    if (dialog.exec() == QDialog::Accepted)
    {
        QString method = methodComboBox->currentText();
        QString paramValueStr = paramInput->text();
        bool isValid;
        double paramValue = paramValueStr.toDouble(&isValid);

        if (!isValid || paramValue <= 0)
        {
            qDebug() << "Invalid parameter value.";
            return;
        }

        // 根据选中的算法调用不同的处理方法
        if (method == "贝塞尔抽稀")
        {
            applyBezierDownsampling(lineLayer, selectedFeatures, static_cast<int>(paramValue)); // 参数为整数
        }
        else if (method == "固定间隔抽稀")
        {
            applyIntervalDownsampling(lineLayer, selectedFeatures, static_cast<int>(paramValue)); // 参数为整数
        }
        else if (method == "道格拉斯算法")
        {
            applyDouglasPeuckerSimplification(lineLayer, selectedFeatures, paramValue); // 参数为浮点数
        }
        else
        {
            qDebug() << "Unsupported smoothing method.";
        }
    }
}

//贝塞尔抽稀
void applyBezierDownsampling(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, int subdivisionFactor)
{
    // 检查图层是否有效
    if (!layer)
    {
        qDebug() << "Invalid vector layer.";
        return;
    }

    // 检查并开启图层的编辑模式，确保可以对几何进行修改
    if (!layer->isEditable())
    {
        layer->startEditing(); // 开启编辑模式
    }

    // 遍历所有选中的要素进行处理
    for (const QgsFeature& feature : selectedFeatures)
    {
        QVector<QgsPointXY> points; // 存储当前要素的点集合

        // 提取几何
        Qgis::WkbType geomType = feature.geometry().wkbType();
        if (geomType == Qgis::WkbType::LineString)
        {
            points = feature.geometry().asPolyline();
        }
        else if (geomType == Qgis::WkbType::MultiLineString)
        {
            const auto multiPolyline = feature.geometry().asMultiPolyline();
            for (const auto& line : multiPolyline)
            {
                points.append(line);
            }
        }
        else
        {
            qDebug() << "Unsupported geometry type. Skipping.";
            continue;
        }

        // 如果点数量不足，跳过
        if (points.size() < 4)
        {
            qDebug() << "Not enough points for downsampling.";
            continue;
        }

        QVector<QgsPointXY> downsampledPoints; // 存储抽稀后的点
        downsampledPoints.append(points[0]);   // 保留起点

        // 对点集合进行分段抽稀
        for (int i = 0; i < points.size() - 3; i += 3)
        {
            QgsPointXY p0 = points[i];
            QgsPointXY p1 = points[i + 1];
            QgsPointXY p2 = points[i + 2];
            QgsPointXY p3 = points[i + 3];

            // 定义细分数量，降低分辨率
            const int subdivision = subdivisionFactor; // 可调整此值以控制抽稀程度

            // 每段仅保留部分点
            for (int j = 1; j <= subdivision; j += 2) // 每隔一个点保留
            {
                double t = j / static_cast<double>(subdivision);
                double oneMinusT = 1.0 - t;

                double x = oneMinusT * oneMinusT * oneMinusT * p0.x() +
                    3 * oneMinusT * oneMinusT * t * p1.x() +
                    3 * oneMinusT * t * t * p2.x() +
                    t * t * t * p3.x();

                double y = oneMinusT * oneMinusT * oneMinusT * p0.y() +
                    3 * oneMinusT * oneMinusT * t * p1.y() +
                    3 * oneMinusT * t * t * p2.y() +
                    t * t * t * p3.y();

                downsampledPoints.append(QgsPointXY(x, y));
            }
        }
        downsampledPoints.append(points.last()); // 保留终点
        // 创建新的几何对象
        QgsGeometry newGeometry = QgsGeometry::fromPolylineXY(downsampledPoints);
        // 更新几何
        if (!layer->changeGeometry(feature.id(), newGeometry))
        {
            qDebug() << "Failed to update geometry for feature ID: " << feature.id();
        }
    }
    // 刷新图层
    layer->triggerRepaint();
    qDebug() << "Bezier downsampling applied successfully.";
}


//固定距离法
void applyIntervalDownsampling(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, int interval)
{
    // 检查图层是否有效
    if (!layer)
    {
        qDebug() << "Invalid vector layer.";
        return;
    }

    // 检查并开启图层的编辑模式，确保可以对几何进行修改
    if (!layer->isEditable())
    {
        layer->startEditing(); // 开启编辑模式
    }

    // 遍历所有选中的要素
    for (const QgsFeature& feature : selectedFeatures)
    {
        QVector<QgsPointXY> points; // 存储当前要素的点集合

        // 提取几何
        Qgis::WkbType geomType = feature.geometry().wkbType();
        if (geomType == Qgis::WkbType::LineString)
        {
            points = feature.geometry().asPolyline();
        }
        else if (geomType == Qgis::WkbType::MultiLineString)
        {
            const auto multiPolyline = feature.geometry().asMultiPolyline();
            for (const auto& line : multiPolyline)
            {
                points.append(line);
            }
        }
        else
        {
            qDebug() << "Unsupported geometry type. Skipping.";
            continue;
        }

        // 如果点数量不足，跳过
        if (points.size() < interval)
        {
            qDebug() << "Not enough points for interval downsampling.";
            continue;
        }

        QVector<QgsPointXY> intervalPoints; // 存储简化后的点
        for (int i = 0; i < points.size(); i += interval)
        {
            intervalPoints.append(points[i]);
        }

        // 确保保留最后一个点
        if (intervalPoints.last() != points.last())
        {
            intervalPoints.append(points.last());
        }

        // 创建新的几何对象
        QgsGeometry newGeometry = QgsGeometry::fromPolylineXY(intervalPoints);

        // 更新几何
        if (!layer->changeGeometry(feature.id(), newGeometry))
        {
            qDebug() << "Failed to update geometry for feature ID: " << feature.id();
        }
    }

    // 刷新图层
    layer->triggerRepaint();
    qDebug() << "Interval downsampling applied successfully.";
}

double pointToSegmentDistance(const QgsPointXY& point, const QgsPointXY& segmentStart, const QgsPointXY& segmentEnd)
{
    double x = point.x();
    double y = point.y();
    double x1 = segmentStart.x();
    double y1 = segmentStart.y();
    double x2 = segmentEnd.x();
    double y2 = segmentEnd.y();

    // 线段的长度平方
    double segmentLengthSquared = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
    if (segmentLengthSquared == 0.0)
    {
        // 线段是一个点，直接计算两点之间的距离
        return sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1));
    }

    // 投影计算 (t 表示点在线段上的投影比例)
    double t = ((x - x1) * (x2 - x1) + (y - y1) * (y2 - y1)) / segmentLengthSquared;
    if (t < 0.0)
    {
        // 点在线段起点之前
        return sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1));
    }
    else if (t > 1.0)
    {
        // 点在线段终点之后
        return sqrt((x - x2) * (x - x2) + (y - y2) * (y - y2));
    }
    // 点在线段中间，计算投影点坐标
    double projectionX = x1 + t * (x2 - x1);
    double projectionY = y1 + t * (y2 - y1);

    // 返回点到投影点的距离
    return sqrt((x - projectionX) * (x - projectionX) + (y - projectionY) * (y - projectionY));
}
// 道格拉斯-普克抽稀算法
QVector<QgsPointXY> douglasPeuckerSimplify(const QVector<QgsPointXY>& points, double epsilon)
{
    if (points.size() < 2)
    {
        return points; // 如果点数不足2个，直接返回原始点
    }
    // 找到距离首尾线段最大偏离的点
    double maxDistance = 0.0;
    int maxIndex = -1;
    for (int i = 1; i < points.size() - 1; ++i)
    {
        // 使用自定义函数计算点到线段的距离
        double distance = pointToSegmentDistance(points[i], points.first(), points.last());
        if (distance > maxDistance)
        {
            maxDistance = distance;
            maxIndex = i;
        }
    }
    // 如果最大距离小于阈值epsilon，保留首尾点
    if (maxDistance < epsilon)
    {
        return { points.first(), points.last() };
    }

    // 递归处理前半部分和后半部分
    QVector<QgsPointXY> leftSimplified = douglasPeuckerSimplify(points.mid(0, maxIndex + 1), epsilon);
    QVector<QgsPointXY> rightSimplified = douglasPeuckerSimplify(points.mid(maxIndex), epsilon);

    // 合并结果，去掉重复的中间点
    leftSimplified.pop_back();
    leftSimplified.append(rightSimplified);

    return leftSimplified;
}

// 应用道格拉斯-普克抽稀
void applyDouglasPeuckerSimplification(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, double epsilon)
{
    // 检查图层有效性
    if (!layer)
    {
        qDebug() << "Invalid vector layer.";
        return;
    }

    // 开启编辑模式以允许修改
    if (!layer->isEditable())
    {
        layer->startEditing();
    }

    // 遍历所有选中的要素
    for (const QgsFeature& feature : selectedFeatures)
    {
        QVector<QgsPointXY> points;

        // 提取几何类型和点集合
        Qgis::WkbType geomType = feature.geometry().wkbType();
        if (geomType == Qgis::WkbType::LineString)
        {
            points = feature.geometry().asPolyline();
        }
        else if (geomType == Qgis::WkbType::MultiLineString)
        {
            const auto multiPolyline = feature.geometry().asMultiPolyline();
            for (const auto& line : multiPolyline)
            {
                points.append(line);
            }
        }
        else
        {
            qDebug() << "Unsupported geometry type. Skipping.";
            continue;
        }

        // 如果点数量不足2个，跳过
        if (points.size() < 2)
        {
            qDebug() << "Not enough points for simplification. Skipping feature ID:" << feature.id();
            continue;
        }

        // 使用道格拉斯-普克算法简化点集合
        QVector<QgsPointXY> simplifiedPoints = douglasPeuckerSimplify(points, epsilon);

        // 创建新的几何对象
        QgsGeometry newGeometry = QgsGeometry::fromPolylineXY(simplifiedPoints);

        // 更新要素的几何
        if (!layer->changeGeometry(feature.id(), newGeometry))
        {
            qDebug() << "Failed to update geometry for feature ID:" << feature.id();
        }
    }

    // 刷新图层显示
    layer->triggerRepaint();
    qDebug() << "Douglas-Peucker simplification applied successfully.";
}


