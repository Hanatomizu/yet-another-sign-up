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

#include <string>
#include <QString>
#include <QFile>
#include <QDir>
#include <QIODevice>
#include <QTextStream>
#include <QDebug>
#include <QVector>
#include <QDateTime>
#include <QCoreApplication>

#ifndef SIGNUP_H
#define SIGNUP_H

class Yasu{
private:
    struct Student{
        int id;
        QString name;
    };
    struct SignUpTime{
        int id;
        QString name;
        QString stime;
        SignUpTime(int _id, QString _name, QString _stime):
            id(_id),
            name(_name),
            stime(_stime){}
    };
public:


    // Directory
    QString mainConfigDirectory;
    // Directory for namelist file
    QString namelistDirectory;
    // Students' counts
    int studentcnts;
    // Student Sign up Status
    Student stu[100];

    // Current Date Logs File
    QString logFilePath;
    QString dataFilePath;

    // Storages
    QVector<SignUpTime> signups;
    QVector<SignUpTime> lates;

    int initConfigFiles();

    // init QString Names
    int initNamelist();

    QPair<int, QString> sign_up(QString);
    };

#endif // SIGNUP_H
