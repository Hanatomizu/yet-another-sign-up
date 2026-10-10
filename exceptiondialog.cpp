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

#include "exceptiondialog.h"
#include "signlogparser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QAbstractItemView>
#include <algorithm>

namespace {

/// One entry per (date, period): replace an existing row or append.
void upsert(QVector<PeriodException> &list, const PeriodException &ex)
{
    for (auto &existing : list) {
        if (existing.date == ex.date && existing.period == ex.period) {
            existing = ex;
            return;
        }
    }
    list.append(ex);
}

/// Keep the list ordered by date, then by period.
void sortExceptions(QVector<PeriodException> &list)
{
    std::sort(list.begin(), list.end(),
              [](const PeriodException &a, const PeriodException &b) {
                  if (a.date != b.date) {
                      return a.date < b.date;
                  }
                  return a.period < b.period;
              });
}

} // namespace

ExceptionDialog::ExceptionDialog(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QString("设置例外"));
    setFixedSize(660, 560);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto *titleLabel = new QLabel(
        QString("为指定日期的某个时段设置例外的分界时间与迟到时间。\n")
        + QString("「分界时间」为该时段结束、下一时段开始的时间；")
        + QString("「迟到时间」为该时段的签到截止时间。\n")
        + QString("仅对所选日期生效，其他日期仍使用「设置迟到时间」中的全局配置。"),
        this);
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);

    // Editors for one exception entry
    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    dateEdit = new QDateEdit(this);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setCalendarPopup(true);
    dateEdit->setDate(QDate::currentDate());
    formLayout->addRow(QString("日期:"), dateEdit);

    periodCombo = new QComboBox(this);
    periodCombo->addItem(QString("早上"));
    periodCombo->addItem(QString("中午"));
    periodCombo->addItem(QString("晚上"));
    formLayout->addRow(QString("时段:"), periodCombo);

    splitEdit = new QTimeEdit(this);
    splitEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("分界时间:"), splitEdit);

    deadlineEdit = new QTimeEdit(this);
    deadlineEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("迟到时间:"), deadlineEdit);

    mainLayout->addLayout(formLayout);

    hintLabel = new QLabel(this);
    hintLabel->setWordWrap(true);
    mainLayout->addWidget(hintLabel);

    auto *editButtonLayout = new QHBoxLayout();
    addButton = new QPushButton(QString("添加 / 更新"), this);
    removeButton = new QPushButton(QString("删除选中"), this);
    editButtonLayout->addWidget(addButton);
    editButtonLayout->addWidget(removeButton);
    editButtonLayout->addStretch();
    mainLayout->addLayout(editButtonLayout);

    // Exception list
    table = new QTableWidget(0, 4, this);
    table->setHorizontalHeaderLabels(
        {QString("日期"), QString("时段"), QString("分界时间"),
         QString("迟到时间")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(table, 1);

    statusLabel = new QLabel(this);
    mainLayout->addWidget(statusLabel);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    auto *saveButton = new QPushButton(QString("保存"), this);
    auto *closeButton = new QPushButton(QString("关闭"), this);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(periodCombo, &QComboBox::currentTextChanged,
            this, &ExceptionDialog::onPeriodChanged);
    connect(addButton, &QPushButton::released,
            this, &ExceptionDialog::onAddClicked);
    connect(removeButton, &QPushButton::released,
            this, &ExceptionDialog::onRemoveClicked);
    connect(table, &QTableWidget::itemSelectionChanged,
            this, &ExceptionDialog::onTableSelectionChanged);
    connect(table, &QTableWidget::cellDoubleClicked,
            this, &ExceptionDialog::onTableDoubleClicked);
    connect(saveButton, &QPushButton::released,
            this, &ExceptionDialog::onSaveClicked);
    connect(closeButton, &QPushButton::released,
            this, &ExceptionDialog::close);

    loadExceptions();
    updateSplitEnabled();
}

ExceptionDialog::~ExceptionDialog() {}

void ExceptionDialog::loadExceptions()
{
    ConfigData config = ConfigManager::loadConfig();
    exceptions = config.exceptions;
    sortExceptions(exceptions);
    refreshTable();
}

void ExceptionDialog::refreshTable()
{
    sortExceptions(exceptions);

    const int previousRow = table->currentRow();
    table->setRowCount(0);
    for (const auto &ex : exceptions) {
        int r = table->rowCount();
        table->insertRow(r);
        table->setItem(r, 0, new QTableWidgetItem(
            ex.date.toString("yyyy-MM-dd")));
        table->setItem(r, 1, new QTableWidgetItem(
            signPeriodName(ex.period)));
        table->setItem(r, 2, new QTableWidgetItem(
            (ex.period == 2 || !ex.split.isValid())
                ? QString("—")
                : ex.split.toString("HH:mm")));
        table->setItem(r, 3, new QTableWidgetItem(
            ex.deadline.isValid() ? ex.deadline.toString("HH:mm")
                                  : QString("—")));
    }

    statusLabel->setText(QString("共 %1 条例外设置").arg(exceptions.size()));

    if (previousRow >= 0 && previousRow < table->rowCount()) {
        table->selectRow(previousRow);
    }
    onTableSelectionChanged();
}

void ExceptionDialog::selectRow(int row)
{
    if (row >= 0 && row < table->rowCount()) {
        table->selectRow(row);
        table->scrollToItem(table->item(row, 0));
    }
}

void ExceptionDialog::loadRowIntoEditors(int row)
{
    if (row < 0 || row >= table->rowCount() || row >= exceptions.size()) {
        return;
    }
    const PeriodException &ex = exceptions[row];
    dateEdit->setDate(ex.date);
    periodCombo->setCurrentIndex(ex.period);
    if (ex.split.isValid()) {
        splitEdit->setTime(ex.split);
    }
    if (ex.deadline.isValid()) {
        deadlineEdit->setTime(ex.deadline);
    }
    // Note: switching the combo above already refreshed the enabled state and
    // the hint via updateSplitEnabled(); the times set here must win, so the
    // helper is deliberately not called again.
}

void ExceptionDialog::updateSplitEnabled()
{
    const int period = periodCombo->currentIndex();

    // Defaults come from the global [periods] configuration; switching the
    // period reloads that period's global values so the user has a starting
    // point to adjust.
    ConfigData config = ConfigManager::loadConfig();
    switch (period) {
    case 0:
        splitEdit->setEnabled(true);
        splitEdit->setTime(config.morningNoonSplit);
        deadlineEdit->setTime(config.morningDeadline);
        hintLabel->setText(
            QString("早上：分界时间默认取全局「早/午分界」，迟到时间默认取「早上签到截止」。"));
        break;
    case 1:
        splitEdit->setEnabled(true);
        splitEdit->setTime(config.noonEveningSplit);
        deadlineEdit->setTime(config.noonDeadline);
        hintLabel->setText(
            QString("中午：分界时间默认取全局「午/晚分界」，迟到时间默认取「中午签到截止」。"));
        break;
    default:
        // 晚上 is the last period of the day — nothing follows it, so the
        // 分界时间 is meaningless and the input is disabled.
        splitEdit->setEnabled(false);
        splitEdit->setTime(config.eveningDeadline);
        deadlineEdit->setTime(config.eveningDeadline);
        hintLabel->setText(
            QString("晚上：当天最后一个时段，没有分界时间；只能设置迟到时间。"));
        break;
    }
}

void ExceptionDialog::onPeriodChanged()
{
    updateSplitEnabled();
}

void ExceptionDialog::onAddClicked()
{
    PeriodException candidate;
    candidate.date = dateEdit->date();
    candidate.period = periodCombo->currentIndex();
    candidate.deadline = deadlineEdit->time();
    // 晚上 has no boundary after it; keep the value invalid so it is ignored.
    candidate.split = (candidate.period == 2)
        ? QTime()
        : splitEdit->time();

    QVector<PeriodException> next = exceptions;
    upsert(next, candidate);

    ConfigData probe = ConfigManager::loadConfig();
    probe.exceptions = next;
    const QString error = ConfigManager::validateExceptions(probe);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QString("例外设置无效"), error,
                             QMessageBox::Ok);
        return;
    }

    exceptions = next;
    refreshTable();

    // Highlight the row that was just added/updated.
    for (int r = 0; r < exceptions.size(); ++r) {
        if (exceptions[r].date == candidate.date
            && exceptions[r].period == candidate.period) {
            selectRow(r);
            break;
        }
    }
}

void ExceptionDialog::onRemoveClicked()
{
    const int row = table->currentRow();
    if (row < 0 || row >= exceptions.size()) {
        QMessageBox::information(this, QString("未选择例外"),
                                 QString("请先在列表中选择要删除的例外。"),
                                 QMessageBox::Ok);
        return;
    }

    const PeriodException removed = exceptions[row];
    exceptions.removeAt(row);
    refreshTable();
    statusLabel->setText(
        QString("已移除 %1 %2 的例外（尚未保存）")
            .arg(removed.date.toString("yyyy-MM-dd"),
                 signPeriodName(removed.period)));
}

void ExceptionDialog::onTableSelectionChanged()
{
    removeButton->setEnabled(table->currentRow() >= 0);
}

void ExceptionDialog::onTableDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    loadRowIntoEditors(row);
}

void ExceptionDialog::onSaveClicked()
{
    ConfigData config = ConfigManager::loadConfig();
    config.exceptions = exceptions;

    const QString error = ConfigManager::validateExceptions(config);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QString("例外设置无效"), error,
                             QMessageBox::Ok);
        return;
    }

    ConfigManager::saveConfig(config);

    QMessageBox::information(this,
                             QString("保存成功"),
                             QString("已保存 %1 条例外设置到 config.toml")
                                 .arg(config.exceptions.size()),
                             QMessageBox::Ok);
}
