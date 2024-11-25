/*
FileName: AnalyticalFunction.cpp
Author:RYB
Date:2024.7.26
Description:
	矢量分析与栅格分析功能实现
Function Lists:
	1. double getDistance(const QgsPoint *p1, const QgsPoint *p2) 计算两点之间的距离
	2. double getDistance(const QgsGeometry *p1, const QgsPoint *p2) 计算点到面中心的距离
	3. QList<QgsPoint *> getQgsPoints(QgsVectorLayer *qvlLayer) 获取矢量图层中所有点要素
	4. QList<QgsPolylineXY *> getQgsPolylines(QgsVectorLayer *qvlLayer) 获取矢量图层中所有线要素
	5. QList<QgsMultiPolygonXY *> getQgsPolygons(QgsVectorLayer *qvlLayer) 获取矢量图层中所有面要素
	6. QList<QgsGeometry *> getQgsGeometries(QgsVectorLayer *qvlLayer) 获取矢量图层中所有几何体要素
	7. void colorFeaturesByField(QgsVectorLayer *qvlLayer, const QString &fieldName) 根据字段上色
	8. QList<int> calculateKMeans(QList<QgsPoint *> liPoints, int nClusterNum) 计算KMeans
	9. void createKMeans() 新建KMeans聚类分析图层
	10. void connectFiled(QgsVectorLayer *pvlLayerResult, QgsVectorLayer *pvlLayerCompare, int nResultIdx, int nCompareIdx) 连接字段
	11. void connectByLocation() 构建空间连接图层
	12. void rasterStatistics() 统计栅格数据
*/

#include "MainWidget.h"
#include "SelectDialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <Qgsvectorlayer.h>
#include <Qgis.h>
#include <qDebug>
#include <QgsVectorDataProvider.h>
#include <QgsCategorizedSymbolRenderer.h>
#include <QgsPolygon.h>
#include <QgsMultiPolygon.h>
#include <QgsSymbol.h>
#include <QgsRasterBandStats.h>
#include <QgsRasterDataProvider.h>
#include <QStringListModel>
#include <mutex>
#include <qprogressdialog.h>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
// #include "dataanalysis.h"

// 计算两点之间的距离
double getDistance(const QgsPoint *p1, const QgsPoint *p2)
{
	double dDeltax = p1->x() - p2->x();
	double dDeltay = p1->y() - p2->y();
	return sqrt(dDeltax * dDeltax + dDeltay * dDeltay);
}
// 计算点到面中心的距离
double getDistance(const QgsGeometry *p1, const QgsPoint *p2)
{
	QgsPoint p1Point = (QgsPoint)p1->centroid().asPoint();
	return getDistance(&p1Point, p2);
}
// 获取矢量图层中所有点要素
QList<QgsPoint *> MainWidget::getQgsPoints(QgsVectorLayer *qvlLayer)
{
	QList<QgsPoint *> liPoints;
	QgsFeatureIterator featureIterator = qvlLayer->getFeatures();
	QgsFeature feature;
	while (featureIterator.nextFeature(feature))
	{
		QgsGeometry geometry = feature.geometry();
		if (geometry.isNull())
			continue;

		if (geometry.type() == Qgis::GeometryType::Point)
		{
			QgsPoint *point = new QgsPoint(geometry.asPoint());
			liPoints.append(point);
		}
	}
	return liPoints;
}
// 获取矢量图层中所有线要素
QList<QgsPolylineXY *> MainWidget::getQgsPolylines(QgsVectorLayer *qvlLayer)
{
	QList<QgsPolylineXY *> liPolylines;
	QgsFeatureIterator featureIterator = qvlLayer->getFeatures();
	QgsFeature feature;
	while (featureIterator.nextFeature(feature))
	{
		QgsGeometry geometry = feature.geometry();
		if (geometry.isNull())
			continue;

		if (geometry.type() == Qgis::GeometryType::Line)
		{
			QgsPolylineXY *polyline = new QgsPolylineXY(geometry.asPolyline());
			liPolylines.append(polyline);
		}
	}
	return liPolylines;
}
// 获取矢量图层中所有面要素
QList<QgsMultiPolygonXY *> MainWidget::getQgsPolygons(QgsVectorLayer *qvlLayer)
{
	QList<QgsMultiPolygonXY *> liPolygons;
	QgsFeatureIterator featureIterator = qvlLayer->getFeatures();
	QgsFeature feature;
	while (featureIterator.nextFeature(feature))
	{
		QgsGeometry geometry = feature.geometry();
		// qDebug() << "geometry:" << geometry.asWkt();
		if (geometry.isNull())
			continue;

		if (geometry.type() == Qgis::GeometryType::Polygon)
		{
			QgsMultiPolygonXY *polygon = new QgsMultiPolygonXY(geometry.asMultiPolygon());
			liPolygons.append(polygon);
		}
	}
	return liPolygons;
}
// 获取矢量图层中所有几何体要素
QList<QgsGeometry *> MainWidget::getQgsGeometries(QgsVectorLayer *qvlLayer)
{
	QList<QgsGeometry *> liGeometries;
	QgsFeatureIterator featureIterator = qvlLayer->getFeatures();
	QgsFeature feature;
	while (featureIterator.nextFeature(feature))
	{
		QgsGeometry geometry = feature.geometry();
		if (geometry.isNull())
			continue;

		liGeometries.append(new QgsGeometry(geometry));
	}
	return liGeometries;
}
// 根据字段上色
void MainWidget::colorFeaturesByField(QgsVectorLayer *qvlLayer, const QString &fieldName, int bClusterNum)
{
	// 获取图层中的所有要素及其字段值
	QgsFeatureIterator featureIterator = qvlLayer->getFeatures();
	QgsFeature feature;
	QMap<QString, QgsSymbol *> symbolMap;
	int n = 0;
	while (featureIterator.nextFeature(feature) && bClusterNum == 0)
	{
		QString fieldValue = feature.attribute(fieldName).toString();
		if (!symbolMap.contains(fieldValue) && fieldValue != "")
		{
			// 为每个唯一的字段值创建一个符号
			QgsSymbol *symbol = QgsSymbol::defaultSymbol(qvlLayer->geometryType());
			symbol->setColor(QColor::fromRgb(qrand() % 256, qrand() % 256, qrand() % 256));
			symbolMap.insert(fieldValue, symbol);
			qDebug() << "fieldValue:" << fieldValue << "symbol:" << symbol;
			n++;
		}
	}
	if (bClusterNum != 0)
	{
		for (int i = 0; i < bClusterNum; i++)
		{
			QgsSymbol *symbol = QgsSymbol::defaultSymbol(qvlLayer->geometryType());
			symbol->setColor(QColor::fromRgb(qrand() % 256, qrand() % 256, qrand() % 256));
			symbolMap.insert(QString::number(i), symbol);
		}
	}
	// 创建分类渲染器
	QList<QgsRendererCategory> categories;
	for (auto it = symbolMap.begin(); it != symbolMap.end(); ++it)
	{
		categories.append(QgsRendererCategory(it.key(), it.value(), it.key()));
	}

	QgsCategorizedSymbolRenderer *renderer = new QgsCategorizedSymbolRenderer(fieldName, categories);
	qvlLayer->setRenderer(renderer);

	// 刷新图层
	qvlLayer->triggerRepaint();
}

// 计算KMeans
QList<int> MainWidget::calculateKMeans(QList<QgsPoint *> liPoints, int nClusterNum)
{
	// 结果数据集
	QList<int> liKMeansResult;
	// 初始化数据集，大小与liPoints一致
	for (int nIndex = 0; nIndex < liPoints.size(); nIndex++)
	{
		liKMeansResult.append(0);
	}
	// 随机选取nClusterNum个点作为初始聚类中心
	QList<QgsPoint *> qgskmeansCenter;
	for (int nIndex = 0; nIndex < nClusterNum; nIndex++)
	{
		qgskmeansCenter.append((QgsPoint *)(liPoints[nIndex * (liPoints.size() / (nClusterNum + 1))]));
	}
	// 分配各个数据对象到最近的聚类中心
	bool bFlag = true;
	int nFlag = 0;
	while (bFlag)
	{
		bFlag = false;
		int i = 0;
		int nCount = 0;
		// 计算最近中心
		// for (QgsPoint *point : liPoints)
		for (int nPtListIndex = 0; nPtListIndex < liKMeansResult.size(); nPtListIndex++)
		{
			QgsPoint *point = liPoints[nPtListIndex];
			// QgsGeometry *point = liPoints[nPtListIndex];
			int nMinIndex = liKMeansResult[i];
			double dMinDistance = getDistance(point, qgskmeansCenter[nMinIndex]);
			for (int nIndex = 0; nIndex < qgskmeansCenter.size(); nIndex++)
			{
				double dDistance = getDistance(point, qgskmeansCenter[nIndex]);
				if (dDistance < dMinDistance)
				{
					dMinDistance = dDistance;
					nMinIndex = nIndex;
					bFlag = true;
					nCount++;
				};
			};
			liKMeansResult[i] = nMinIndex;
			i++;
		};
		// 重新计算聚类中心
		// 重置中心
		qgskmeansCenter.clear();
		QList<int> liPtKMeansCount;
		for (int nIndex = 0; nIndex < nClusterNum; nIndex++)
		{
			qgskmeansCenter.append(new QgsPoint(0, 0));
			liPtKMeansCount.append(0);
		};
		// 记录每个聚类的点数
		for (int j = 0; j < liPoints.size(); j++)
		{
			int ptCenterIndex = liKMeansResult[j];
			QgsPoint *ptCenter = qgskmeansCenter[ptCenterIndex];
			// ptCenter->setX(ptCenter->x() + liPoints[j]->centroid().asPoint().x());
			// ptCenter->setY(ptCenter->y() + liPoints[j]->centroid().asPoint().y());
			ptCenter->setX(ptCenter->x() + liPoints[j]->x());
			ptCenter->setY(ptCenter->y() + liPoints[j]->y());
			qgskmeansCenter[ptCenterIndex] = ptCenter;
			liPtKMeansCount[ptCenterIndex]++;
		};
		// 取平均
		for (int nIdx = 0; nIdx < qgskmeansCenter.size(); nIdx++)
		{
			double dPointNum = liPtKMeansCount[nIdx];
			qgskmeansCenter[nIdx]->setX(qgskmeansCenter[nIdx]->x() / dPointNum);
			qgskmeansCenter[nIdx]->setY(qgskmeansCenter[nIdx]->y() / dPointNum);
			// qDebug() << "center:" << nIdx << " x:" << qgskmeansCenter[nIdx]->x() << " y:" << qgskmeansCenter[nIdx]->y();
		};
		//qDebug() << "nCount:" << nCount;
		int progress = (liPoints.size() - nCount) * 100 / liPoints.size();
		progress<0?progress=0:progress;
		qDebug() << "progress：" << progress;
		
		emit updateProgressValue(progress);
	}
	return liKMeansResult;
}

// alglib计算KMeans
// BUG: 局部最优  链接器效率低
// QList<int> MainWidget::calculateKMeans(QList<QgsPoint *> liPoints, int nClusterNum)
//{
//	// 将QgsPoint转换为alglib库可以处理的格式
//	alglib::real_2d_array data;
//	data.setlength(liPoints.size(), 2);
//	for (int i = 0; i < liPoints.size(); ++i)
//	{
//		data[i][0] = liPoints[i]->x();
//		data[i][1] = liPoints[i]->y();
//	}
//
//	// 创建聚类器状态和报告
//	alglib::clusterizerstate s;
//	alglib::kmeansreport rep;
//
//	// 初始化聚类器
//	clusterizercreate(s);
//	clusterizersetpoints(s, data, 2);		   // 使用欧几里得距离
//	clusterizersetkmeanslimits(s, 5, 0);	   // 设置随机重启次数为5
//	clusterizerrunkmeans(s, nClusterNum, rep); // 运行KMeans算法
//
//	// 将聚类结果转换回QList<int>格式
//	QList<int> liKMeansResult;
//	for (int i = 0; i < liPoints.size(); ++i)
//	{
//		liKMeansResult.append(rep.cidx[i]);
//	}
//
//	return liKMeansResult;
//}

// 新建KMeans聚类分析图层
void MainWidget::createKMeans()
{
	// 新建矢量图层
	QgsVectorLayer *pqvlKMeansLayer = new QgsVectorLayer("Point?crs=epsg:4326", "KMeans", "memory");
	// 选择窗口
	KMeansSelect *ctrlChooseWidget = new KMeansSelect();
	ctrlChooseWidget->setMapLayers(mliLayersList); // 传入所有图层
	ctrlChooseWidget->exec();					   // 显示窗口

	QProgressDialog *progressDialog = new QProgressDialog("KMeans聚类中...", "取消", 0, 100, this);
	progressDialog->setWindowModality(Qt::WindowModal);
	progressDialog->setMinimumDuration(0);
	progressDialog->setAutoClose(true);
	progressDialog->setAutoReset(true);
	progressDialog->setCancelButton(nullptr);
	progressDialog->show();
	// 连接信号
	connect(this, &MainWidget::updateProgressValue, progressDialog, &QProgressDialog::setValue);

	// 如果未选择图层，返回
	if (ctrlChooseWidget->getSelectLayer() == nullptr)
	{
		delete ctrlChooseWidget;
		return;
	}
	// 获取选择的图层和聚类数
	QgsVectorLayer *qvlLayer = ctrlChooseWidget->getSelectLayer();
	int nClusterNum = ctrlChooseWidget->getClusterNum();
	// 将选择的图层深拷贝到新图层
	pqvlKMeansLayer = qvlLayer->clone();
	// 获取矢量图层中所有点的坐标
	QList<QgsPoint *> liPoints = getQgsPoints(pqvlKMeansLayer);
	//  获取字段数量
	int nFieldCount = pqvlKMeansLayer->fields().count();
	//  获取图层名
	QString qstrLayerName = pqvlKMeansLayer->name();

	 //聚类
	QList<int> liKMeansResult = calculateKMeans(liPoints, nClusterNum);
	delete ctrlChooseWidget;
	

	// 启用编辑
	pqvlKMeansLayer->startEditing();
	// 给当前图层添加字段
	QString qstrFieldName = "Cluster";
	// 判断是否已经存在该字段  qgs会自己添加后缀，程序会丢失字段名
	int nDiffName = 0;
	if (pqvlKMeansLayer->fields().indexOf(qstrFieldName) != -1)
	{
		QString _qstrTemp = qstrFieldName + QString("%1").arg(nDiffName);
		while (pqvlKMeansLayer->fields().indexOf(_qstrTemp) != -1)
		{
			qDebug() << "字段名已存在";
			nDiffName++;
			_qstrTemp = qstrFieldName + QString("%1").arg(nDiffName);
		}
		qstrFieldName = qstrFieldName + QString("%1").arg(nDiffName);
	}
	// 新建字段
	QgsField qfNewFiled(qstrFieldName, QVariant::Int);
	QgsVectorDataProvider *pqdpProvider = pqvlKMeansLayer->dataProvider();
	pqdpProvider->addAttributes({qfNewFiled});
	pqvlKMeansLayer->updatedFields();
	pqvlKMeansLayer->commitChanges();

	pqvlKMeansLayer->startEditing();
	QgsFeatureList lifeatureList;
	int nFieldCountNew = pqvlKMeansLayer->fields().count();
	qDebug() << "字段数量:" << nFieldCountNew;
	int nNewFieldIndex = pqvlKMeansLayer->fields().indexOf(qstrFieldName);
	qDebug() << "字段索引：" << nNewFieldIndex;
	progressDialog->setValue(0);
	progressDialog->setLabelText("将结果写入到新图层");
	/*QgsFeatureIterator featureIterator = pqvlKMeansLayer->getFeatures();
	QgsFeature feature;*/
  //#pragma omp parallel for
	for (int nIndex = 0; nIndex < liKMeansResult.size(); nIndex++)
	{
		QgsFeature feature = pqvlKMeansLayer->getFeature(nIndex);
		feature.setAttributes(pqvlKMeansLayer->getFeature(nIndex).attributes());
		feature.setAttribute(nNewFieldIndex, liKMeansResult[nIndex]);
		lifeatureList.append(feature);
		emit updateProgressValue(nIndex * 100 / liKMeansResult.size()+1);
		qDebug() << "id:" << nIndex << "src:" << liKMeansResult[nIndex] << " cluster:" << feature.attribute(qstrFieldName).toInt();
	}

	// 清理pqvlKMeansLayer所有要素
	pqvlKMeansLayer->dataProvider()->deleteFeatures(QgsFeatureIds(pqvlKMeansLayer->allFeatureIds()));
	pqvlKMeansLayer->addFeatures(lifeatureList);
	qDebug() << "cluster finish";
	// 结束编辑并保存
	pqvlKMeansLayer->commitChanges();
	// 将新建图层添加到地图
	pqvlKMeansLayer->setName(qstrLayerName + "_KMeans");
	setLayerToMap(static_cast<QgsMapLayer *>(pqvlKMeansLayer));
	// 根据字段上色
	colorFeaturesByField(pqvlKMeansLayer, qstrFieldName, nClusterNum);
	QMessageBox::information(this, "info", "聚类完成");
}
// 连接字段
/*
Description:
	连接字段
Parameters:
	QgsVectorLayer *pvlLayerResult  结果图层
	QgsVectorLayer *pvlLayerCompare  比对图层
	int nResultIdx  结果图层要素索引
	int nCompareIdx  比对图层要素索引
Return:
	void 
*/
void MainWidget::connectFiled(QgsVectorLayer *pvlLayerResult, QgsVectorLayer *pvlLayerCompare, int nResultIdx, int nCompareIdx)
{

	int nCompareFieldNum = pvlLayerCompare->fields().count();
	int nResutlFieldNum = pvlLayerResult->fields().count();
	// 新建要素集
	//pvlLayerResult->startEditing();
	QgsFeature featureResult = pvlLayerResult->getFeature(nResultIdx);
	 QgsFeature featureCompare = pvlLayerCompare->getFeature(nCompareIdx);
	 featureResult.setAttributes(pvlLayerResult->getFeature(nResultIdx).attributes());
	//pvlLayerResult->updateFeature(featureResult);
	 for (int nNewFieldIdx = 0; nNewFieldIdx < nCompareFieldNum; nNewFieldIdx++)
	{
		int nWriteFieldIdx = nNewFieldIdx + nResutlFieldNum - nCompareFieldNum;
		featureResult.setAttribute(nWriteFieldIdx, featureCompare.attribute(nNewFieldIdx));
		pvlLayerResult->updateFeature(featureResult);
	 }
	//pvlLayerResult->commitChanges();
}
// 构建空间连接图层
void MainWidget::connectByLocation()
{
	// 新建矢量图层
	QgsVectorLayer *pqvlConnectLayer = new QgsVectorLayer("Point?crs=epsg:4326", "Connect", "memory");
	// 选择图层窗口
	ConnectSelect *ctrlChooseWidget = new ConnectSelect();
	ctrlChooseWidget->setMapLayers(mliLayersList);
	ctrlChooseWidget->exec();

	QProgressDialog* progressDialog = new QProgressDialog("空间连接中。。。", "取消", 0, 100, this);
	progressDialog->setWindowModality(Qt::WindowModal);
	progressDialog->setMinimumDuration(0);
	progressDialog->setAutoClose(true);
	progressDialog->setAutoReset(true);
	progressDialog->setCancelButton(nullptr);
	progressDialog->show();
	progressDialog->setValue(0);
	connect(this, &MainWidget::updateProgressValue, progressDialog, &QProgressDialog::setValue);
	// 获取选择的图层
	QgsVectorLayer *qvlLayerResult = ctrlChooseWidget->getSelectLayerResult();
	QgsVectorLayer *qvlLayerCompare = ctrlChooseWidget->getSelectLayerCompare();
	QString qstrConnectType = ctrlChooseWidget->getConnectType();
	// 删除选择窗口
	delete ctrlChooseWidget;
	// 如果未选择图层，返回
	if (qvlLayerResult == nullptr || qvlLayerCompare == nullptr)
	{
		QMessageBox::critical(this, "error", "请选择图层");
		return;
	}
	// 如果未选择连接类型，返回
	if (qstrConnectType.isEmpty())
	{
		QMessageBox::critical(this, "error", "请选择连接类型");
		return;
	}
	// 深拷贝连接图层
	pqvlConnectLayer = qvlLayerResult->clone();
	// 新建要素集
	QList<QgsGeometry *> liGeometriesResult;
	QList<QgsGeometry *> liGeometriesCompare;
	liGeometriesResult = getQgsGeometries(pqvlConnectLayer);
	liGeometriesCompare = getQgsGeometries(qvlLayerCompare);

	// 获取目标图层字段数量
	int nResultFieldNum = pqvlConnectLayer->fields().count();
	// 获取比对图层字段数量
	int nCompareFieldNum = qvlLayerCompare->fields().count();
	// 添加字段到结果图层
	pqvlConnectLayer->startEditing();
	QgsVectorDataProvider *pqdpProvider = pqvlConnectLayer->dataProvider();
	QgsFields qfFields = qvlLayerCompare->fields();
	for (int nIndex = 0; nIndex < qfFields.count(); nIndex++)
	{
		pqdpProvider->addAttributes({qfFields[nIndex]});
	}
	pqvlConnectLayer->updateFields();
	pqvlConnectLayer->commitChanges();
	// 一对一
	// 判断比对图层中的点要素是否在结果图层的面要素内部中
	QgsFeatureList lifeatureList;
	pqvlConnectLayer->startEditing();
#pragma omp parallel for
	for (int nResultIdx = 0; nResultIdx < liGeometriesResult.size(); nResultIdx++)
	{
		// QgsPoint* point = liGeometriesCompare[nCompareIdx];
		for (int nCompareIdx = 0; nCompareIdx < liGeometriesCompare.size(); nCompareIdx++)
		{
			QgsGeometry pointGeometry = *liGeometriesCompare[nCompareIdx];
			QgsGeometry polygonGeometry = *liGeometriesResult[nResultIdx];
			// 包含
			if (polygonGeometry.contains(pointGeometry) && qstrConnectType == "包含")
			{
				connectFiled(pqvlConnectLayer, qvlLayerCompare, nResultIdx, nCompareIdx);
				break;
			}
			// 相交
			else if (polygonGeometry.intersects(pointGeometry) && qstrConnectType == "相交")
			{
				connectFiled(pqvlConnectLayer, qvlLayerCompare, nResultIdx, nCompareIdx);
				break;
			}
			// 相接
			else if (polygonGeometry.touches(pointGeometry) && qstrConnectType == "相离")
			{
				connectFiled(pqvlConnectLayer, qvlLayerCompare, nResultIdx, nCompareIdx);
				break;
			}
			// 相等
			else if (polygonGeometry.equals(pointGeometry) && qstrConnectType == "重叠")
			{
				connectFiled(pqvlConnectLayer, qvlLayerCompare, nResultIdx, nCompareIdx);
				break;
			}
			// 交叉
			else if (polygonGeometry.crosses(pointGeometry) && qstrConnectType == "交叉")
			{
				connectFiled(pqvlConnectLayer, qvlLayerCompare, nResultIdx, nCompareIdx);
				break;
			}
			// 重叠
			else if (polygonGeometry.overlaps(pointGeometry) && qstrConnectType == "重叠")
			{
				connectFiled(pqvlConnectLayer, qvlLayerCompare, nResultIdx, nCompareIdx);
				break;
			}
		}
		//emit updateProgressValue(nResultIdx * 100 / liGeometriesResult.size()+1);
		qDebug() << "nResultIdx:" << nResultIdx;
	}
	pqvlConnectLayer->commitChanges();
	// 添加到当前画布
	pqvlConnectLayer->setName(qvlLayerResult->name() + "_Connect");
	setLayerToMap(static_cast<QgsMapLayer *>(pqvlConnectLayer));
	QMessageBox::information(this, "info", "连接完成");
}
// 统计栅格数据
void MainWidget::rasterStatistics()
{
	// 新建选择对话框
	RasterStatisticsSelect *ctrlChooseWidget = new RasterStatisticsSelect();
	ctrlChooseWidget->setMapLayers(mliLayersList); // 传入所有图层
	ctrlChooseWidget->exec();					   // 显示窗口
	// 获取选择的图层
	QgsRasterLayer *rasterLayer = ctrlChooseWidget->getSelectLayer();
	int nBandID = ctrlChooseWidget->getBandID();
	delete ctrlChooseWidget;
	// 如果未选择图层，返回
	if (rasterLayer == nullptr)
	{
		QMessageBox::critical(this, "error", "请选择图层");
		return;
	}

	if (!rasterLayer->isValid())
	{
		qDebug() << "栅格图像加载失败！";
		return;
	}

	// 获取栅格图像的数据提供者
	QgsRasterDataProvider *provider = rasterLayer->dataProvider();

	// 获取栅格图像的统计信息
	QgsRasterBandStats stats = provider->bandStatistics(1, QgsRasterBandStats::All, rasterLayer->extent(), 0);
	// 获取图层名
	QString qstrLayerName = rasterLayer->name();
	// 显示统计信息
	QStringList qstrResult;
	qstrResult.append("输入信息");
	qstrResult.append(QString("    图层名：%1").arg(qstrLayerName));
	qstrResult.append(QString("    统计波段：%1").arg(nBandID));
	qstrResult.append("统计结果");
	// 固定9位小数
	qstrResult.append(QString("    最大值：%1").arg(stats.maximumValue, 0, 'f', 9));
	qstrResult.append(QString("    最小值：%1").arg(stats.minimumValue, 0, 'f', 9));
	qstrResult.append(QString("    平均值：%1").arg(stats.mean, 0, 'f', 9));
	qstrResult.append(QString("    范围：%1").arg(stats.range, 0, 'f', 9));
	qstrResult.append(QString("    总和：%1").arg(stats.sum, 0, 'f', 9));
	qstrResult.append(QString("    标准差：%1").arg(stats.stdDev, 0, 'f', 9));
	qstrResult.append(QString("    平方和：%1").arg(stats.sumOfSquares, 0, 'f', 9));

	// 将统计结果显示到 listView 中
	// 清空listView
	if (ui.ctrlStatisticsView->model() != nullptr)
	{
		ui.ctrlStatisticsView->model()->destroyed();
	}
	// 创建模型
	QStringListModel *pslmmodel = new QStringListModel();
	// 设置数据
	pslmmodel->setStringList(qstrResult);
	// 将模型设置为 QListView 的模型
	ui.ctrlStatisticsView->setModel(pslmmodel);
	// 刷新listview
	ui.ctrlStatisticsView->update();

	// 打印调试信息
	qDebug() << "最大值：" << stats.maximumValue;
	qDebug() << "最小值：" << stats.minimumValue;
	qDebug() << "平均值：" << stats.mean;
	qDebug() << "范围：" << stats.range;
	qDebug() << "总和：" << stats.sum;
	qDebug() << "标准差：" << stats.stdDev;
	qDebug() << "平方和：" << stats.sumOfSquares;
}