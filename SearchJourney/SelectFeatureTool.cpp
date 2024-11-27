#include "SelectFeatureTool.h"
#include "qgsapplication.h"
#include "qgsvectorlayer.h"

QgsMapToolSelectFeatures::QgsMapToolSelectFeatures(QgsMapCanvas* mapCanvas, QgsVectorLayer* targetLayer)
    : QgsMapToolIdentify(mapCanvas), mpTargetLayer(targetLayer)
{

}

void QgsMapToolSelectFeatures::canvasMoveEvent(QgsMapMouseEvent* e)
{
    if (e->buttons() != Qt::LeftButton)
        return;
    QRect rect;
    if (!mSelectionActive)
    {
        mSelectionActive = true;
        rect = QRect(e->pos(), e->pos());
    }
    else
    {
        rect = QRect(e->pos(), mInitDragPos);
    }
    if (mSelectionRubberBand)
        mSelectionRubberBand->setToCanvasRectangle(rect);
}

void QgsMapToolSelectFeatures::canvasPressEvent(QgsMapMouseEvent* e)
{
    if (!mSelectionRubberBand)
        initRubberBand();
    mInitDragPos = e->pos();
}

void QgsMapToolSelectFeatures::canvasReleaseEvent(QgsMapMouseEvent* e)
{
    QPoint point = e->pos() - mInitDragPos;
    //点
    if (!mSelectionActive || (point.manhattanLength() < QApplication::startDragDistance()))
    {
        mSelectionActive = false;
        mSelectGeometry = QgsGeometry::fromPointXY(toMapCoordinates(e->pos()));
        identifyFromGeometry();
    }
    //矩形
    if (mSelectionRubberBand && mSelectionActive)
    {
        mSelectGeometry = mSelectionRubberBand->asGeometry();
        mSelectionRubberBand.reset();
        identifyFromGeometry();
    }
    mSelectionActive = false;
}

void QgsMapToolSelectFeatures::initRubberBand()
{
    mSelectionRubberBand.reset(new QgsRubberBand(mCanvas, Qgis::GeometryType::Polygon));
    mSelectionRubberBand->setFillColor(mFillColor);
    mSelectionRubberBand->setStrokeColor(mStrokeColor);
}

void QgsMapToolSelectFeatures::identifyFromGeometry()
{
    if (mCanvas && mpTargetLayer)
    {
        mCanvas->setSelectionColor(Qt::red);
        mpTargetLayer->removeSelection();

        // 使用目标图层构建图层列表
        QList<QgsMapLayer*> layerList = { mpTargetLayer };

        // 执行 identify 操作，仅在指定图层中查找符合选择几何体的图元
        QList<IdentifyResult> results = QgsMapToolIdentify::identify(
            mSelectGeometry, IdentifyMode::TopDownAll, layerList, AllLayers);

        QList<QgsFeature> selectFeatures;
        QgsFeatureIds ids;

        for (const IdentifyResult& var : results)
        {
            QgsFeature _Feature = var.mFeature;
            ids.insert(_Feature.id());
            selectFeatures.append(_Feature);
        }

        if (ids.count() > 0)
            mpTargetLayer->selectByIds(ids);

        // 发出选中的 Feature 信息信号
        if (selectFeatures.count() > 0)
            emit sigSelectFeatureChange(selectFeatures);
    }
}