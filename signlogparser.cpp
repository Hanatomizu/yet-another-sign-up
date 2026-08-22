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

#include "signlogparser.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

int determineSignPeriod(const QTime &time, const ConfigData &config)
{
    // Period 0 = morning: time <= morning_noon_split
    // Period 1 = noon:    morning_noon_split < time <= noon_evening_split
    // Period 2 = evening: time > noon_evening_split
    if (time <= config.morningNoonSplit) {
        return 0;
    } else if (time <= config.noonEveningSplit) {
        return 1;
    } else {
        return 2;
    }
}

QString determineSignStatus(const QTime &time, int period,
                            const ConfigData &config)
{
    QTime deadline;
    switch (period) {
    case 0: deadline = config.morningDeadline; break;
    case 1: deadline = config.noonDeadline;    break;
    case 2: deadline = config.eveningDeadline; break;
    default: return QString("迟到");
    }

    if (time <= deadline) {
        return QString("签到");
    } else {
        return QString("迟到");
    }
}

QString signPeriodName(int period)
{
    switch (period) {
    case 0: return QString("早上");
    case 1: return QString("中午");
    case 2: return QString("晚上");
    default: return QString();
    }
}

QVector<SignRecord> parseSignLogFile(const QString &logFilePath,
                                     const QDate &date,
                                     const ConfigData &config)
{
    QVector<SignRecord> records;
    QFile file(logFilePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open log file:" << logFilePath;
        return records;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip header/session markers
        if (line.isEmpty() || line.startsWith('-') || line.startsWith('=')) {
            continue;
        }

        // Parse log format: "yyyy.MM.dd hh:mm:ss, Name, Status"
        // Use comma-splitting for robustness instead of fixed offsets
        QStringList parts = line.split(", ");
        if (parts.size() < 3) {
            continue;
        }

        QString statusField = parts[2].trimmed();
        // Only process successful sign-ins, skip "Resigned" duplicates
        if (statusField != QString("Signed")) {
            continue;
        }

        QString name = parts[1].trimmed();
        if (name.isEmpty()) {
            continue;
        }

        // Parse time from the datetime portion
        // Format: "yyyy.MM.dd hh:mm:ss"
        QString dateTimePart = parts[0].trimmed();
        QStringList dtParts = dateTimePart.split(' ');
        if (dtParts.size() < 2) {
            continue;
        }

        QString timeStr = dtParts[1].trimmed();
        // Handle possible double-colon typo in log: "hh:mm::ss" → normalize
        timeStr.replace("::", ":");
        QTime signTime = QTime::fromString(timeStr, "HH:mm:ss");
        if (!signTime.isValid()) {
            // Also try "HH:mm" in case seconds are missing
            signTime = QTime::fromString(timeStr.left(5), "HH:mm");
            if (!signTime.isValid()) {
                continue;
            }
        }

        int period = determineSignPeriod(signTime, config);
        QString status = determineSignStatus(signTime, period, config);

        records.append({name, date, signTime, period, status});
    }

    file.close();
    return records;
}
