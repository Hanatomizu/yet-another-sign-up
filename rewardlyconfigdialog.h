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

#ifndef REWARDLYCONFIGDIALOG_H
#define REWARDLYCONFIGDIALOG_H

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>

/**
 * @brief Dialog for configuring the Rewardly scoring rules.
 *
 * Provides one QSpinBox (morning bonus recipient count) and seven
 * QDoubleSpinBox controls (2 decimal places):
 * - 早上签到加分人数 (morningSignBonusCount)
 * - 早上签到加分分数 (morningSignBonusScore)
 * - 早上/中午/晚上迟到扣分分数
 * - 早上/中午/晚上未签到扣分分数
 *
 * Score values are signed: positive = 加分, negative = 扣分.
 * Values are saved to config.toml via ConfigManager.
 */
class RewardlyConfigDialog : public QWidget
{
    Q_OBJECT

public:
    explicit RewardlyConfigDialog(QWidget *parent = nullptr);
    ~RewardlyConfigDialog();

private slots:
    void onSaveClicked();

private:
    QSpinBox *morningSignCountEdit;
    QDoubleSpinBox *morningSignScoreEdit;
    QDoubleSpinBox *morningLateEdit;
    QDoubleSpinBox *morningAbsentEdit;
    QDoubleSpinBox *noonLateEdit;
    QDoubleSpinBox *noonAbsentEdit;
    QDoubleSpinBox *eveningLateEdit;
    QDoubleSpinBox *eveningAbsentEdit;

    void loadCurrentConfig();
};

#endif // REWARDLYCONFIGDIALOG_H
