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

Q_DECLARE_METATYPE(QgsColorRamp*)

class RasterStyle : public QDockWidget {
    Q_OBJECT

public:
    RasterStyle(QString strLayerName, QgsRasterLayer* rasLayer, MainWidget* widMain, QString rampPath,QWidget* parent = nullptr);
    ~RasterStyle();

public slots:
    // 点击“Apply”时应用样式
    void onActionApplyStyle_ras();           
    // 获取用户更改
    void getSelectedBand(int rasBand);             
    void getSelectedColorRamp(QgsColorRamp* ramp); 
    void getMinValue(double rasMinValue);          
    void getMaxValue(double rasMaxValue);          
    void getNumClasses(int rasNumClasses);      
    // 窗口样式创建
    void createRasterSymbolDock(MainWidget* widMain);                 

private:
    Ui::RasterStyleClass ui;
    QgsRasterLayer* mRasLayer;       
    QString mstrRampPath;
    int mrasBand = 1;                
    QgsColorRamp* mrasColorRamp = nullptr; 
    double mrasMinValue = -1300.0;      
    double mrasMaxValue = 1821.0;        
    int mrasNumClasses = 6;            
    int mInterpType = 0;             
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
                gradient.setColorAt(0, ramp->color(0));  
                gradient.setColorAt(1, ramp->color(1));  

                painter->fillRect(option.rect, gradient);
            }
        }

        // 绘制色带名称
        painter->setPen(Qt::black);
        painter->drawText(option.rect, Qt::AlignLeft | Qt::AlignVCenter, rampName);

        painter->restore();
    }
};
