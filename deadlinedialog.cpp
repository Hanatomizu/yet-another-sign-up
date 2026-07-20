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

#include "deadlinedialog.h"
#include "configmanager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

DeadlineDialog::DeadlineDialog(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QString("设置迟到时间"));
    setFixedSize(380, 320);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title label
    auto *titleLabel = new QLabel(QString("配置各时段签到截止时间与分界时间"), this);
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);

    // Form layout for time edits
    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    morningDeadlineEdit = new QTimeEdit(this);
    morningDeadlineEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("早上签到截止:"), morningDeadlineEdit);

    morningNoonSplitEdit = new QTimeEdit(this);
    morningNoonSplitEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("早/午分界:"), morningNoonSplitEdit);

    noonDeadlineEdit = new QTimeEdit(this);
    noonDeadlineEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("中午签到截止:"), noonDeadlineEdit);

    noonEveningSplitEdit = new QTimeEdit(this);
    noonEveningSplitEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("午/晚分界:"), noonEveningSplitEdit);

    eveningDeadlineEdit = new QTimeEdit(this);
    eveningDeadlineEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("晚上签到截止:"), eveningDeadlineEdit);

    mainLayout->addLayout(formLayout);

    // Button row
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto *saveButton = new QPushButton(QString("保存"), this);
    auto *cancelButton = new QPushButton(QString("关闭"), this);

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::released, this, &DeadlineDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::released, this, &DeadlineDialog::close);

    loadCurrentConfig();
}

DeadlineDialog::~DeadlineDialog() {}

void DeadlineDialog::loadCurrentConfig()
{
    ConfigData config = ConfigManager::loadConfig();

    morningDeadlineEdit->setTime(config.morningDeadline);
    noonDeadlineEdit->setTime(config.noonDeadline);
    eveningDeadlineEdit->setTime(config.eveningDeadline);
    morningNoonSplitEdit->setTime(config.morningNoonSplit);
    noonEveningSplitEdit->setTime(config.noonEveningSplit);
}

void DeadlineDialog::onSaveClicked()
{
    ConfigData config = ConfigManager::loadConfig();

    // Update times from UI controls
    config.morningDeadline = morningDeadlineEdit->time();
    config.noonDeadline = noonDeadlineEdit->time();
    config.eveningDeadline = eveningDeadlineEdit->time();
    config.morningNoonSplit = morningNoonSplitEdit->time();
    config.noonEveningSplit = noonEveningSplitEdit->time();

    ConfigManager::saveConfig(config);

    QMessageBox::information(this,
                             QString("保存成功"),
                             QString("时间配置已保存到 config.toml"),
                             QMessageBox::Ok);
}
