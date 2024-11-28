#include "MainWidget.h"
#include "SelectFeatureTool.h"
#include "MoveFeatureTool.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QPainterPath>
#include <QgsMapMouseEvent.h>
#include <QgsGeometry.h>
#include <QgsAbstractGeometry.h>
#include <QgsPoint.h> 
#include <QgsvectorLayer.h>
#include <QgsFeature.h>
#include <QgsGeometry.h>
#include <QgsVectorFileWriter.h>
#include <QgsProject.h>
#include <QgsVectorLayerEditUtils.h>
#include <qinputdialog.h>
#include <qobject.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <cmath>
#include <vector>

void MainWidget::on_ctrlDeleteAction_triggered() {
    // 获取当前的矢量图层
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::deleteFeature);
    mcanMapCanvas->unsetMapTool(mcanMapCanvas->mapTool());
    mcanMapCanvas->setMapTool(pSelectTool);
}

void MainWidget::on_ctrlEditAttriAction_triggered() {
    // 获取当前的矢量图层
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::editAttribute);
    mcanMapCanvas->unsetMapTool(mcanMapCanvas->mapTool());
    mcanMapCanvas->setMapTool(pSelectTool);
}

void MainWidget::on_ctrlMoveAction_triggered() {
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::moveFeature);
    mcanMapCanvas->unsetMapTool(mcanMapCanvas->mapTool());
    mcanMapCanvas->setMapTool(pSelectTool);
}

void MainWidget::on_ctrlCopyAction_triggered() {
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::copyFeature);
    mcanMapCanvas->unsetMapTool(mcanMapCanvas->mapTool());
    mcanMapCanvas->setMapTool(pSelectTool);
}

void MainWidget::selectFeatures(const QList<QgsFeature>& selectedFeatures) {
    mpfSelectFeature = selectedFeatures;
}
//添加点
void MainWidget::on_ctrlAddPointAction_triggered()
{
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::selectFeatures);
    // 设置编辑工具
    PointEdit* pointEditTool = new PointEdit(mcanMapCanvas, vectorLayer);
    mcanMapCanvas->setMapTool(pointEditTool);
    ui.ctrlAddPointAction->setEnabled(false);
}
//撤销
void MainWidget::on_ctrlUndoAction_triggered()
{
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    //调用QGIS的撤销重做
    if (vectorLayer->undoStack()->canUndo())
    {
        vectorLayer->undoStack()->undo();
        mcanMapCanvas->refresh();
    }
}
//重做
void MainWidget::on_ctrlRedoAction_triggered()
{
    //mGeometryEditTool->redo();
    //调用QGIS的撤销重做
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    if (vectorLayer->undoStack()->canRedo())
    {
        vectorLayer->undoStack()->redo();
        mcanMapCanvas->refresh();
    }
}
//保存编辑
void MainWidget::on_ctrlSaveEditAction_triggered()
{
    //结束编辑
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    vectorLayer->commitChanges();
    //刷新地图
    mcanMapCanvas->refresh();
}

//剪断线
void MainWidget::on_ctrlSmoothLineAction_triggered() {
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::smoothLines);
    mcanMapCanvas->setMapTool(pSelectTool);
    
}

//光滑线
void MainWidget::on_ctrlPolygonToLineAction_triggered() {
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::selectFeatures);
    //判断激活图层类型，如果为面图层，则执行面转线
    if (vectorLayer->geometryType() == Qgis::GeometryType::Polygon)
    {
        //面转线,弹出对话框，选择保存路径
        QString qstrSavePath = QFileDialog::getSaveFileName(this, tr("保存shp文件"), "", "*.shp");
        if (qstrSavePath.isEmpty())
        {
            return;
        }
        QgsVectorLayer* qvlLineLayer = polygonToLines(vectorLayer, qstrSavePath);
        if (qvlLineLayer)
        {
            mppjProject->addMapLayer(qvlLineLayer);
            setLayerToMap(static_cast<QgsMapLayer*>(qvlLineLayer));
        }
    }
}

void MainWidget::on_ctrlThiningLineAction_triggered() {
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    QgsMapToolSelectFeatures* pSelectTool = new QgsMapToolSelectFeatures(mcanMapCanvas, vectorLayer);
    connect(pSelectTool, &QgsMapToolSelectFeatures::sigSelectFeatureChange,
        this, &MainWidget::thiningLines);
    mcanMapCanvas->setMapTool(pSelectTool);
}

//光滑线
void MainWidget::smoothLines(const QList<QgsFeature>& selectedFeatures)
{
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    if (!vectorLayer || selectedFeatures.isEmpty())
    {
        qDebug() << "Invalid layer or no features selected.";
        return;
    }

    // 创建弹窗
    QDialog dialog;
    dialog.setWindowTitle("选择光滑算法");
    dialog.resize(300, 250);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 创建算法选择框
    QComboBox* comboBox = new QComboBox(&dialog);
    comboBox->addItem("贝塞尔光滑");
    comboBox->addItem("样条光滑");
    comboBox->addItem("高斯光滑"); // 新算法
    layout->addWidget(new QLabel("选择光滑算法:", &dialog));
    layout->addWidget(comboBox);

    // 参数输入区
    QLabel* paramLabel1 = new QLabel("细分数量:", &dialog);
    QSpinBox* paramInput1 = new QSpinBox(&dialog);
    paramInput1->setRange(1, 100);
    paramInput1->setValue(20); // 贝塞尔默认细分数量
    paramInput1->setVisible(true);

    QLabel* paramLabel2 = new QLabel("张力系数:", &dialog);
    QDoubleSpinBox* paramInput2 = new QDoubleSpinBox(&dialog);
    paramInput2->setRange(0.0, 1.0);
    paramInput2->setDecimals(2);
    paramInput2->setSingleStep(0.1);
    paramInput2->setValue(0.2); // 样条默认张力
    paramInput2->setVisible(false);

    QLabel* paramLabel3 = new QLabel("插值点数:", &dialog);
    QSpinBox* paramInput3 = new QSpinBox(&dialog);
    paramInput3->setRange(1, 50);
    paramInput3->setValue(10); // 样条默认插值点数
    paramInput3->setVisible(false);

    QLabel* paramLabel4 = new QLabel("窗口大小:", &dialog); // 高斯平滑参数
    QSpinBox* paramInput4 = new QSpinBox(&dialog);
    paramInput4->setRange(1, 20);
    paramInput4->setValue(5); // 默认窗口大小
    paramInput4->setVisible(false);

    QLabel* paramLabel5 = new QLabel("标准差 (Sigma):", &dialog); // 高斯平滑参数
    QDoubleSpinBox* paramInput5 = new QDoubleSpinBox(&dialog);
    paramInput5->setRange(0.1, 10.0); // 标准差范围
    paramInput5->setValue(1.0);       // 默认标准差
    paramInput5->setVisible(false);

    layout->addWidget(paramLabel1);
    layout->addWidget(paramInput1);
    layout->addWidget(paramLabel2);
    layout->addWidget(paramInput2);
    layout->addWidget(paramLabel3);
    layout->addWidget(paramInput3);
    layout->addWidget(paramLabel4);
    layout->addWidget(paramInput4);
    layout->addWidget(paramLabel5);
    layout->addWidget(paramInput5);

    // 确认按钮
    QPushButton* okButton = new QPushButton("确定", &dialog);
    layout->addWidget(okButton);

    // 动态调整界面参数设置
    QObject::connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
        if (comboBox->itemText(index) == "贝塞尔光滑")
        {
            paramLabel1->setVisible(true);
            paramInput1->setVisible(true);

            paramLabel2->setVisible(false);
            paramInput2->setVisible(false);
            paramLabel3->setVisible(false);
            paramInput3->setVisible(false);
            paramLabel4->setVisible(false);
            paramInput4->setVisible(false);
            paramLabel5->setVisible(false);
            paramInput5->setVisible(false);
        }
        else if (comboBox->itemText(index) == "样条光滑")
        {
            paramLabel1->setVisible(false);
            paramInput1->setVisible(false);

            paramLabel2->setVisible(true);
            paramInput2->setVisible(true);
            paramLabel3->setVisible(true);
            paramInput3->setVisible(true);
            paramLabel4->setVisible(false);
            paramInput4->setVisible(false);
            paramLabel5->setVisible(false);
            paramInput5->setVisible(false);
        }
        else if (comboBox->itemText(index) == "高斯光滑")
        {
            paramLabel1->setVisible(false);
            paramInput1->setVisible(false);

            paramLabel2->setVisible(false);
            paramInput2->setVisible(false);
            paramLabel3->setVisible(false);
            paramInput3->setVisible(false);

            paramLabel4->setVisible(true);
            paramInput4->setVisible(true);
            paramLabel5->setVisible(true);
            paramInput5->setVisible(true);
        }
        });

    // 点击确认按钮，调用对应算法
    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    // 显示弹窗
    if (dialog.exec() == QDialog::Accepted)
    {
        QString method = comboBox->currentText();
        if (method == "贝塞尔光滑")
        {
            int subdivision = paramInput1->value();
            applyBezierSmoothing(vectorLayer, selectedFeatures, subdivision);
        }
        else if (method == "样条光滑")
        {
            double tension = paramInput2->value();
            int interpolationPoints = paramInput3->value();
            applySplineSmoothing(vectorLayer, selectedFeatures, tension, interpolationPoints);
        }
        else if (method == "高斯光滑")
        {
            int kernelSize = paramInput4->value();
            double sigma = paramInput5->value();
            applyGaussianSmoothing(vectorLayer, selectedFeatures, kernelSize, sigma);
        }
        else
        {
            qDebug() << "Unsupported smoothing method.";
        }
    }
}
//抽稀线
void MainWidget::thiningLines(const QList<QgsFeature>& selectedFeatures)
{
    QList<QgsMapLayer*> layers = mcanMapCanvas->layers();
    QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layers[mnActiveLayerIndex]);
    if (!vectorLayer || selectedFeatures.isEmpty())
    {
        qDebug() << "Invalid layer or no features selected.";
        return;
    }

    // 创建弹窗
    QDialog dialog;
    dialog.resize(300, 200);
    dialog.setWindowTitle("Select Simplification Method");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // 创建第一个下拉框（选择算法）
    QComboBox* methodComboBox = new QComboBox(&dialog);
    methodComboBox->addItem("贝塞尔抽稀");
    methodComboBox->addItem("固定间隔抽稀");
    methodComboBox->addItem("道格拉斯算法");
    layout->addWidget(new QLabel("选择抽稀算法:", &dialog));
    layout->addWidget(methodComboBox);

    // 参数输入框
    QLabel* paramLabel = new QLabel("算法参数:", &dialog);
    layout->addWidget(paramLabel);

    QLineEdit* paramInput = new QLineEdit(&dialog);
    paramInput->setPlaceholderText("请输入算法参数");
    layout->addWidget(paramInput);

    // 确认按钮
    QPushButton* okButton = new QPushButton("确定", &dialog);
    layout->addWidget(okButton);

    // 动态更新参数提示
    QObject::connect(methodComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
        QString method = methodComboBox->itemText(index);
        if (method == "贝塞尔抽稀")
        {
            paramLabel->setText("细分数量 (整数):");
            paramInput->setPlaceholderText("例如：20");
        }
        else if (method == "固定间隔抽稀")
        {
            paramLabel->setText("间隔 (整数):");
            paramInput->setPlaceholderText("例如：10");
        }
        else if (method == "道格拉斯算法")
        {
            paramLabel->setText("阈值 (小数):");
            paramInput->setPlaceholderText("例如：5.0");
        }
        });
    // 点击确认按钮后获取输入
    QObject::connect(okButton, &QPushButton::clicked, [&dialog]() { dialog.accept(); });

    // 弹窗显示
    if (dialog.exec() == QDialog::Accepted)
    {
        QString method = methodComboBox->currentText();
        QString paramValueStr = paramInput->text();
        bool isValid;
        double paramValue = paramValueStr.toDouble(&isValid);

        if (!isValid || paramValue <= 0)
        {
            qDebug() << "Invalid parameter value.";
            return;
        }

        // 根据选中的算法调用不同的处理方法
        if (method == "贝塞尔抽稀")
        {
            applyBezierDownsampling(vectorLayer, selectedFeatures, static_cast<int>(paramValue)); // 参数为整数
        }
        else if (method == "固定间隔抽稀")
        {
            applyIntervalDownsampling(vectorLayer, selectedFeatures, static_cast<int>(paramValue)); // 参数为整数
        }
        else if (method == "道格拉斯算法")
        {
            applyDouglasPeuckerSimplification(vectorLayer, selectedFeatures, paramValue); // 参数为浮点数
        }
        else
        {
            qDebug() << "Unsupported smoothing method.";
        }
    }
}