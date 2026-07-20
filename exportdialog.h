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

/**
 * @brief Internal structure representing a parsed sign-in record.
 */
struct SignRecord {
    QString name;
    QDate date;
    QTime time;
    int period;       // 0 = morning, 1 = noon, 2 = evening
    QString status;   // "签到" or "迟到"
};

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

    /**
     * @brief Parse a single log file and return all valid sign-in records.
     * @param logFilePath Absolute path to the log file.
     * @param date The date associated with this log file.
     * @param config Current deadline/period configuration.
     * @return Vector of parsed SignRecord entries.
     */
    QVector<SignRecord> parseLogFile(const QString &logFilePath,
                                     const QDate &date,
                                     const ConfigData &config);

    /**
     * @brief Determine which period a sign-in time belongs to.
     * @param time The sign-in time.
     * @param config Period boundary configuration.
     * @return 0 = morning, 1 = noon, 2 = evening.
     */
    static int determinePeriod(const QTime &time, const ConfigData &config);

    /**
     * @brief Determine if a sign-in is on-time or late for its period.
     * @param time The sign-in time.
     * @param period 0 = morning, 1 = noon, 2 = evening.
     * @param config Deadline configuration.
     * @return "签到" if on-time, "迟到" if late.
     */
    static QString determineStatus(const QTime &time, int period,
                                   const ConfigData &config);
};

#endif // EXPORTDIALOG_H
