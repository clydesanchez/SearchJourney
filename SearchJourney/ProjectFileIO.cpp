/*
FileName: ProjectFileIO.cpp
Author: RYB
Date: 2024-7-28
Description:
    项目文件读写功能、分析结果保存功能
Function Lists:
    1. saveProjectAsQGZ() 保存项目为QGZ
    2. openProjectFromQGZ() 从QGZ打开项目
    3. saveAsSHP() 保存为SHP
    4. saveAsTxt() 保存为TXT
*/

#include "MainWidget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <Qgsproject.h>
#include <QgsVectorLayer.h>
#include <QgsVectorFileWriter.h>
#include <QFile>
#include <QStringListModel>
#include <QInputDialog>
#include <QgsCoordinateReferenceSystem.h>
#include "SelectDialog.h"
void MainWidget::saveProjectAsQGZ()
{
    // 打开文件对话框以选择保存位置
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存项目为"), "", tr("QGIS 项目文件 (*.qgz)"));

    // 如果用户取消了文件选择，返回
    if (fileName.isEmpty())
    {
        return;
    }

    // 确保文件名以 .qgz 结尾
    if (!fileName.endsWith(".qgz"))
    {
        fileName += ".qgz";
    }

    // 获取当前项目实例
    QgsProject *project = QgsProject::instance();
    // 将当前图层添加到项目中
    for (QgsMapLayer *layer : mliLayersList)
    {
        project->addMapLayer(layer);
    }
    // 尝试保存项目
    if (project->write(fileName))
    {
        QMessageBox::information(this, tr("成功"), tr("项目已成功保存到 %1").arg(fileName));
    }
    else
    {
        QMessageBox::critical(this, tr("错误"), tr("无法保存项目"));
    }
}

void MainWidget::openProjectFromQGZ()
{
    // 提示用户当前项目将被关闭
    if (QMessageBox::question(this, tr("警告"), tr("打开项目将关闭当前项目，是否继续？"), 
    QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
    {
        return;
    }
    // 清空当前项目
    mliLayersList.clear();
    mliVisibleLayers.clear();
    // 打开文件对话框以选择项目文件
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开项目文件"), "", tr("QGIS 项目文件 (*.qgz *.qgs)"));

    // 如果用户取消了文件选择，返回
    if (fileName.isEmpty())
    {
        return;
    }

    // 获取当前项目实例
    QgsProject *project = QgsProject::instance();
    // 尝试加载项目
    if (project->read(fileName))
    {
        // 获取项目中的图层
        QgsMapLayer *layer = nullptr;
        QStringList layerNames = project->mapLayers().keys();
        for (const QString &layerName : layerNames)
        {
            layer = project->mapLayers().value(layerName);
            if (layer)
            {
                mliLayersList.append(layer);
            }
            else
            {
                QMessageBox::critical(this, tr("错误"), tr("无法加载图层"));
            }
        }
        mppjProject = project;
        mcanMapCanvas->setLayers(mliLayersList);
        mcanMapCanvas->refresh();
        QMessageBox::information(this, tr("成功"), tr("项目已成功加载"));
    }
    else
    {
        QMessageBox::critical(this, tr("错误"), tr("无法加载项目"));
    }
}

void MainWidget::saveAsSHP()
{
    // 选择窗口
    FileSaveSelect *ctrlChooseWidget = new FileSaveSelect();
    ctrlChooseWidget->setMapLayers(mliLayersList);
    ctrlChooseWidget->exec();
    QgsVectorLayer *pqmlLayer = ctrlChooseWidget->getSelectLayer();
    QString qstrFileName = ctrlChooseWidget->getFilePath();
    if (!pqmlLayer)
    {
        QMessageBox::critical(this, tr("错误"), tr("请选择图层"));
        return;
    }

    QgsVectorLayer *pqvlLayer = dynamic_cast<QgsVectorLayer *>(pqmlLayer);
    if (!pqvlLayer)
    {
        QMessageBox::critical(this, tr("错误"), tr("无法获取矢量图层"));
        return;
    }

    // 保存图层
    QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormat(
        pqvlLayer,
        qstrFileName,
        "UTF-8",
        pqvlLayer->crs(),
        "ESRI Shapefile");
    if (error == QgsVectorFileWriter::NoError)
    {
        QMessageBox::information(this, tr("成功"), tr("图层已成功保存到 %1").arg(qstrFileName));
    }
    else
    {
        QMessageBox::critical(this, tr("错误"), tr("保存图层失败"));
    }
}
void MainWidget::saveAsTxt()
{
    // 将ui.ctrlStatisticsView中的数据保存到txt文件
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存统计结果为"), "", tr("文本文件 (*.txt)"));
    QFile fileTarget(fileName);
    if (!fileTarget.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, tr("错误"), tr("无法打开文件"));
        return;
    }
    // 获取模型
    QStringListModel *pslmDataModel = (QStringListModel *)ui.ctrlStatisticsView->model();
    if (pslmDataModel == nullptr)
    {
        QMessageBox::critical(this, tr("错误"), tr("统计数据为空"));
        return;
    }
    // 提取数据
    QStringList qstrData = pslmDataModel->stringList();
    QTextStream outWirter(&fileTarget);
    for (const QString &line : qstrData)
    {
        outWirter << line << "\n";
    }
    fileTarget.close();
    QMessageBox::information(this, tr("成功"), tr("统计结果已成功保存到 %1").arg(fileName));
}

void MainWidget::on_ctrlCRSAction_triggered() {
    // 获取当前工程坐标系
    QgsCoordinateReferenceSystem srcCRS = QgsProject::instance()->crs();
    int srcCRScode = srcCRS.postgisSrid();
    qDebug()  << srcCRScode;
    bool* confirm = false;
    int newCRScode = QInputDialog::getInt(this, "设置坐标系", "输入坐标系编号", srcCRScode,0,99999999,1,confirm);
    if(&confirm==false) return;
    // 设置工程文件坐标系
    QgsProject::instance()->setCrs(QgsCoordinateReferenceSystem(newCRScode));
    // 保存
    QgsProject::instance()->write();
    QMessageBox::information(this, tr("成功"), tr("坐标系已成功设置为 %1").arg(newCRScode));
}