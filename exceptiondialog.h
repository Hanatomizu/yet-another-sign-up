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
#include <QTimeEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>

#include "configmanager.h"

/**
 * @brief Dialog for managing per-date period exceptions (设置例外).
 *
 * Lets the user pick a date and one of the three periods (早上/中午/晚上)
 * and give that period a different 分界时间 (the time at which the period
 * ends) and 迟到时间 (deadline). Entries are kept in a working list shown
 * in a table; 「保存」 writes them to config.toml as [[exceptions]].
 *
 * 晚上 is the last period of the day, so it has no following boundary:
 * its 分界时间 input is disabled and only the 迟到时间 applies.
 */
class ExceptionDialog : public QWidget
{
    Q_OBJECT

public:
    explicit ExceptionDialog(QWidget *parent = nullptr);
    ~ExceptionDialog();

private slots:
    void onPeriodChanged();
    void onAddClicked();
    void onRemoveClicked();
    void onTableSelectionChanged();
    void onTableDoubleClicked(int row, int column);
    void onSaveClicked();

private:
    QDateEdit *dateEdit;
    QComboBox *periodCombo;
    QTimeEdit *splitEdit;
    QTimeEdit *deadlineEdit;
    QLabel *hintLabel;
    QTableWidget *table;
    QLabel *statusLabel;
    QPushButton *addButton;
    QPushButton *removeButton;

    /// Working copy of the exception list; only written out on 保存.
    QVector<PeriodException> exceptions;

    void loadExceptions();
    void refreshTable();
    void selectRow(int row);
    void loadRowIntoEditors(int row);
    void updateSplitEnabled();
};

#endif // EXCEPTIONDIALOG_H
