#include "CustomAttributeTableView.h"
#include <QAbstractItemView>
#include <QlineEdit>
#include <QgsAttributeTableModel.h>
#include <QMessageBox>
#include <QHeaderView>
//#include "CustomAttributeFliterModel.h"
CustomAttributeTableView::CustomAttributeTableView(QWidget* parent)
	: QgsAttributeTableView(parent)
{
	this->setEditTriggers(QAbstractItemView::DoubleClicked);
}
CustomAttributeTableView::~CustomAttributeTableView()
{}

void CustomAttributeTableView::mousePressEvent(QMouseEvent* event)
{	
	mClickedIndex = indexAt(event->pos());
	QModelIndex index = mClickedIndex;
	emit idxSender(index);
	QgsAttributeTableView::mousePressEvent(event);
}
void CustomAttributeTableView::mouseDoubleClickEvent(QMouseEvent* event)
{
	//双击单元格，在单元格中显示编辑框
	if (event->buttons() == Qt::LeftButton)
	{
		mClickedIndex = indexAt(event->pos());
		QModelIndex index = mClickedIndex;
		if (index.isValid()&& mEditMode == true)
		{
			emit idxSender(index);
			showLineEdit(index);
		}
		else {
			// 返回一个不可用的index
			QModelIndex Unableindex;
			emit idxSender(Unableindex);
		}
	}
	//QgsAttributeTableView::mouseDoubleClickEvent(event);
}

//void CustomAttributeTableView::finished(const QVariant &value)
//{
//	QgsAttributeTableView::finished();
//}

void CustomAttributeTableView::showLineEdit(QModelIndex index) {
	// 获取点击单元格的矩形区域
	QRect rect = visualRect(index);

	//将rect转换为相对于视图的坐标
	rect = rect.translated(this->pos());
	//修正表头的高度和列头的宽度
	rect.setTop(rect.top() - this->horizontalHeader()->height() -18);
	rect.setBottom(rect.bottom() - this->horizontalHeader()->height()-18 );

	rect.setLeft(rect.left() + this->verticalHeader()->width() -10);
	rect.setRight(rect.right() + this->verticalHeader()->width() -10);
	QLineEdit* lineEdit = new QLineEdit(this);
	QString srcValue = index.data().toString();
	lineEdit->setGeometry(rect);
	lineEdit->setText(srcValue);
	lineEdit->setFocus();
	lineEdit->show();
	connect(lineEdit, &QLineEdit::editingFinished, this,[this, lineEdit, index,srcValue]() {
		// 避免失去焦点时触发多次信号
		lineEdit->blockSignals(true);
		// 弹出确认框
		QMessageBox::StandardButton ret;
		ret = QMessageBox::question(this, "确认", "是否确认修改数据？", QMessageBox::Yes | QMessageBox::No);
		// 如果输入框的值和原值相同则不写入数据
		if(lineEdit->text() == srcValue||ret==QMessageBox::No)
		{
			lineEdit->deleteLater();
			return;
		}
		// 确认框如果点击了取消则不写入数据
		qDebug() << "确认修改数据";
		QString str = lineEdit->text();
		//CustomAttributeFliterModel* filterModel = dynamic_cast<CustomAttributeFliterModel*>(this->model());
		//filterModel->sourceModel()->setData(index, str, Qt::EditRole);
		emit inputData(str);
		lineEdit->deleteLater();
		this->update();
		});
}

void CustomAttributeTableView::setEditMode(bool editMode)
{
	mEditMode = editMode;
}