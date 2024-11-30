#include "Clip.h"
#include <QFileDialog>
Clip::Clip(QWidget *parent, QgsMapCanvas* mapcanvas, QgsProject* project)
	: QDialog(parent), mpMapCanvas(mapcanvas), mppjProject(project)
{
	ui.setupUi(this);

    ui.checkBox_output->setChecked(true);

	//populateLayers();
	connect(ui.comboBox_Type, QOverload<int>::of(&QComboBox::activated), this, &Clip::userChoice);
	connect(ui.pushButton_ok_, &QPushButton::clicked, this, &Clip::beginClip);
    connect(ui.pushButton_output, &QPushButton::clicked, this, &Clip::output);

}

Clip::~Clip()
{}

void Clip::setMapLayers(QList<QgsMapLayer*> liMapLayers)
{
    this->mliMapLayers = liMapLayers;
    populateLayers();
}

void Clip::populateLayers() {
    ui.comboBox_Input->clear();
    ui.comboBox_Type->addItem("矩形");
    ui.comboBox_Type->addItem("圆形");
    ui.comboBox_Type->addItem("自定义多边形");

    for (int nIndex = 0; nIndex < mliMapLayers.count(); nIndex++)
    {
        // 判断是否为矢量图层
        if (mliMapLayers[nIndex]->type() != Qgis::LayerType::Vector)
        {
            continue;
        }
        ui.comboBox_Input->addItem(mliMapLayers[nIndex]->name());
    }
}

void Clip::userChoice() {

    QString targetLayerId = ui.comboBox_Input->currentText();
    QgsVectorLayer* targetLayer;

    // 根据图层名，找到对应矢量图层
    for (QgsMapLayer* layer : mliMapLayers)
    {
        if (layer->name() == targetLayerId)
        {
            targetLayer = dynamic_cast<QgsVectorLayer*>(layer);
        }
        else {
            QMessageBox::warning(this, "Error", "Invalid target layer.");
            return;
        }
    }

    if (ui.comboBox_Type->currentText() == "矩形") {
        mRectDraw = new RectDrawingTool(mpMapCanvas, targetLayer);
        mpMapCanvas->setMapTool(mRectDraw);
    }
    else if (ui.comboBox_Type->currentText() == "圆形") {
        mCircleDraw = new CircleDrawingTool(mpMapCanvas, targetLayer);
        mpMapCanvas->setMapTool(mCircleDraw);
    }
    else if (ui.comboBox_Type->currentText() == "自定义多边形") {
        mPolygonDraw = new PolygonDrawingTool(mpMapCanvas, targetLayer);
        mpMapCanvas->setMapTool(mPolygonDraw);
    }
}

void Clip::beginClip() {
    if (ui.comboBox_Type->currentText() == "矩形") {
        mClipGeometry = mRectDraw->getGeometry();
        mpMapCanvas->unsetMapTool(mRectDraw);
    }
    else if (ui.comboBox_Type->currentText() == "圆形") {
        mClipGeometry = mCircleDraw->getGeometry();
        mpMapCanvas->unsetMapTool(mCircleDraw);
    }
    else if (ui.comboBox_Type->currentText() == "自定义多边形") {
        mClipGeometry = mPolygonDraw->getGeometry();
        mpMapCanvas->unsetMapTool(mPolygonDraw);
    }
    clipVectorLayers();
};

void Clip::clipVectorLayers() {
    QString targetLayerId = ui.comboBox_Input->currentText();
    QgsVectorLayer* targetLayer;

    // 根据图层名，找到对应矢量图层
    for (QgsMapLayer* layer : mliMapLayers)
    {
        if (layer->name() == targetLayerId)
        {
            targetLayer = dynamic_cast<QgsVectorLayer*>(layer);
        }
    }

    // 检查目标图层是否有效
    if (!targetLayer)
    {
        QMessageBox::warning(this, "Error", "Invalid target layer.");
        return;
    }

    // 准备结果图层
    QgsFields targetFields = targetLayer->fields();
    QgsCoordinateReferenceSystem crs = targetLayer->crs();

    QgsFeatureIterator targetFeatures = targetLayer->getFeatures();
    QgsVectorLayer* clippedLayer = nullptr;

    QgsFeature targetFeature;
    if (targetLayer->geometryType() == Qgis::GeometryType::Polygon) {
        clippedLayer = new QgsVectorLayer("Polygon?crs=" + crs.authid(), "Clipped Layer", "memory");
    }
    else if (targetLayer->geometryType() == Qgis::GeometryType::Line) {
        clippedLayer = new QgsVectorLayer("LineString?crs=" + crs.authid(), "Clipped Layer", "memory");
    }
    else {
        clippedLayer = new QgsVectorLayer("Point?crs=" + crs.authid(), "Clipped Layer", "memory");
    }

    QgsVectorDataProvider* clippedProvider = clippedLayer->dataProvider();
    clippedProvider->addAttributes(targetFields.toList());
    clippedLayer->updateFields();

    // 执行裁剪
    QgsFeatureList clippedFeatures;

    // 遍历目标图层中的所有要素
    while (targetFeatures.nextFeature(targetFeature))
    {
        QgsGeometry targetGeometry = targetFeature.geometry();

        // 针对面和多面体要素（Polygon, MultiPolygon）
        if (targetGeometry.type() == Qgis::GeometryType::Polygon)
        {
            QgsGeometry intersectionGeometry = targetGeometry.intersection(mClipGeometry);
            if (!intersectionGeometry.isEmpty())
            {
                QgsFeature clippedFeature(targetFeature);
                clippedFeature.setGeometry(intersectionGeometry);
                clippedFeatures.append(clippedFeature);
            }
        }
        // 针对线要素（LineString, MultiLineString）
        else if (targetGeometry.type() == Qgis::GeometryType::Line)
        {
            QgsGeometry intersectionGeometry = targetGeometry.intersection(mClipGeometry);
            if (!intersectionGeometry.isEmpty())
            {
                QgsFeature clippedFeature(targetFeature);
                clippedFeature.setGeometry(intersectionGeometry);
                clippedFeatures.append(clippedFeature);
            }
        }
        // 针对点要素（Point）
        else if (targetGeometry.type() == Qgis::GeometryType::Point)
        {
            // 判断点是否在裁剪几何内
            if (mClipGeometry.contains(targetGeometry))
            {
                QgsFeature clippedFeature(targetFeature);
                clippedFeatures.append(clippedFeature);
            }
        }
    }

    // 将裁剪结果添加到临时图层
    clippedProvider->addFeatures(clippedFeatures);

    if (ui.checkBox_output->isChecked()) {
        QString newFileName = ui.textEdit_output->toPlainText();

        if (newFileName.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Please specify a file name for the output layer."));
            return;
        }

        QgsVectorFileWriter::writeAsVectorFormat(clippedLayer, newFileName, "UTF-8", clippedLayer->crs(), "ESRI Shapefile");
        QMessageBox::information(this, tr("Success"), tr("New layer saved to file: %1").arg(newFileName));
    }

    // 将结果图层添加到项目
    mppjProject->addMapLayer(clippedLayer);
    QMessageBox::information(this, "Success", "Clipping completed and layer added to project.");
}
void Clip::output() {
    QString outputPath = QFileDialog::getSaveFileName(this, "Save File", "", "Shapefile (*.shp)");
    if (outputPath.isEmpty()) {
        return;
    }
    ui.textEdit_output->setText(outputPath);
};