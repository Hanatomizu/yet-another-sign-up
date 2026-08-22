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

#ifndef REWARDLYEXPORTDIALOG_H
#define REWARDLYEXPORTDIALOG_H

#include <QWidget>
#include <QDateEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QSet>

#include "configmanager.h"
#include "signlogparser.h"

/**
 * @brief One row of Rewardly scoring data ready for Excel export.
 */
struct RewardlyRow {
    QString name;
    QString dateStr;    // yyyy-MM-dd
    QString periodName; // 早上 / 中午 / 晚上
    double points;      // signed, 2 decimals: positive = 加分, negative = 扣分
    QString reason;     // 早上签到 / 早上迟到 / 中午迟到 / 晚上迟到 / 早上未签到 / ...
};

/**
 * @brief Dialog for building Rewardly scoring data and exporting it to Excel.
 *
 * The user picks a date range; the dialog then shows every generated row
 * (morning top-N bonus sign-ins + all morning/noon/evening lates and
 * absents) in a table. Each row has a checkbox; the save button exports
 * only the checked rows as an .xlsx file with columns:
 * 姓名 | 日期 (with 早上/中午/晚上) | 加分 | 原因
 */
class RewardlyExportDialog : public QWidget
{
    Q_OBJECT

public:
    explicit RewardlyExportDialog(QWidget *parent = nullptr);
    ~RewardlyExportDialog();

    /**
     * @brief Build the Rewardly rows for one day's records.
     *
     * Pure computation (no UI), kept public so it can be unit tested.
     *
     * @param records Parsed sign-in records of one day.
     * @param allStudents Full student roster (used to compute absents).
     * @param date The date those records belong to.
     * @param config Scoring configuration.
     * @return Rewardly rows: morning top-N on-time sign-ins (bonus), all
     *         late records (deduction) and all absent students per period
     *         (deduction).
     */
    static QVector<RewardlyRow> buildRows(const QVector<SignRecord> &records,
                                          const QSet<QString> &allStudents,
                                          const QDate &date,
                                          const ConfigData &config);

private slots:
    void refreshData();
    void onSaveClicked();

private:
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;
    QTableWidget *table;
    QLabel *statusLabel;

    void appendRowToTable(const RewardlyRow &row);
};

#endif // REWARDLYEXPORTDIALOG_H
