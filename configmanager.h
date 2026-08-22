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

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QTime>

/**
 * @brief Holds all application configuration loaded from config.toml.
 */
struct ConfigData {
    QString namelistDirectory;
    QTime morningDeadline;
    QTime noonDeadline;
    QTime eveningDeadline;
    QTime morningNoonSplit;
    QTime noonEveningSplit;

    // --- Rewardly scoring configuration ([rewardly] section) ---
    // Deductions are stored as positive numbers; the export applies them
    // as negative points.
    int morningSignBonusCount;   // 早上签到加分人数
    int morningSignBonusScore;   // 早上签到加分分数
    int morningLateDeduction;    // 早上迟到扣分分数
    int morningAbsentDeduction;  // 早上未签到扣分分数
    int noonLateDeduction;       // 中午迟到扣分分数
    int noonAbsentDeduction;     // 中午未签到扣分分数
    int eveningLateDeduction;    // 晚上迟到扣分分数
    int eveningAbsentDeduction;  // 晚上未签到扣分分数
};

/**
 * @brief Centralized configuration manager using TOML format.
 *
 * Reads/writes config.toml in the application directory.
 * Provides default values when the config file does not exist.
 * Handles migration from the legacy key-value config format.
 */
class ConfigManager
{
public:
    /**
     * @brief Load configuration from config.toml.
     *
     * If config.toml does not exist, attempts migration from the
     * legacy 'config' file. Falls back to defaults if neither exists.
     *
     * @return ConfigData with all settings populated.
     */
    static ConfigData loadConfig();

    /**
     * @brief Save configuration to config.toml.
     * @param config The configuration data to persist.
     */
    static void saveConfig(const ConfigData &config);

    /**
     * @brief Get the full path to config.toml.
     * @return Absolute path to the TOML configuration file.
     */
    static QString configFilePath();

    /**
     * @brief Get default configuration values.
     * @return ConfigData populated with sensible defaults.
     */
    static ConfigData defaultConfig();

private:
    /**
     * @brief Attempt to migrate namelist directory from legacy 'config' file.
     * @return The namelist directory string, or empty QString if not found.
     */
    static QString migrateLegacyNamelist();
};

#endif // CONFIGMANAGER_H
