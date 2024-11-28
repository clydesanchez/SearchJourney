#include "RasterCalculatorTool.h"
#include <QMessageBox>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDebug>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFileDialog>
#include <QDir>
#include <QList>
#include <QTimer>
#include <QDateTime>

#include <QgsRasterCalculator.h>
#include <QgsRasterLayer.h>
#include <QgsMapLayer.h>
#include <QgsRasterFileWriter.h>
#include <QgsRasterPipe.h>

RasterCalculatorTool::RasterCalculatorTool(QWidget* parent)
    : QDialog(parent)// 初始化父类 QDialog
{
    ui.setupUi(this);

    // 初始化默认表达式
    m_calculationExpression = QStringLiteral("\"band%1\"+\"band%1\"");

    // 禁用双击节点重命名功能
    ui.ctrlBandsTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 连接双击信号到槽函数
    connect(ui.ctrlBandsTreeView, &QTreeView::doubleClicked, this, &RasterCalculatorTool::onTreeViewDoubleClicked);
}

RasterCalculatorTool::~RasterCalculatorTool()
{}

//加载栅格图层
int RasterCalculatorTool::addRasterLayer()
{
    // 打开文件资源管理器选取栅格文件，并记录路径
    QString _qstrRasterFilePath = QFileDialog::getOpenFileName(
        this,
        tr(u8"选择进行计算的栅格文件："),
        "",
        tr("Raster Files (*.tif *.tiff)")
    );

    // 如果文件路径非空
    if (!_qstrRasterFilePath.isEmpty()) {
        // 将文件路径显示到 QLineEdit 控件
        ui.ctrlCurrentRasterPathLineEdit->setText(_qstrRasterFilePath);

        // 将路径添加到栅格路径列表 mqlAllRasterPaths 中
        mqlAllRasterPaths.append(_qstrRasterFilePath);

        // 通过文件路径加载栅格图层
        QgsRasterLayer* rasterLayer = new QgsRasterLayer(_qstrRasterFilePath, QFileInfo(_qstrRasterFilePath).fileName());

        // 检查栅格图层是否有效
        if (!rasterLayer->isValid()) {
            QMessageBox::warning(this, tr(u8"无效的栅格图层"), tr(u8"无法加载栅格图层"));
            return -1; // 如果图层无效，则返回错误
        }

        // 将图层添加到 mqlAllCalcuRaster 列表中
        mqlAllCalcuRaster.append(rasterLayer);

        // 更新树视图显示栅格图层及其波段
        QStandardItemModel* model = new QStandardItemModel(this);

        // 遍历所有栅格图层
        for (QgsMapLayer* mapLayer : mqlAllCalcuRaster) {
            QgsRasterLayer* rasterLayer = dynamic_cast<QgsRasterLayer*>(mapLayer);
            if (rasterLayer) {
                // 创建根节点，显示栅格图层的名称
                QStandardItem* layerItem = new QStandardItem(rasterLayer->name());
                model->appendRow(layerItem);
                
                // 建立图层和节点的映射
                m_itemToLayerMap.insert(layerItem, rasterLayer);

                // 遍历波段并添加为子节点
                int bandCount = rasterLayer->bandCount();
                for (int i = 1; i <= bandCount; ++i) {
                    QStandardItem* bandItem = new QStandardItem(QString("Band %1").arg(i));
                    layerItem->appendRow(bandItem);
                }
                // 输出映射内容到 ctrlDebugTextBrowser
                QString debugMessage = tr("成功加载栅格图层 '%1'，建立节点映射。\n").arg(rasterLayer->name());
                debugMessage += tr("当前映射内容：\n");
                for (auto it = m_itemToLayerMap.constBegin(); it != m_itemToLayerMap.constEnd(); ++it) {
                    debugMessage += tr("Item: %1 -> Layer: %2 (%3)\n")
                        .arg(reinterpret_cast<quintptr>(it.key())) // 输出指针地址
                        .arg(it.value() ? it.value()->name() : tr("null")) // 图层名称
                        .arg(it.value() && it.value()->isValid() ? tr("有效") : tr("无效"));
                }
                ui.ctrlDebugTextBrowser->append(debugMessage);
            }
        }
        // 设置模型到树视图控件
        ui.ctrlBandsTreeView->setModel(model);
    }

    return 0;
}

//保存栅格结果
int RasterCalculatorTool::saveRasterResult()
{
    // 设置默认路径为当前目录下的 "CalcuResult" 文件夹
    QString _qstrResultDefaultPath = QDir::currentPath() + "/CalcuResult";

    // 如果文件夹不存在，创建该文件夹
    QDir dir;
    if (!dir.exists(_qstrResultDefaultPath)) {
        if (!dir.mkpath(_qstrResultDefaultPath)) {
            // 如果创建文件夹失败，给出提示
            QMessageBox::warning(this, tr(u8"文件夹创建失败"), tr(u8"无法创建结果文件夹"));
            return -1;  // 返回错误代码
        }
    }

    // 打开文件资源管理器选择保存文件路径，并记录路径
    QString _qstrResultFilePath = QFileDialog::getSaveFileName(
        this,  // 父窗口
        tr(u8"选择保存计算结果的栅格文件："),  // 对话框标题
        _qstrResultDefaultPath,  // 默认路径设置为 CalcuResult 文件夹
        tr("Raster Files (*.tif *.tiff)")  // 文件过滤器，选择栅格文件类型
    );

    // 如果文件路径非空
    if (!_qstrResultFilePath.isEmpty()) {
        // 将文件路径显示到 QLineEdit 控件
        ui.ctrlSavePathLineEdit->setText(_qstrResultFilePath);
    }
    else {
        // 如果没有选择文件路径，则返回
        return -1;
    }

    // 遍历 mqlResultRaster 中的所有栅格图层，保存每个结果图层
    for (QgsMapLayer* mapLayer : mqlResultRaster) {
        QgsRasterLayer* resultLayer = dynamic_cast<QgsRasterLayer*>(mapLayer);
        if (resultLayer && resultLayer->isValid()) {
            // 获取保存路径，并确保文件名唯一
            QString resultFilePath = _qstrResultFilePath;

            // 如果多个结果，自动在文件名后加上一个序号
            if (mqlResultRaster.size() > 1) {
                QFileInfo fileInfo(resultFilePath);
                resultFilePath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + "_result_" + QString::number(mqlResultRaster.indexOf(mapLayer) + 1) + ".tif";
            }

            // 获取栅格图层的数据提供者
            QgsRasterDataProvider* provider = resultLayer->dataProvider();

            // 创建 QgsRasterPipe 对象
            QgsRasterPipe pipe;
            pipe.set(provider);

            // 获取栅格图层的范围、宽度、高度和坐标参考系
            QgsRectangle extent = resultLayer->extent();
            int width = resultLayer->width();
            int height = resultLayer->height();
            QgsCoordinateReferenceSystem crs = resultLayer->crs();

            // 创建反馈对象，便于显示保存进度（可选）
            QgsRasterBlockFeedback* feedback = nullptr; // 可选择传递一个反馈对象

            // 使用 QgsRasterFileWriter 保存栅格图层
            QgsRasterFileWriter writer(resultFilePath);

            // 调用 writeRaster
            Qgis::RasterFileWriterResult writeStatus = writer.writeRaster(
                &pipe,    // 数据管道
                width,    // 栅格宽度
                height,   // 栅格高度
                extent,   // 范围
                crs,      // 坐标参考系
                feedback  // 可选反馈
            );

            // 检查保存是否成功
            if (writeStatus != Qgis::RasterFileWriterResult::Success) {
                QMessageBox::warning(this, tr(u8"保存失败"), tr(u8"图层保存失败"));
                return -1;
            }
        }
        else {
            QMessageBox::warning(this, tr(u8"错误"), tr(u8"无效的栅格图层"));
            return -1;
        }
    }

    QMessageBox::information(this, tr(u8"保存成功"), tr(u8"栅格计算结果保存位置设置成功"));
    return 0;
}

//执行波段运算
int RasterCalculatorTool::performRasterCalculation()
{
    // 从输入框获取表达式
    QString calculationExpression = ui.ctrlCalcExpressionTextEdit->toPlainText();
    if (calculationExpression.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("计算表达式为空，请输入计算表达式"));
        return -1;
    }

    // 创建参与计算的波段集
    QVector<QgsRasterCalculatorEntry> entries;

    // 解析表达式中的波段引用
    QRegularExpression regex("\"([^\"]+)_band(\\d+)\"");
    QRegularExpressionMatchIterator it = regex.globalMatch(calculationExpression);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        if (!match.hasMatch()) continue;

        QString layerName = match.captured(1); // 图层名称
        int bandNumber = match.captured(2).toInt(); // 波段编号

        // 查找对应图层
        QgsRasterLayer* layer = nullptr;
        for (QgsMapLayer* mapLayer : mqlAllCalcuRaster) {
            QgsRasterLayer* rasterLayer = dynamic_cast<QgsRasterLayer*>(mapLayer);
            if (rasterLayer && rasterLayer->name() == layerName) {
                layer = rasterLayer;
                break;
            }
        }

        if (!layer || !layer->isValid()) {
            QMessageBox::warning(this, tr("错误"), tr("找不到有效的图层 '%1'").arg(layerName));
            return -1;
        }

        // 添加到计算条目
        QgsRasterCalculatorEntry entry;
        entry.bandNumber = bandNumber;
        entry.raster = layer;
        entry.ref = QStringLiteral("%1_band%2").arg(layerName).arg(bandNumber);
        entries.append(entry);
    }

    if (entries.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("表达式中没有找到有效的波段引用"));
        return -1;
    }

    // 设置计算范围、坐标参考系等
    QgsCoordinateReferenceSystem crs = entries.first().raster->crs();
    QgsRectangle extent = entries.first().raster->extent();
    int width = entries.first().raster->width();
    int height = entries.first().raster->height();

    // 创建临时文件
    QTemporaryFile tmpFile;
    tmpFile.open();
    QString tmpName = tmpFile.fileName();
    tmpFile.close();

    // 初始化进度条
    ui.ctrlRCProgressBar->setValue(0);
    ui.ctrlRCProgressBar->setVisible(true);

    // 创建定时器，用于模拟进度
    QTimer timer;
    timer.setInterval(100); // 每100ms更新一次进度
    int progress = 0;

    // 连接定时器到槽函数，更新进度条
    connect(&timer, &QTimer::timeout, [&]() {
        if (progress < 95) { // 在计算过程中，模拟进度达到 95% 为止
            progress += 5; // 每次递增5
            ui.ctrlRCProgressBar->setValue(progress);
        }
        });

    // 开始模拟计算（真实计算执行在下面）
    timer.start();

    // 执行栅格计算
    QgsRasterCalculator rc(
        calculationExpression,
        tmpName,
        QStringLiteral("GTiff"),
        extent,
        crs,
        width,
        height,
        entries);

    if (rc.processCalculation() != 0) {
        timer.stop(); // 停止进度更新
        QMessageBox::warning(this, tr("错误"), tr("计算失败"));
        ui.ctrlRCProgressBar->setValue(0);  // 重置进度条
        ui.ctrlRCProgressBar->setVisible(false);
        return -1;
    }

    // 停止定时器，并将进度条设置为 100%
    timer.stop();
    ui.ctrlRCProgressBar->setValue(100);

    // 创建结果图层
    QgsRasterLayer* resultLayer = new QgsRasterLayer(tmpName, QStringLiteral("CalculationResult"));
    if (!resultLayer->isValid()) {
        QMessageBox::warning(this, tr("错误"), tr("计算结果无效"));
        ui.ctrlRCProgressBar->setVisible(false);
        return -1;
    }

    // 根据按钮状态处理图层添加逻辑
    if (ui.checkBox->isChecked()) {
        // 勾选时，添加图层到项目中并记录到列表
        QgsProject::instance()->addMapLayer(resultLayer);
        mqlResultRaster.append(resultLayer);
    }
    else {
        // 未勾选时执行其他逻辑，或不添加图层
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        ui.ctrlDebugTextBrowser->append(currentTime + " - " + "控件未勾选，未添加图层。");
    }

    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui.ctrlDebugTextBrowser->append(currentTime + " - 计算完成，结果已保存。");
    return 0;
}



// 树形视图双击添加波段
void RasterCalculatorTool::onTreeViewDoubleClicked(const QModelIndex& index)
{
    // 获取模型
    QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(ui.ctrlBandsTreeView->model());
    if (!model) {
        QMessageBox::warning(this, tr("错误"), tr("树视图模型无效"));
        return;
    }

    // 获取双击的节点
    QStandardItem* item = model->itemFromIndex(index);
    if (!item) {
        QMessageBox::warning(this, tr("错误"), tr("未能获取双击的节点"));
        return;
    }

    // 检查是否是波段节点（叶节点）
    if (item->parent() == nullptr) {
        QMessageBox::information(this, tr("提示"), tr("请选择波段节点"));
        return; // 如果是根节点，则返回
    }

    // 获取父节点（图层节点）
    QStandardItem* parentItem = item->parent();
    if (!parentItem) {
        ui.ctrlDebugTextBrowser->append(tr("错误：未找到图层节点。\n"));
        return;
    }

    // 从映射中找到对应的栅格图层
    QgsRasterLayer* layer = m_itemToLayerMap.value(parentItem, nullptr);

    // 输出调试信息到 ctrlDebugTextBrowser
    if (!layer || !layer->isValid()) {
        QMessageBox::warning(this, tr("错误"), tr("无效的栅格图层"));
        return;
    }

    // 获取波段编号
    QString bandText = item->text(); // 波段节点文本，例如 "Band 1"
    bool ok;
    int bandNumber = bandText.remove("Band ").toInt(&ok); // 提取波段编号
    if (!ok || bandNumber <= 0) {
        ui.ctrlDebugTextBrowser->append(tr("错误：波段名称格式无效。\n"));
        return;
    }

    // 在控件中插入波段引用
    QString bandReference = QString("\"%1_band%2\"").arg(layer->name()).arg(bandNumber);
    ui.ctrlCalcExpressionTextEdit->insertPlainText(bandReference);

    // 提示用户选择
    QString message = tr("您选择了图层 '%1' 的波段 %2").arg(layer->name()).arg(bandNumber);
    ui.ctrlDebugTextBrowser->append(message);
}


//槽函数
// 打开栅格文件
void RasterCalculatorTool::on_ctrlOpenRasterPushButton_clicked()
{
    // 调用 addRasterLayer 函数，打开栅格文件并存储路径
    addRasterLayer();
}


// 加法
void RasterCalculatorTool::on_ctrlPlussPushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" + ");
}

// 减法
void RasterCalculatorTool::on_ctrlMinusPushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" - ");
}

// 乘法
void RasterCalculatorTool::on_ctrlMultiplyPushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" * ");
}

// 除法
void RasterCalculatorTool::on_ctrlDividePushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" / ");
}

// 指数
void RasterCalculatorTool::on_ctrlExpPushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" ^ ");
}
// 自然对数
void RasterCalculatorTool::on_ctrlLnPushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" ln( )");
    QTextCursor cursor = ui.ctrlCalcExpressionTextEdit->textCursor();
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
    ui.ctrlCalcExpressionTextEdit->setTextCursor(cursor);
}
// 10的对数
void RasterCalculatorTool::on_ctrlLog10PushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" log10( )");
    QTextCursor cursor = ui.ctrlCalcExpressionTextEdit->textCursor();
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
    ui.ctrlCalcExpressionTextEdit->setTextCursor(cursor);
}
// 左括号
void RasterCalculatorTool::on_ctrlLeftParenPushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" ( ");
}
// 右括号
void RasterCalculatorTool::on_ctrlRightParenPushButton_clicked()
{
    ui.ctrlCalcExpressionTextEdit->insertPlainText(" ) ");
}

// 保存路径选择
void RasterCalculatorTool::on_ctrlSetSavePathPushButton_clicked()
{
    saveRasterResult();
}

// 执行计算
void RasterCalculatorTool::on_ctrlRunPushButton_clicked()
{
    performRasterCalculation();
}

// 修改计算表达式
/*
在需要修改表达式的地方，可以调用 setCalculationExpression
RasterCalculatorTool tool;
tool.setCalculationExpression("\"band%1\" + \"band%2\"");  // 修改为自定义表达式
或者
从界面控件中获取用户输入：
QString userExpression = ui.ctrlCalcExpressionTextEdit->toPlainText();
tool.setCalculationExpression(userExpression);
*/
void RasterCalculatorTool::setCalculationExpression(const QString& expression)
{
    m_calculationExpression = expression;
}
