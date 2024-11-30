#include "Buffer.h"

Buffer::Buffer(QWidget *parent, GISMapCanvas* mapCanvas, QgsProject* project)
	: QDialog(parent), mpMapCanvas(mapCanvas),mppjProject(project)
{
	ui.setupUi(this);

	ui.doubleSpinBox_radius->setValue(10);
	ui.spinBox_segments->setValue(36);
	ui.checkBox_output->setChecked(true);

	// 添加端点样式选项
	ui.endCapStyleComboBox->addItem("平直端点");
	ui.endCapStyleComboBox->addItem("圆形端点");
	ui.endCapStyleComboBox->addItem("方形端点");

	// 添加连接样式选项
	ui.joinStyleComboBox->addItem("圆角连接");
	ui.joinStyleComboBox->addItem("斜切连接");
	ui.joinStyleComboBox->addItem("尖角连接");

	connect(ui.pushButton_output, &QPushButton::clicked, this, &Buffer::onButtonOutputClicked);
	connect(ui.pushButton_ok, &QPushButton::clicked, this, &Buffer::onButtonOKClicked);
    connect(ui.pushButton_cancel, &QPushButton::clicked, this, &QWidget::close);
}

Buffer::~Buffer()
{
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

    QgsVectorDataProvider* dataProvider = sourceLayer->dataProvider();

    QgsFields fields = dataProvider->fields();

	// 获取图层的 CRS
	QgsCoordinateReferenceSystem layerCRS = sourceLayer->crs();

	QgsVectorLayer* bufferLayer = new QgsVectorLayer("Polygon?crs=" + layerCRS.authid(), "Buffer Layer", "memory");
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
	
	// 获取端点样式
	QString endCapStyleStr = ui.endCapStyleComboBox->currentText();
	Qgis::EndCapStyle endCapStyle = Qgis::EndCapStyle::Flat; // 默认值
	if (endCapStyleStr == "圆形端点") {
		endCapStyle = Qgis::EndCapStyle::Round;
	}
	else if (endCapStyleStr == "方形端点") {
		endCapStyle = Qgis::EndCapStyle::Square;
	}

	// 获取连接样式
	QString joinStyleStr = ui.joinStyleComboBox->currentText();
	Qgis::JoinStyle joinStyle = Qgis::JoinStyle::Round; // 默认值
	if (joinStyleStr == "斜切连接") {
		joinStyle = Qgis::JoinStyle::Bevel;
	}
	else if (joinStyleStr == "尖角连接") {
		joinStyle = Qgis::JoinStyle::Miter;
	}

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
		factor = 1.0 / 111000.0; // 每度大约 111 公里
	}
	double distance = factor * trans;
	return geo.buffer(distance, segments, endCapStyle, joinStyle, 10.0);
}