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

#include "rewardlyconfigdialog.h"
#include "configmanager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

RewardlyConfigDialog::RewardlyConfigDialog(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QString("配置加分项"));
    setFixedSize(400, 380);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    auto *titleLabel = new QLabel(
        QString("配置 Rewardly 加分规则。\n")
        + QString("扣分分数以正数填写，导出时自动记为负数。"),
        this);
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);

    // Form layout for score edits
    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    morningSignCountEdit = new QSpinBox(this);
    morningSignCountEdit->setRange(0, 1000);
    formLayout->addRow(QString("早上签到加分人数:"), morningSignCountEdit);

    morningSignScoreEdit = new QSpinBox(this);
    morningSignScoreEdit->setRange(0, 1000);
    formLayout->addRow(QString("早上签到加分分数:"), morningSignScoreEdit);

    morningLateEdit = new QSpinBox(this);
    morningLateEdit->setRange(0, 1000);
    formLayout->addRow(QString("早上迟到扣分分数:"), morningLateEdit);

    morningAbsentEdit = new QSpinBox(this);
    morningAbsentEdit->setRange(0, 1000);
    formLayout->addRow(QString("早上未签到扣分分数:"), morningAbsentEdit);

    noonLateEdit = new QSpinBox(this);
    noonLateEdit->setRange(0, 1000);
    formLayout->addRow(QString("中午迟到扣分分数:"), noonLateEdit);

    noonAbsentEdit = new QSpinBox(this);
    noonAbsentEdit->setRange(0, 1000);
    formLayout->addRow(QString("中午未签到扣分分数:"), noonAbsentEdit);

    eveningLateEdit = new QSpinBox(this);
    eveningLateEdit->setRange(0, 1000);
    formLayout->addRow(QString("晚上迟到扣分分数:"), eveningLateEdit);

    eveningAbsentEdit = new QSpinBox(this);
    eveningAbsentEdit->setRange(0, 1000);
    formLayout->addRow(QString("晚上未签到扣分分数:"), eveningAbsentEdit);

    mainLayout->addLayout(formLayout);

    // Button row
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto *saveButton = new QPushButton(QString("保存"), this);
    auto *cancelButton = new QPushButton(QString("关闭"), this);

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::released,
            this, &RewardlyConfigDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::released,
            this, &RewardlyConfigDialog::close);

    loadCurrentConfig();
}

RewardlyConfigDialog::~RewardlyConfigDialog() {}

void RewardlyConfigDialog::loadCurrentConfig()
{
    ConfigData config = ConfigManager::loadConfig();

    morningSignCountEdit->setValue(config.morningSignBonusCount);
    morningSignScoreEdit->setValue(config.morningSignBonusScore);
    morningLateEdit->setValue(config.morningLateDeduction);
    morningAbsentEdit->setValue(config.morningAbsentDeduction);
    noonLateEdit->setValue(config.noonLateDeduction);
    noonAbsentEdit->setValue(config.noonAbsentDeduction);
    eveningLateEdit->setValue(config.eveningLateDeduction);
    eveningAbsentEdit->setValue(config.eveningAbsentDeduction);
}

void RewardlyConfigDialog::onSaveClicked()
{
    ConfigData config = ConfigManager::loadConfig();

    // Update scoring values from UI controls
    config.morningSignBonusCount = morningSignCountEdit->value();
    config.morningSignBonusScore = morningSignScoreEdit->value();
    config.morningLateDeduction = morningLateEdit->value();
    config.morningAbsentDeduction = morningAbsentEdit->value();
    config.noonLateDeduction = noonLateEdit->value();
    config.noonAbsentDeduction = noonAbsentEdit->value();
    config.eveningLateDeduction = eveningLateEdit->value();
    config.eveningAbsentDeduction = eveningAbsentEdit->value();

    ConfigManager::saveConfig(config);

    QMessageBox::information(this,
                             QString("保存成功"),
                             QString("加分配置已保存到 config.toml"),
                             QMessageBox::Ok);
}
