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
    QCOMPARE(cfg.morningSignBonusScore, 2);
    QCOMPARE(cfg.morningLateDeduction, 1);
    QCOMPARE(cfg.morningAbsentDeduction, 2);
    QCOMPARE(cfg.noonLateDeduction, 1);
    QCOMPARE(cfg.noonAbsentDeduction, 2);
    QCOMPARE(cfg.eveningLateDeduction, 1);
    QCOMPARE(cfg.eveningAbsentDeduction, 2);
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
    cfg.morningSignBonusScore = 3;
    cfg.morningLateDeduction = 2;
    cfg.morningAbsentDeduction = 4;
    cfg.noonLateDeduction = 3;
    cfg.noonAbsentDeduction = 5;
    cfg.eveningLateDeduction = 4;
    cfg.eveningAbsentDeduction = 6;

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
    QCOMPARE(cfg.morningSignBonusScore, 2);
    QCOMPARE(cfg.morningLateDeduction, 1);
    QCOMPARE(cfg.morningAbsentDeduction, 2);
    QCOMPARE(cfg.noonLateDeduction, 1);
    QCOMPARE(cfg.noonAbsentDeduction, 2);
    QCOMPARE(cfg.eveningLateDeduction, 1);
    QCOMPARE(cfg.eveningAbsentDeduction, 2);
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

QTEST_GUILESS_MAIN(TestConfig)
#include "test_config.moc"
