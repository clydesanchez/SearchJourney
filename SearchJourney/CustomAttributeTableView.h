#pragma once

#include <QgsAttributeTableView.h>
#include <qvariant.h>
#include <QMouseEvent>
#include <qtableview>
#include <QgsAttributeTableModel.h>
#include <QMouseEvent>
#include <QModelIndex>
#include <QString>
class CustomAttributeTableView  : public QgsAttributeTableView
{
	Q_OBJECT
private:
	QModelIndex mClickedIndex;

	bool mEditMode = false;
public:
	CustomAttributeTableView(QWidget* parent = nullptr);
	~CustomAttributeTableView();
public:
	void showLineEdit(QModelIndex index);
	void setEditMode(bool editMode);
signals:
	void inputData(QString val);
	void idxSender(QModelIndex pos);
private:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	//void finished(const QVariant &value);
};
