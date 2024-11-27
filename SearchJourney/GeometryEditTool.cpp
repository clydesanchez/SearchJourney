#include "GeometryEditTool.h"
#include <QMessageBox>

// 命令类：基础编辑命令
#include <QUndoCommand>

#include <QUndoCommand>
#include <qgsvectorlayer.h>
#include <qgsfeature.h>

// 添加几何体的命令类
class AddGeometryCommand : public QUndoCommand {
public:
    AddGeometryCommand(QgsVectorLayer* layer, const QgsFeature& feature)
        : mLayer(layer), mFeature(feature) {}

    void undo() override {
        // 开启编辑模式，删除刚刚添加的几何体
        mLayer->startEditing();
        if (!mLayer->deleteFeature(mFeature.id())) {
            QMessageBox::warning(nullptr, "错误", "撤销失败，无法删除要素！");
        }
        qDebug() << "Redo adding feature with ID:" << mFeature.id();
        mLayer->commitChanges();  // 提交更改
        mLayer->triggerRepaint(); // 刷新图层
    }

    void redo() override {
        // 开启编辑模式，重新添加几何体
        mLayer->startEditing();
        if (!mLayer->addFeature(mFeature)) {
            QMessageBox::warning(nullptr, "错误", "重做失败，无法添加要素！");
        }
        mLayer->commitChanges();  // 提交更改
        mLayer->triggerRepaint(); // 刷新图层
    }

private:
    QgsVectorLayer* mLayer; // 矢量图层
    QgsFeature mFeature;    // 要添加的几何体
};


// 删除几何体的命令类
class DeleteGeometryCommand : public QUndoCommand {
public:
    DeleteGeometryCommand(QgsVectorLayer* layer, const QgsFeature& feature)
        : mLayer(layer), mFeature(feature) {}

    // 撤销删除：重新添加被删除的几何体
    void undo() override {
        mLayer->startEditing();                       // 开启编辑模式
        mLayer->addFeature(mFeature);                 // 重新添加要素
        mLayer->commitChanges();                      // 提交更改
        mLayer->triggerRepaint();                     // 刷新图层
    }

    // 重做删除：删除几何体
    void redo() override {
        mLayer->startEditing();                       // 开启编辑模式
        mLayer->deleteFeature(mFeature.id());         // 删除要素
        mLayer->commitChanges();                      // 提交更改
        mLayer->triggerRepaint();                     // 刷新图层
    }

private:
    QgsVectorLayer* mLayer;    // 矢量图层
    QgsFeature mFeature;       // 要删除的几何体
};

// 更新几何体的命令类
class UpdateGeometryCommand : public QUndoCommand {
public:
    UpdateGeometryCommand(QgsVectorLayer* layer, const QgsFeature& oldFeature, const QgsFeature& newFeature)
        : mLayer(layer), mOldFeature(oldFeature), mNewFeature(newFeature) {}

    // 撤销更新：将几何体恢复为旧状态
    void undo() override {
        mLayer->startEditing();                       // 开启编辑模式
        mLayer->updateFeature(mOldFeature);           // 恢复旧几何
        mLayer->commitChanges();                      // 提交更改
        mLayer->triggerRepaint();                     // 刷新图层
    }

    // 重做更新：将几何体更新为新状态
    void redo() override {
        mLayer->startEditing();                       // 开启编辑模式
        mLayer->updateFeature(mNewFeature);           // 更新为新几何
        mLayer->commitChanges();                      // 提交更改
        mLayer->triggerRepaint();                     // 刷新图层
    }

private:
    QgsVectorLayer* mLayer;    // 矢量图层
    QgsFeature mOldFeature;    // 旧几何体
    QgsFeature mNewFeature;    // 新几何体
};

// 构造函数：初始化命令栈和成员变量
GeometryEditTool::GeometryEditTool(QgsMapCanvas* canvas)
    : mCanvas(canvas), mVectorLayer(nullptr) {
    mUndoStack = new QUndoStack(this); // 创建命令栈
}

// 析构函数：释放命令栈内存
GeometryEditTool::~GeometryEditTool() {
    delete mUndoStack; // 删除命令栈
}
// 设置当前编辑的矢量图层
void GeometryEditTool::setVectorLayer(QgsVectorLayer* layer) {
    mVectorLayer = layer;
}
// 添加几何体操作
void GeometryEditTool::addGeometry(const QgsFeature& feature) {
    if (!mVectorLayer) return;

    // 创建添加几何体命令，并推入命令栈
    AddGeometryCommand* cmd = new AddGeometryCommand(mVectorLayer, feature);
    mUndoStack->push(cmd); // 添加到撤销栈
}

// 删除几何体操作
void GeometryEditTool::deleteGeometry(const QgsFeature& feature) {
    if (!mVectorLayer) return;

    // 创建删除几何体命令，并推入命令栈
    DeleteGeometryCommand* cmd = new DeleteGeometryCommand(mVectorLayer, feature);
    mUndoStack->push(cmd); // 添加到撤销栈
}
// 更新几何体操作
void GeometryEditTool::updateGeometry(const QgsFeature& oldFeature, const QgsFeature& newFeature) {
    if (!mVectorLayer) return;

    // 创建更新几何体命令，并推入命令栈
    UpdateGeometryCommand* cmd = new UpdateGeometryCommand(mVectorLayer, oldFeature, newFeature);
    mUndoStack->push(cmd); // 添加到撤销栈
}
// 撤销上一步操作
void GeometryEditTool::undo() {
    if (mUndoStack->canUndo()) { // 检查是否有可以撤销的操作
        mUndoStack->undo();
        mCanvas->refresh(); // 刷新地图画布
    }
}
// 重做最近撤销的操作
void GeometryEditTool::redo() {
    if (mUndoStack->canRedo()) { // 检查是否有可以重做的操作
        mUndoStack->redo();
        mCanvas->refresh(); // 刷新地图画布
    }
}
