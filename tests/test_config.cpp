/**
 * Unit tests for ConfigManager (config.toml read/write + legacy migration).
 *
 * NOTE: ConfigManager resolves config.toml relative to the application
 * directory, so these tests operate on files next to the test binary and
 * clean them up in init()/cleanup().
 */

#include <QtTest>
#include <QFile>
#include <QDir>
#include <QTextStream>

#include "configmanager.h"

class TestConfig : public QObject
{
    Q_OBJECT

private:
    QString appDir() const { return QCoreApplication::applicationDirPath(); }
    QString legacyConfigPath() const
    {
        return QDir::cleanPath(appDir() + QString("/config"));
    }

private slots:
    void init();
    void cleanup();

    void defaultConfigValues();
    void saveAndLoadRoundtrip();
    void loadCreatesDefaultFileWhenMissing();
    void loadPartialFileKeepsDefaults();
    void loadInvalidTomlFallsBackToDefaults();
    void migratesLegacyNamelist();
    void configFilePathPointsToToml();

    void exceptionsAreNotPersisted();
    void forDateAppliesExceptions();
    void validateExceptionsChecksOrdering();
};

void TestConfig::init()
{
    QFile::remove(ConfigManager::configFilePath());
    QFile::remove(legacyConfigPath());
}

void TestConfig::cleanup()
{
    QFile::remove(ConfigManager::configFilePath());
    QFile::remove(legacyConfigPath());
}

void TestConfig::defaultConfigValues()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    QCOMPARE(cfg.namelistDirectory, QString("names"));
    QCOMPARE(cfg.morningDeadline, QTime(9, 0));
    QCOMPARE(cfg.noonDeadline, QTime(12, 30));
    QCOMPARE(cfg.eveningDeadline, QTime(18, 0));
    QCOMPARE(cfg.morningNoonSplit, QTime(12, 0));
    QCOMPARE(cfg.noonEveningSplit, QTime(17, 0));

    QCOMPARE(cfg.morningSignBonusCount, 10);
    QCOMPARE(cfg.morningSignBonusScore, 2.0);
    QCOMPARE(cfg.morningLateDeduction, -1.0);
    QCOMPARE(cfg.morningAbsentDeduction, -2.0);
    QCOMPARE(cfg.noonLateDeduction, -1.0);
    QCOMPARE(cfg.noonAbsentDeduction, -2.0);
    QCOMPARE(cfg.eveningLateDeduction, -1.0);
    QCOMPARE(cfg.eveningAbsentDeduction, -2.0);
}

void TestConfig::saveAndLoadRoundtrip()
{
    ConfigData cfg;
    cfg.namelistDirectory = QString("students");
    cfg.morningDeadline = QTime(8, 15);
    cfg.noonDeadline = QTime(13, 0);
    cfg.eveningDeadline = QTime(19, 30);
    cfg.morningNoonSplit = QTime(11, 45);
    cfg.noonEveningSplit = QTime(16, 20);

    cfg.morningSignBonusCount = 5;
    cfg.morningSignBonusScore = 3.5;
    cfg.morningLateDeduction = -2.25;
    cfg.morningAbsentDeduction = -4.5;
    cfg.noonLateDeduction = -3.75;
    cfg.noonAbsentDeduction = -5.25;
    cfg.eveningLateDeduction = -4.0;
    cfg.eveningAbsentDeduction = -6.5;

    ConfigManager::saveConfig(cfg);
    QVERIFY(QFile::exists(ConfigManager::configFilePath()));

    ConfigData loaded = ConfigManager::loadConfig();
    QCOMPARE(loaded.namelistDirectory, cfg.namelistDirectory);
    QCOMPARE(loaded.morningDeadline, cfg.morningDeadline);
    QCOMPARE(loaded.noonDeadline, cfg.noonDeadline);
    QCOMPARE(loaded.eveningDeadline, cfg.eveningDeadline);
    QCOMPARE(loaded.morningNoonSplit, cfg.morningNoonSplit);
    QCOMPARE(loaded.noonEveningSplit, cfg.noonEveningSplit);

    QCOMPARE(loaded.morningSignBonusCount, cfg.morningSignBonusCount);
    QCOMPARE(loaded.morningSignBonusScore, cfg.morningSignBonusScore);
    QCOMPARE(loaded.morningLateDeduction, cfg.morningLateDeduction);
    QCOMPARE(loaded.morningAbsentDeduction, cfg.morningAbsentDeduction);
    QCOMPARE(loaded.noonLateDeduction, cfg.noonLateDeduction);
    QCOMPARE(loaded.noonAbsentDeduction, cfg.noonAbsentDeduction);
    QCOMPARE(loaded.eveningLateDeduction, cfg.eveningLateDeduction);
    QCOMPARE(loaded.eveningAbsentDeduction, cfg.eveningAbsentDeduction);
}

void TestConfig::loadCreatesDefaultFileWhenMissing()
{
    QVERIFY(!QFile::exists(ConfigManager::configFilePath()));

    ConfigData cfg = ConfigManager::loadConfig();
    QCOMPARE(cfg.namelistDirectory, QString("names"));
    QCOMPARE(cfg.morningDeadline, QTime(9, 0));

    // A default config.toml should now exist and be loadable again.
    QVERIFY(QFile::exists(ConfigManager::configFilePath()));
    ConfigData again = ConfigManager::loadConfig();
    QCOMPARE(again.morningDeadline, QTime(9, 0));
}

void TestConfig::loadPartialFileKeepsDefaults()
{
    QFile f(ConfigManager::configFilePath());
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write("[namelist]\ndirectory = 'list.txt'\n");
    f.close();

    ConfigData cfg = ConfigManager::loadConfig();
    QCOMPARE(cfg.namelistDirectory, QString("list.txt"));
    // Missing [periods] section must fall back to defaults.
    QCOMPARE(cfg.morningDeadline, QTime(9, 0));
    QCOMPARE(cfg.noonDeadline, QTime(12, 30));
    QCOMPARE(cfg.eveningDeadline, QTime(18, 0));
    QCOMPARE(cfg.morningNoonSplit, QTime(12, 0));
    QCOMPARE(cfg.noonEveningSplit, QTime(17, 0));
    // Missing [rewardly] section must fall back to defaults too.
    QCOMPARE(cfg.morningSignBonusCount, 10);
    QCOMPARE(cfg.morningSignBonusScore, 2.0);
    QCOMPARE(cfg.morningLateDeduction, -1.0);
    QCOMPARE(cfg.morningAbsentDeduction, -2.0);
    QCOMPARE(cfg.noonLateDeduction, -1.0);
    QCOMPARE(cfg.noonAbsentDeduction, -2.0);
    QCOMPARE(cfg.eveningLateDeduction, -1.0);
    QCOMPARE(cfg.eveningAbsentDeduction, -2.0);
}

void TestConfig::loadInvalidTomlFallsBackToDefaults()
{
    QFile f(ConfigManager::configFilePath());
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write("this is not valid toml\n");
    f.close();

    ConfigData cfg = ConfigManager::loadConfig();
    QCOMPARE(cfg.namelistDirectory, QString("names"));
    QCOMPARE(cfg.morningDeadline, QTime(9, 0));
}

void TestConfig::migratesLegacyNamelist()
{
    QFile legacy(legacyConfigPath());
    QVERIFY(legacy.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&legacy);
    out << "[namelistDirectory]\n";
    out << "legacy-names\n";
    legacy.close();

    ConfigData cfg = ConfigManager::loadConfig();
    QCOMPARE(cfg.namelistDirectory, QString("legacy-names"));

    // A fresh TOML config should be created so future runs use the new format.
    QVERIFY(QFile::exists(ConfigManager::configFilePath()));
}

void TestConfig::configFilePathPointsToToml()
{
    QString path = ConfigManager::configFilePath();
    QVERIFY(QDir::isAbsolutePath(path));
    QVERIFY(path.endsWith(QString("config.toml")));
}

void TestConfig::exceptionsAreNotPersisted()
{
    // 设置例外 is a one-off, in-memory setting: config.toml has no section
    // for it, and a hand-written [[exceptions]] block must be ignored
    // rather than being applied.
    ConfigData cfg = ConfigManager::defaultConfig();
    ConfigManager::saveConfig(cfg);

    QFile f(ConfigManager::configFilePath());
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString saved = QString::fromUtf8(f.readAll());
    f.close();
    QVERIFY(!saved.contains(QString("exception")));

    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << saved;
    out << "\n[[exceptions]]\n";
    out << "date = '2026-10-01'\n";
    out << "period = 0\n";
    out << "split = '12:40'\n";
    out << "deadline = '09:30'\n";
    f.close();

    ConfigData loaded = ConfigManager::loadConfig();
    QCOMPARE(loaded.morningDeadline, QTime(9, 0));    // defaults untouched
    QCOMPARE(loaded.morningNoonSplit, QTime(12, 0));

    // Re-saving drops the unknown section again.
    ConfigManager::saveConfig(loaded);
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(!QString::fromUtf8(f.readAll()).contains(QString("exception")));
    f.close();
}

void TestConfig::forDateAppliesExceptions()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    PeriodException morning;
    morning.date = QDate(2026, 10, 1);
    morning.period = 0;
    morning.split = QTime(13, 0);
    morning.deadline = QTime(10, 30);

    PeriodException evening;
    evening.date = QDate(2026, 10, 1);
    evening.period = 2;
    evening.split = QTime(15, 0);    // meaningless for 晚上: must be ignored
    evening.deadline = QTime(20, 0);

    const QVector<PeriodException> exceptions = {morning, evening};

    ConfigData day = ConfigManager::forDate(cfg, QDate(2026, 10, 1), exceptions);
    QCOMPARE(day.morningNoonSplit, QTime(13, 0));
    QCOMPARE(day.morningDeadline, QTime(10, 30));
    QCOMPARE(day.eveningDeadline, QTime(20, 0));
    // Untouched values stay global.
    QCOMPARE(day.noonEveningSplit, cfg.noonEveningSplit);
    QCOMPARE(day.noonDeadline, cfg.noonDeadline);

    // Other dates keep the global configuration untouched.
    ConfigData other = ConfigManager::forDate(cfg, QDate(2026, 10, 2), exceptions);
    QCOMPARE(other.morningNoonSplit, cfg.morningNoonSplit);
    QCOMPARE(other.morningDeadline, cfg.morningDeadline);
    QCOMPARE(other.eveningDeadline, cfg.eveningDeadline);

    // An empty exception list leaves the day on the global configuration.
    ConfigData none = ConfigManager::forDate(cfg, QDate(2026, 10, 1), {});
    QCOMPARE(none.morningNoonSplit, cfg.morningNoonSplit);
    QCOMPARE(none.morningDeadline, cfg.morningDeadline);
    QCOMPARE(none.eveningDeadline, cfg.eveningDeadline);
}

void TestConfig::validateExceptionsChecksOrdering()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    // Defaults and an empty exception list are fine.
    QVERIFY(ConfigManager::validateExceptions(cfg, {}).isEmpty());

    // A deadline later than its own period boundary is rejected.
    PeriodException badDeadline;
    badDeadline.date = QDate(2026, 10, 1);
    badDeadline.period = 0;
    badDeadline.split = QTime(11, 0);
    badDeadline.deadline = QTime(11, 30);
    QVERIFY(!ConfigManager::validateExceptions(cfg, {badDeadline}).isEmpty());

    // 早/午分界 after 午/晚分界 is rejected.
    PeriodException lateMorning;
    lateMorning.date = QDate(2026, 10, 1);
    lateMorning.period = 0;
    lateMorning.split = QTime(18, 0);
    lateMorning.deadline = QTime(9, 0);

    PeriodException noon;
    noon.date = QDate(2026, 10, 1);
    noon.period = 1;
    noon.split = QTime(17, 0);
    noon.deadline = QTime(12, 30);

    QVERIFY(!ConfigManager::validateExceptions(cfg, {lateMorning, noon})
                 .isEmpty());

    // The same morning override alone is still invalid: it would push the
    // 早/午分界 past the global 午/晚分界.
    QVERIFY(!ConfigManager::validateExceptions(cfg, {lateMorning}).isEmpty());

    // A consistent morning override is accepted.
    PeriodException goodMorning = lateMorning;
    goodMorning.split = QTime(13, 0);
    QVERIFY(ConfigManager::validateExceptions(cfg, {goodMorning}).isEmpty());
}

QTEST_GUILESS_MAIN(TestConfig)
#include "test_config.moc"
