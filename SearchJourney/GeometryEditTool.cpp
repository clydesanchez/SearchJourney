#include "GeometryEditTool.h"
#include <QMessageBox>

// �����ࣺ�����༭����
#include <QUndoCommand>

#include <QUndoCommand>
#include <qgsvectorlayer.h>
#include <qgsfeature.h>

// ��Ӽ������������
class AddGeometryCommand : public QUndoCommand {
public:
    AddGeometryCommand(QgsVectorLayer* layer, const QgsFeature& feature)
        : mLayer(layer), mFeature(feature) {}

    void undo() override {
        // ����༭ģʽ��ɾ���ո���ӵļ�����
        mLayer->startEditing();
        if (!mLayer->deleteFeature(mFeature.id())) {
            QMessageBox::warning(nullptr, "����", "����ʧ�ܣ��޷�ɾ��Ҫ�أ�");
        }
        qDebug() << "Redo adding feature with ID:" << mFeature.id();
        mLayer->commitChanges();  // �ύ����
        mLayer->triggerRepaint(); // ˢ��ͼ��
    }

    void redo() override {
        // ����༭ģʽ��������Ӽ�����
        mLayer->startEditing();
        if (!mLayer->addFeature(mFeature)) {
            QMessageBox::warning(nullptr, "����", "����ʧ�ܣ��޷����Ҫ�أ�");
        }
        mLayer->commitChanges();  // �ύ����
        mLayer->triggerRepaint(); // ˢ��ͼ��
    }

private:
    QgsVectorLayer* mLayer; // ʸ��ͼ��
    QgsFeature mFeature;    // Ҫ��ӵļ�����
};


// ɾ���������������
class DeleteGeometryCommand : public QUndoCommand {
public:
    DeleteGeometryCommand(QgsVectorLayer* layer, const QgsFeature& feature)
        : mLayer(layer), mFeature(feature) {}

    // ����ɾ����������ӱ�ɾ���ļ�����
    void undo() override {
        mLayer->startEditing();                       // ����༭ģʽ
        mLayer->addFeature(mFeature);                 // �������Ҫ��
        mLayer->commitChanges();                      // �ύ����
        mLayer->triggerRepaint();                     // ˢ��ͼ��
    }

    // ����ɾ����ɾ��������
    void redo() override {
        mLayer->startEditing();                       // ����༭ģʽ
        mLayer->deleteFeature(mFeature.id());         // ɾ��Ҫ��
        mLayer->commitChanges();                      // �ύ����
        mLayer->triggerRepaint();                     // ˢ��ͼ��
    }

private:
    QgsVectorLayer* mLayer;    // ʸ��ͼ��
    QgsFeature mFeature;       // Ҫɾ���ļ�����
};

// ���¼������������
class UpdateGeometryCommand : public QUndoCommand {
public:
    UpdateGeometryCommand(QgsVectorLayer* layer, const QgsFeature& oldFeature, const QgsFeature& newFeature)
        : mLayer(layer), mOldFeature(oldFeature), mNewFeature(newFeature) {}

    // �������£���������ָ�Ϊ��״̬
    void undo() override {
        mLayer->startEditing();                       // ����༭ģʽ
        mLayer->updateFeature(mOldFeature);           // �ָ��ɼ���
        mLayer->commitChanges();                      // �ύ����
        mLayer->triggerRepaint();                     // ˢ��ͼ��
    }

    // �������£������������Ϊ��״̬
    void redo() override {
        mLayer->startEditing();                       // ����༭ģʽ
        mLayer->updateFeature(mNewFeature);           // ����Ϊ�¼���
        mLayer->commitChanges();                      // �ύ����
        mLayer->triggerRepaint();                     // ˢ��ͼ��
    }

private:
    QgsVectorLayer* mLayer;    // ʸ��ͼ��
    QgsFeature mOldFeature;    // �ɼ�����
    QgsFeature mNewFeature;    // �¼�����
};

// ���캯������ʼ������ջ�ͳ�Ա����
GeometryEditTool::GeometryEditTool(QgsMapCanvas* canvas)
    : mCanvas(canvas), mVectorLayer(nullptr) {
    mUndoStack = new QUndoStack(this); // ��������ջ
}

// �����������ͷ�����ջ�ڴ�
GeometryEditTool::~GeometryEditTool() {
    delete mUndoStack; // ɾ������ջ
}
// ���õ�ǰ�༭��ʸ��ͼ��
void GeometryEditTool::setVectorLayer(QgsVectorLayer* layer) {
    mVectorLayer = layer;
}
// ��Ӽ��������
void GeometryEditTool::addGeometry(const QgsFeature& feature) {
    if (!mVectorLayer) return;

    // ������Ӽ������������������ջ
    AddGeometryCommand* cmd = new AddGeometryCommand(mVectorLayer, feature);
    mUndoStack->push(cmd); // ��ӵ�����ջ
}

// ɾ�����������
void GeometryEditTool::deleteGeometry(const QgsFeature& feature) {
    if (!mVectorLayer) return;

    // ����ɾ���������������������ջ
    DeleteGeometryCommand* cmd = new DeleteGeometryCommand(mVectorLayer, feature);
    mUndoStack->push(cmd); // ��ӵ�����ջ
}
// ���¼��������
void GeometryEditTool::updateGeometry(const QgsFeature& oldFeature, const QgsFeature& newFeature) {
    if (!mVectorLayer) return;

    // �������¼������������������ջ
    UpdateGeometryCommand* cmd = new UpdateGeometryCommand(mVectorLayer, oldFeature, newFeature);
    mUndoStack->push(cmd); // ��ӵ�����ջ
}
// ������һ������
void GeometryEditTool::undo() {
    if (mUndoStack->canUndo()) { // ����Ƿ��п��Գ����Ĳ���
        mUndoStack->undo();
        mCanvas->refresh(); // ˢ�µ�ͼ����
    }
}
// ������������Ĳ���
void GeometryEditTool::redo() {
    if (mUndoStack->canRedo()) { // ����Ƿ��п��������Ĳ���
        mUndoStack->redo();
        mCanvas->refresh(); // ˢ�µ�ͼ����
    }
}
