#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QgsMapToolEdit.h>
#include <QgsVectorLayer.h>
#include <QgsPointXY.h>
#include <QgsGeometry.h>
#include <QgsFeature.h>
#include <QgsRubberBand.h>
#include <QList>


QgsVectorLayer* polygonToLines(QgsVectorLayer* polygonLayer, const QString& outputLineLayerPath);				// 区边界转线

void applyBezierSmoothing(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, int subdivision = 20);						// 应用贝塞尔平滑
void applySplineSmoothing(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, double tension = 0.2, int interpolationPoints = 10);						// 应用样条平滑
void applyGaussianSmoothing(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, int kernelSize = 5, double sigma = 1.0);				// 应用高斯平滑

//抽稀线

void applyBezierDownsampling(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, int subdivisionFactor);						// 应用贝塞尔抽稀
void applyIntervalDownsampling(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, int interval);		//固定间隔抽稀
void applyDouglasPeuckerSimplification(QgsVectorLayer* layer, const QList<QgsFeature>& selectedFeatures, double epsilon);		// 应用道格拉斯-普克抽稀
QVector<QgsPointXY> douglasPeuckerSimplify(const QVector<QgsPointXY>& points, double epsilon);		// 道格拉斯-普克抽稀算法
double pointToSegmentDistance(const QgsPointXY& point, const QgsPointXY& segmentStart, const QgsPointXY& segmentEnd);		// 点到线段的距离




#endif // LINEEDIT_H
