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
#include <qgssldexportcontext.h>
#include <qgsmaplayer.h>
#include "qgssymbol.h" 
#include "qgis_core.h"


StyleManager::StyleManager(QString strLayerName, Qgis::GeometryType layerType, MainWidget* widMain, QgsVectorLayer* veclayer, QWidget* parent)
    : QMainWindow(parent) {
    ui.setupUi(this);

    this->mStlLayerName = strLayerName;
    this->mStlLayerType = layerType;
    this->mVeclayer = veclayer;

    // 创建 Tab Widget
    QTabWidget* tabWidget = new QTabWidget(this);
    tabWidget->addTab(new QWidget(), tr("Marker"));
    tabWidget->addTab(new QWidget(), tr("Line"));
    tabWidget->addTab(new QWidget(), tr("Fill"));
    tabWidget->setMaximumHeight(50);
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mListViewMode = "Marker";

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
    mainLayout->setContentsMargins(0, 0, 0, 0); 
    mainLayout->setSpacing(5);

    // 将控件添加到主布局
    mainLayout->addWidget(tabWidget);     
    mainLayout->addWidget(listView);     
    mainLayout->addLayout(buttonLayout); 

    // 创建主窗口部件
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget); 

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
        default: break;
        }
        listModel->setStringList(items);
        });

    listView->setIconSize(QSize(200, 200));
    listView->setGridSize(QSize(220, 240));

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
        mListViewMode = "Marker";
        break;
    case 1: // Line 标签页
        mListViewMode = "Line";
        break;
    case 2: // Fill 标签页
        mListViewMode = "Fill";
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
    QString folderPath = "../styles/mystyle";
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

    QStandardItemModel* model = new QStandardItemModel(this);

    for (int i = 0; i < mMarkerSymbols.size(); ++i) {
        CustomMarkerSymbol* markerSymbol = mMarkerSymbols[i];
        QString symbolName = markerSymbol->getName();

        // 创建符号预览图
        QImage image = markerSymbol->getSymbol()->bigSymbolPreviewImage(nullptr, Qgis::SymbolPreviewFlags(), QgsScreenProperties());
        QPixmap pixmap = QPixmap::fromImage(image);

        // 创建模型项
        QStandardItem* item = new QStandardItem();
        item->setText(symbolName);
        item->setIcon(QIcon(pixmap));
        model->appendRow(item);
    }

    listView->setModel(model);
    listView->setViewMode(QListView::IconMode);    // 图标模式
    listView->setFlow(QListView::LeftToRight);    // 从左到右排列
    listView->setResizeMode(QListView::Adjust);   // 自动调整布局
    listView->setSpacing(10);                     // 项目之间的间距

}



void StyleManager::lineViewer(QListView* listView) {
    if (!listView) return;

    // 创建模型用于显示符号名称和预览
    QStandardItemModel* model = new QStandardItemModel(this);

    for (int i = 0; i < mLineSymbols.size(); ++i) {
        CustomLineSymbol* lineSymbol = mLineSymbols[i];

        // 假设符号名称已经在 XML 中解析并存储
        QString symbolName = lineSymbol->getName(); 

        // 创建符号预览图
        QImage image = lineSymbol->getSymbol()->bigSymbolPreviewImage(nullptr, Qgis::SymbolPreviewFlags(), QgsScreenProperties());
        QPixmap pixmap = QPixmap::fromImage(image);

        // 创建模型项，设置名称和图标
        QStandardItem* item = new QStandardItem();
        item->setText(symbolName); 
        item->setIcon(QIcon(pixmap));
        model->appendRow(item);
    }

    listView->setModel(model);
    listView->setViewMode(QListView::IconMode);    // 图标模式
    listView->setFlow(QListView::LeftToRight);    // 从左到右排列
    listView->setResizeMode(QListView::Adjust);   // 自动调整布局
    listView->setSpacing(10);
}


void StyleManager::fillViewer(QListView* listView) {
    if (!listView) return;

    // 创建模型用于显示符号名称和预览
    QStandardItemModel* model = new QStandardItemModel(this);

    for (int i = 0; i < mFillSymbols.size(); ++i) {
        CustomFillSymbol* fillSymbol = mFillSymbols[i];

        // 假设符号名称已经在 XML 中解析并存储到 QList<QgsFillSymbol*> 时通过额外的列表保存
        QString symbolName = fillSymbol->getName(); 

        // 创建符号预览图
        QImage image = fillSymbol->getSymbol()->bigSymbolPreviewImage(nullptr, Qgis::SymbolPreviewFlags(), QgsScreenProperties());
        QPixmap pixmap = QPixmap::fromImage(image);

        // 创建模型项，设置名称和图标
        QStandardItem* item = new QStandardItem();
        item->setText(symbolName); 
        item->setIcon(QIcon(pixmap));

        // 添加到模型
        model->appendRow(item);
    }

    listView->setModel(model);
    listView->setViewMode(QListView::IconMode);    // 图标模式
    listView->setFlow(QListView::LeftToRight);    // 从左到右排列
    listView->setResizeMode(QListView::Adjust);   // 自动调整布局
    listView->setSpacing(10);
}

void StyleManager::onActionIptStl() {
    // 弹出文件选择框，允许用户选择 XML 文件
    QString filePath = QFileDialog::getOpenFileName(
        nullptr,
        "Import Symbol Library",
        "",
        "XML Files (*.xml)"
    );

    // 如果用户未选择文件，直接返回
    if (filePath.isEmpty()) {
        qDebug() << "No file selected.";
        return;
    }

    // 定义目标文件夹路径
    QString destFolderPath = "../styles/mystyle";
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

    // 刷新符号视图
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
    QString symbolName;
    QgsSymbol* symbol = nullptr;

    // 获取符号和名称
    if (mListViewMode == "Marker" && row >= 0 && row < mMarkerSymbols.size()) {
        CustomMarkerSymbol* selectedSymbol = mMarkerSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else if (mListViewMode == "Line" && row >= 0 && row < mLineSymbols.size()) {
        CustomLineSymbol* selectedSymbol = mLineSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else if (mListViewMode == "Fill" && row >= 0 && row < mFillSymbols.size()) {
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

    // 创建 SLD 导出上下文
    QgsSldExportContext exportContext;
    exportContext.setExportFilePath(saveFilePath);

    // 调用 saveSldStyleV2
    bool resultFlag = false;
    QString statusMessage = mVeclayer->saveSldStyleV2(resultFlag, exportContext);

    if (resultFlag) {
        QMessageBox::information(this, tr("Export Successful"), tr("SLD exported successfully to %1").arg(saveFilePath));
    }
    else {
        QMessageBox::warning(this, tr("Export Failed"), tr("Failed to export SLD:\n%1").arg(statusMessage));
    }
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
    if (mListViewMode == "Marker" && row >= 0 && row < mMarkerSymbols.size()) {
        CustomMarkerSymbol* selectedSymbol = mMarkerSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else if (mListViewMode == "Line" && row >= 0 && row < mLineSymbols.size()) {
        CustomLineSymbol* selectedSymbol = mLineSymbols[row];
        symbol = selectedSymbol->getSymbol();
        symbolName = selectedSymbol->getName();
    }
    else if (mListViewMode == "Fill" && row >= 0 && row < mFillSymbols.size()) {
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
