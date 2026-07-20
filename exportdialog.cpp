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

#include "exportdialog.h"
#include "signup.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QSet>
#include <QDebug>
#include <algorithm>

#include "xlsxdocument.h"

ExportDialog::ExportDialog(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QString("导出数据"));
    setFixedSize(420, 220);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    auto *titleLabel = new QLabel(
        QString("选择日期范围，导出签到数据为 Excel 文件。\n")
        + QString("导出内容：每日早签前10名 + 全部迟到/未签到记录。"),
        this);
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);

    // Date range form
    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    startDateEdit = new QDateEdit(this);
    startDateEdit->setDisplayFormat("yyyy-MM-dd");
    startDateEdit->setCalendarPopup(true);
    // Default: first day of current month
    QDate today = QDate::currentDate();
    startDateEdit->setDate(QDate(today.year(), today.month(), 1));
    formLayout->addRow(QString("开始日期:"), startDateEdit);

    endDateEdit = new QDateEdit(this);
    endDateEdit->setDisplayFormat("yyyy-MM-dd");
    endDateEdit->setCalendarPopup(true);
    endDateEdit->setDate(today);
    formLayout->addRow(QString("结束日期:"), endDateEdit);

    mainLayout->addLayout(formLayout);

    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto *exportButton = new QPushButton(QString("导出"), this);
    auto *cancelButton = new QPushButton(QString("关闭"), this);

    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(exportButton, &QPushButton::released,
            this, &ExportDialog::onExportClicked);
    connect(cancelButton, &QPushButton::released,
            this, &ExportDialog::close);
}

ExportDialog::~ExportDialog() {}

void ExportDialog::onExportClicked()
{
    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();

    if (startDate > endDate) {
        QMessageBox::warning(this,
                             QString("日期错误"),
                             QString("开始日期不能晚于结束日期！"),
                             QMessageBox::Ok);
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        QString("保存导出文件"),
        QString("signup_export.xlsx"),
        QString("Excel 文件 (*.xlsx)")
    );

    if (filePath.isEmpty()) {
        // User cancelled
        return;
    }

    bool success = exportToExcel(startDate, endDate, filePath);

    if (success) {
        QMessageBox::information(this,
                                 QString("导出成功"),
                                 QString("数据已成功导出到:\n") + filePath,
                                 QMessageBox::Ok);
    }
}

bool ExportDialog::exportToExcel(const QDate &startDate,
                                  const QDate &endDate,
                                  const QString &filePath)
{
    ConfigData config = ConfigManager::loadConfig();

    // Collect all records across the date range
    // We build three categories:
    //   1. morningOnTime: top-10 morning "签到" per day
    //   2. lateRecords: all "迟到" across all periods
    //   3. absentRecords: all "未签到" (one row per missed period per student)

    struct ExportRow {
        QString name;
        QString dateStr;   // yyyy-MM-dd
        QString type;      // "签到" / "迟到" / "未签到"
    };
    QVector<ExportRow> exportRows;

    // Track if we have any data at all
    bool hasAnyData = false;

    // Iterate over each date in range
    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        QString logDir = QDir::cleanPath(
            QCoreApplication::applicationDirPath() +
            QDir::separator() + QString("logs")
        );
        QString logFilePath = logDir + QDir::separator() +
                              date.toString("yyyy-MM-dd") + QString(".log");

        if (!QFile::exists(logFilePath)) {
            continue; // No log file for this date — skip
        }

        QVector<SignRecord> records = parseLogFile(logFilePath, date, config);
        if (records.isEmpty()) {
            continue;
        }
        hasAnyData = true;

        QString dateStr = date.toString("yyyy-MM-dd");

        // Group records by period
        QVector<SignRecord> morningRecords;
        QVector<SignRecord> noonRecords;
        QVector<SignRecord> eveningRecords;

        for (const auto &rec : records) {
            switch (rec.period) {
            case 0: morningRecords.append(rec); break;
            case 1: noonRecords.append(rec); break;
            case 2: eveningRecords.append(rec); break;
            }
        }

        // 1. Morning top-10 on-time (签到)
        QVector<SignRecord> morningOnTime;
        for (const auto &rec : morningRecords) {
            if (rec.status == QString("签到")) {
                morningOnTime.append(rec);
            }
        }
        // Sort by time ascending (earliest first)
        std::sort(morningOnTime.begin(), morningOnTime.end(),
                  [](const SignRecord &a, const SignRecord &b) {
                      return a.time < b.time;
                  });
        // Take top 10
        int takeCount = qMin(10, morningOnTime.size());
        for (int i = 0; i < takeCount; ++i) {
            exportRows.append({morningOnTime[i].name, dateStr, QString("签到")});
        }

        // 2. All late records across all periods
        for (const auto &rec : records) {
            if (rec.status == QString("迟到")) {
                exportRows.append({rec.name, dateStr, QString("迟到")});
            }
        }

        // 3. Absent students per period
        // Get the full student list (skip index 0 — it's a placeholder)
        QSet<QString> allStudents;
        for (int i = 1; i < static_cast<int>(extstunames.size()); ++i) {
            if (!extstunames[i].isEmpty()) {
                allStudents.insert(extstunames[i]);
            }
        }

        // Helper lambda: compute absent students for a period
        auto computeAbsent = [&](const QVector<SignRecord> &periodRecords,
                                  const QString &periodName) {
            QSet<QString> signedIn;
            for (const auto &rec : periodRecords) {
                signedIn.insert(rec.name);
            }
            QSet<QString> absent = allStudents - signedIn;
            for (const auto &name : absent) {
                Q_UNUSED(periodName);
                exportRows.append({name, dateStr, QString("未签到")});
            }
        };

        // Compute absent for each period. Each missed period = one row.
        // To avoid duplicate rows when a student is absent from multiple periods,
        // we use a per-period set.
        QSet<QString> morningSigned;
        for (const auto &rec : morningRecords) {
            morningSigned.insert(rec.name);
        }
        QSet<QString> morningAbsent = allStudents - morningSigned;
        for (const auto &name : morningAbsent) {
            exportRows.append({name, dateStr, QString("未签到")});
        }

        QSet<QString> noonSigned;
        for (const auto &rec : noonRecords) {
            noonSigned.insert(rec.name);
        }
        QSet<QString> noonAbsent = allStudents - noonSigned;
        for (const auto &name : noonAbsent) {
            exportRows.append({name, dateStr, QString("未签到")});
        }

        QSet<QString> eveningSigned;
        for (const auto &rec : eveningRecords) {
            eveningSigned.insert(rec.name);
        }
        QSet<QString> eveningAbsent = allStudents - eveningSigned;
        for (const auto &name : eveningAbsent) {
            exportRows.append({name, dateStr, QString("未签到")});
        }
    }

    if (!hasAnyData || exportRows.isEmpty()) {
        QMessageBox::information(this,
                                 QString("没有数据"),
                                 QString("所选日期范围内没有签到数据可导出。"),
                                 QMessageBox::Ok);
        return false;
    }

    // Write Excel file using QXlsx
    QXlsx::Document xlsx;

    // Header row
    xlsx.write(1, 1, QString("姓名"));
    xlsx.write(1, 2, QString("日期"));
    xlsx.write(1, 3, QString("类型"));

    // Data rows
    for (int i = 0; i < exportRows.size(); ++i) {
        int row = i + 2; // 1-indexed, row 1 is header
        xlsx.write(row, 1, exportRows[i].name);
        xlsx.write(row, 2, exportRows[i].dateStr);
        xlsx.write(row, 3, exportRows[i].type);
    }

    bool saved = xlsx.saveAs(filePath);
    if (!saved) {
        QMessageBox::critical(this,
                              QString("导出失败"),
                              QString("无法保存文件:\n") + filePath,
                              QMessageBox::Ok);
        return false;
    }

    return true;
}

QVector<SignRecord> ExportDialog::parseLogFile(const QString &logFilePath,
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

        int period = determinePeriod(signTime, config);
        QString status = determineStatus(signTime, period, config);

        records.append({name, date, signTime, period, status});
    }

    file.close();
    return records;
}

int ExportDialog::determinePeriod(const QTime &time, const ConfigData &config)
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

QString ExportDialog::determineStatus(const QTime &time, int period,
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
