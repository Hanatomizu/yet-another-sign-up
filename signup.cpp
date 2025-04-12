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
int qstringToInt(QString s) {
    int res = 0;
    for (auto i : s) {
        if (isdigit(i)) {
            res = res*10 + i - '0';
        } else {
            return -1;
        }
    }
    return 0;
}
}

int Yasu::initConfigFiles(){
    QFile mainConfig(Yasu::mainConfigDirectory);
    QTextStream in(&mainConfig);
    while (!in.atEnd()) {
        QString key = in.readLine();
        QString val = in.readLine();
        if (key == "[namelistDirectory]") {
            Yasu::namelistDirectory = val;
        } else if (key == "[memberNumber]") {
            Yasu::studentcnts = SignUpAlgorithms::qstringToInt(val);
        }
    }

    // init date time log file

    QDateTime curtime = QDateTime::currentDateTime();
    Yasu::logFilePath = "./logs/" + curtime.toString("yyyy-MM-dd.log");
    Yasu::dataFilePath = "./data/" + curtime.toString("yyyy-MM-dd.data");


    mainConfig.close();
    return 0;
}
int Yasu::initNamelist(){
    QFile file(Yasu::namelistDirectory);
    if (!QFile::exists(Yasu::namelistDirectory)) {
        return -1;
    }
    // bool ok = file.open(QIODevice::ReadOnly);
    // file.close();
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    for (int i = 1; !in.atEnd(); ++i) {
        Yasu::stu[i].id = i, Yasu::stu[i].name = in.readLine();
    }
    qDebug() << "Read namelist Finished\n";
    file.close();
    return 0;
}



QPair<int, QString> Yasu::sign_up(QString s) {
    int number = SignUpAlgorithms::qstringToInt(s);
    if (number == -1) return qMakePair(-1, "");
    QDateTime curtime = QDateTime::currentDateTime();
    Yasu::signups.push_back(
        Yasu::SignUpTime(
            number,
            stu[number].name,
            curtime.toString("yyyy.MM.dd hh:mm::ss")
        )
    );

    QFile logs(Yasu::logFilePath);
    logs.open(QIODevice::WriteOnly | QIODevice::Text);



    logs.close();

    return qMakePair(0, stu[number].name);
}
