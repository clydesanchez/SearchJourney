#include "SymbolManger.h"
#include "MainWidget.h"
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
#include <QPainter>
#include <QCombobox>
//#include <qgsstylemanagerdialog.h>
//#include <qgssymbolselectordialog.h>
SymbolManger::SymbolManger(QString strLayerName, Qgis::GeometryType layerType, MainWidget* widMain , QgsSymbolList Srcsymbol,QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	this->mstrLayerName = strLayerName;
    this->mLayerType = layerType;
    // 创建颜色按钮
    mctrlFillColorBtn = new QgsColorButton(ui.tab_2);
    mctrlStrokeColorBtn = new QgsColorButton(ui.tab_2);
    mctrlFillColorBtn->setShowNoColor(true);
    mctrlStrokeColorBtn->setShowNoColor(true);

    mctrlFillColorBtn->setGeometry(ui.btn_fillcolor->geometry());
    mctrlStrokeColorBtn->setGeometry(ui.btn_strokecolor->geometry());
    // 设置颜色按钮的默认颜色
    mctrlFillColorBtn->setColor(Qt::red);
    mctrlStrokeColorBtn->setColor(Qt::black);
    // 删除原有按钮
    delete ui.btn_fillcolor;
    delete ui.btn_strokecolor;
    ui.ctrlSymbolPreview->setAlignment(Qt::AlignCenter);

	mfsmModel = new QFileSystemModel(this);
	mfsmModel->setRootPath("");  // 设置根路径为电脑根目录
    mfsmModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);  // 仅显示目录
	ui.ctrlTreeView->setModel(mfsmModel);
	ui.ctrlTreeView->setRootIndex(mfsmModel->index(""));
	ui.ctrlTreeView->setColumnWidth(0, 200);

	// 仅显示文件名称，隐藏其他列
	ui.ctrlTreeView->setColumnHidden(1, true);  // 隐藏文件大小列
	ui.ctrlTreeView->setColumnHidden(2, true);  // 隐藏文件类型列
	ui.ctrlTreeView->setColumnHidden(3, true);  // 隐藏修改日期列

    mSvgTableModel = new SvgTableModel(this);
    ui.tableView->setModel(mSvgTableModel);
    ui.tableView->setIconSize(QSize(50, 50));  // 设置图标显示大小
    ui.tableView->verticalHeader()->setDefaultSectionSize(60);  // 调整行高
    ui.tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // 文件名列自动调整

    // 设置初始符号
    mFillColor = Srcsymbol.at(0)->symbolLayer(0)->color();
    mStrokeColor = Srcsymbol.at(0)->symbolLayer(0)->strokeColor();
    //mStrokeWidth = Srcsymbol.at(0)->symbolLayer(0)->dxfWidth();
    mPenStyle = Srcsymbol.at(0)->symbolLayer(0)->dxfPenStyle();
    mctrlFillColorBtn->setColor(mFillColor);
    mctrlStrokeColorBtn->setColor(mStrokeColor);
    
    // 设置符号预览
    previewSymbol();
    // 连接 QTreeView 的点击信号到槽函数
    connect(ui.ctrlTreeView, &QTreeView::clicked, this, &SymbolManger::onDirectoryClicked);
    // 连接 QTableView 的点击信号到槽函数
    connect(mSvgTableModel, &SvgTableModel::signalCellClicked, this, &SymbolManger::slotCellClicked);
    connect(ui.tableView, &QTableView::clicked, mSvgTableModel, &SvgTableModel::onCellClicked);
    // 连接 Mainwidget
    connect(this, &SymbolManger::signalApplySymbol, widMain, &MainWidget::slotApplySymbol);
    // 连接应用按钮
    connect(ui.ctrlConfirm, &QPushButton::clicked, this, &SymbolManger::onConfirmBtnClicked);
    connect(ui.ctrlCancel,&QPushButton::clicked, this, &SymbolManger::close);
    // 预览符号
    connect(mctrlFillColorBtn, &QgsColorButton::colorChanged, this, &SymbolManger::getSelectFilleColor);
    connect(mctrlStrokeColorBtn, &QgsColorButton::colorChanged, this, &SymbolManger::getSelectStrokeColor);
    connect(ui.ctrlStrokeWidth, SIGNAL(valueChanged(double)), this, SLOT(getSelectStrokeWidth(double)));
    connect(ui.ctrlPenStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(getSelecctPenStyle(int)));
}

SymbolManger::~SymbolManger()
{

}

//void SymbolManger::onDirectoryClicked(const QModelIndex& index) {
//    QString path = mfsmModel->filePath(index);  // 获取选中的目录路径
//    QDir directory(path);
//
//    if (directory.exists() && directory.isReadable()) {
//        // 获取目录中的所有 .svg 文件
//        QStringList svgFiles = directory.entryList(QStringList() << "*.svg", QDir::Files);
//        int row = 0;
//        int col = 0;
//
//        QGridLayout* gridLayout = new QGridLayout();  // 使用网格布局显示多个图像和文件名
//        gridLayout->setSpacing(5);  // 设置控件之间的间距
//            // 清空当前的布局中的内容
//        if (ui.ctrlScrollWidget->layout() != nullptr) {
//            QLayout* oldLayout = ui.ctrlScrollWidget->layout();
//            QLayoutItem* child;
//            while ((child = ui.ctrlScrollWidget->layout()->takeAt(0)) != nullptr) {
//                delete child->widget();  // 删除控件
//                delete child;            // 删除布局项
//            }
//            delete oldLayout;  // 删除布局
//        }
//        ui.ctrlScrollWidget->setLayout(gridLayout);
//        foreach(const QString & fileName, svgFiles) {
//            // 创建显示图像的 QLabel
//            QLabel* imageLabel = new QLabel(this);
//            QSvgRenderer renderer(directory.filePath(fileName));
//            QPixmap pixmap(30, 30);  // 创建指定大小的图像
//            pixmap.fill(Qt::transparent);  // 背景透明
//            QPainter painter(&pixmap);
//            renderer.render(&painter);  // 使用 QSvgRenderer 渲染 .svg 文件
//            imageLabel->setPixmap(pixmap);  // 将渲染后的图像设置到 QLabel
//
//            // 创建显示文件名的 QLabel
//            QLabel* nameLabel = new QLabel(fileName, this);
//
//            imageLabel->setAlignment(Qt::AlignCenter);  // 居中显示图像
//            nameLabel->setAlignment(Qt::AlignCenter);  // 居中显示文件名
//            // 将图像和文件名添加到网格布局中
//            gridLayout->addWidget(imageLabel, row, col);
//            gridLayout->addWidget(nameLabel, row + 1, col);
//
//            // 控制网格布局中每行显示几个元素（例如，每行3个）
//            col++;
//            if (col >= 3) {
//                col = 0;
//                row += 2;  // 两行一组（图像 + 文件名）
//            }
//        }
//    }
//}


void SymbolManger::onDirectoryClicked(const QModelIndex& index) {
    QString path = mfsmModel->filePath(index);  // 获取选中的目录路径
    QDir directory(path);

    if (directory.exists() && directory.isReadable()) {
        // 获取目录中的所有 .svg 文件
        QStringList svgFiles = directory.entryList(QStringList() << "*.svg", QDir::Files);
        QStringList filePaths;
        foreach(const QString & fileName, svgFiles) {
            filePaths.append(directory.filePath(fileName));  // 获取完整路径
        }

        // 更新模型中的数据
        mSvgTableModel->setSvgFiles(filePaths);
    }
}
// 根据图层类型判断符号类型
void SymbolManger::setSymbolByLayerType(Qgis::GeometryType layerType, QgsSymbol* psSymbol,QString svgPath)
{
    switch (layerType)
    {
    case Qgis::GeometryType::Point:
    {
        QgsSvgMarkerSymbolLayer* svgSymbolLayer = new QgsSvgMarkerSymbolLayer(svgPath);
        svgSymbolLayer->setSize(10);  // 设置符号大小
        // 设置边界线
        svgSymbolLayer->setStrokeColor(mStrokeColor);
        svgSymbolLayer->setStrokeWidth(mStrokeWidth);

        psSymbol->changeSymbolLayer(0, svgSymbolLayer);
    }
    break;
    case Qgis::GeometryType::Line:
    {
        QFile xmlFile("C:/Users/17993/Desktop/topo_steps.xml");
        if (!xmlFile.open(QIODevice::ReadOnly))
        {
            qDebug() << "无法打开文件";
            return;
        }
        QString xmlContent = xmlFile.readAll();
        QDomDocument doc;
        break;
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
        if(psSymbol->symbolLayerCount() > 1)
		{
			psSymbol->changeSymbolLayer(1, fillLayer);
		}
		else
		{
			psSymbol->appendSymbolLayer(fillLayer);
		}

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
     previewSymbol();
 }
 void SymbolManger::getSelectStrokeWidth(double strokewidth) {
	 mStrokeWidth = strokewidth;
     previewSymbol();
 }
 void SymbolManger::getSelecctPenStyle(int penStyle) {
	 mPenStyle = penStyle;
	 previewSymbol();
 }

 void SymbolManger::previewSymbol() {
     // 新建画布
     QPixmap pixmap(100, 100);
     pixmap.fill(Qt::transparent);
     QPainter painter(&pixmap);
     painter.setBrush(mFillColor);
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