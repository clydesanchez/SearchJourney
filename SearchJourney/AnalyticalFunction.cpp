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
#include <QToolButton>
#include <QFileDialog>
#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <gdal_alg.h>
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

void MainWidget::on_ctrlVecToRasAction_triggered() {
	QDialog* pDl = new QDialog();
	QVBoxLayout* pVbl = new QVBoxLayout();
	QHBoxLayout* pHbl1 = new QHBoxLayout();
	QHBoxLayout* pHbl2 = new QHBoxLayout();
	QHBoxLayout* pHbl3 = new QHBoxLayout();
	QHBoxLayout* pHbl4 = new QHBoxLayout();
	QHBoxLayout* pHbl5 = new QHBoxLayout();
	QHBoxLayout* pHbl6 = new QHBoxLayout();
	QHBoxLayout* pHbl7 = new QHBoxLayout();
	QLabel* pL1 = new QLabel();
	QLabel* pL2 = new QLabel();
	QLabel* pL3 = new QLabel();
	QLabel* pL4 = new QLabel();
	QLabel* pL5 = new QLabel();
	QLabel* pL6 = new QLabel();
	QLineEdit* pLe1 = new QLineEdit();
	QLineEdit* pLe2 = new QLineEdit();
	QLineEdit* pLe3 = new QLineEdit();
	QLineEdit* pLe4 = new QLineEdit();
	QLineEdit* pLe5 = new QLineEdit();
	QLineEdit* pLe6 = new QLineEdit();
	QToolButton* pTb1 = new QToolButton();
	QToolButton* pTb2 = new QToolButton();
	QPushButton* pPb1 = new QPushButton();
	QPushButton* pPb2 = new QPushButton();
	
	pL1->setText(QStringLiteral("输入矢量数据文件"));
	pHbl1->addWidget(pL1);
	pHbl1->addWidget(pLe1);
	pHbl1->addWidget(pTb1);
	
	pL2->setText(QStringLiteral("输入栅格数据输出路径"));
	pHbl2->addWidget(pL2);
	pHbl2->addWidget(pLe2);
	pHbl2->addWidget(pTb2);
	
	pL3->setText(QStringLiteral("输入栅格宽度"));
	pHbl3->addWidget(pL3);
	pHbl3->addWidget(pLe3);
	
	pL4->setText(QStringLiteral("输入栅格高度"));
	pHbl4->addWidget(pL4);
	pHbl4->addWidget(pLe4);

	pL5->setText(QStringLiteral("输入块大小"));
	pHbl5->addWidget(pL5);
	pHbl5->addWidget(pLe5);

	pL6->setText(QStringLiteral("指定属性字段"));
	pHbl6->addWidget(pL6);
	pHbl6->addWidget(pLe6);
	
	pPb1->setText(QStringLiteral("确定"));
	pPb2->setText(QStringLiteral("取消"));
	pHbl7->addWidget(pPb1);
	pHbl7->addWidget(pPb2);
	
	pVbl->addLayout(pHbl1);
	pVbl->addLayout(pHbl2);
	pVbl->addLayout(pHbl3);
	pVbl->addLayout(pHbl4);
	pVbl->addLayout(pHbl5);
	pVbl->addLayout(pHbl6);
	pVbl->addLayout(pHbl7);
	pDl->setLayout(pVbl);
	pDl->resize(800, 600);

	connect(pTb1, &QToolButton::clicked, this, [=]() {
		QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择矢量数据文件"), "", "shapefile (*.shp)");
		pLe1->setText(filePath);
		});

	connect(pTb2, &QToolButton::clicked, this, [=]() {
		QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("选择输出位置"), "", "GeoTiff (*.tif)");
		pLe2->setText(filePath);
		});

	connect(pPb1, &QPushButton::clicked, this, [=]() {
		QString ShpFile = pLe1->text();
		QString TifFile = pLe2->text();

		CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO"); // 支持中文路径
		GDALAllRegister(); // 注册所有驱动
		OGRRegisterAll();  // 注册OGR驱动

		// 打开矢量文件
		GDALDataset* shpData = (GDALDataset*)GDALOpenEx(ShpFile.toStdString().c_str(), GDAL_OF_READONLY, NULL, NULL, NULL);
		if (shpData == NULL)
		{
			return;
		}

		OGRLayer* shpLayer = shpData->GetLayer(0); // 获取第一个图层
		OGREnvelope env;
		shpLayer->GetExtent(&env); // 获取矢量图层的坐标范围

		// 设置栅格的大小
		bool ok1, ok2;
		int m_nImageWidth = pLe3->text().toInt(&ok1); // 宽度
		int m_nImageHeight = pLe4->text().toInt(&ok2); // 高度
		if (!ok1 || !ok2) {
			QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("宽度和高度必须为整数！"));
		}

		OGRSpatialReference* pOgrSRS = shpLayer->GetSpatialRef(); // 获取矢量图层的空间参考
		char* pPrj = NULL;
		if (pOgrSRS == NULL)
		{
			m_nImageHeight = (int)(env.MaxX - env.MinX);
			m_nImageWidth = (int)(env.MinY - env.MaxY);
		}
		else
		{
			pOgrSRS->exportToWkt(&pPrj); // 导出投影信息
		}

		// 获取GTiff驱动并创建新的栅格数据集
		GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
		GDALDataset* poNewDS = poDriver->Create(TifFile.toStdString().c_str(), m_nImageWidth, m_nImageHeight, 1, GDT_Float32, NULL);
		if (poNewDS == NULL)
		{
			GDALClose(shpData);
			return;
		}

		// 设置栅格的地理变换信息
		double adfGeoTransform[6];
		adfGeoTransform[0] = env.MinX; // 左上角X坐标
		adfGeoTransform[1] = (env.MaxX - env.MinX) / m_nImageWidth; // 像元宽度
		adfGeoTransform[2] = 0; // 影像旋转系数
		adfGeoTransform[3] = env.MaxY; // 左上角Y坐标
		adfGeoTransform[4] = 0; // 影像旋转系数
		adfGeoTransform[5] = (env.MinY - env.MaxY) / m_nImageHeight; // 像元高度
		GDALSetGeoTransform(poNewDS, adfGeoTransform); // 设置地理信息

		// 如果有投影信息，设置栅格的投影
		if (pOgrSRS != NULL)
		{
			poNewDS->SetProjection(pPrj);
		}

		// 设置矢量图层栅格化的选项
		char** papszOptions = NULL;
		papszOptions = CSLSetNameValue(papszOptions, "CHUNKSIZE", pLe5->text().toStdString().c_str()); // 设置块大小
		papszOptions = CSLSetNameValue(papszOptions, "ATTRIBUTE", pLe6->text().toStdString().c_str()); // 设置使用的字段名

		// 使用GDAL栅格化函数将矢量数据栅格化到新建的栅格数据集中
		int* pnbandlist = new int[1];
		pnbandlist[0] = 1;
		OGRLayerH* player = new OGRLayerH[1];
		player[0] = (OGRLayerH)shpLayer;

		CPLErr err = GDALRasterizeLayers((GDALDatasetH)poNewDS, 1, pnbandlist, 1, player,
			NULL, NULL, NULL, papszOptions, NULL, NULL);

		// 清理资源
		GDALClose(shpData);  // 关闭矢量数据集
		GDALClose(poNewDS);  // 关闭栅格数据集
		delete[] player;     // 删除临时指针
		delete[] pnbandlist; // 删除临时指针

		// 如果栅格化出现错误，返回0
		if (err != CE_None)
		{
			GDALDestroyDriverManager();
			return;
		}

		// 返回成功
		GDALDestroyDriverManager(); // 销毁驱动管理器
		pDl->accept();
		});

	connect(pPb2, &QPushButton::clicked, this, [=]() {
		pDl->accept();
		});

	pDl->exec();
}