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
SymbolManger::SymbolManger(QString strLayerName, Qgis::GeometryType layerType, MainWidget* widMain , QgsSymbolList Srcsymbol,QWidget *parent)
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

	this->mstrLayerName = strLayerName;
    this->mLayerType = layerType;
    // 创建颜色按钮
    mctrlFillColorBtn = new QgsColorButton(ui.tab_2);
    mctrlStrokeColorBtn = new QgsColorButton(ui.tab_2);
    mctrlStrokeColorBtn_svg = new QgsColorButton(ui.tab);
    mctrlFillColorBtn->setShowNoColor(true);
    mctrlStrokeColorBtn->setShowNoColor(true);

    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(ui.ctrlSymbolWidget->layout());
    QGridLayout* gridLayout_svg = qobject_cast<QGridLayout*>(ui.ctrlSvgWidget_2->layout());
    gridLayout->addWidget(mctrlFillColorBtn, 0, 1);
    gridLayout->addWidget(mctrlStrokeColorBtn, 1, 1);
    gridLayout_svg->addWidget(mctrlStrokeColorBtn_svg, 0, 1);
    // 设置颜色按钮的默认颜色
    mctrlFillColorBtn->setColor(Qt::red);
    mctrlStrokeColorBtn->setColor(Qt::black);
    // 删除原有按钮
    delete ui.btn_fillcolor;
    delete ui.btn_strokecolor;
    delete ui.btn_strokecolor_svg;
    ui.ctrlSymbolPreview->setAlignment(Qt::AlignCenter);

	mfsmModel = new QFileSystemModel(this);
	mfsmModel->setRootPath("");  // 设置根路径为电脑根目录
    mfsmModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);  // 仅显示目录
	ui.ctrlTreeView->setModel(mfsmModel);
	ui.ctrlTreeView->setRootIndex(mfsmModel->index(""));
	//ui.ctrlTreeView->setColumnWidth(0, 200);

	// 仅显示文件名称，隐藏其他列
	ui.ctrlTreeView->setColumnHidden(1, true);  // 隐藏文件大小列
	ui.ctrlTreeView->setColumnHidden(2, true);  // 隐藏文件类型列
	ui.ctrlTreeView->setColumnHidden(3, true);  // 隐藏修改日期列

    mSvgTableModel = new SvgTableModel(this);

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
    // 连接应用按钮
    connect(ui.ctrlConfirm, &QPushButton::clicked, this, &SymbolManger::onConfirmBtnClicked);
    connect(ui.ctrlCancel,&QPushButton::clicked, this, &SymbolManger::close);
    // 获取输入样式
    connect(mctrlFillColorBtn, &QgsColorButton::colorChanged, this, &SymbolManger::getSelectFilleColor);
    connect(mctrlStrokeColorBtn, &QgsColorButton::colorChanged, this, &SymbolManger::getSelectStrokeColor);
    connect(ui.ctrlStrokeWidth, SIGNAL(valueChanged(double)), this, SLOT(getSelectStrokeWidth(double)));
    connect(ui.ctrlPenStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(getSelecctPenStyle(int)));
    connect(ui.ctrlOpacity, SIGNAL(valueChanged(double)), this, SLOT(getSelectOpacity(double)));
    // svg与简单符号的切换
    connect(ui.ctrlSymbolType_simple, SIGNAL(currentIndexChanged(int)), this, SLOT(onSymbolTypeChanged(int)));
    connect(ui.ctrlSymbolType_svg, SIGNAL(currentIndexChanged(int)), this, SLOT(onSymbolTypeChanged(int)));
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

    QWidget::resizeEvent(event);
}
// 更换符号类型
void SymbolManger::onSymbolTypeChanged(int nIndex) {
    // 更改tab页
    ui.tabWidget->setCurrentIndex(nIndex);
    ui.ctrlSymbolType_simple->setCurrentIndex(nIndex);
    ui.ctrlSymbolType_svg->setCurrentIndex(nIndex);
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
void SymbolManger::onSymbolSelect(QString strSvgPath) {
    QgsSymbol* srcSymbol = QgsSymbol::defaultSymbol(mLayerType);
    qDebug() << strSvgPath;
    // 创建符号对象，并添加SVG符号层
    setSymbolByLayerType(mLayerType, srcSymbol, strSvgPath);
    emit signalApplySymbol(mstrLayerName, srcSymbol);
}


//void SymbolManger::onDirectoryClicked(const QModelIndex& index) {
//    QString path = mfsmModel->filePath(index);  // 获取选中的目录路径
//    QDir directory(path);
//
//    if (directory.exists() && directory.isReadable()) {
//        // 获取目录中的所有 .svg 文件
//        QStringList svgFiles = directory.entryList(QStringList() << "*.svg", QDir::Files);
//        QStringList filePaths;
//        foreach(const QString & fileName, svgFiles) {
//            filePaths.append(directory.filePath(fileName));  // 获取完整路径
//        }
//
//        // 更新模型中的数据
//        mSvgTableModel->setSvgFiles(filePaths);
//    }
//}
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
 void SymbolManger::slotCellClicked(const QString& filePath) {

     QgsSymbol* srcSymbol = QgsSymbol::defaultSymbol(mLayerType);
     QString svgPath = filePath;
     qDebug() << svgPath;
     // 创建符号对象，并添加SVG符号层
     setSymbolByLayerType(mLayerType, srcSymbol,filePath);
     emit signalApplySymbol(mstrLayerName, srcSymbol);
 }
 void SymbolManger::getSelectFilleColor(QColor fillcolor) {
	 mFillColor = fillcolor;
     previewSymbol();
 }
 void SymbolManger::getSelectStrokeColor(QColor StrokeColor) {
	 mStrokeColor = StrokeColor;
     mctrlStrokeColorBtn_svg->setColor(StrokeColor);
     previewSymbol();
 }
 void SymbolManger::getSelectStrokeWidth(double strokewidth) {
	 mStrokeWidth = strokewidth;
     ui.ctrlStrokeWidth_svg->setValue(strokewidth);
     previewSymbol();
 }
 void SymbolManger::getSelecctPenStyle(int penStyle) {
	 mPenStyle = penStyle;
     ui.ctrlPenStyle_svg->setCurrentIndex(penStyle);
	 previewSymbol();
 }
 void SymbolManger::getSelectOpacity(double opacity) {
	 mOpacity = opacity;
     ui.ctrlOpacity_svg->setValue(opacity);
	 previewSymbol();
 }
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


/***********************************************************************************************************************/
void SvgTableModel::setSvgFiles(const QStringList& filePaths) {
    beginResetModel();
    mSvgFiles = filePaths;
    endResetModel();
}

int SvgTableModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
	return mSvgFiles.size();
}

int SvgTableModel::columnCount(const QModelIndex& parent) const {
	Q_UNUSED(parent);
	return 2;
}

QVariant SvgTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    const QString& filePath = mSvgFiles.at(index.row());
    if (role == Qt::DecorationRole && index.column() == 0) {  // 第一列显示图片
        QSvgRenderer renderer(filePath);
        QPixmap pixmap(70, 70);  // 设置图片大小
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);  // 渲染 .svg 文件
        return pixmap;
    }

    if (role == Qt::DisplayRole && index.column() == 1) {  // 第二列显示文件名
        return QFileInfo(filePath).fileName();
    }

    return QVariant();
}

QVariant SvgTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            if (section == 0)
                return QStringLiteral("Image");
            else if (section == 1)
                return QStringLiteral("File Name");
        }
    }
    return QVariant();
}

void SvgTableModel::onCellClicked(const QModelIndex& index) {
	emit signalCellClicked(mSvgFiles.at(index.row()));
}