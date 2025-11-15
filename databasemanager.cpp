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

#include "databasemanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

DatabaseManager::DatabaseManager()
    : QObject()
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initializeDatabase()
{
    // Create database connection with unique name
    static int connectionCounter = 0;
    QString connectionName = QString("yasu_connection_%1").arg(connectionCounter++);
    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    
    // Set database file path in application directory
    QString dbPath = QCoreApplication::applicationDirPath() + QDir::separator() + "yasu.db";
    m_db.setDatabaseName(dbPath);
    
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qDebug() << "Failed to open database:" << m_lastError;
        return false;
    }
    
    qDebug() << "Database initialized successfully at:" << dbPath;
    return true;
}

bool DatabaseManager::createTables()
{
    QSqlQuery query(m_db);
    
    // Create students table
    QString createStudentsTable = R"(
        CREATE TABLE IF NOT EXISTS students (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE
        )
    )";
    
    if (!query.exec(createStudentsTable)) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to create students table:" << m_lastError;
        return false;
    }
    
    // Create signups table
    QString createSignupsTable = R"(
        CREATE TABLE IF NOT EXISTS signups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            student_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            timestamp DATETIME NOT NULL,
            date DATE NOT NULL,
            period INTEGER NOT NULL,  -- 0 = morning, 1 = afternoon, 2 = evening
            FOREIGN KEY (student_id) REFERENCES students (id)
        )
    )";
    
    if (!query.exec(createSignupsTable)) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to create signups table:" << m_lastError;
        return false;
    }
    
    // Create indexes for better performance
    QString createStudentIndex = "CREATE INDEX IF NOT EXISTS idx_student_id ON signups (student_id)";
    if (!query.exec(createStudentIndex)) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to create student index:" << m_lastError;
        return false;
    }
    
    QString createDateIndex = "CREATE INDEX IF NOT EXISTS idx_date ON signups (date)";
    if (!query.exec(createDateIndex)) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to create date index:" << m_lastError;
        return false;
    }
    
    qDebug() << "Database tables created successfully";
    return true;
}

bool DatabaseManager::addStudent(int id, const QString &name)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO students (id, name) VALUES (?, ?)");
    query.addBindValue(id);
    query.addBindValue(name);
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to add student:" << m_lastError;
        return false;
    }
    
    return true;
}

bool DatabaseManager::isStudentExists(int id)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM students WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to check student existence:" << m_lastError;
        return false;
    }
    
    if (query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

QString DatabaseManager::getStudentName(int id)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT name FROM students WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to get student name:" << m_lastError;
        return QString();
    }
    
    if (query.next()) {
        return query.value(0).toString();
    }
    
    return QString();
}

int DatabaseManager::getStudentCount()
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM students");
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to get student count:" << m_lastError;
        return -1;
    }
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    return -1;
}

bool DatabaseManager::recordSignUp(int studentId, const QString &name, const QDateTime &timestamp)
{
    // Determine the period based on time
    int period = 0; // 0 = morning (0:00-11:00), 1 = afternoon (11:00-16:00), 2 = evening (16:00-23:59)
    QTime time = timestamp.time();
    if (time >= QTime(11, 0) && time < QTime(16, 0)) {
        period = 1; // afternoon
    } else if (time >= QTime(16, 0)) {
        period = 2; // evening
    }
    // else period = 0 (morning, default)
    
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO signups (student_id, name, timestamp, date, period) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(studentId);
    query.addBindValue(name);
    query.addBindValue(timestamp);
    query.addBindValue(timestamp.date());
    query.addBindValue(period);
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to record signup:" << m_lastError;
        return false;
    }
    
    return true;
}

bool DatabaseManager::isStudentSignedUp(int studentId, const QDate &date)
{
    // Determine the period based on current time
    QDateTime currentDateTime = QDateTime::currentDateTime();
    int period = 0; // 0 = morning (0:00-11:00), 1 = afternoon (11:00-16:00), 2 = evening (16:00-23:59)
    QTime time = currentDateTime.time();
    if (time >= QTime(11, 0) && time < QTime(16, 0)) {
        period = 1; // afternoon
    } else if (time >= QTime(16, 0)) {
        period = 2; // evening
    }
    // else period = 0 (morning, default)
    
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM signups WHERE student_id = ? AND date = ? AND period = ?");
    query.addBindValue(studentId);
    query.addBindValue(date);
    query.addBindValue(period);
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to check student signup status:" << m_lastError;
        return false;
    }
    
    if (query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

QVector<QPair<QString, QDateTime>> DatabaseManager::getSignUpsForDate(const QDate &date)
{
    QVector<QPair<QString, QDateTime>> result;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT name, timestamp FROM signups WHERE date = ? ORDER BY timestamp");
    query.addBindValue(date);
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to get signups for date:" << m_lastError;
        return result;
    }
    
    while (query.next()) {
        QString name = query.value(0).toString();
        QDateTime timestamp = query.value(1).toDateTime();
        result.append(qMakePair(name, timestamp));
    }
    
    return result;
}

QVector<QPair<QString, QDateTime>> DatabaseManager::getSignUpsForDateAndPeriod(const QDate &date, int period)
{
    QVector<QPair<QString, QDateTime>> result;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT name, timestamp FROM signups WHERE date = ? AND period = ? ORDER BY timestamp");
    query.addBindValue(date);
    query.addBindValue(period);
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to get signups for date and period:" << m_lastError;
        return result;
    }
    
    while (query.next()) {
        QString name = query.value(0).toString();
        QDateTime timestamp = query.value(1).toDateTime();
        result.append(qMakePair(name, timestamp));
    }
    
    return result;
}

QVector<QPair<QString, QDateTime>> DatabaseManager::getSignUpsForStudent(int studentId)
{
    QVector<QPair<QString, QDateTime>> result;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT name, timestamp FROM signups WHERE student_id = ? ORDER BY timestamp");
    query.addBindValue(studentId);
    
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Failed to get signups for student:" << m_lastError;
        return result;
    }
    
    while (query.next()) {
        QString name = query.value(0).toString();
        QDateTime timestamp = query.value(1).toDateTime();
        result.append(qMakePair(name, timestamp));
    }
    
    return result;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}
