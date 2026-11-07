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

#ifndef EXCEPTIONDIALOG_H
#define EXCEPTIONDIALOG_H

#include <QWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTimeEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QListWidget>
#include <QStringList>

#include "configmanager.h"

/**
 * @brief Window for managing one-off per-date period exceptions (设置例外).
 *
 * Lets the user pick a date and one of the three periods (早上/中午/晚上)
 * and give that period a different 分界时间 (the time at which the period
 * ends) and 迟到时间 (deadline). Entries are kept in a working list shown
 * in a table.
 *
 * The exceptions are a one-off, in-memory setting: 「确定」 emits applied()
 * with the edited list and closes the window, 「取消」 discards the edits.
 * Nothing is written to config.toml — the list only lives as long as the
 * window that owns it (the 「导出到 Rewardly」 window).
 *
 * 晚上 is the last period of the day, so it has no following boundary:
 * its 分界时间 input is disabled and only the 迟到时间 applies.
 *
 * A period can also be ignored entirely (忽略该时段) for that date: the
 * ignored period produces no Rewardly rows at all — no 早上签到 bonus, no
 * 迟到 and no 未签到 deduction. The 分界时间/迟到时间 inputs are disabled
 * in that case and the entry stores no times.
 *
 * The window also manages per-student leave exceptions (按人请假) on a
 * second tab: pick a date range, one or more periods and any number of
 * students from the roster. A student on leave gets no 迟到 and no 未签到
 * deduction in the selected periods (a timely 早上签到 bonus is kept).
 */
class ExceptionDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ExceptionDialog(
        const QVector<PeriodException> &initial,
        const QVector<StudentLeaveException> &initialLeaves = {},
        QWidget *parent = nullptr);
    ~ExceptionDialog();

    /// Period exceptions currently listed in the table.
    QVector<PeriodException> exceptions() const { return list; }

    /// Per-student leave exceptions currently listed in the table.
    QVector<StudentLeaveException> studentLeaves() const { return leaveList; }

public slots:
    // Public so tests can drive them directly. MSVC mangles member access
    // into symbol names, so a test TU that re-exposes private members via
    // `#define private public` cannot link against private functions.
    void onAddClicked();
    void onRemoveClicked();
    void onAddLeaveClicked();
    void onRemoveLeaveClicked();
    void onAcceptClicked();

signals:
    /**
     * @brief Emitted right before the window closes with the edited lists.
     *
     * Only 确定 emits this; closing with 取消 leaves the owner's lists as-is.
     */
    void applied(const QVector<PeriodException> &exceptions,
                 const QVector<StudentLeaveException> &studentLeaves);

private slots:
    void onPeriodChanged();
    void onTableSelectionChanged();
    void onTableDoubleClicked(int row, int column);
    void onLeaveTableSelectionChanged();
    void onLeaveTableDoubleClicked(int row, int column);

private:
    // --- 时段例外 tab ---
    QDateEdit *dateEdit;
    QComboBox *periodCombo;
    QCheckBox *ignoreCheck;
    QTimeEdit *splitEdit;
    QTimeEdit *deadlineEdit;
    QLabel *hintLabel;
    QTableWidget *table;
    QLabel *statusLabel;
    QPushButton *addButton;
    QPushButton *removeButton;

    // --- 按人请假 tab ---
    QDateEdit *leaveStartEdit;
    QDateEdit *leaveEndEdit;
    QCheckBox *leaveMorningCheck;
    QCheckBox *leaveNoonCheck;
    QCheckBox *leaveEveningCheck;
    QListWidget *studentList;
    QTableWidget *leaveTable;
    QLabel *leaveStatusLabel;
    QPushButton *leaveAddButton;
    QPushButton *leaveRemoveButton;

    /// Working copy of the period exception list; handed out via applied().
    QVector<PeriodException> list;

    /// Working copy of the per-student leave list; handed out via applied().
    QVector<StudentLeaveException> leaveList;

    void refreshTable();
    void selectRow(int row);
    void loadRowIntoEditors(int row);
    void updateSplitEnabled();

    void populateStudentList();
    QStringList checkedStudentNames() const;
    void setCheckedStudent(const QString &name);
    QSet<int> checkedLeavePeriods() const;
    void refreshLeaveTable();
    void selectLeaveRow(int row);
    void loadLeaveRowIntoEditors(int row);
};

#endif // EXCEPTIONDIALOG_H
