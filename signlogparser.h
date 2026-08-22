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

#ifndef SIGNLOGPARSER_H
#define SIGNLOGPARSER_H

#include <QString>
#include <QDate>
#include <QTime>
#include <QVector>

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
 * @brief Determine which period a sign-in time belongs to.
 * @param time The sign-in time.
 * @param config Period boundary configuration.
 * @return 0 = morning, 1 = noon, 2 = evening.
 */
int determineSignPeriod(const QTime &time, const ConfigData &config);

/**
 * @brief Determine if a sign-in is on-time or late for its period.
 * @param time The sign-in time.
 * @param period 0 = morning, 1 = noon, 2 = evening.
 * @param config Deadline configuration.
 * @return "签到" if on-time, "迟到" if late.
 */
QString determineSignStatus(const QTime &time, int period,
                            const ConfigData &config);

/**
 * @brief Map a period index to its display name.
 * @param period 0 = morning, 1 = noon, 2 = evening.
 * @return "早上", "中午", "晚上"; empty string for unknown periods.
 */
QString signPeriodName(int period);

/**
 * @brief Parse a single log file and return all valid sign-in records.
 * @param logFilePath Absolute path to the log file.
 * @param date The date associated with this log file.
 * @param config Current deadline/period configuration.
 * @return Vector of parsed SignRecord entries ("Signed" records only;
 *         "Resigned" duplicate attempts are dropped).
 */
QVector<SignRecord> parseSignLogFile(const QString &logFilePath,
                                     const QDate &date,
                                     const ConfigData &config);

/**
 * @brief Parse a single log file and return all sign-up attempts.
 *
 * Same parsing as parseSignLogFile(), but "Resigned" (duplicate-attempt)
 * lines are kept with status "Resigned" so repeated sign-ins can be
 * detected (used by the arbiter statistics window).
 *
 * @param logFilePath Absolute path to the log file.
 * @param date The date associated with this log file.
 * @param config Current deadline/period configuration.
 * @return Vector of parsed SignRecord entries: "Signed" lines get
 *         "签到"/"迟到", "Resigned" lines keep status "Resigned".
 */
QVector<SignRecord> parseSignLogFileAll(const QString &logFilePath,
                                        const QDate &date,
                                        const ConfigData &config);

#endif // SIGNLOGPARSER_H
