#ifndef POINTEDIT_H
#define POINTEDIT_H

#include <QgsMapToolEdit.h>
#include <QgsPointXY.h>
#include <QgsVectorLayer.h>
#include <qstring.h>
#include <QUndoStack>

class PointEdit : public QgsMapToolEdit
{
	Q_OBJECT
public:
	PointEdit(QgsMapCanvas* canvas);
	PointEdit(QgsMapCanvas* canvas, QgsVectorLayer* layer);
	~PointEdit();
	void addPoint(QgsMapMouseEvent* e);					// ��ӵ㹦�ܣ���ͼ���ע�ǣ�
	void addAnnotation(const QgsPointXY& position);		// ���ע��
	void setVectorLayer(QgsVectorLayer* layer);	// �����ⲿ�����ʸ��ͼ��

private:
	QString mPointType;									// ������ͣ���ͼ�㡢ע�ǵȣ�
	void showAddPointDialog();							// ��ʾ��ӵ�����ѡ��Ի���
	QgsVectorLayer* mVectorLayer;						// �ⲿ�����ʸ��ͼ��

protected:
	void canvasPressEvent(QgsMapMouseEvent* e) override;	// ��갴���¼�

signals:
	void pointAdded(QgsPointXY, QString);				// �����źţ���λ�ú�����
};

#endif // POINTEDIT-H

