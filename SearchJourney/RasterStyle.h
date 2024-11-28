#pragma once

#include "ui_RasterStyle.h"
#include "MainWidget.h"
#include <QTableWidget>
#include <QMainWindow>
#include <QWidget>
#include <qgscolorbutton.h>
#include <qgscolordialog.h>

#include <qgsrasterlayer.h>
#include <qgscolorramp.h>

#include <QMetaType>
#include <qgscolorramp.h>
#include <QColor>
#include <QList>
#include <QDockWidget>

// 声明 QgsColorRamp* 类型
Q_DECLARE_METATYPE(QgsColorRamp*)

class RasterStyle : public QDockWidget {
    Q_OBJECT

public:
    RasterStyle(QString strLayerName, QgsRasterLayer* rasLayer, MainWidget* widMain, QWidget* parent = nullptr);
    ~RasterStyle();

public slots:
    void onActionApplyStyle_ras();                 // 点击“Apply”时应用样式
    void getSelectedBand(int rasBand);             // 获取用户选择的波段
    void getSelectedColorRamp(QgsColorRamp* ramp); // 获取用户选择的色带
    void getMinValue(double rasMinValue);          // 获取分层设色的最小值
    void getMaxValue(double rasMaxValue);          // 获取分层设色的最大值
    void getNumClasses(int rasNumClasses);         // 获取分层设色的分级数

    void createRasterSymbolDock(MainWidget* widMain);                 // 样式设置的 Dock 创建逻辑

private:
    Ui::RasterStyleClass ui;
    QgsRasterLayer* mRasLayer;        // 当前栅格图层
    int mrasBand = 1;                 // 当前选择的波段
    QgsColorRamp* mrasColorRamp = nullptr; // 当前选择的色带
    double mrasMinValue = -9764.0;        // 分层设色的最小值
    double mrasMaxValue = 1821.0;        // 分层设色的最大值
    int mrasNumClasses = 6;             // 分层设色的分级数
    int mInterpType = 0;               // 当前选择的插值类型
    int mModType = 0;                

};

#include <QStyledItemDelegate>
#include <QPainter>

class ColorRampDelegate : public QStyledItemDelegate {
public:
    explicit ColorRampDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        // 获取色带名称
        QString rampName = index.data(Qt::DisplayRole).toString();
        QVariant colorRampVariant = index.data(Qt::UserRole);

        // 绘制背景
        painter->save();

        if (colorRampVariant.isValid()) {
            QgsColorRamp* ramp = colorRampVariant.value<QgsColorRamp*>();
            if (ramp) {
                // 渐变色带的绘制
                QLinearGradient gradient(option.rect.left(), option.rect.top(), option.rect.right(), option.rect.top());
                gradient.setColorAt(0, ramp->color(0));  // 渐变的起始颜色
                gradient.setColorAt(1, ramp->color(1));  // 渐变的结束颜色

                painter->fillRect(option.rect, gradient);
            }
        }

        // 绘制色带名称
        painter->setPen(Qt::black);
        painter->drawText(option.rect, Qt::AlignLeft | Qt::AlignVCenter, rampName);

        painter->restore();
    }
};
