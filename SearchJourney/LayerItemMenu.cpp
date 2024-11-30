#include "LayerItemMenu.h"
#include <QMenu>
#include <qgsvectorlayer.h>
#include <qgsrasterlayer.h>
#include <QgsLayertree.h>
#include <QgsLayerTreeViewDefaultActions.h>
#include <qgsattributetableview.h>
#include <qgsattributetablemodel.h>
#include <qgsattributetablefiltermodel.h>
#include <qgsvectorlayercache.h>
#include <qmessagebox.h>
#include <qgsstylemanagerdialog.h>
#include <qgssymbolselectordialog.h>
#include <qgssymbol.h>
#include "SymbolManger.h"
#include "CustomAttributeTableView.h"
#include <QgsAttributeTableModel.h>
#include "AttributeViewWidget.h"
#include <qgsfeature.h>
#include <qgsattributedialog.h>
#include <qgsattributeeditorcontext.h>
#include <QInputDialog>
#include"RasterStyle.h"
#include "StyleManager.h"
#include <qgsrasterdataprovider.h>
#include <QMessageBox>
#include <qgssinglebandpseudocolorrenderer.h>

#include <QgsSingleSymbolRenderer.h>
#include <QgsTextFormat.h>
LayerItemMenu::LayerItemMenu(QgsLayerTreeView*view, QgsMapCanvas *canvas, MainWidget* widMain,QgsProject* prjSrc)
	: QgsLayerTreeViewMenuProvider()
{
	mcanMapCanvas = canvas;
	mctrlLayerItem = new QgsLayerTreeView(view);
	mctrlLayerItem = view;
	mwidMain = widMain;
	mprjProject = prjSrc;
}

//LayerItemMenu::~LayerItemMenu()
//{
//	delete mctrlLayerItem;
//}
// 创建右键菜单
QMenu* LayerItemMenu::createContextMenu()
{
	QMenu* menu = new QMenu(mctrlLayerItem);
	QModelIndex nIndex = mctrlLayerItem->currentIndex();
	QgsLayerTreeNode* node = mctrlLayerItem->index2node(nIndex);


	if (QgsLayerTree::isLayer(node)) {
		QgsMapLayer* layer = QgsLayerTree::toLayer(node)->layer();
		// 矢量图层菜单
		if (layer&& layer->type() == Qgis::LayerType::Vector) {
			QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
			Qgis::GeometryType layerType = vectorLayer->geometryType();


			// 创建一个 QgsMapSettings 对象
			QgsMapSettings mapSettings;
			mapSettings.setLayers(QList<QgsMapLayer*>() << vectorLayer); // 设置图层
			QgsRenderContext renderContext = QgsRenderContext::fromMapSettings(mapSettings);
			// 获取当前图层的符号
			QgsSymbolList symbollist = vectorLayer->renderer()->symbols(renderContext);

			menu->addAction(actionZoomToLayer(mcanMapCanvas,menu));//缩放到图层
			menu->addAction(actionRemoveLayer(layer->name()));	//移除图层
			menu->addAction(actionSymbolManger(layer->name(), symbollist));	//符号管理
			menu->addAction(actionLabelManger(layer->name()));	//标注管理
			menu->addAction(actionShowProperties(layer->name(),vectorLayer));	//属性表
			menu->addAction(actionCrsTransform_vec( vectorLayer));	//坐标转换
            menu->addAction(actionStyleManager(layer->name(), layerType, vectorLayer));		//符号库
		}
		// 栅格图层菜单
		else if (layer) {
			QgsRasterLayer* rasterLayer = qobject_cast<QgsRasterLayer*>(layer);
			menu->addAction(actionZoomToLayer(mcanMapCanvas, menu));//缩放到图层
			menu->addAction(actionRemoveLayer(layer->name()));	//移除图层
			menu->addAction(actionSymbolManger_ras(layer->name(), rasterLayer));	//符号管理
			menu->addAction(actionRasterOpacity(rasterLayer));	// 不透明度
			menu->addAction(actionCrsTransform_ras( rasterLayer));	//坐标转换
		}
	}
	return menu;
}
// 缩放到图层
QAction* LayerItemMenu::actionZoomToLayer(QgsMapCanvas* canvas, QMenu* menu)
{
    QAction* action = new QAction("缩放到图层");
	// 点击节点
    QgsLayerTreeNode* node = mctrlLayerItem->index2node(mctrlLayerItem->currentIndex());
    QgsMapLayer* layer = QgsLayerTree::toLayer(node)->layer();
    QObject::connect(action, &QAction::triggered, [canvas, layer]() {
        QgsRectangle extent = layer->extent();
        canvas->setExtent(extent);
        canvas->refresh();
    });
    return action;
}
// 移除图层
QAction* LayerItemMenu::actionRemoveLayer(QString strLayerName)
{
	QAction* action = new QAction("移除图层");
	QgsProject* prjSrc = mprjProject;
	QgsMapCanvas* canvas = mcanMapCanvas;
	QObject::connect(action, &QAction::triggered, [strLayerName, prjSrc,canvas]() {
		// 根据名称获取图层
		QgsVectorLayer* pvlLayer = nullptr;
		QgsRasterLayer* prlLayer = nullptr;
		QList<QgsMapLayer*> layers = prjSrc->mapLayersByName(strLayerName);
		if (layers.size() > 0)
		{
			pvlLayer = dynamic_cast<QgsVectorLayer*>(layers.at(0));
			prlLayer = dynamic_cast<QgsRasterLayer*>(layers.at(0));
		}
		if (!pvlLayer&&!prlLayer)
		{
			QMessageBox::critical(nullptr, "error", QString("图层不存在: \n") + strLayerName);
			return;
		}
		// 删除图层
		prjSrc->removeMapLayer(pvlLayer);
		prjSrc->removeMapLayer(prlLayer);
		// 更新画布
		canvas->refresh();
		});
	return action;
}
// 符号管理右键菜单
QAction* LayerItemMenu::actionSymbolManger(QString strLayerName, QgsSymbolList Srcsymbol)
{
	QAction* action = new QAction("符号管理");
	MainWidget* widMain = mwidMain;
	QgsProject* prjSrc = mprjProject;
	QObject::connect(action, &QAction::triggered, [strLayerName, prjSrc,widMain,Srcsymbol]() {
		// 根据名称获取图层
		QgsVectorLayer* pvlLayer = nullptr;
		QList<QgsMapLayer*> layers = prjSrc->mapLayersByName(strLayerName);
		if (layers.size() > 0)
		{
			pvlLayer = dynamic_cast<QgsVectorLayer*>(layers.at(0));
		}
		// 弹出新的符号管理窗口
		SymbolManger* symbolManger = new SymbolManger(pvlLayer,widMain,Srcsymbol);
		symbolManger->show();
		//QgsStyleManagerDialog* styleManager = new QgsStyleManagerDialog();
		//styleManager->show();
		});
	return action;
}
// 连接栅格图层符号管理
QAction* LayerItemMenu::actionSymbolManger_ras(QString strLayerName, QgsRasterLayer* rasLayer) {
	QAction* action = new QAction("符号管理");
	MainWidget* widMain = mwidMain;
	QObject::connect(action, &QAction::triggered, [strLayerName, rasLayer, widMain]() {
		// 弹出新的符号管理窗口
		QString strRampPath = widMain->mstrRampPath;
		RasterStyle* rasterSymbolManager = new RasterStyle(strLayerName, rasLayer, widMain, strRampPath);
		rasterSymbolManager->show();
		//QgsStyleManagerDialog* styleManager = new QgsStyleManagerDialog();
		//styleManager->show();
		});
	return action;
}
// 属性表右键菜单
QAction* LayerItemMenu::actionShowProperties(QString strLayerName, QgsVectorLayer* vectorlayer)
{
	QAction* action = new QAction("属性表");
	QgsMapCanvas* canvas = mcanMapCanvas;
	QgsProject* prjSrc  = mprjProject;
	QObject::connect(action, &QAction::triggered, [strLayerName, prjSrc,canvas,vectorlayer]() {
		// 根据名称获取图层
		
		QgsVectorLayer* pvlLayer = nullptr;
		QList<QgsMapLayer*> layers = prjSrc->mapLayersByName(strLayerName);
		if (layers.size() > 0)
		{
			pvlLayer = dynamic_cast<QgsVectorLayer*>(layers.at(0));
		}
		if (!pvlLayer)
		{
			QMessageBox::critical(nullptr, "error", QString("图层不存在: \n") + strLayerName);
			return;
		}
		// 弹出新的属性表窗口
		if (pvlLayer) {
			// 设置属性表模型
			AttributeViewWidget* attriView = new AttributeViewWidget(pvlLayer,canvas);
			// 设置属性表窗口大小
			attriView->setWindowTitle(strLayerName + "属性表");
			attriView->show();
		}
		});
	return action;
}
// 设置标注
QAction* LayerItemMenu::actionLabelManger(QString strLayerName)
{
	QAction* action = new QAction("标注管理");
	QgsMapCanvas* canvas = mcanMapCanvas;
	QgsProject* prjSrc = mprjProject;
	// 点击节点
	QgsLayerTreeNode* node = mctrlLayerItem->index2node(mctrlLayerItem->currentIndex());
	QgsVectorLayer* pvLayer = qobject_cast<QgsVectorLayer*>(QgsLayerTree::toLayer(node)->layer());
	QObject::connect(action, &QAction::triggered, [pvLayer, prjSrc, canvas]() {
		// 获取所有字段名
		QStringList vfieldNames = pvLayer->fields().names();
        vfieldNames.prepend("关闭标注");
		QString attriType = QInputDialog::getItem(nullptr, "标注", "选择字段作为标注", vfieldNames);
		if (attriType.isEmpty()||attriType=="关闭标注")
		{
			pvLayer->setLabelsEnabled(false);
			pvLayer->triggerRepaint();
			return;
		}
		// 启用注记
		pvLayer->setLabelsEnabled(true);
		// 创建文本格式
		QgsTextFormat textFormat;
		textFormat.setFont(QFont("Arial", 12)); // 设置字体
		textFormat.setSize(12);                 // 设置字号
		textFormat.setColor(Qt::blue);          // 设置颜色

		QgsPalLayerSettings settings;
		settings.setLegendString(attriType);
		settings.setFormat(textFormat);
		settings.fieldName = attriType;
		QgsVectorLayerSimpleLabeling* labeling = new QgsVectorLayerSimpleLabeling(settings);
		pvLayer->setLabeling(labeling);
		pvLayer->setLabelsEnabled(true);
		pvLayer->triggerRepaint();
	});
	return action;
}
// 栅格图层透明度
QAction* LayerItemMenu::actionRasterOpacity(QgsRasterLayer* rasLayer)
{
	QAction* action = new QAction("透明度");
	QgsMapCanvas* canvas = mcanMapCanvas;
	QgsProject* prjSrc = mprjProject;
	QObject::connect(action, &QAction::triggered, [rasLayer, canvas]() {
		// 设置输入框
		QWidget* widget = new QWidget();
		QGridLayout* layout = new QGridLayout(widget);
		QLabel* label = new QLabel("不透明度");
		QSlider* slider = new QSlider(Qt::Horizontal);
		slider->setRange(0, 100);
		slider->setSingleStep(1);
		slider->setValue(rasLayer->opacity()*100);
		layout->addWidget(label, 0, 0);
		layout->addWidget(slider, 0, 1);
		widget->setLayout(layout);
		widget->show();
		// 实时设置透明度
		QObject::connect(slider, &QSlider::valueChanged, [rasLayer](int val) {
			float opacity = (float)val / 100;
			rasLayer->setOpacity(opacity);
			rasLayer->triggerRepaint();
		});
	});
	return action;
}
// 矢量图层坐标转换
QAction* LayerItemMenu::actionCrsTransform_vec( QgsVectorLayer* veclayer)
{
	QAction* action = new QAction("坐标转换");
	QgsMapCanvas* canvas = mcanMapCanvas;
	QgsProject* prjSrc = mprjProject;
	MainWidget* widMain = mwidMain;
	QObject::connect(action, &QAction::triggered, [widMain, prjSrc, canvas, veclayer]() {
			int SrcCRScode = veclayer->crs().postgisSrid();
			int newCRScode = QInputDialog::getInt(nullptr, "坐标转换", "请输入新的坐标系代码", SrcCRScode);
			if(newCRScode == SrcCRScode)
			{
				return;
			}
			// 转换坐标系
			QgsVectorLayer* resultLayer = MainWidget::TransformCRS_Vec(veclayer, newCRScode);
			if (resultLayer == nullptr)
			{
				return;
			}
			// 设置默认符号
			QgsSymbol* symbol = QgsSymbol::defaultSymbol(veclayer->geometryType());
			QgsSingleSymbolRenderer* renderer = new QgsSingleSymbolRenderer(symbol);
			resultLayer->setRenderer(renderer);
			widMain->setLayerToMap(resultLayer);
		});
	return action;
}
// 栅格图层坐标转换
QAction* LayerItemMenu::actionCrsTransform_ras( QgsRasterLayer* rasLayer)
{
	QAction* action = new QAction("坐标转换");
	QgsMapCanvas* canvas = mcanMapCanvas;
	QgsProject* prjSrc = mprjProject;
	MainWidget* widMain = mwidMain;
	QObject::connect(action, &QAction::triggered, [widMain, prjSrc, canvas, rasLayer]() {
		int SrcCRScode = rasLayer->crs().postgisSrid();
		int newCRScode = QInputDialog::getInt(nullptr, "坐标转换", "请输入新的坐标系代码", SrcCRScode);
		if (newCRScode == SrcCRScode)
		{
			return;
		}
		// 转换坐标系
		QgsRasterLayer* resultLayer = MainWidget::TransformCRS_Ras(rasLayer, newCRScode);
		if(resultLayer==nullptr)
		{
			return;
		}
		widMain->setLayerToMap(resultLayer);
		});
	return action;
}
// 样式管理
QAction* LayerItemMenu::actionStyleManager(QString strLayerName, Qgis::GeometryType layerType, QgsVectorLayer* veclayer) {
	QAction* action = new QAction("符号库");
	MainWidget* widMain = mwidMain;
	QObject::connect(action, &QAction::triggered, [strLayerName, layerType, widMain, veclayer]() {
		QString strStylePath = widMain->mstrStylePath;
		// 弹出新的符号库窗口
		StyleManager* styleManager = new StyleManager(strLayerName, layerType, widMain, veclayer,strStylePath);
		styleManager->show();
		});
	return action;
}

