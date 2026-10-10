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
#include <QDate>
#include <QVector>

/**
 * @brief One per-day period exception (设置例外).
 *
 * For a single date and period, replaces the period boundary and/or the
 * period deadline that would otherwise come from the global [periods]
 * configuration.
 *
 * Exceptions are a one-off, in-memory setting: they are never written to
 * config.toml and only live as long as the caller holds them (the
 * 「导出到 Rewardly」 window keeps its own list).
 *
 * - period 0 (早上): split overrides morning_noon_split,
 *                    deadline overrides morning_deadline
 * - period 1 (中午): split overrides noon_evening_split,
 *                    deadline overrides noon_deadline
 * - period 2 (晚上): deadline overrides evening_deadline;
 *                    split is unused — evening is the last period of the
 *                    day, so it has no following boundary.
 *
 * The @c split is therefore "the time at which this period ends and the
 * next one begins". An invalid QTime means "keep the global value".
 */
struct PeriodException {
    QDate date;
    int period = 0;   // 0 = morning, 1 = noon, 2 = evening
    QTime split;
    QTime deadline;

    bool operator==(const PeriodException &other) const
    {
        return date == other.date
            && period == other.period
            && split == other.split
            && deadline == other.deadline;
    }
};

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
    // Scores are signed decimals (2 decimal places):
    // positive = 加分, negative = 扣分.
    int morningSignBonusCount;   // 早上签到加分人数
    double morningSignBonusScore;   // 早上签到加分分数
    double morningLateDeduction;    // 早上迟到扣分分数
    double morningAbsentDeduction;  // 早上未签到扣分分数
    double noonLateDeduction;       // 中午迟到扣分分数
    double noonAbsentDeduction;     // 中午未签到扣分分数
    double eveningLateDeduction;    // 晚上迟到扣分分数
    double eveningAbsentDeduction;  // 晚上未签到扣分分数
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

    /**
     * @brief Build the effective configuration for one specific date.
     *
     * Returns a copy of @p config with every exception in @p exceptions
     * that matches @p date applied (period boundaries and deadlines
     * overridden). Exceptions are not part of config.toml, so the caller
     * passes its own (in-memory) list.
     *
     * Other dates are untouched, so callers must ask for the effective
     * config of the day they are parsing.
     *
     * @param config Base configuration loaded from config.toml.
     * @param date The day the returned configuration applies to.
     * @param exceptions Per-date overrides to apply (may be empty).
     * @return Configuration with that day's exceptions applied.
     */
    static ConfigData forDate(const ConfigData &config, const QDate &date,
                              const QVector<PeriodException> &exceptions);

    /**
     * @brief Validate a list of period exceptions.
     *
     * Checks that every entry has a valid date and period, that a period's
     * 迟到时间 (deadline) is not later than its 分界时间 (split), and that
     * on any given date the 早/午分界 does not come after the 午/晚分界.
     *
     * @param config Base configuration (global boundaries/deadlines).
     * @param exceptions The (in-memory) exception list to check.
     * @return Empty string when valid, otherwise a Chinese error message.
     */
    static QString validateExceptions(const ConfigData &config,
                                      const QVector<PeriodException> &exceptions);

private:
    /**
     * @brief Attempt to migrate namelist directory from legacy 'config' file.
     * @return The namelist directory string, or empty QString if not found.
     */
    static QString migrateLegacyNamelist();
};

#endif // CONFIGMANAGER_H
