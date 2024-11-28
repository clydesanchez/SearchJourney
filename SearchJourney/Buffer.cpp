#include "Buffer.h"

Buffer::Buffer(QWidget *parent, GISMapCanvas* mapCanvas, QgsProject* project)
	: QDialog(parent), mpMapCanvas(mapCanvas),mppjProject(project)
{
	ui.setupUi(this);

    
    //populateLayers();

	ui.doubleSpinBox_radius->setValue(10);
	ui.spinBox_segments->setValue(36);
	ui.checkBox_output->setChecked(true);

	connect(ui.pushButton_output, &QPushButton::clicked, this, &Buffer::onButtonOutputClicked);
	connect(ui.pushButton_ok, &QPushButton::clicked, this, &Buffer::onButtonOKClicked);
    connect(ui.pushButton_cancel, &QPushButton::clicked, this, &QWidget::close);
}

Buffer::~Buffer()
{
	delete mpMapCanvas;
}

void Buffer::onButtonOKClicked() {
	runBuffer();
};

void Buffer::onButtonOutputClicked() {
	QString fileName = QFileDialog::getSaveFileName(this, tr(""), "", "*.shp");

	if (fileName.isEmpty()) {
		return;
	}

	ui.textEdit_output->setText(fileName);
};

void Buffer::setMapLayers(QList<QgsMapLayer*> liMapLayers)
{
	this->mliMapLayers = liMapLayers;
	addLayerItems();
}

void Buffer::addLayerItems()
{
	for (int nIndex = 0; nIndex < mliMapLayers.count(); nIndex++)
	{
		// 判断是否为矢量图层
		if (mliMapLayers[nIndex]->type() != Qgis::LayerType::Vector)
		{
			continue;
		}
		ui.comboBox_input->addItem(mliMapLayers[nIndex]->name());
	}
}

/*void Buffer::populateLayers()
{
    ui.comboBox_input->clear();


    for (QgsMapLayer* layer : QgsProject::instance()->mapLayers().values())
    {
        ui.comboBox_input->addItem(layer->name(), layer->id());
    }
}*/

void Buffer::runBuffer() {
    QString sourceLayerId = ui.comboBox_input->currentText();
	qDebug() << sourceLayerId;

    double distance = ui.doubleSpinBox_radius->value();
	int segments = ui.spinBox_segments->value();
	QgsVectorLayer* sourceLayer = nullptr;
	// 根据图层名，找到对应矢量图层
	for (QgsMapLayer* layer : mliMapLayers)
	{
		if (layer->name() == sourceLayerId)
		{
			sourceLayer = dynamic_cast<QgsVectorLayer*>(layer);
		}
	}

	if (!sourceLayer) {
		qDebug() << "not found layer";
	}

    //QgsVectorLayer* sourceLayer = qobject_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayer(sourceLayerId));

    QgsVectorDataProvider* dataProvider = sourceLayer->dataProvider();

    QgsFields fields = dataProvider->fields();

	
	QgsVectorLayer* bufferLayer = new QgsVectorLayer("Polygon?crs=EPSG:4326", "Buffer Layer", "memory");
	QgsVectorDataProvider* bufferDataProvider = bufferLayer->dataProvider();

	
	bufferDataProvider->addAttributes(fields.toList());

	
	bufferLayer->startEditing();

	
	QgsFeatureIterator featureIterator = sourceLayer->getFeatures();
	QgsFeatureList bufferFeatures;

	qDebug() << distance;

	QgsFeature feature;
	while (featureIterator.nextFeature(feature))
	{
		QgsGeometry geom = feature.geometry();   
		QgsGeometry bufferedGeometry = bufferByMeter(geom, distance, segments, sourceLayer);
		//QgsGeometry bufferedGeometry = geom.buffer(distance, segments);

		QgsFeature bufferFeature;
		bufferFeature.setGeometry(bufferedGeometry);
		bufferFeature.setAttributes(feature.attributes());

		bufferFeatures.append(bufferFeature);
	}


	bufferDataProvider->addFeatures(bufferFeatures);


	bufferLayer->commitChanges();


	bufferLayer->setName(QString("%1_buffer").arg(sourceLayer->name()));

	if (ui.checkBox_output->isChecked()) {
		QString newFileName = ui.textEdit_output->toPlainText();

		if (newFileName.isEmpty()) {
			QMessageBox::warning(this, tr("Error"), tr("Please specify a file name for the output layer."));
			return;
		}

		QgsVectorFileWriter::writeAsVectorFormat(bufferLayer, newFileName, "UTF-8", bufferLayer->crs(), "ESRI Shapefile");
		QMessageBox::information(this, tr("Success"), tr("New layer saved to file: %1").arg(newFileName));
	}

	mppjProject->addMapLayer(bufferLayer);

	mpMapCanvas->refresh();
	QMessageBox::information(this, "Success", "Buffer completed and layer added to project.");
}

QgsGeometry Buffer::bufferByMeter(QgsGeometry geo, float trans, int segments, QgsVectorLayer* sourceLayer)
{
	if (!sourceLayer)return geo;
	if (trans <= 0)return geo;

	/*// 获取图层的 CRS
	QgsCoordinateReferenceSystem layerCRS = sourceLayer->crs();

	// 获取图层的单位
	Qgis::DistanceUnit unit = layerCRS.mapUnits();
	double factor = QgsUnitTypes::fromUnitToUnitFactor(Qgis::DistanceUnit::Meters, unit);

	qDebug()<< "factor: " << factor;

	double distance = trans / factor;
	
	qDebug()<< "distance: " << distance;

	return geo.buffer(distance, segments);*/

	
	// 获取图层的 CRS
	QgsCoordinateReferenceSystem layerCRS = sourceLayer->crs();

	// 获取图层的单位
	Qgis::DistanceUnit unit = layerCRS.mapUnits();
	double factor = 1.0;

	// 检查单位类型并手动处理转换因子
	if (unit == Qgis::DistanceUnit::Meters) {
		factor = 1.0; // 米到米
	}
	else if (unit == Qgis::DistanceUnit::Degrees) {
		factor = 1.0 / 111000.0; // 大约每度 111 公里
	}
	double distance = factor * trans;
	return geo.buffer(distance, 36);
}