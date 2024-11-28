#include "PointEdit.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <qgsattributes.h>
#include <qgsmapmouseevent.h>
#include <qgsmapcanvas.h>
#include <qgsgeometry.h>
#include<qgstextannotation.h>
#include <qgsmapcanvasannotationitem.h>


// 构造函数
PointEdit::PointEdit(QgsMapCanvas* canvas)
    : QgsMapToolEdit(canvas), mVectorLayer(nullptr) ,mPointType("地图点")// 默认类型为地图点，图层为空
{
}
PointEdit::PointEdit(QgsMapCanvas* canvas, QgsVectorLayer* layer)
	: QgsMapToolEdit(canvas), mPointType(u8"地图点"), mVectorLayer(layer) // 默认类型为地图点，设置图层
{
}

// 析构函数
PointEdit::~PointEdit()
{
}

// 设置外部传入的矢量图层
void PointEdit::setVectorLayer(QgsVectorLayer* layer)
{
    if (layer && layer->geometryType() == Qgis::GeometryType::Point) {
        mVectorLayer = layer; // 仅允许设置点几何类型的矢量图层
    }
    else {
        QMessageBox::warning(nullptr, "错误", "传入的图层类型不支持或为空！");
    }
}

// 鼠标按下事件
void PointEdit::canvasPressEvent(QgsMapMouseEvent* e)
{
     showAddPointDialog(); // 弹出对话框让用户选择点类型
     addPoint(e);          // 执行添加操作
}
// 显示添加点类型选择对话框
void PointEdit::showAddPointDialog()
{
    QDialog dialog;
    dialog.setWindowTitle("选择点类型");
    dialog.setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QComboBox* comboBox = new QComboBox();
    comboBox->addItem("地图点");
    comboBox->addItem("注记");

    QPushButton* confirmButton = new QPushButton("确定");

    layout->addWidget(new QLabel("请选择点的类型："));
    layout->addWidget(comboBox);
    layout->addWidget(confirmButton);

    connect(confirmButton, &QPushButton::clicked, [&]() {
        mPointType = comboBox->currentText(); // 获取选择的点类型
        dialog.accept();
        });

    dialog.exec();
}
// 添加点操作
void PointEdit::addPoint(QgsMapMouseEvent* e)
{
    if (!mCanvas) {
        QMessageBox::warning(nullptr, "错误", "画布未加载，无法添加点！");
        return;
    }

    if (!mVectorLayer) {
        QMessageBox::warning(nullptr, "错误", "未设置矢量图层！");
        return;
    }

    QgsPointXY position = toMapCoordinates(e->pos());

    if (mPointType == "地图点") {
        // 创建点要素
        QgsFeature feature;
        feature.setGeometry(QgsGeometry::fromPointXY(position));

        // 确保属性字段匹配图层字段
        QgsAttributes attributes(mVectorLayer->fields().size()); // 假设图层有字段
        attributes[0] = mPointType;  // 假设字段第一个存储类型
        feature.setAttributes(attributes);
		mVectorLayer->addFeature(feature);
    }

    else if (mPointType == "注记") {
        addAnnotation(position);
    }
}
// 解析注记文本，处理 ==、++、-- 规则
QString parseAnnotationText(const QString& rawText)
{
    QString processedText = rawText;

    // 替换正常字符：==后内容
    QRegExp normalRegex("==([^+\\-]*)");
    processedText.replace(normalRegex, R"(\1)");

    // 替换上标：++后内容
    QRegExp supRegex("\\+\\+([^+\\-]*)");
    processedText.replace(supRegex, R"(<sup>\1</sup>)");

    // 替换下标：--后内容
    QRegExp subRegex("--([^+\\-]*)");
    processedText.replace(subRegex, R"(<sub>\1</sub>)");

    return processedText;
}
// 添加注记
void PointEdit::addAnnotation(const QgsPointXY& position)
{
    // 创建注记对象
    QgsTextAnnotation* annotation = new QgsTextAnnotation(mCanvas);

    // 设置注记位置
    annotation->setMapPosition(position);

    // 获取用户输入的注记文本
    QString rawAnnotationText = QInputDialog::getText(nullptr, u8"输入注记文本", u8"请输入注记内容（格式：==内容++内容--内容）：");
    if (rawAnnotationText.isEmpty()) {
        delete annotation;
        QMessageBox::warning(nullptr,"警告","注记文本不能为空！");
        return;
    }
    // 解析文本，处理 ==、++、-- 规则
    QString formattedText = parseAnnotationText(rawAnnotationText);
    // 创建 HTML 格式文档并设置内容
    QTextDocument* document = new QTextDocument();
    document->setHtml(formattedText);
    // 将解析后的文档设置到注记
    annotation->setDocument(document);
    // 创建注记画布项
    QgsMapCanvasAnnotationItem* annotationItem = new QgsMapCanvasAnnotationItem(annotation, mCanvas);
    // 添加到画布上
    mCanvas->scene()->addItem(annotationItem);
    // 发出注记添加信号
    //emit annotationAdded(position);

    QMessageBox::information(nullptr, u8"成功", u8"注记添加成功！");
}
