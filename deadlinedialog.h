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

#ifndef DEADLINEDIALOG_H
#define DEADLINEDIALOG_H

#include <QWidget>
#include <QTimeEdit>
#include <QPushButton>

/**
 * @brief Dialog for configuring sign-in deadlines and period boundaries.
 *
 * Provides five QTimeEdit controls:
 * - Morning deadline (早签截止)
 * - Morning→Noon split (早/午分界)
 * - Noon deadline (午签截止)
 * - Noon→Evening split (午/晚分界)
 * - Evening deadline (晚签截止)
 *
 * Values are saved to config.toml via ConfigManager.
 */
class DeadlineDialog : public QWidget
{
    Q_OBJECT

public:
    explicit DeadlineDialog(QWidget *parent = nullptr);
    ~DeadlineDialog();

private slots:
    void onSaveClicked();

private:
    QTimeEdit *morningDeadlineEdit;
    QTimeEdit *noonDeadlineEdit;
    QTimeEdit *eveningDeadlineEdit;
    QTimeEdit *morningNoonSplitEdit;
    QTimeEdit *noonEveningSplitEdit;

    void loadCurrentConfig();
};

#endif // DEADLINEDIALOG_H
