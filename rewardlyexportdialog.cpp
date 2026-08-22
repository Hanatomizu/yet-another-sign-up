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

#include "rewardlyexportdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <algorithm>

#include "xlsxdocument.h"

RewardlyExportDialog::RewardlyExportDialog(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QString("导出到 Rewardly"));
    setFixedSize(680, 520);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Title
    auto *titleLabel = new QLabel(
        QString("选择日期范围，生成 Rewardly 加分数据。\n")
        + QString("导出内容：每日早上签到前N名（加分）+ 全部早/中/晚迟到（扣分）。\n")
        + QString("勾选左侧复选框选择要导出的记录，点击保存写入 Excel。"),
        this);
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);

    // Date range form
    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    startDateEdit = new QDateEdit(this);
    startDateEdit->setDisplayFormat("yyyy-MM-dd");
    startDateEdit->setCalendarPopup(true);
    QDate today = QDate::currentDate();
    startDateEdit->setDate(QDate(today.year(), today.month(), 1));
    formLayout->addRow(QString("开始日期:"), startDateEdit);

    endDateEdit = new QDateEdit(this);
    endDateEdit->setDisplayFormat("yyyy-MM-dd");
    endDateEdit->setCalendarPopup(true);
    endDateEdit->setDate(today);
    formLayout->addRow(QString("结束日期:"), endDateEdit);

    mainLayout->addLayout(formLayout);

    // Data table: checkbox | 姓名 | 日期 | 加分 | 原因
    table = new QTableWidget(0, 5, this);
    table->setHorizontalHeaderLabels(
        {QString("选择"), QString("姓名"), QString("日期"),
         QString("加分"), QString("原因")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setColumnWidth(0, 60);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(table, 1);

    // Status line + buttons
    statusLabel = new QLabel(this);
    mainLayout->addWidget(statusLabel);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto *saveButton = new QPushButton(QString("保存"), this);
    auto *cancelButton = new QPushButton(QString("关闭"), this);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::released,
            this, &RewardlyExportDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::released,
            this, &RewardlyExportDialog::close);
    connect(startDateEdit, &QDateEdit::dateChanged,
            this, &RewardlyExportDialog::refreshData);
    connect(endDateEdit, &QDateEdit::dateChanged,
            this, &RewardlyExportDialog::refreshData);

    refreshData();
}

RewardlyExportDialog::~RewardlyExportDialog() {}

QVector<RewardlyRow> RewardlyExportDialog::buildRows(
    const QVector<SignRecord> &records,
    const QDate &date,
    const ConfigData &config)
{
    QVector<RewardlyRow> rows;
    QString dateStr = date.toString("yyyy-MM-dd");

    // 1. Morning top-N on-time sign-ins (bonus)
    QVector<SignRecord> morningOnTime;
    for (const auto &rec : records) {
        if (rec.period == 0 && rec.status == QString("签到")) {
            morningOnTime.append(rec);
        }
    }
    // Sort by time ascending (earliest first)
    std::sort(morningOnTime.begin(), morningOnTime.end(),
              [](const SignRecord &a, const SignRecord &b) {
                  return a.time < b.time;
              });
    int takeCount = qMin(config.morningSignBonusCount, morningOnTime.size());
    for (int i = 0; i < takeCount; ++i) {
        rows.append({morningOnTime[i].name, dateStr, signPeriodName(0),
                     config.morningSignBonusScore, QString("早上签到")});
    }

    // 2. All late records across all periods (deduction)
    for (const auto &rec : records) {
        if (rec.status != QString("迟到")) {
            continue;
        }
        int deduction = 0;
        QString reason;
        switch (rec.period) {
        case 0: deduction = config.morningLateDeduction; reason = QString("早上迟到"); break;
        case 1: deduction = config.noonLateDeduction;    reason = QString("中午迟到"); break;
        case 2: deduction = config.eveningLateDeduction; reason = QString("晚上迟到"); break;
        default: continue;
        }
        rows.append({rec.name, dateStr, signPeriodName(rec.period),
                     -deduction, reason});
    }

    return rows;
}

void RewardlyExportDialog::refreshData()
{
    table->setRowCount(0);

    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();
    if (startDate > endDate) {
        statusLabel->setText(QString("开始日期不能晚于结束日期！"));
        return;
    }

    ConfigData config = ConfigManager::loadConfig();

    int total = 0;
    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        QString logFilePath = QDir::cleanPath(
            QCoreApplication::applicationDirPath() +
            QDir::separator() + QString("logs") + QDir::separator() +
            date.toString("yyyy-MM-dd") + QString(".log"));

        if (!QFile::exists(logFilePath)) {
            continue; // No log file for this date — skip
        }

        QVector<SignRecord> records = parseSignLogFile(logFilePath, date, config);
        if (records.isEmpty()) {
            continue;
        }

        QVector<RewardlyRow> rows = buildRows(records, date, config);
        for (const auto &row : rows) {
            appendRowToTable(row);
        }
        total += rows.size();
    }

    statusLabel->setText(QString("共 %1 条记录").arg(total));
    if (total == 0) {
        statusLabel->setText(QString("所选日期范围内没有可导出的签到数据。"));
    }
}

void RewardlyExportDialog::appendRowToTable(const RewardlyRow &row)
{
    int r = table->rowCount();
    table->insertRow(r);

    auto *checkItem = new QTableWidgetItem;
    checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    checkItem->setCheckState(Qt::Checked);
    table->setItem(r, 0, checkItem);

    table->setItem(r, 1, new QTableWidgetItem(row.name));
    table->setItem(r, 2, new QTableWidgetItem(
        row.dateStr + QString(" ") + row.periodName));
    table->setItem(r, 3, new QTableWidgetItem(QString::number(row.points)));
    table->setItem(r, 4, new QTableWidgetItem(row.reason));
}

void RewardlyExportDialog::onSaveClicked()
{
    // Collect the checked rows only
    QVector<RewardlyRow> selected;
    for (int r = 0; r < table->rowCount(); ++r) {
        if (table->item(r, 0)->checkState() != Qt::Checked) {
            continue;
        }
        RewardlyRow row;
        row.name = table->item(r, 1)->text();
        row.dateStr = table->item(r, 2)->text();
        row.points = table->item(r, 3)->text().toInt();
        row.reason = table->item(r, 4)->text();
        selected.append(row);
    }

    if (selected.isEmpty()) {
        QMessageBox::warning(this,
                             QString("没有选择数据"),
                             QString("请至少勾选一条要导出的记录。"),
                             QMessageBox::Ok);
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        QString("保存导出文件"),
        QString("rewardly_export.xlsx"),
        QString("Excel 文件 (*.xlsx)")
    );
    if (filePath.isEmpty()) {
        return; // User cancelled
    }

    // Write Excel file using QXlsx
    QXlsx::Document xlsx;

    // Header row
    xlsx.write(1, 1, QString("姓名"));
    xlsx.write(1, 2, QString("日期"));
    xlsx.write(1, 3, QString("加分"));
    xlsx.write(1, 4, QString("原因"));

    // Data rows
    for (int i = 0; i < selected.size(); ++i) {
        int row = i + 2; // 1-indexed, row 1 is header
        xlsx.write(row, 1, selected[i].name);
        xlsx.write(row, 2, selected[i].dateStr);
        xlsx.write(row, 3, selected[i].points);
        xlsx.write(row, 4, selected[i].reason);
    }

    if (!xlsx.saveAs(filePath)) {
        QMessageBox::critical(this,
                              QString("导出失败"),
                              QString("无法保存文件:\n") + filePath,
                              QMessageBox::Ok);
        return;
    }

    QMessageBox::information(this,
                             QString("导出成功"),
                             QString("已导出 %1 条记录到:\n%2")
                                 .arg(selected.size())
                                 .arg(filePath),
                             QMessageBox::Ok);
}
