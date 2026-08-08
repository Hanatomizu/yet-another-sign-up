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

#include "configmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>

#include <toml++/toml.hpp>

#include <fstream>

ConfigData ConfigManager::loadConfig()
{
    ConfigData config = defaultConfig();
    QString configPath = configFilePath();

    // Try to migrate legacy config for namelist directory
    QString legacyNamelist = migrateLegacyNamelist();
    if (!legacyNamelist.isEmpty()) {
        config.namelistDirectory = legacyNamelist;
    }

    if (!QFile::exists(configPath)) {
        qDebug() << "config.toml not found, creating with defaults...";
        saveConfig(config);
        return config;
    }

    try {
        toml::table tbl = toml::parse_file(configPath.toStdString());

        // Read [namelist] section
        if (tbl.contains("namelist")) {
            auto namelist = *tbl["namelist"].as_table();
            if (namelist.contains("directory")) {
                config.namelistDirectory = QString::fromStdString(
                    namelist["directory"].value_or<std::string>("names")
                );
            }
        }

        // Read [periods] section
        if (tbl.contains("periods")) {
            auto periods = *tbl["periods"].as_table();

            auto readTime = [&](const char *key, QTime &target) {
                if (periods.contains(key)) {
                    QString timeStr = QString::fromStdString(
                        periods[key].value_or<std::string>("")
                    );
                    QTime parsed = QTime::fromString(timeStr, "HH:mm");
                    if (parsed.isValid()) {
                        target = parsed;
                    }
                }
            };

            readTime("morning_deadline", config.morningDeadline);
            readTime("noon_deadline", config.noonDeadline);
            readTime("evening_deadline", config.eveningDeadline);
            readTime("morning_noon_split", config.morningNoonSplit);
            readTime("noon_evening_split", config.noonEveningSplit);
        }

        qDebug() << "Config loaded from" << configPath;
    } catch (const toml::parse_error &e) {
        qDebug() << "Failed to parse config.toml:" << e.what()
                 << "\nUsing defaults.";
        saveConfig(config);
    } catch (const std::exception &e) {
        qDebug() << "Unexpected error reading config.toml:" << e.what()
                 << "\nUsing defaults.";
        saveConfig(config);
    }

    return config;
}

void ConfigManager::saveConfig(const ConfigData &config)
{
    QString configPath = configFilePath();

    // Ensure directory exists
    QDir().mkpath(QFileInfo(configPath).absolutePath());

    toml::table tbl;

    // [namelist] section
    toml::table namelistTbl;
    namelistTbl.insert("directory", config.namelistDirectory.toStdString());
    tbl.insert("namelist", namelistTbl);

    // [periods] section
    toml::table periodsTbl;
    periodsTbl.insert("morning_deadline",
                      config.morningDeadline.toString("HH:mm").toStdString());
    periodsTbl.insert("noon_deadline",
                      config.noonDeadline.toString("HH:mm").toStdString());
    periodsTbl.insert("evening_deadline",
                      config.eveningDeadline.toString("HH:mm").toStdString());
    periodsTbl.insert("morning_noon_split",
                      config.morningNoonSplit.toString("HH:mm").toStdString());
    periodsTbl.insert("noon_evening_split",
                      config.noonEveningSplit.toString("HH:mm").toStdString());
    tbl.insert("periods", periodsTbl);

    std::ofstream file(configPath.toStdString());
    if (file.is_open()) {
        file << tbl;
        file.close();
        qDebug() << "Config saved to" << configPath;
    } else {
        qDebug() << "Failed to open config.toml for writing:" << configPath;
    }
}

QString ConfigManager::configFilePath()
{
    return QDir::cleanPath(
        QCoreApplication::applicationDirPath() +
        QDir::separator() +
        QString("config.toml")
    );
}

ConfigData ConfigManager::defaultConfig()
{
    ConfigData config;
    config.namelistDirectory = QString("names");
    config.morningDeadline = QTime(9, 0);
    config.noonDeadline = QTime(12, 30);
    config.eveningDeadline = QTime(18, 0);
    config.morningNoonSplit = QTime(12, 0);
    config.noonEveningSplit = QTime(17, 0);
    return config;
}

QString ConfigManager::migrateLegacyNamelist()
{
    QString legacyPath = QDir::cleanPath(
        QCoreApplication::applicationDirPath() +
        QDir::separator() +
        QString("config")
    );

    if (!QFile::exists(legacyPath)) {
        return QString();
    }

    QFile legacyFile(legacyPath);
    if (!legacyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&legacyFile);
    QString namelistDir;
    while (!in.atEnd()) {
        QString key = in.readLine();
        QString val = in.readLine();
        if (key == "[namelistDirectory]") {
            namelistDir = val.trimmed();
        }
    }
    legacyFile.close();

    if (!namelistDir.isEmpty()) {
        qDebug() << "Migrated namelist directory from legacy config:" << namelistDir;
    }

    return namelistDir;
}
