/*
FileName:SvgLabel.h
Date:2024-11-27
Description:SvgLabel类的声明,用于显示svg图片与交互
 */
#ifndef SVGLABEL_H
#define SVGLABEL_H
#include <QLabel>
#include <QString>
class SvgLabel  : public QLabel
{
	Q_OBJECT

public:
	SvgLabel(QString Path, QWidget*parent=nullptr);
	~SvgLabel();
public:
	void mousePressEvent(QMouseEvent *event);
private:
	QString mstrPath;
signals:
	void signalClicked(QString filePath);
};
#endif