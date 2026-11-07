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
#include "signup.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QAbstractItemView>
#include <QTabWidget>
#include <QStringList>
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

/// One entry per (student, dateFrom, dateTo): replace it or append.
void upsertLeave(QVector<StudentLeaveException> &list,
                 const StudentLeaveException &leave)
{
    for (auto &existing : list) {
        if (existing.name == leave.name
            && existing.dateFrom == leave.dateFrom
            && existing.dateTo == leave.dateTo) {
            existing = leave;
            return;
        }
    }
    list.append(leave);
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

/// Keep the leave list ordered by start date, end date, then name.
void sortLeaves(QVector<StudentLeaveException> &list)
{
    std::sort(list.begin(), list.end(),
              [](const StudentLeaveException &a,
                 const StudentLeaveException &b) {
                  if (a.dateFrom != b.dateFrom) {
                      return a.dateFrom < b.dateFrom;
                  }
                  if (a.dateTo != b.dateTo) {
                      return a.dateTo < b.dateTo;
                  }
                  return a.name < b.name;
              });
}

/// Human-readable period list, e.g. "早上、中午".
QString periodSetText(const QSet<int> &periods)
{
    QStringList parts;
    for (int period = 0; period <= 2; ++period) {
        if (periods.contains(period)) {
            parts.append(signPeriodName(period));
        }
    }
    return parts.join(QString("、"));
}

} // namespace

ExceptionDialog::ExceptionDialog(
    const QVector<PeriodException> &initial,
    const QVector<StudentLeaveException> &initialLeaves,
    QWidget *parent)
    : QWidget(parent)
    , list(initial)
    , leaveList(initialLeaves)
{
    setWindowTitle(QString("设置例外"));
    setFixedSize(760, 680);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto *titleLabel = new QLabel(
        QString("「时段例外」页：为指定日期的某个时段设置分界时间与迟到时间，"
                "或忽略该时段。\n")
        + QString("「按人请假」页：为一名或多名学生设置请假日期范围与请假时段，"
                  "请假期间不生成迟到/未签到扣分。\n")
        + QString("例外是临时设置：只在本次「导出到 Rewardly」窗口内生效，"
                  "不会写入 config.toml。"),
        this);
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);

    auto *tabs = new QTabWidget(this);
    mainLayout->addWidget(tabs, 1);

    // ======================= 时段例外 tab =======================
    auto *periodTab = new QWidget(tabs);
    auto *periodLayout = new QVBoxLayout(periodTab);
    periodLayout->setSpacing(8);
    periodLayout->setContentsMargins(8, 8, 8, 8);

    // Editors for one period exception entry
    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    dateEdit = new QDateEdit(periodTab);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setCalendarPopup(true);
    dateEdit->setDate(QDate::currentDate());
    formLayout->addRow(QString("日期:"), dateEdit);

    periodCombo = new QComboBox(periodTab);
    periodCombo->addItem(QString("早上"));
    periodCombo->addItem(QString("中午"));
    periodCombo->addItem(QString("晚上"));
    formLayout->addRow(QString("时段:"), periodCombo);

    ignoreCheck = new QCheckBox(
        QString("忽略该时段（当天不生成加分/迟到/未签到记录）"), periodTab);
    formLayout->addRow(ignoreCheck);

    splitEdit = new QTimeEdit(periodTab);
    splitEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("分界时间:"), splitEdit);

    deadlineEdit = new QTimeEdit(periodTab);
    deadlineEdit->setDisplayFormat("HH:mm");
    formLayout->addRow(QString("迟到时间:"), deadlineEdit);

    periodLayout->addLayout(formLayout);

    hintLabel = new QLabel(periodTab);
    hintLabel->setWordWrap(true);
    periodLayout->addWidget(hintLabel);

    auto *editButtonLayout = new QHBoxLayout();
    addButton = new QPushButton(QString("添加 / 更新"), periodTab);
    removeButton = new QPushButton(QString("删除选中"), periodTab);
    editButtonLayout->addWidget(addButton);
    editButtonLayout->addWidget(removeButton);
    editButtonLayout->addStretch();
    periodLayout->addLayout(editButtonLayout);

    // Exception list
    table = new QTableWidget(0, 5, periodTab);
    table->setHorizontalHeaderLabels(
        {QString("日期"), QString("时段"), QString("分界时间"),
         QString("迟到时间"), QString("忽略")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    periodLayout->addWidget(table, 1);

    statusLabel = new QLabel(periodTab);
    periodLayout->addWidget(statusLabel);

    tabs->addTab(periodTab, QString("时段例外"));

    // ======================= 按人请假 tab =======================
    auto *leaveTab = new QWidget(tabs);
    auto *leaveLayout = new QVBoxLayout(leaveTab);
    leaveLayout->setSpacing(8);
    leaveLayout->setContentsMargins(8, 8, 8, 8);

    auto *leaveForm = new QFormLayout();
    leaveForm->setSpacing(8);

    leaveStartEdit = new QDateEdit(leaveTab);
    leaveStartEdit->setDisplayFormat("yyyy-MM-dd");
    leaveStartEdit->setCalendarPopup(true);
    leaveStartEdit->setDate(QDate::currentDate());
    leaveForm->addRow(QString("开始日期:"), leaveStartEdit);

    leaveEndEdit = new QDateEdit(leaveTab);
    leaveEndEdit->setDisplayFormat("yyyy-MM-dd");
    leaveEndEdit->setCalendarPopup(true);
    leaveEndEdit->setDate(QDate::currentDate());
    leaveForm->addRow(QString("结束日期:"), leaveEndEdit);

    auto *leavePeriodLayout = new QHBoxLayout();
    leaveMorningCheck = new QCheckBox(QString("早上"), leaveTab);
    leaveNoonCheck = new QCheckBox(QString("中午"), leaveTab);
    leaveEveningCheck = new QCheckBox(QString("晚上"), leaveTab);
    leaveMorningCheck->setChecked(true);
    leaveNoonCheck->setChecked(true);
    leaveEveningCheck->setChecked(true);
    leavePeriodLayout->addWidget(leaveMorningCheck);
    leavePeriodLayout->addWidget(leaveNoonCheck);
    leavePeriodLayout->addWidget(leaveEveningCheck);
    leavePeriodLayout->addStretch();
    leaveForm->addRow(QString("请假时段:"), leavePeriodLayout);

    leaveLayout->addLayout(leaveForm);

    auto *rosterLabel = new QLabel(
        QString("勾选请假学生（名单来自 names 文件）:"), leaveTab);
    leaveLayout->addWidget(rosterLabel);

    studentList = new QListWidget(leaveTab);
    studentList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    studentList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leaveLayout->addWidget(studentList, 1);

    auto *rosterButtonLayout = new QHBoxLayout();
    auto *selectAllButton = new QPushButton(QString("全选"), leaveTab);
    auto *selectNoneButton = new QPushButton(QString("全不选"), leaveTab);
    rosterButtonLayout->addWidget(selectAllButton);
    rosterButtonLayout->addWidget(selectNoneButton);
    rosterButtonLayout->addStretch();
    leaveLayout->addLayout(rosterButtonLayout);

    auto *leaveEditButtonLayout = new QHBoxLayout();
    leaveAddButton = new QPushButton(QString("添加 / 更新"), leaveTab);
    leaveRemoveButton = new QPushButton(QString("删除选中"), leaveTab);
    leaveEditButtonLayout->addWidget(leaveAddButton);
    leaveEditButtonLayout->addWidget(leaveRemoveButton);
    leaveEditButtonLayout->addStretch();
    leaveLayout->addLayout(leaveEditButtonLayout);

    leaveTable = new QTableWidget(0, 4, leaveTab);
    leaveTable->setHorizontalHeaderLabels(
        {QString("姓名"), QString("开始日期"), QString("结束日期"),
         QString("请假时段")});
    leaveTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    leaveTable->verticalHeader()->setVisible(false);
    leaveTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    leaveTable->setSelectionMode(QAbstractItemView::SingleSelection);
    leaveTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leaveLayout->addWidget(leaveTable, 1);

    leaveStatusLabel = new QLabel(leaveTab);
    leaveLayout->addWidget(leaveStatusLabel);

    tabs->addTab(leaveTab, QString("按人请假"));

    // Bottom buttons, shared by both tabs.
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    auto *okButton = new QPushButton(QString("确定"), this);
    auto *cancelButton = new QPushButton(QString("取消"), this);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(periodCombo, &QComboBox::currentTextChanged,
            this, &ExceptionDialog::onPeriodChanged);
    connect(ignoreCheck, &QCheckBox::toggled,
            this, &ExceptionDialog::onPeriodChanged);
    connect(addButton, &QPushButton::released,
            this, &ExceptionDialog::onAddClicked);
    connect(removeButton, &QPushButton::released,
            this, &ExceptionDialog::onRemoveClicked);
    connect(table, &QTableWidget::itemSelectionChanged,
            this, &ExceptionDialog::onTableSelectionChanged);
    connect(table, &QTableWidget::cellDoubleClicked,
            this, &ExceptionDialog::onTableDoubleClicked);
    connect(leaveAddButton, &QPushButton::released,
            this, &ExceptionDialog::onAddLeaveClicked);
    connect(leaveRemoveButton, &QPushButton::released,
            this, &ExceptionDialog::onRemoveLeaveClicked);
    connect(leaveTable, &QTableWidget::itemSelectionChanged,
            this, &ExceptionDialog::onLeaveTableSelectionChanged);
    connect(leaveTable, &QTableWidget::cellDoubleClicked,
            this, &ExceptionDialog::onLeaveTableDoubleClicked);
    connect(selectAllButton, &QPushButton::released, this, [this] {
        for (int i = 0; i < studentList->count(); ++i) {
            studentList->item(i)->setCheckState(Qt::Checked);
        }
    });
    connect(selectNoneButton, &QPushButton::released, this, [this] {
        for (int i = 0; i < studentList->count(); ++i) {
            studentList->item(i)->setCheckState(Qt::Unchecked);
        }
    });
    connect(okButton, &QPushButton::released,
            this, &ExceptionDialog::onAcceptClicked);
    connect(cancelButton, &QPushButton::released,
            this, &ExceptionDialog::close);

    // Show the exceptions the owner already has, then fill the editors with
    // the global values of the currently selected period.
    populateStudentList();
    refreshTable();
    refreshLeaveTable();
    updateSplitEnabled();
    onTableSelectionChanged();
    onLeaveTableSelectionChanged();
}

ExceptionDialog::~ExceptionDialog() {}

void ExceptionDialog::refreshTable()
{
    sortExceptions(list);

    const int previousRow = table->currentRow();
    table->setRowCount(0);
    for (const auto &ex : list) {
        int r = table->rowCount();
        table->insertRow(r);
        table->setItem(r, 0, new QTableWidgetItem(
            ex.date.toString("yyyy-MM-dd")));
        table->setItem(r, 1, new QTableWidgetItem(
            signPeriodName(ex.period)));
        table->setItem(r, 2, new QTableWidgetItem(
            (ex.ignore || ex.period == 2 || !ex.split.isValid())
                ? QString("—")
                : ex.split.toString("HH:mm")));
        table->setItem(r, 3, new QTableWidgetItem(
            (ex.ignore || !ex.deadline.isValid())
                ? QString("—")
                : ex.deadline.toString("HH:mm")));
        table->setItem(r, 4, new QTableWidgetItem(
            ex.ignore ? QString("是") : QString("—")));
    }

    statusLabel->setText(QString("共 %1 条例外设置").arg(list.size()));

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
    if (row < 0 || row >= table->rowCount() || row >= list.size()) {
        return;
    }
    const PeriodException &ex = list[row];
    {
        // Set date/period/ignore without triggering updateSplitEnabled():
        // the stored times must win over the global defaults it loads.
        const QSignalBlocker blockPeriod(periodCombo);
        const QSignalBlocker blockIgnore(ignoreCheck);
        dateEdit->setDate(ex.date);
        periodCombo->setCurrentIndex(ex.period);
        ignoreCheck->setChecked(ex.ignore);
    }

    updateSplitEnabled();
    if (!ex.ignore) {
        if (ex.split.isValid()) {
            splitEdit->setTime(ex.split);
        }
        if (ex.deadline.isValid()) {
            deadlineEdit->setTime(ex.deadline);
        }
    }
}

void ExceptionDialog::updateSplitEnabled()
{
    const int period = periodCombo->currentIndex();

    // An ignored period has no boundary or deadline: it produces no Rewardly
    // rows at all, so both time inputs are disabled.
    if (ignoreCheck->isChecked()) {
        splitEdit->setEnabled(false);
        deadlineEdit->setEnabled(false);
        hintLabel->setText(
            QString("已忽略该时段：该日期该时段当天不生成任何加分/迟到/未签到记录，"
                    "无需设置分界时间或迟到时间。"));
        return;
    }

    deadlineEdit->setEnabled(true);

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
    candidate.ignore = ignoreCheck->isChecked();

    if (candidate.ignore) {
        // An ignored period has no boundary/deadline to store.
        candidate.split = QTime();
        candidate.deadline = QTime();
    } else {
        candidate.deadline = deadlineEdit->time();
        // 晚上 has no boundary after it; keep the value invalid so it is
        // ignored.
        candidate.split = (candidate.period == 2)
            ? QTime()
            : splitEdit->time();
    }

    QVector<PeriodException> next = list;
    upsert(next, candidate);

    ConfigData probe = ConfigManager::loadConfig();
    const QString error = ConfigManager::validateExceptions(probe, next);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QString("例外设置无效"), error,
                             QMessageBox::Ok);
        return;
    }

    list = next;
    refreshTable();

    // Highlight the row that was just added/updated.
    for (int r = 0; r < list.size(); ++r) {
        if (list[r].date == candidate.date
            && list[r].period == candidate.period) {
            selectRow(r);
            break;
        }
    }
}

void ExceptionDialog::onRemoveClicked()
{
    const int row = table->currentRow();
    if (row < 0 || row >= list.size()) {
        QMessageBox::information(this, QString("未选择例外"),
                                 QString("请先在列表中选择要删除的例外。"),
                                 QMessageBox::Ok);
        return;
    }

    const PeriodException removed = list[row];
    list.removeAt(row);
    refreshTable();
    statusLabel->setText(
        QString("已移除 %1 %2 的例外")
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

void ExceptionDialog::populateStudentList()
{
    studentList->clear();

    // The roster is the same global list the Rewardly window uses: index 0 is
    // a placeholder, and the rest are student names.
    QSet<QString> seen;
    for (int i = 1; i < static_cast<int>(extstunames.size()); ++i) {
        const QString &name = extstunames[i];
        if (name.isEmpty() || seen.contains(name)) {
            continue;
        }
        seen.insert(name);
        auto *item = new QListWidgetItem(name, studentList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

QStringList ExceptionDialog::checkedStudentNames() const
{
    QStringList names;
    for (int i = 0; i < studentList->count(); ++i) {
        const QListWidgetItem *item = studentList->item(i);
        if (item->checkState() == Qt::Checked) {
            names.append(item->text());
        }
    }
    return names;
}

void ExceptionDialog::setCheckedStudent(const QString &name)
{
    bool found = false;
    for (int i = 0; i < studentList->count(); ++i) {
        QListWidgetItem *item = studentList->item(i);
        if (item->text() == name) {
            item->setCheckState(Qt::Checked);
            found = true;
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }

    // A saved leave can name a student who has since left the roster; keep
    // it editable by adding a checked entry for them.
    if (!found && !name.isEmpty()) {
        auto *item = new QListWidgetItem(name, studentList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }
}

QSet<int> ExceptionDialog::checkedLeavePeriods() const
{
    QSet<int> periods;
    if (leaveMorningCheck->isChecked()) {
        periods.insert(0);
    }
    if (leaveNoonCheck->isChecked()) {
        periods.insert(1);
    }
    if (leaveEveningCheck->isChecked()) {
        periods.insert(2);
    }
    return periods;
}

void ExceptionDialog::refreshLeaveTable()
{
    sortLeaves(leaveList);

    const int previousRow = leaveTable->currentRow();
    leaveTable->setRowCount(0);
    for (const auto &leave : leaveList) {
        const int r = leaveTable->rowCount();
        leaveTable->insertRow(r);
        leaveTable->setItem(r, 0, new QTableWidgetItem(leave.name));
        leaveTable->setItem(r, 1, new QTableWidgetItem(
            leave.dateFrom.toString("yyyy-MM-dd")));
        leaveTable->setItem(r, 2, new QTableWidgetItem(
            leave.dateTo.toString("yyyy-MM-dd")));
        leaveTable->setItem(r, 3, new QTableWidgetItem(
            periodSetText(leave.periods)));
    }

    leaveStatusLabel->setText(
        QString("共 %1 条请假设置").arg(leaveList.size()));

    if (previousRow >= 0 && previousRow < leaveTable->rowCount()) {
        leaveTable->selectRow(previousRow);
    }
    onLeaveTableSelectionChanged();
}

void ExceptionDialog::selectLeaveRow(int row)
{
    if (row >= 0 && row < leaveTable->rowCount()) {
        leaveTable->selectRow(row);
        leaveTable->scrollToItem(leaveTable->item(row, 0));
    }
}

void ExceptionDialog::loadLeaveRowIntoEditors(int row)
{
    if (row < 0 || row >= leaveTable->rowCount() || row >= leaveList.size()) {
        return;
    }
    const StudentLeaveException &leave = leaveList[row];
    leaveStartEdit->setDate(leave.dateFrom);
    leaveEndEdit->setDate(leave.dateTo);
    leaveMorningCheck->setChecked(leave.periods.contains(0));
    leaveNoonCheck->setChecked(leave.periods.contains(1));
    leaveEveningCheck->setChecked(leave.periods.contains(2));
    setCheckedStudent(leave.name);
}

void ExceptionDialog::onAddLeaveClicked()
{
    const QDate from = leaveStartEdit->date();
    const QDate to = leaveEndEdit->date();
    const QSet<int> periods = checkedLeavePeriods();
    const QStringList names = checkedStudentNames();

    if (names.isEmpty()) {
        QMessageBox::warning(this, QString("未选择学生"),
                             QString("请先在名单中勾选请假的学生。"),
                             QMessageBox::Ok);
        return;
    }

    // Every checked student gets its own leave entry. Re-adding the same
    // student with the same date range updates that entry (periods included).
    QVector<StudentLeaveException> next = leaveList;
    for (const QString &name : names) {
        StudentLeaveException leave;
        leave.name = name;
        leave.dateFrom = from;
        leave.dateTo = to;
        leave.periods = periods;
        upsertLeave(next, leave);
    }

    const QString error = ConfigManager::validateStudentLeaves(next);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QString("请假设置无效"), error,
                             QMessageBox::Ok);
        return;
    }

    leaveList = next;
    refreshLeaveTable();

    // Highlight the first entry that was added/updated.
    for (int r = 0; r < leaveList.size(); ++r) {
        if (leaveList[r].name == names.first()
            && leaveList[r].dateFrom == from
            && leaveList[r].dateTo == to) {
            selectLeaveRow(r);
            break;
        }
    }
}

void ExceptionDialog::onRemoveLeaveClicked()
{
    const int row = leaveTable->currentRow();
    if (row < 0 || row >= leaveList.size()) {
        QMessageBox::information(this, QString("未选择请假设置"),
                                 QString("请先在列表中选择要删除的请假设置。"),
                                 QMessageBox::Ok);
        return;
    }

    const StudentLeaveException removed = leaveList[row];
    leaveList.removeAt(row);
    refreshLeaveTable();
    leaveStatusLabel->setText(
        QString("已移除 %1 的请假设置（%2 至 %3）")
            .arg(removed.name,
                 removed.dateFrom.toString("yyyy-MM-dd"),
                 removed.dateTo.toString("yyyy-MM-dd")));
}

void ExceptionDialog::onLeaveTableSelectionChanged()
{
    leaveRemoveButton->setEnabled(leaveTable->currentRow() >= 0);
}

void ExceptionDialog::onLeaveTableDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    loadLeaveRowIntoEditors(row);
}

void ExceptionDialog::onAcceptClicked()
{
    ConfigData config = ConfigManager::loadConfig();

    QString error = ConfigManager::validateExceptions(config, list);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QString("例外设置无效"), error,
                             QMessageBox::Ok);
        return;
    }

    error = ConfigManager::validateStudentLeaves(leaveList);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QString("请假设置无效"), error,
                             QMessageBox::Ok);
        return;
    }

    // One-off setting: hand the lists to the owning window (in memory only)
    // and close. Nothing is written to config.toml.
    emit applied(list, leaveList);
    close();
}
