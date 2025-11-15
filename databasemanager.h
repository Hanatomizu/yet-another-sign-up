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

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QString>
#include <QVector>
#include <QPair>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    DatabaseManager();
    ~DatabaseManager();

    bool initializeDatabase();
    bool createTables();
    
    // Student management
    bool addStudent(int id, const QString &name);
    bool isStudentExists(int id);
    QString getStudentName(int id);
    int getStudentCount();
    
    // Sign up management
    bool recordSignUp(int studentId, const QString &name, const QDateTime &timestamp);
    bool isStudentSignedUp(int studentId, const QDate &date);
    QVector<QPair<QString, QDateTime>> getSignUpsForDate(const QDate &date);
    QVector<QPair<QString, QDateTime>> getSignUpsForDateAndPeriod(const QDate &date, int period);
    QVector<QPair<QString, QDateTime>> getSignUpsForStudent(int studentId);
    
    QString lastError() const;

private:
    QSqlDatabase m_db;
    QString m_lastError;
};

#endif // DATABASEMANAGER_H
