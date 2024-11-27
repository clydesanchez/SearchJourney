#pragma once

#include <QObject>
#include <QUndoStack>
#include <qgsmapcanvas.h>
#include <qgsvectorlayer.h>
#include <qgsfeature.h>
#include <QUndoCommand>
#include <qgstextannotation.h>


// �༭�����֧࣬�ֵ㡢�ߡ��漸�������ӡ�ɾ���͸��²�����ͬʱ�ṩ��������������
class GeometryEditTool : public QObject {
    Q_OBJECT

public:
    // ���캯�������� QGIS �ĵ�ͼ������Ϊ����
    explicit GeometryEditTool(QgsMapCanvas* canvas);

    // �����������ͷ�����ջ�ڴ�
    ~GeometryEditTool();

    // ���õ�ǰ�༭��ʸ��ͼ��
    void setVectorLayer(QgsVectorLayer* layer);

    // ��Ӽ���Ҫ��
    void addGeometry(const QgsFeature& feature);

    // ɾ������Ҫ��
    void deleteGeometry(const QgsFeature& feature);

    // ���¼���Ҫ��
    void updateGeometry(const QgsFeature& oldFeature, const QgsFeature& newFeature);

    // ��������
    void undo();

    // ��������
    void redo();

private:
    QUndoStack* mUndoStack;       // ����ջ�����ڴ洢�༭����
    QgsVectorLayer* mVectorLayer; // ��ǰ�༭��ʸ��ͼ��
    QgsMapCanvas* mCanvas;        // ��ͼ���������ڴ���ͼ��ˢ��
};
