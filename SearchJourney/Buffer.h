#pragma once

#include <QDialog>
#include <qfiledialog>
#include <qgsmapcanvas.h>
#include <QString>
#include <qdebug.h>
#include <QMessageBox>
#include <QgsProject.h>
#include <QgsVectorLayer.h>
#include <qgsrasterdataprovider.h>
#include "qgscoordinatereferencesystem.h"
#include <qgsunittypes.h>
#include <qgsvectorfilewriter.h>
#include "GISMapCanvas.h"
#include "ui_Buffer.h"

class Buffer : public QDialog
{
	Q_OBJECT

public:
	Buffer(QWidget *parent = nullptr, GISMapCanvas* mapCanvas = nullptr, QgsProject* project = nullptr);
	~Buffer();
	//void populateLayers();
	void setMapLayers(QList<QgsMapLayer*> liMapLayers);
	void addLayerItems();
	void runBuffer();
	QgsGeometry bufferByMeter(QgsGeometry geo, float off, int segments , QgsVectorLayer* sourceLayer);

private slots:
	void onButtonOutputClicked();
	void onButtonOKClicked();

private:
	Ui::BufferClass ui;

	GISMapCanvas* mpMapCanvas;
	QgsProject* mppjProject;
	QList<QgsMapLayer*> mliMapLayers;
};
