/**
 *     Yet Another Sign Up - A new sign up system for class managements
 *     Copyright (C) 2025  知念夏世 <chart11from21@outlook.com>

 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.

 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.

 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "adminpanel.h"
#include "ui_adminpanel.h"
#include "exportdialog.h"
#include "deadlinedialog.h"
#include "rewardlyexportdialog.h"
#include "rewardlyconfigdialog.h"

AdminPanel::AdminPanel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AdminPanel)
{
    ui->setupUi(this);

    connect(ui->ButtonClose, &QPushButton::released, this, &AdminPanel::close);
    connect(ui->ButtonCheckStat, &QPushButton::released, this, &AdminPanel::onArbiterClicked);

    // Add "导出数据" button programmatically below the existing stat button
    buttonExportData = new QPushButton(QString("导出数据"), this);
    buttonExportData->setGeometry(40, 70, 130, 28);
    connect(buttonExportData, &QPushButton::released,
            this, &AdminPanel::onExportClicked);

    // Add "设置迟到时间" button below the export button
    buttonSetDeadline = new QPushButton(QString("设置迟到时间"), this);
    buttonSetDeadline->setGeometry(40, 110, 130, 28);
    connect(buttonSetDeadline, &QPushButton::released,
            this, &AdminPanel::onDeadlineClicked);

    // Add "导出到 Rewardly" button below the deadline button
    buttonRewardlyExport = new QPushButton(QString("导出到 Rewardly"), this);
    buttonRewardlyExport->setGeometry(40, 150, 150, 28);
    connect(buttonRewardlyExport, &QPushButton::released,
            this, &AdminPanel::onRewardlyExportClicked);

    // Add "配置加分项" button below the Rewardly export button
    buttonRewardlyConfig = new QPushButton(QString("配置加分项"), this);
    buttonRewardlyConfig->setGeometry(40, 190, 150, 28);
    connect(buttonRewardlyConfig, &QPushButton::released,
            this, &AdminPanel::onRewardlyConfigClicked);
}


void AdminPanel::onArbiterClicked(){
    arbiter *ab = new arbiter;
    ab->setAttribute(Qt::WA_DeleteOnClose);
    ab->setWindowFlag(Qt::WindowStaysOnTopHint);
    ab->show();
}

void AdminPanel::onExportClicked()
{
    ExportDialog *dlg = new ExportDialog;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowFlag(Qt::WindowStaysOnTopHint);
    dlg->show();
}

void AdminPanel::onDeadlineClicked()
{
    DeadlineDialog *dlg = new DeadlineDialog;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowFlag(Qt::WindowStaysOnTopHint);
    dlg->show();
}

void AdminPanel::onRewardlyExportClicked()
{
    RewardlyExportDialog *dlg = new RewardlyExportDialog;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowFlag(Qt::WindowStaysOnTopHint);
    dlg->show();
}

void AdminPanel::onRewardlyConfigClicked()
{
    RewardlyConfigDialog *dlg = new RewardlyConfigDialog;
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowFlag(Qt::WindowStaysOnTopHint);
    dlg->show();
}


AdminPanel::~AdminPanel()
{
    delete ui;
}
