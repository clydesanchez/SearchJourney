#include "RasterStyle.h"

#include <QFile>
#include <QDomDocument>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include <qgscolorrampshader.h>
#include <qgssinglebandpseudocolorrenderer.h>
#include <qgsrastershader.h>
#include <qgsstyle.h>
#include <QgsColorRampShader.h>

RasterStyle::RasterStyle(QString strLayerName, QgsRasterLayer* rasLayer, MainWidget* widMain, QWidget* parent)
    : QDockWidget(parent), mRasLayer(rasLayer), mrasColorRamp(nullptr) {

    createRasterSymbolDock(widMain);
}

RasterStyle::~RasterStyle() {
    if (mrasColorRamp) {
        delete mrasColorRamp;
        mrasColorRamp = nullptr;
    }
}

void RasterStyle::createRasterSymbolDock(MainWidget* widMain) {

    // 设置窗口标题
    setWindowTitle(tr("Raster Style"));

    // 创建容器控件
    QWidget* dockWidgetContent = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(dockWidgetContent);

    // 波段选择控件
    QLabel* lblBand = new QLabel(tr("Band:"));
    QComboBox* cmbBand = new QComboBox();
    layout->addWidget(lblBand);
    layout->addWidget(cmbBand);

    // 获取栅格波段信息
    if (mRasLayer) {
        int bandCount = mRasLayer->bandCount(); // 获取波段总数
        for (int i = 1; i <= bandCount; ++i) {
            cmbBand->addItem(tr("Band %1").arg(i), i); // 填充下拉框，数据存储波段索引
        }
    }
    else {
        cmbBand->addItem(tr("No bands available")); // 栅格图层为空时提示
    }

    // Interpolation 选择控件
    QLabel* lblInterpolation = new QLabel(tr("Interpolation:"));
    QComboBox* cmbInterpolation = new QComboBox();
    cmbInterpolation->addItem(tr("Discrete"), 0);  // Discrete 对应值 0
    cmbInterpolation->addItem(tr("Linear"), 1);   // Linear 对应值 1
    cmbInterpolation->addItem(tr("Exact"), 2);    // Exact 对应值 2
    layout->addWidget(lblInterpolation);
    layout->addWidget(cmbInterpolation);

    // Mode 选择控件
    QLabel* lblMode = new QLabel(tr("Mode:"));
    QComboBox* cmbMode = new QComboBox();
    cmbMode->addItem(tr("Continuous"), 0);  // Continuous 对应值 0
    cmbMode->addItem(tr("Equal Interval"), 1);   // Equal Interval 对应值 1
    cmbMode->addItem(tr("Quantile"), 2);    // Quantile 对应值 2
    layout->addWidget(lblMode);
    layout->addWidget(cmbMode);

    // 最小值和最大值输入控件
    QLabel* lblMinMax = new QLabel(tr("Min and Max Values:"));
    QLineEdit* txtMin = new QLineEdit();
    QLineEdit* txtMax = new QLineEdit();
    QHBoxLayout* minMaxLayout = new QHBoxLayout();
    minMaxLayout->addWidget(new QLabel(tr("Min:")));
    minMaxLayout->addWidget(txtMin);
    minMaxLayout->addWidget(new QLabel(tr("Max:")));
    minMaxLayout->addWidget(txtMax);
    layout->addWidget(lblMinMax);
    layout->addLayout(minMaxLayout);

    // 分级数选择框
    QLabel* lblClasses = new QLabel(tr("Number of Classes:"));
    QSpinBox* spinBoxClasses = new QSpinBox();
    spinBoxClasses->setRange(2, 10);  // 设置最小分级数为 2，最大分级数为 10
    spinBoxClasses->setValue(5);      // 默认值为 5
    layout->addWidget(lblClasses);
    layout->addWidget(spinBoxClasses);

    // 色带选择控件
    QLabel* lblColorRamp = new QLabel(tr("Color Ramp:"));
    QComboBox* cmbColorRamp = new QComboBox();
    layout->addWidget(lblColorRamp);
    layout->addWidget(cmbColorRamp);

    // 设置自定义委托来绘制色带样式
    cmbColorRamp->setItemDelegate(new ColorRampDelegate(cmbColorRamp));

    // 加载色带文件
    QString colorRampFilePath = "E:/GISdes/styles/Color/pokemon_UCHqlV3.xml"; // 考虑替换为相对路径
    QFile file(colorRampFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDomDocument doc;
        if (doc.setContent(&file)) {
            QDomElement root = doc.documentElement();
            QDomElement colorRampsElement = root.firstChildElement("colorramps");
            if (!colorRampsElement.isNull()) {
                QDomNodeList ramps = colorRampsElement.elementsByTagName("colorramp");
                for (int i = 0; i < ramps.count(); ++i) {
                    QDomNode rampNode = ramps.at(i);
                    if (rampNode.isElement()) {
                        QDomElement rampElement = rampNode.toElement();
                        QString rampName = rampElement.attribute("name");
                        
                        // 提取所有颜色信息
                        QList<QColor> colorList;

                        // 获取所有的 `prop` 节点
                        QDomNodeList props = rampElement.elementsByTagName("prop");

                        // 遍历 `prop` 节点，提取颜色
                        for (int k = 0; k < props.count(); ++k) {
                            QDomElement propElement = props.at(k).toElement();
                            if (!propElement.isNull()) {
                                QString colorValue = propElement.attribute("v"); // 获取颜色值
                                QString key = propElement.attribute("k");       // 可选：获取键名（如有其他用途）

                                // 检查颜色值是否是 RGBA 格式
                                QStringList rgba = colorValue.split(",");
                                if (rgba.size() == 4) {  // 确保有四个值：R, G, B, A
                                    QColor color(rgba[0].toInt(), rgba[1].toInt(), rgba[2].toInt(), rgba[3].toInt());
                                    colorList.append(color);
                                }
                            }
                        }

                        // 检查是否成功提取了至少两个颜色
                        if (colorList.size() >= 2) {
                            // 使用颜色列表创建渐变色带
                            auto* colorRamp = new QgsGradientColorRamp(colorList.first(), colorList.last());

                            // 如果需要支持多点渐变（非线性），可以用 QgsGradientStop 或类似扩展
                            // 示例：遍历 colorList，添加中间点到 colorRamp

                            cmbColorRamp->addItem(rampName, QVariant::fromValue<QgsColorRamp*>(colorRamp)); // 存储色带对象
                        }
                    }
                }
            }
        }
        file.close();
    }
    else {
        qDebug() << "Failed to open file: " << colorRampFilePath;
    }


    // 按钮：Apply 和 Cancel
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* btnApply = new QPushButton(tr("Apply"));
    QPushButton* btnCancel = new QPushButton(tr("Cancel"));
    buttonLayout->addWidget(btnApply);
    buttonLayout->addWidget(btnCancel);
    layout->addLayout(buttonLayout);

    // 设置内容布局
    dockWidgetContent->setLayout(layout);
    setWidget(dockWidgetContent);

    // 添加到主窗口右侧
    widMain->addDockWidget(Qt::RightDockWidgetArea, this);

    // 显示 Dock
    show();

    // 信号槽连接
    connect(cmbBand, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RasterStyle::getSelectedBand);

    // 连接信号槽，将用户选择的插值方式传递给成员变量 mInterpType
    connect(cmbInterpolation, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        mInterpType = cmbInterpolation->itemData(index).toInt(); // 获取插值类型
        qDebug() << "Selected interpolation type:" << mInterpType;
        });

    connect(cmbMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        mModType = cmbMode->itemData(index).toInt(); // 获取插值类型
        qDebug() << "Selected interpolation type:" << mModType;
        });

    // 连接信号槽
    connect(cmbColorRamp, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        QVariant rampVariant = cmbColorRamp->itemData(index, Qt::UserRole);
        if (rampVariant.isValid()) {
            QgsColorRamp* ramp = rampVariant.value<QgsColorRamp*>();
            getSelectedColorRamp(ramp);
        }
        else {
            QMessageBox::warning(this, tr("Error"), tr("Invalid color ramp selected."));
        }
        });


    connect(txtMin, &QLineEdit::editingFinished, this, [=]() {
        bool ok;
        double minValue = txtMin->text().toDouble(&ok);
        if (ok) {
            getMinValue(minValue);
        }
        else {
            QMessageBox::warning(this, tr("Error"), tr("Invalid minimum value."));
        }
        });
    connect(txtMax, &QLineEdit::editingFinished, this, [=]() {
        bool ok;
        double maxValue = txtMax->text().toDouble(&ok);
        if (ok) {
            getMaxValue(maxValue);
        }
        else {
            QMessageBox::warning(this, tr("Error"), tr("Invalid maximum value."));
        }
        });
    connect(btnApply, &QPushButton::clicked, this, &RasterStyle::onActionApplyStyle_ras);
    connect(btnCancel, &QPushButton::clicked, this, [=]() {
        this->close(); // 点击 Cancel 时关闭窗口
        });

    connect(spinBoxClasses, QOverload<int>::of(&QSpinBox::valueChanged), this, &RasterStyle::getNumClasses);

}


void RasterStyle::getSelectedBand(int rasBand) {
    mrasBand = rasBand;
    qDebug() << "Selected band:" << mrasBand;
}

void RasterStyle::getSelectedColorRamp(QgsColorRamp* ramp) {
    if (mrasColorRamp) {
        delete mrasColorRamp;
    }
    mrasColorRamp = ramp->clone();
    qDebug() << "Selected color ramp updated.";
}


void RasterStyle::getMinValue(double rasMinValue) {
    mrasMinValue = rasMinValue;
    qDebug() << "Minimum value set to:" << mrasMinValue;
}

void RasterStyle::getMaxValue(double rasMaxValue) {
    mrasMaxValue = rasMaxValue;
    qDebug() << "Maximum value set to:" << mrasMaxValue;
}

void RasterStyle::getNumClasses(int rasNumClasses) {
    mrasNumClasses = rasNumClasses;
    qDebug() << "Number of classes set to:" << mrasNumClasses;
}

void RasterStyle::onActionApplyStyle_ras() {
    if (!mRasLayer) {
        QMessageBox::warning(this, tr("Error"), tr("No valid raster layer provided."));
        return;
    }

    if (!mrasColorRamp) {
        QMessageBox::warning(this, tr("Error"), tr("No valid color ramp selected."));
        return;
    }

    if (mrasMinValue >= mrasMaxValue) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid min/max values."));
        return;
    }

    QgsColorRampShader* colorRampShader = new QgsColorRampShader(mrasMinValue, mrasMaxValue, mrasColorRamp);
    switch (mInterpType) {
    case 0: // Discrete
        colorRampShader->setColorRampType(QgsColorRampShader::Discrete);
        break;
    case 1: // Linear
        colorRampShader->setColorRampType(QgsColorRampShader::Interpolated);
        break;
    case 2: // Exact
        colorRampShader->setColorRampType(QgsColorRampShader::Exact);
        break;
    }

    switch (mModType) {
    case 0: // Continuous
        colorRampShader->setClassificationMode(QgsColorRampShader::ClassificationMode::Continuous);
        break;
    case 1: // Equal Interval
        colorRampShader->setClassificationMode(QgsColorRampShader::ClassificationMode::EqualInterval);
        break;
    case 2: // Quantile
        colorRampShader->setClassificationMode(QgsColorRampShader::ClassificationMode::Quantile);
        break;
    }

    QList<QgsColorRampShader::ColorRampItem> rampItems;
    double stepSize = (mrasMaxValue - mrasMinValue) / mrasNumClasses;
    for (int i = 0; i <= mrasNumClasses; ++i) {
        double value = mrasMinValue + i * stepSize;
        double ratio = static_cast<double>(i) / mrasNumClasses;
        rampItems.append(QgsColorRampShader::ColorRampItem(value, mrasColorRamp->color(ratio)));
    }
    colorRampShader->setColorRampItemList(rampItems);

    QgsRasterShader* rasterShader = new QgsRasterShader();
    rasterShader->setRasterShaderFunction(colorRampShader);

    QgsSingleBandPseudoColorRenderer* renderer = new QgsSingleBandPseudoColorRenderer(
        mRasLayer->dataProvider(), mrasBand, rasterShader);
    mRasLayer->setRenderer(renderer);
    mRasLayer->triggerRepaint();

    QMessageBox::information(this, tr("Success"), tr("Raster style applied successfully."));

    // 关闭窗口并清理资源
    this->close();
}
