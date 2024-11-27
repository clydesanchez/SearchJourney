#include "SymbolManger.h"
#include "MainWidget.h"
#include "SvgLabel.h"
#include <QLabel>
#include <QSpinBox>
#include <qsvgrenderer.h>
#include <QPainter>
#include <QLayout>
#include <Qgssymbol.h>
#include <Qgssinglesymbolrenderer.h>
#include <qgsmarkersymbol.h>
#include <qgsmarkersymbollayer.h>
#include <qgssymbollayer.h>
#include <qgsfillsymbol.h>
#include <qgsfillsymbollayer.h>
#include <qgslinesymbol.h>
#include <qgslinesymbollayer.h>
#include <QgsSymbolLayerRegistry.h>
#include <qgscolorbutton.h>
#include <qgscolordialog.h>
#include <QPainter>
#include <QCombobox>
//#include <qgsstylemanagerdialog.h>
//#include <qgssymbolselectordialog.h>
SymbolManger::SymbolManger(QgsVectorLayer* pvLayer, MainWidget* widMain , QgsSymbolList Srcsymbol,QWidget *parent)
	: QDockWidget(parent)
{
	ui.setupUi(this);
    // 设置窗口属性
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->setAcceptDrops(true);
    this->setAutoFillBackground(true);
    // 添加至主窗口
    widMain->addDockWidget(Qt::RightDockWidgetArea, this);
    widMain->getToolDock()->setVisible(false);
    widMain->tabifyDockWidget(this,widMain->getToolDock());

	this->mstrLayerName = pvLayer->name();
    this->mLayerType = pvLayer->geometryType();
    this->mpvLayer = pvLayer;
    // 创建颜色按钮
    mctrlFillColorBtn = new QgsColorButton(ui.tab_simple);
    mctrlStrokeColorBtn = new QgsColorButton(ui.tab_simple);
    mctrlStrokeColorBtn_svg = new QgsColorButton(ui.tab_svg);
    mctrlFontColor = new QgsColorButton(ui.tab_mark);
    mctrlFillColorBtn->setShowNoColor(true);
    mctrlStrokeColorBtn->setShowNoColor(true);

    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui.ctrlSymbolWidget->layout());
    QGridLayout* gridLayout_svg = qobject_cast<QGridLayout*>(ui.ctrlSvgWidget_2->layout());
    QGridLayout* gridLayout_mark = qobject_cast<QGridLayout*>(ui.ctrlMarkWidget->layout());
    gridLayout->addWidget(mctrlFillColorBtn, 0, 1);
    gridLayout->addWidget(mctrlStrokeColorBtn, 1, 1);
    gridLayout_svg->addWidget(mctrlStrokeColorBtn_svg, 0, 1);
    gridLayout_mark->addWidget(mctrlFontColor, 3, 1);
    // 设置颜色按钮的默认颜色
    mctrlFillColorBtn->setColor(Qt::red);
    mctrlStrokeColorBtn->setColor(Qt::black);
    // 删除原有按钮
    delete ui.btn_fillcolor;
    delete ui.btn_strokecolor;
    delete ui.btn_strokecolor_svg;
    delete ui.btn_fontcolor;
    ui.ctrlSymbolPreview->setAlignment(Qt::AlignCenter);

    // 设置文件系统模型
	mfsmModel = new QFileSystemModel(this);
	mfsmModel->setRootPath("");  // 设置根路径为电脑根目录
    mfsmModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);  // 仅显示目录
	ui.ctrlTreeView->setModel(mfsmModel);
	ui.ctrlTreeView->setRootIndex(mfsmModel->index(""));
	//ui.ctrlTreeView->setColumnWidth(0, 200);
    // 获取图层所有字段名
    QgsFields fields = pvLayer->fields();
    for (int i = 0; i < fields.count(); i++) {
		ui.ctrlField->addItem(fields.at(i).name());
	}

	// 仅显示文件名称，隐藏其他列
	ui.ctrlTreeView->setColumnHidden(1, true);  // 隐藏文件大小列
	ui.ctrlTreeView->setColumnHidden(2, true);  // 隐藏文件类型列
	ui.ctrlTreeView->setColumnHidden(3, true);  // 隐藏修改日期列

    // 设置初始符号
    mFillColor = Srcsymbol.at(0)->symbolLayer(0)->color();
    mStrokeColor = Srcsymbol.at(0)->symbolLayer(0)->strokeColor();
    mPenStyle = Srcsymbol.at(0)->symbolLayer(0)->dxfPenStyle();
    mctrlFillColorBtn->setColor(mFillColor);
    mctrlStrokeColorBtn->setColor(mStrokeColor);
    // 隐藏tab按钮
    ui.tabWidget->tabBar()->hide();
    
    // 设置符号预览
    previewSymbol();
    // 显示当前dock
    this->raise();
    // 连接 QTreeView 的点击信号到槽函数
    connect(ui.ctrlTreeView, &QTreeView::clicked, this, &SymbolManger::onDirectoryClicked);
    // 连接 Mainwidget
    connect(this, &SymbolManger::signalApplySymbol, widMain, &MainWidget::slotApplySymbol);
    connect(this, &SymbolManger::signalApplyMark, widMain, &MainWidget::slotApplyMark);
    // 连接应用按钮
    // 简单符号
    connect(ui.ctrlConfirm, &QPushButton::clicked, this, &SymbolManger::onConfirmBtnClicked);
    connect(ui.ctrlCancel,&QPushButton::clicked, this, &SymbolManger::close);
    // 标注
    connect(ui.ctrlConfirm_mark, &QPushButton::clicked, this, &SymbolManger::onConfirmBtnClicked_Mark);
    connect(ui.ctrlCancel_mark, &QPushButton::clicked, this, &SymbolManger::close);
    // 获取输入样式
    connect(mctrlFillColorBtn, &QgsColorButton::colorChanged, this, &SymbolManger::getSelectFilleColor);
    connect(mctrlStrokeColorBtn, &QgsColorButton::colorChanged, this, &SymbolManger::getSelectStrokeColor);
    connect(ui.ctrlStrokeWidth, SIGNAL(valueChanged(double)), this, SLOT(getSelectStrokeWidth(double)));
    connect(ui.ctrlPenStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(getSelecctPenStyle(int)));
    connect(ui.ctrlOpacity, SIGNAL(valueChanged(double)), this, SLOT(getSelectOpacity(double)));
    // 三个标签页的切换
    connect(ui.ctrlSymbolType_simple, SIGNAL(currentIndexChanged(int)), this, SLOT(onSymbolTypeChanged(int)));
    connect(ui.ctrlSymbolType_svg, SIGNAL(currentIndexChanged(int)), this, SLOT(onSymbolTypeChanged(int)));
    connect(ui.ctrlSymbolType_mark, SIGNAL(currentIndexChanged(int)), this, SLOT(onSymbolTypeChanged(int)));
    // svg与简单符号的边框符号连接
    connect(mctrlStrokeColorBtn_svg, &QgsColorButton::colorChanged, mctrlStrokeColorBtn,&QgsColorButton::setColor);
    // 因有多个重载函数，需要使用static_cast转换
    connect(ui.ctrlStrokeWidth_svg, 
        static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), 
        ui.ctrlStrokeWidth, 
        &QDoubleSpinBox::setValue);
    connect(ui.ctrlPenStyle_svg,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        ui.ctrlPenStyle,
        &QComboBox::setCurrentIndex);
    connect(ui.ctrlOpacity_svg,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		ui.ctrlOpacity,
		&QDoubleSpinBox::setValue);
}

SymbolManger::~SymbolManger()
{

}

void SymbolManger::resizeEvent(QResizeEvent* event) {
    // 设置网格布局大小

    ui.ctrlSvgWidget->setFixedWidth(event->size().width() - 20);
    ui.ctrlSvgWidget_2->setFixedWidth(event->size().width() - 80);
    ui.ctrlSymbolWidget->setFixedWidth(event->size().width() - 80);
    ui.ctrlMarkWidget->setFixedWidth(event->size().width() - 80);
    QWidget::resizeEvent(event);
}
// 更换符号类型
void SymbolManger::onSymbolTypeChanged(int nIndex) {
    // 更改tab页
    ui.tabWidget->setCurrentIndex(nIndex);
    ui.ctrlSymbolType_simple->setCurrentIndex(nIndex);
    ui.ctrlSymbolType_svg->setCurrentIndex(nIndex);
    ui.ctrlSymbolType_mark->setCurrentIndex(nIndex);
}

// 展示svg图像
void SymbolManger::onDirectoryClicked(const QModelIndex& index) {
    QString path = mfsmModel->filePath(index);  // 获取选中的目录路径
    QDir directory(path);

    if (directory.exists() && directory.isReadable()) {
        // 获取目录中的所有 .svg 文件
        QStringList svgFiles = directory.entryList(QStringList() << "*.svg", QDir::Files);
        int row = 0;
        int col = 0;
        QWidget* scrollContent = new QWidget(this);
        QGridLayout* gridLayout = new QGridLayout(scrollContent);  // 使用网格布局显示多个图像和文件名
        gridLayout->setSpacing(5);  // 设置控件之间的间距
            // 清空当前的布局中的内容
        if (ui.ctrlScrollArea->layout() != nullptr) {
            QLayout* oldLayout = ui.ctrlScrollArea->layout();
            QLayoutItem* child;
            while ((child = ui.ctrlScrollArea->layout()->takeAt(0)) != nullptr) {
                delete child->widget();  // 删除控件
                delete child;            // 删除布局项
            }
            delete oldLayout;  // 删除布局
        }
        //ui.ctrlScrollArea->setLayout(gridLayout);
        ui.ctrlScrollArea->setWidget(scrollContent);  // 设置滚动区域的内容
        int nScrollWidth = ui.ctrlScrollArea->width();
        int nMaxCol = nScrollWidth / 50;  // 计算每行最多显示的列数
        for (int i = 0; i < svgFiles.size(); i++) {
            // 新建label
            QString strFileName = svgFiles.at(i);
            QString strFilePath = path+"/"+strFileName;
            SvgLabel* imageLabel = new SvgLabel(strFilePath, this);
            connect(imageLabel, &SvgLabel::signalClicked, this, &SymbolManger::onSymbolSelect);
            // 渲染svg图像
            QSvgRenderer renderer(directory.filePath(strFileName));
            QPixmap pixmap(40, 40);
            pixmap.fill(Qt::transparent);
            imageLabel->setPixmap(pixmap);
            QPainter painter(&pixmap);
            renderer.render(&painter);
            // 设置到label
            imageLabel->setPixmap(pixmap);
            gridLayout->addWidget(imageLabel, row, col);
            col++;
            if (col >= nMaxCol) {
                col = 0;
                row ++;
            }
        }
    }
}
// 响应svg符号选择
void SymbolManger::onSymbolSelect(QString strSvgPath) {
    QgsSymbol* srcSymbol = QgsSymbol::defaultSymbol(mLayerType);
    qDebug() << strSvgPath;
    // 创建符号对象，并添加SVG符号层
    setSymbolByLayerType(mLayerType, srcSymbol, strSvgPath);
    emit signalApplySymbol(mstrLayerName, srcSymbol);
}


// 根据图层类型判断符号类型
void SymbolManger::setSymbolByLayerType(Qgis::GeometryType layerType, QgsSymbol* psSymbol,QString svgPath)
{
    switch (layerType)
    {
    case Qgis::GeometryType::Point:
    {
        QgsSvgMarkerSymbolLayer* svgSymbolLayer = new QgsSvgMarkerSymbolLayer(svgPath);
        svgSymbolLayer->setSize(40);  // 设置符号大小
        // 设置边界线
        svgSymbolLayer->setStrokeColor(mStrokeColor);
        svgSymbolLayer->setStrokeWidth(mStrokeWidth);
        psSymbol->changeSymbolLayer(0, svgSymbolLayer);
        psSymbol->setOpacity(mOpacity);
    }
    break;
    case Qgis::GeometryType::Line:
    {
        
    }
    case Qgis::GeometryType::Polygon:
    {
        QgsSVGFillSymbolLayer* svgSymbolLayer = new QgsSVGFillSymbolLayer(svgPath);
        // 固定纹理宽度
        svgSymbolLayer->setPatternWidth(5);
        psSymbol->changeSymbolLayer(0,svgSymbolLayer);
        //填充样式，颜色为透明，边框为黑色作为边框
        QgsSimpleFillSymbolLayer* fillLayer = new QgsSimpleFillSymbolLayer();
        fillLayer->setFillColor(QColor(0, 0, 0, 0));//填充透明
        // 设置边界线
        fillLayer->setStrokeColor(mStrokeColor);
        fillLayer->setStrokeWidth(mStrokeWidth);
        fillLayer->setStrokeStyle(static_cast<Qt::PenStyle>(mPenStyle));
        
        if(psSymbol->symbolLayerCount() > 1)
		{
			psSymbol->changeSymbolLayer(1, fillLayer);
		}
		else
		{
			psSymbol->appendSymbolLayer(fillLayer);
		}
        psSymbol->setOpacity(mOpacity);
        break;
    }
    default:
        break;
    }
}
// 单元格点击事件
 void SymbolManger::slotCellClicked(const QString& filePath) {

     QgsSymbol* srcSymbol = QgsSymbol::defaultSymbol(mLayerType);
     QString svgPath = filePath;
     qDebug() << svgPath;
     // 创建符号对象，并添加SVG符号层
     setSymbolByLayerType(mLayerType, srcSymbol,filePath);
     emit signalApplySymbol(mstrLayerName, srcSymbol);
 }
 // 获取填充颜色
 void SymbolManger::getSelectFilleColor(QColor fillcolor) {
	 mFillColor = fillcolor;
     previewSymbol();
 }
 // 获取边界颜色_svg
 void SymbolManger::getSelectStrokeColor(QColor StrokeColor) {
	 mStrokeColor = StrokeColor;
     mctrlStrokeColorBtn_svg->setColor(StrokeColor);
     previewSymbol();
 }
 // 获取边界宽度_svg
 void SymbolManger::getSelectStrokeWidth(double strokewidth) {
	 mStrokeWidth = strokewidth;
     ui.ctrlStrokeWidth_svg->setValue(strokewidth);
     previewSymbol();
 }
 // 获取边界样式_svg
 void SymbolManger::getSelecctPenStyle(int penStyle) {
	 mPenStyle = penStyle;
     ui.ctrlPenStyle_svg->setCurrentIndex(penStyle);
	 previewSymbol();
 }
 // 获取透明度
 void SymbolManger::getSelectOpacity(double opacity) {
	 mOpacity = opacity;
     ui.ctrlOpacity_svg->setValue(opacity);
	 previewSymbol();
 }
 // 预览符号
 void SymbolManger::previewSymbol() {
     // 新建画布
     QPixmap pixmap(100, 100);
     pixmap.fill(Qt::transparent);
     QPainter painter(&pixmap);
     painter.setOpacity(mOpacity);//设置透明度
     painter.setBrush(mFillColor);// 设置填充颜色
     painter.drawRect(20, 20, 70, 70);  // 绘制填充
     QPen strokePen(mStrokeColor,mStrokeWidth*7);
    strokePen.setStyle(static_cast<Qt::PenStyle>(mPenStyle));
     painter.setPen(strokePen);
     painter.setBrush(Qt::NoBrush);  // 无填充
     painter.drawRect(20, 20, 70, 70);  // 绘制边框
     ui.ctrlSymbolPreview->setPixmap(pixmap);

     qDebug() <<"fillcolor:"<< mFillColor.name() << "strokecolor:" << mStrokeColor.name() << "strokewidth:" << mStrokeWidth;
 }
 // 简单符号应用按钮
 void SymbolManger::onConfirmBtnClicked() {
	 // 创建符号对象
     QgsSymbol* srcSymbol = QgsSymbol::defaultSymbol(mLayerType);
     if (mLayerType == Qgis::GeometryType::Polygon) {
         // 创建填充符号层
         QgsSimpleFillSymbolLayer* fillLayer = new QgsSimpleFillSymbolLayer();
         fillLayer->setFillColor(mFillColor);
         fillLayer->setStrokeColor(mStrokeColor);
         fillLayer->setStrokeWidth(mStrokeWidth);
         fillLayer->setStrokeStyle(static_cast<Qt::PenStyle>(mPenStyle));
         // 添加填充符号层
         srcSymbol->changeSymbolLayer(0, fillLayer);
         srcSymbol->setOpacity(mOpacity);
     }
     else if(mLayerType == Qgis::GeometryType::Point){
		 // 创建简单标记符号层
         QgsSimpleMarkerSymbolLayer* markerLayer = new QgsSimpleMarkerSymbolLayer();
         markerLayer->setColor(mFillColor);
         markerLayer->setStrokeColor(mStrokeColor);
         markerLayer->setStrokeWidth(mStrokeWidth);
         markerLayer->setStrokeStyle(static_cast<Qt::PenStyle>(mPenStyle));
         // 添加简单标记符号层
         srcSymbol->changeSymbolLayer(0, markerLayer);
         srcSymbol->setOpacity(mOpacity);
	 }
     else if (mLayerType == Qgis::GeometryType::Line) {
         // 创建简单线符号层
         QgsSimpleLineSymbolLayer* lineLayer = new QgsSimpleLineSymbolLayer();
         lineLayer->setColor(mFillColor);
         lineLayer->setWidth(mStrokeWidth);
         lineLayer->setPenStyle(static_cast<Qt::PenStyle>(mPenStyle));
         // 修改样式
         //lineLayer->setPenStyle(Qt::DotLine);
         // 添加简单线符号层
         srcSymbol->changeSymbolLayer(0, lineLayer);
         srcSymbol->setOpacity(mOpacity);
     }
     emit signalApplySymbol(mstrLayerName, srcSymbol);
 }
 // 标注应用按钮
 void SymbolManger::onConfirmBtnClicked_Mark() {
     // 读取标注样式
     QColor fontColor = mctrlFontColor->color();
     QFont font = ui.fontComboBox->currentFont();
     double fontSize = ui.ctrlFontSize->value();
     int fieldIdx = ui.ctrlField->currentIndex();
     ui.ctrlField->addItem("关闭标注");
     // 获取字段名
     QgsFields fields = mpvLayer->fields();
     for (int i = 0; i < fields.count(); i++) {
         ui.ctrlField->addItem(fields.at(i).name());
     }
     QString attriType = fields.at(fieldIdx).name();
     // 创建文本格式
     QgsTextFormat textFormat;
     textFormat.setFont(font); // 设置字体
     textFormat.setSize(fontSize);  // 设置字号
     textFormat.setColor(fontColor);// 设置颜色
     // 创建标注设置
     QgsPalLayerSettings settings;
     settings.setLegendString(attriType);
     settings.setFormat(textFormat);
     settings.fieldName = attriType;
     emit signalApplyMark(mstrLayerName, settings);
 }