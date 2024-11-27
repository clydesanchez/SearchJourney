#pragma once

#include <QObject>
#include <QUndoStack>
#include <qgsmapcanvas.h>
#include <qgsvectorlayer.h>
#include <qgsfeature.h>
#include <QUndoCommand>
#include <qgstextannotation.h>


// 编辑工具类，支持点、线、面几何体的添加、删除和更新操作，同时提供撤销和重做功能
class GeometryEditTool : public QObject {
    Q_OBJECT

public:
    // 构造函数：接收 QGIS 的地图画布作为参数
    explicit GeometryEditTool(QgsMapCanvas* canvas);

    // 析构函数：释放命令栈内存
    ~GeometryEditTool();

    // 设置当前编辑的矢量图层
    void setVectorLayer(QgsVectorLayer* layer);

    // 添加几何要素
    void addGeometry(const QgsFeature& feature);

    // 删除几何要素
    void deleteGeometry(const QgsFeature& feature);

    // 更新几何要素
    void updateGeometry(const QgsFeature& oldFeature, const QgsFeature& newFeature);

    // 撤销操作
    void undo();

    // 重做操作
    void redo();

private:
    QUndoStack* mUndoStack;       // 命令栈，用于存储编辑命令
    QgsVectorLayer* mVectorLayer; // 当前编辑的矢量图层
    QgsMapCanvas* mCanvas;        // 地图画布，用于触发图层刷新
};
