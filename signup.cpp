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


#include "signup.h"

namespace SignUpAlgorithms{
int qstringToInt(QString _s) {
    int res = 0;
    std::string s = _s.toStdString();
    for (auto i : s) {
        if (isdigit(i)) {
            res = res*10 + i - '0';
        } else {
            return -1;
        }
    }
    return res;
}
}

int Yasu::initConfigFiles(){
    Yasu::mainConfigDirectory = QDir::cleanPath(
        QCoreApplication::applicationDirPath() +
        QDir::separator()+
        QString("config")
    );
    if (!QFile::exists(Yasu::mainConfigDirectory)) {
        // Load Default Config
        qDebug() << QString("Main Config File Does not Exist!!!") << '\n';
        Yasu::namelistDirectory = QDir::cleanPath(
            QCoreApplication::applicationDirPath() +
            QDir::separator()+
            QString("names")
        );

        Yasu::studentcnts = 50;
    } else {
        QFile mainConfig(Yasu::mainConfigDirectory);
        if (mainConfig.open(QIODevice::ReadOnly | QIODevice::Text) ){
            QTextStream in(&mainConfig);
            while (!in.atEnd()) {
                QString key = in.readLine();
                QString val = in.readLine();
                if (key == "[namelistDirectory]") {
                    Yasu::namelistDirectory = QDir::cleanPath(
                        QCoreApplication::applicationDirPath() +
                        QDir::separator() +
                        val
                    );
                }
            }
            mainConfig.close();
        } else {
            qDebug() << "Failed to read config File: " << mainConfigDirectory;
            qDebug() << mainConfig.errorString() << '\n';
        }
    }

    // init date time log file

    QDateTime curtime = QDateTime::currentDateTime();
    Yasu::logFilePath = QCoreApplication::applicationDirPath() +
                        QDir::separator() +
                        QString("logs") +
                        QDir::separator() +
                        curtime.toString("yyyy-MM-dd.log");
    Yasu::dataFilePath =
                        QCoreApplication::applicationDirPath() +
                        QDir::separator();
                        QString("data") +
                        QDir::separator() +
                        curtime.toString("yyyy-MM-dd.data");

    QString logDirPath =
                        QCoreApplication::applicationDirPath() +
                        QDir::separator() +
                        QString("logs");
    QString dataDirPath =
                        QCoreApplication::applicationDirPath() +
                        QDir::separator() +
                        QString("data");

    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        logDir.mkdir(logDirPath);
        qDebug() << "Log Directory Created!";
    }
    QDir dataDir(dataDirPath);
    if (!dataDir.exists()) {
        dataDir.mkdir(dataDirPath);
        qDebug() << "Data Directory Created!";
    }

    if (!QFile::exists(logFilePath)) {
        QFile logFile(logFilePath);
        if(logFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            logFile.write((QString("- ") + curtime.toString("yyyy-MM-dd") + QString(" yasu created this file\n")).toUtf8());
        } else {
            qDebug() << "Error: log file cannot be created" << logFile.errorString();
        }
        logFile.close();
    } else {
        QFile logFile(logFilePath);
        if(logFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
            logFile.write((QString("= ") + curtime.toString("yyyy-MM-dd") + QString(" yasu rechecked this file\n")).toUtf8());
        } else {
            qDebug() << "Error: log file cannot be rechecked" << logFile.errorString();
        }
        logFile.close();
    }

    if (!QFile::exists(dataFilePath)) {
        QFile dataFile(dataFilePath);
        if (dataFile.open(QIODevice::ReadWrite | QIODevice::Text)){
            dataFile.write(curtime.toString("- yyyy-MM-dd yasu created this file\n").toUtf8());
        } else {
            qDebug() << "Error: data file cannot be created" << dataFile.errorString();
        }
        dataFile.close();
    }

    return 0;
}
int Yasu::initNamelist(){
    QFile file(Yasu::namelistDirectory);
    if (!QFile::exists(Yasu::namelistDirectory)) {
        qDebug() << "Name List File Does not Exists" << '\n';
        return -1;
    }
    // bool ok = file.open(QIODevice::ReadWrite);
    // file.close();
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream in(&file);
        // take the place of zero.
        extstunames.push_back("");
        for (Yasu::studentcnts = 1; !in.atEnd(); ++Yasu::studentcnts) {
            Yasu::stu[Yasu::studentcnts].id = Yasu::studentcnts;
            Yasu::stu[Yasu::studentcnts].name = in.readLine();
            extstunames.push_back( Yasu::stu[Yasu::studentcnts].name );
            nti[Yasu::stu[Yasu::studentcnts].name] = Yasu::studentcnts;
            Yasu::isSigned[Yasu::studentcnts] = 0;
        }
        --Yasu::studentcnts;
        qDebug() << "Read namelist Finished\n";
        file.close();
    } else {
        qDebug() << "Error on reading namelist!\n";
    }
    return 0;
}

int Yasu::writeLog(QString _contents){
    QFile logs(Yasu::logFilePath);
    int stat = 1;
    if (logs.open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)) {
        QTextStream log(&logs);
        log << _contents;
        stat = 0;
    } else {
        qDebug() << "Failed to append the file: " << logs.errorString();
        stat = -1;
    }
    logs.close();
    return stat;
}

QPair<int, QString> Yasu::sign_up(QString s) {
    int number = SignUpAlgorithms::qstringToInt(s);
    qDebug() << "New Sign Up Request Received, Number: " << number;
    if (number == -1) {
        qDebug() << "Error: Illeagl Input!";
        return qMakePair(-1, QString());
    }
    if (number < 1 || number > studentcnts) {
        qDebug() << "Error: Number Out of Range.";
        return qMakePair(-1, QString());
    }

    QDateTime curtime = QDateTime::currentDateTime();

    if (Yasu::isSigned[number]) {
        writeLog((curtime.toString("yyyy.MM.dd hh:mm:ss") + ", " + stu[number].name + ", Resigned\n"));
        return qMakePair(2, QString());
    } else {
        isSigned[number] = 1;
    }

    Yasu::signups.push_back(
        Yasu::SignUpTime(
            number,
            stu[number].name,
            curtime.toString("yyyy.MM.dd hh:mm::ss")
        )
    );

    writeLog((curtime.toString("yyyy.MM.dd hh:mm:ss") + ", " + stu[number].name + ", Signed\n"));

    return qMakePair(0, stu[number].name);
}
