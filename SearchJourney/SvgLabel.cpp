#include "SvgLabel.h"
#include <QLabel>
SvgLabel::SvgLabel(QString Path, QWidget *parent)
	: QLabel(parent), mstrPath(Path)
{

}

SvgLabel::~SvgLabel()
{}

void SvgLabel::mousePressEvent(QMouseEvent *event)
{
	// 发送文件路径
	emit signalClicked(mstrPath);
	QLabel::mousePressEvent(event);
}