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

#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QWidget>
#include <QDateEdit>
#include <QPushButton>
#include <QDate>

#include "configmanager.h"
#include "signlogparser.h"

/**
 * @brief Dialog for exporting sign-in data to Excel.
 *
 * Allows the user to select a date range and export filtered
 * sign-in records as a .xlsx file via the QXlsx library.
 *
 * Export rules per day:
 * - Top 10 morning (早) on-time sign-ins → type "签到"
 * - All late sign-ins across all periods → type "迟到"
 * - All absent students per period → type "未签到"
 *
 * The "日期" column appends the concrete sign-in period
 * (早上 / 中午 / 晚上) to the date.
 */
class ExportDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ExportDialog(QWidget *parent = nullptr);
    ~ExportDialog();

private slots:
    void onExportClicked();

private:
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;

    /**
     * @brief Run the full export pipeline.
     * @param startDate Beginning of date range (inclusive).
     * @param endDate End of date range (inclusive).
     * @param filePath Destination .xlsx file path.
     * @return true on success, false on failure or no data.
     */
    bool exportToExcel(const QDate &startDate, const QDate &endDate,
                       const QString &filePath);
};

#endif // EXPORTDIALOG_H
