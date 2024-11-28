#include "StyleManager.h"
#include <QTabWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringListModel>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDomDocument>
#include <QFileDialog>
#include <QgsReadWriteContext.h>
#include <qgsxmlutils.h>
#include <qgsscreenproperties.h>
#include <qgsexpressioncontext.h>
#include <qgssymbollayerutils.h>
#include <qgsreadwritecontext.h>
#include <qgsscreenproperties.h>
#include "qgssymbol.h" 
#include "qgis_core.h"


StyleManager::StyleManager(QString strLayerName, Qgis::GeometryType layerType, MainWidget* widMain, QWidget* parent)
    : QMainWindow(parent) {
    ui.setupUi(this);

    this->mStlLayerName = strLayerName;
    this->mStlLayerType = layerType;

    // 创建 Tab Widget
    QTabWidget* tabWidget = new QTabWidget(this);
    tabWidget->addTab(new QWidget(), tr("Marker"));
    tabWidget->addTab(new QWidget(), tr("Line"));
    tabWidget->addTab(new QWidget(), tr("Fill"));
    //tabWidget->addTab(new QWidget(), tr("Color Ramp"));
    tabWidget->setMaximumHeight(50);
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 默认 listViewMode 为 Marker
    listViewMode = "Marker";

    // 绑定 tabWidget 的 currentChanged 信号
    connect(tabWidget, &QTabWidget::currentChanged, this, &StyleManager::onTabChanged);

    // 创建 QListView
    QListView* listView = new QListView(this);
    QStringListModel* listModel = new QStringListModel();
    listView->setModel(listModel);
    listView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 创建按钮
    QPushButton* importButton = new QPushButton(tr("Import"), this);
    QPushButton* exportButton = new QPushButton(tr("Export"), this);
    QPushButton* applyButton = new QPushButton(tr("Apply"), this);
    QPushButton* closeButton = new QPushButton(tr("Close"), this);

    // 布局按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(importButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(closeButton);

    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0); // 去掉边距
    mainLayout->setSpacing(5);

    // 将控件添加到主布局
    mainLayout->addWidget(tabWidget);     // TabWidget 占用扩展空间
    mainLayout->addWidget(listView);      // QListView 占用扩展空间
    mainLayout->addLayout(buttonLayout);  // 按钮区域

    // 创建主窗口部件
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget); // 设置为主窗口的中心部件

    // 信号槽连接
    connect(tabWidget, &QTabWidget::currentChanged, [=](int index) {
        QStringList items;
        switch (index) {
        case 0: 
            markerViewer(listView);
            break;
        case 1: 
            lineViewer(listView);
            break;
        case 2: 
            fillViewer(listView);
            break;
        /*case 3: 
            items << "Color Ramp 1" << "Color Ramp 2" << "Color Ramp 3";
            colorRmapViewer(); 
            break;*/
        default: break;
        }
        listModel->setStringList(items);
        });

    connect(importButton, &QPushButton::clicked, this, &StyleManager::onActionIptStl);


    connect(exportButton, &QPushButton::clicked, this, [this, listView]() {
        onActionStlExport(listView);
        });


    connect(closeButton, &QPushButton::clicked, this, &StyleManager::close);

    connect(applyButton, &QPushButton::clicked, this, [this, listView]() {
        onActionStlApply(listView);
        });

    // 连接 Mainwidget
    connect(this, &StyleManager::signalApplyStlSymbol, widMain, &MainWidget::slotApplySymbol);

    // 设置窗口大小
    resize(800, 600);

    refreshStlView();
    markerViewer(listView);
}

StyleManager::~StyleManager() {}


void StyleManager::onTabChanged(int index) {
    switch (index) {
    case 0: // Marker 标签页
        listViewMode = "Marker";
        break;
    case 1: // Line 标签页
        listViewMode = "Line";
        break;
    case 2: // Fill 标签页
        listViewMode = "Fill";
        break;
    default:
        QMessageBox::warning(this, tr("Error"), tr("Unknown tab index."));
        break;
    }
}


void StyleManager::refreshStlView() {
    // 清空现有符号列表
    mMarkerSymbols.clear();
    mLineSymbols.clear();
    mFillSymbols.clear();

    // 定义符号文件夹路径
    QString folderPath = "E:/GISdes/styles/mystyle";
    QDir dir(folderPath);
    if (!dir.exists()) {
        qDebug() << "Directory does not exist:" << folderPath;
        return;
    }

    // 查找所有 XML 文件
    QStringList xmlFiles = dir.entryList(QStringList() << "*.xml", QDir::Files);
    if (xmlFiles.isEmpty()) {
        qDebug() << "No XML files found in:" << folderPath;
        return;
    }

    // 设置读取上下文
    QgsReadWriteContext readWriteContext;

    for (const QString& fileName : xmlFiles) {
        QString filePath = folderPath + "/" + fileName;

        // 打开并解析 XML 文件
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open file:" << filePath;
            continue;
        }

        QDomDocument doc;
        if (!doc.setContent(&file)) {
            qDebug() << "Failed to parse XML file:" << filePath;
            file.close();
            continue;
        }
        file.close();

        // 获取 <symbol> 节点
        QDomNodeList symbols = doc.elementsByTagName("symbol");
        for (int i = 0; i < symbols.size(); ++i) {
            QDomElement symbolElem = symbols.at(i).toElement();
            if (symbolElem.isNull()) continue;

            // 使用 QgsSymbolLayerUtils::loadSymbol 加载符号
            QgsSymbol* symbol = QgsSymbolLayerUtils::loadSymbol(symbolElem, readWriteContext);
            if (!symbol) {
                qDebug() << "Failed to load symbol from file:" << filePath;
                continue;
            }

            // 获取符号名称
            QString symbolName = symbolElem.attribute("name", "Unnamed Symbol");

            // 根据符号类型分类存储
            if (QgsMarkerSymbol* markerSymbol = dynamic_cast<QgsMarkerSymbol*>(symbol)) {
                CustomMarkerSymbol* customMarkerSymbol = new CustomMarkerSymbol(markerSymbol, symbolName);
                mMarkerSymbols.append(customMarkerSymbol);
            }
            else if (QgsLineSymbol* lineSymbol = dynamic_cast<QgsLineSymbol*>(symbol)) {
                CustomLineSymbol* customLineSymbol = new CustomLineSymbol(lineSymbol, symbolName);
                mLineSymbols.append(customLineSymbol);
            }
            else if (QgsFillSymbol* fillSymbol = dynamic_cast<QgsFillSymbol*>(symbol)) {
                CustomFillSymbol* customFillSymbol = new CustomFillSymbol(fillSymbol, symbolName);
                mFillSymbols.append(customFillSymbol);
            }
            else {
                qDebug() << "Unsupported symbol type in file:" << filePath;
                delete symbol; // 释放无用符号
            }
        }
    }
}


void StyleManager::markerViewer(QListView* listView) {
    if (!listView) return;

    // 创建模型用于显示符号名称和预览
    QStandardItemModel* model = new QStandardItemModel(this);

    for (int i = 0; i < mMarkerSymbols.size(); ++i) {
        CustomMarkerSymbol* markerSymbol = mMarkerSymbols[i];

        QString symbolName = markerSymbol->getName();

        // 创建符号预览图
        QImage image = markerSymbol->getSymbol()->bigSymbolPreviewImage(nullptr, Qgis::SymbolPreviewFlags(), QgsScreenProperties());
        QPixmap pixmap = QPixmap::fromImage(image);

        // 创建模型项，设置名称和图标
        QStandardItem* item = new QStandardItem();
        item->setText(symbolName); // 使用符号名称作为显示文本
        item->setIcon(QIcon(pixmap));

        // 添加到模型
        model->appendRow(item);
    }

    // 将模型绑定到 QListView
    listView->setModel(model);
    listView->setIconSize(QSize(128, 128)); // 设置列表项图标的大小
    listView->setGridSize(QSize(160, 160)); // 设置列表项的网格大小
    listView->setViewMode(QListView::IconMode); // 图标模式
    listView->setSpacing(10); // 设置间距
}


void StyleManager::lineViewer(QListView* listView) {
    if (!listView) return;

    // 创建模型用于显示符号名称和预览
    QStandardItemModel* model = new QStandardItemModel(this);

    for (int i = 0; i < mLineSymbols.size(); ++i) {
        CustomLineSymbol* lineSymbol = mLineSymbols[i];

        // 假设符号名称已经在 XML 中解析并存储
        QString symbolName = lineSymbol->getName(); // 如果没有名称，可以设置默认名称

        // 创建符号预览图
        QImage image = lineSymbol->getSymbol()->bigSymbolPreviewImage(nullptr, Qgis::SymbolPreviewFlags(), QgsScreenProperties());
        QPixmap pixmap = QPixmap::fromImage(image);

        // 创建模型项，设置名称和图标
        QStandardItem* item = new QStandardItem();
        item->setText(symbolName); // 使用符号名称作为显示文本
        item->setIcon(QIcon(pixmap));

        // 添加到模型
        model->appendRow(item);
    }

    // 将模型绑定到 QListView
    listView->setModel(model);
    listView->setIconSize(QSize(128, 128)); // 设置列表项图标的大小
    listView->setGridSize(QSize(160, 160)); // 设置列表项的网格大小
    listView->setViewMode(QListView::IconMode); // 图标模式
    listView->setSpacing(10); // 设置间距
}


void StyleManager::fillViewer(QListView* listView) {
    if (!listView) return;

    // 创建模型用于显示符号名称和预览
    QStandardItemModel* model = new QStandardItemModel(this);

    for (int i = 0; i < mFillSymbols.size(); ++i) {
        CustomFillSymbol* fillSymbol = mFillSymbols[i];

        // 假设符号名称已经在 XML 中解析并存储到 QList<QgsFillSymbol*> 时通过额外的列表保存
        QString symbolName = fillSymbol->getName(); // 如果没有名称，可以设置默认名称

        // 创建符号预览图
        QImage image = fillSymbol->getSymbol()->bigSymbolPreviewImage(nullptr, Qgis::SymbolPreviewFlags(), QgsScreenProperties());
        QPixmap pixmap = QPixmap::fromImage(image);

        // 创建模型项，设置名称和图标
        QStandardItem* item = new QStandardItem();
        item->setText(symbolName); // 使用符号名称作为显示文本
        item->setIcon(QIcon(pixmap));

        // 添加到模型
        model->appendRow(item);
    }

    // 将模型绑定到 QListView
    listView->setModel(model);
    listView->setIconSize(QSize(128, 128)); // 设置列表项图标的大小
    listView->setGridSize(QSize(160, 160)); // 设置列表项的网格大小
    listView->setViewMode(QListView::IconMode); // 图标模式
    listView->setSpacing(10); // 设置间距
}

void StyleManager::onActionIptStl() {
    // 弹出文件选择框，允许用户选择 XML 文件
    QString filePath = QFileDialog::getOpenFileName(
        nullptr,
        "Import Symbol Library",
        "",
        "XML Files (*.xml)" // 仅显示 XML 文件
    );

    // 如果用户未选择文件，直接返回
    if (filePath.isEmpty()) {
        qDebug() << "No file selected.";
        return;
    }

    // 定义目标文件夹路径
    QString destFolderPath = "E:/GISdes/styles/mystyle";
    QDir destDir(destFolderPath);

    // 检查目标文件夹是否存在
    if (!destDir.exists()) {
        qDebug() << "Destination folder does not exist:" << destFolderPath;
        return;
    }

    // 获取目标文件路径
    QFileInfo fileInfo(filePath);
    QString destFilePath = destFolderPath + "/" + fileInfo.fileName();

    // 复制文件到目标文件夹
    if (QFile::exists(destFilePath)) {
        qDebug() << "File already exists in destination folder:" << destFilePath;
        return;
    }

    if (!QFile::copy(filePath, destFilePath)) {
        qDebug() << "Failed to copy file to:" << destFilePath;
        return;
    }

    qDebug() << "File successfully copied to:" << destFilePath;

    // 调用 refreshStlView() 刷新符号视图
    refreshStlView();
}

void StyleManager::onActionStlExport(QListView* listView) {
    if (!listView) {
        QMessageBox::warning(this, tr("Error"), tr("No symbol list available."));
        return;
    }

    QModelIndex selectedIndex = listView->currentIndex();
    if (!selectedIndex.isValid()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a symbol to export."));
        return;
    }

    int row = selectedIndex.row();
    QgsSymbol* symbol = nullptr;
    QString symbolName;

    // 获取用户选择的符号
    if (listViewMode == "Marker" && row >= 0 && row < mMarkerSymbols.size()) {
        CustomMarkerSymbol* selectedSymbol = mMarkerSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else if (listViewMode == "Line" && row >= 0 && row < mLineSymbols.size()) {
        CustomLineSymbol* selectedSymbol = mLineSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else if (listViewMode == "Fill" && row >= 0 && row < mFillSymbols.size()) {
        CustomFillSymbol* selectedSymbol = mFillSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("Unknown or invalid symbol type."));
        return;
    }

    if (!symbol) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to retrieve the symbol."));
        return;
    }

    // 弹出保存文件对话框
    QString saveFilePath = QFileDialog::getSaveFileName(
        this,
        tr("Export SLD"),
        QString("%1.sld").arg(symbolName),
        tr("SLD Files (*.sld)")
    );

    if (saveFilePath.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("No file path selected for export."));
        return;
    }

    // 创建 SLD 文档
    QDomDocument doc;
    QDomElement root = doc.createElement("sld:StyledLayerDescriptor");
    root.setAttribute("version", "1.0.0");
    root.setAttribute("xsi:schemaLocation", "http://www.opengis.net/sld StyledLayerDescriptor.xsd");
    root.setAttribute("xmlns:sld", "http://www.opengis.net/sld");
    root.setAttribute("xmlns:ogc", "http://www.opengis.net/ogc");
    root.setAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
    root.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    doc.appendChild(root);

    QDomElement namedLayer = doc.createElement("sld:NamedLayer");
    root.appendChild(namedLayer);

    QDomElement nameElement = doc.createElement("sld:Name");
    nameElement.appendChild(doc.createTextNode(symbolName));
    namedLayer.appendChild(nameElement);

    QDomElement userStyle = doc.createElement("sld:UserStyle");
    namedLayer.appendChild(userStyle);

    QDomElement titleElement = doc.createElement("sld:Title");
    titleElement.appendChild(doc.createTextNode(tr("Style for %1").arg(symbolName)));
    userStyle.appendChild(titleElement);

    QDomElement featureTypeStyle = doc.createElement("sld:FeatureTypeStyle");
    userStyle.appendChild(featureTypeStyle);

    QDomElement rule = doc.createElement("sld:Rule");
    featureTypeStyle.appendChild(rule);

    QDomElement ruleTitle = doc.createElement("sld:Title");
    ruleTitle.appendChild(doc.createTextNode(tr("Rule for %1").arg(symbolName)));
    rule.appendChild(ruleTitle);

    // 根据符号类型动态生成 Symbolizer
    if (QgsMarkerSymbol* markerSymbol = dynamic_cast<QgsMarkerSymbol*>(symbol)) {
        // 点符号
        QDomElement pointSymbolizer = doc.createElement("sld:PointSymbolizer");
        QDomElement graphic = doc.createElement("sld:Graphic");
        pointSymbolizer.appendChild(graphic);

        QDomElement mark = doc.createElement("sld:Mark");
        graphic.appendChild(mark);

        QDomElement wellKnownName = doc.createElement("sld:WellKnownName");
        wellKnownName.appendChild(doc.createTextNode("circle")); // 示例：圆形
        mark.appendChild(wellKnownName);

        // 填充颜色
        QDomElement fill = doc.createElement("sld:Fill");
        QDomElement fillColor = doc.createElement("sld:CssParameter");
        fillColor.setAttribute("name", "fill");
        fillColor.appendChild(doc.createTextNode(markerSymbol->color().name())); // 动态填充颜色
        fill.appendChild(fillColor);
        mark.appendChild(fill);

        // 大小
        QDomElement size = doc.createElement("sld:Size");
        size.appendChild(doc.createTextNode(QString::number(markerSymbol->size()))); // 动态获取大小
        graphic.appendChild(size);

        rule.appendChild(pointSymbolizer);

    }
    else if (QgsLineSymbol* lineSymbol = dynamic_cast<QgsLineSymbol*>(symbol)) {
        // 线符号
        QDomElement lineSymbolizer = doc.createElement("sld:LineSymbolizer");
        QDomElement stroke = doc.createElement("sld:Stroke");

        // 线颜色
        QDomElement strokeColor = doc.createElement("sld:CssParameter");
        strokeColor.setAttribute("name", "stroke");
        strokeColor.appendChild(doc.createTextNode(lineSymbol->color().name())); // 动态线颜色
        stroke.appendChild(strokeColor);

        // 线宽
        QDomElement strokeWidth = doc.createElement("sld:CssParameter");
        strokeWidth.setAttribute("name", "stroke-width");
        strokeWidth.appendChild(doc.createTextNode(QString::number(lineSymbol->width()))); // 动态线宽
        stroke.appendChild(strokeWidth);

        lineSymbolizer.appendChild(stroke);
        rule.appendChild(lineSymbolizer);

    }
    else if (QgsFillSymbol* fillSymbol = dynamic_cast<QgsFillSymbol*>(symbol)) {
        QDomElement polygonSymbolizer = doc.createElement("sld:PolygonSymbolizer");

        // 获取填充颜色
        QDomElement fill = doc.createElement("sld:Fill");
        QDomElement fillColor = doc.createElement("sld:CssParameter");
        fillColor.setAttribute("name", "fill");
        fillColor.appendChild(doc.createTextNode(fillSymbol->color().name())); // 获取填充颜色
        fill.appendChild(fillColor);
        polygonSymbolizer.appendChild(fill);

        // 获取边框颜色和宽度
        QColor strokeColor = Qt::black; // 默认边框颜色
        double strokeWidth = 1.0;       // 默认边框宽度

        // 创建 Stroke
        QDomElement stroke = doc.createElement("sld:Stroke");

        // 设置边框颜色
        QDomElement strokeColorElement = doc.createElement("sld:CssParameter");
        strokeColorElement.setAttribute("name", "stroke");
        strokeColorElement.appendChild(doc.createTextNode(strokeColor.name())); // 设置边框颜色
        stroke.appendChild(strokeColorElement);

        // 设置边框宽度
        QDomElement strokeWidthElement = doc.createElement("sld:CssParameter");
        strokeWidthElement.setAttribute("name", "stroke-width");
        strokeWidthElement.appendChild(doc.createTextNode(QString::number(strokeWidth))); // 设置边框宽度
        stroke.appendChild(strokeWidthElement);

        polygonSymbolizer.appendChild(stroke);

        // 将生成的符号器添加到规则中
        rule.appendChild(polygonSymbolizer);
    }


    // 写入 SLD 文件
    QFile file(saveFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to write file: %1").arg(saveFilePath));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << doc.toString();
    file.close();

    QMessageBox::information(this, tr("Export Successful"), tr("Symbol exported successfully to %1").arg(saveFilePath));
}


void StyleManager::onActionStlApply(QListView* listView) {
    if (!listView) {
        QMessageBox::warning(this, tr("Error"), tr("No symbol list available."));
        return;
    }

    QModelIndex selectedIndex = listView->currentIndex();
    if (!selectedIndex.isValid()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a symbol to export."));
        return;
    }

    int row = selectedIndex.row();
    QgsSymbol* symbol = nullptr;
    QString symbolName;

    // 根据 listViewMode 判断符号类型
    if (listViewMode == "Marker" && row >= 0 && row < mMarkerSymbols.size()) {
        CustomMarkerSymbol* selectedSymbol = mMarkerSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else if (listViewMode == "Line" && row >= 0 && row < mLineSymbols.size()) {
        CustomLineSymbol* selectedSymbol = mLineSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else if (listViewMode == "Fill" && row >= 0 && row < mFillSymbols.size()) {
        CustomFillSymbol* selectedSymbol = mFillSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("Unknown or invalid symbol type."));
        return;
    }

    if (!symbol) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to retrieve the symbol."));
        return;
    }

    // 将符号类型映射到几何类型
    Qgis::GeometryType expectedGeometryType;

    // 发出信号
    emit signalApplyStlSymbol(mStlLayerName, symbol);

    QMessageBox::information(this, tr("Success"), tr("Symbol successfully applied."));
}
