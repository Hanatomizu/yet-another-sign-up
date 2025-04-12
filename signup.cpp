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

int Yasu::initConfigFiles(){

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
        Yasu::stunames[i] = in.readLine();
    }
    qDebug() << "Read namelist Finished\n";
    return 0;
}
int Yasu::sign_up(QString s) {
    return -1;
}
