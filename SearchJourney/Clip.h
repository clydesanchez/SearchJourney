#pragma once

#include <QDialog>
#include <QDialog>
#include <qgsmapcanvas.h>
#include <QgsVectorLayer.h>
#include <QgsProject.h>
#include <QgsVectorFileWriter.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsFields.h>
#include <QMessageBox>
#include <QPushButton>
#include "ClipMapTool.h"
#include "ui_Clip.h"

class Clip : public QDialog
{
	Q_OBJECT

public:
	Clip(QWidget *parent = nullptr, QgsMapCanvas* mapcanvas = nullptr, QgsProject* project = nullptr);
	~Clip();

	void userChoice();
	void populateLayers();
	void clipVectorLayers();
	void beginClip();
	void setMapLayers(QList<QgsMapLayer*> liMapLayers);

private:
	Ui::ClipClass ui;

	QgsMapCanvas* mpMapCanvas;
	QgsProject* mppjProject;
	QList<QgsMapLayer*> mliMapLayers;

	QgsGeometry mClipGeometry;

	RectDrawingTool* mRectDraw;
	CircleDrawingTool* mCircleDraw;
	PolygonDrawingTool* mPolygonDraw;
};
