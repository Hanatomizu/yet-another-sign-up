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
    void ignoredPeriodLeavesConfigUntouched();
    void ignoredPeriodsForDateFiltersByDate();
    void validateExceptionsAcceptsIgnoredPeriod();

    void excusedPeriodsForDateFiltersByRange();
    void validateStudentLeavesChecksFields();
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

void TestConfig::ignoredPeriodLeavesConfigUntouched()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    PeriodException ignored;
    ignored.date = QDate(2026, 10, 1);
    ignored.period = 0;
    ignored.ignore = true;
    // A stale split/deadline on an ignored entry must not leak into the
    // day's effective configuration.
    ignored.split = QTime(13, 0);
    ignored.deadline = QTime(10, 30);

    ConfigData day = ConfigManager::forDate(cfg, ignored.date, {ignored});
    QCOMPARE(day.morningNoonSplit, cfg.morningNoonSplit);
    QCOMPARE(day.morningDeadline, cfg.morningDeadline);
    QCOMPARE(day.noonEveningSplit, cfg.noonEveningSplit);
    QCOMPARE(day.noonDeadline, cfg.noonDeadline);
    QCOMPARE(day.eveningDeadline, cfg.eveningDeadline);
}

void TestConfig::ignoredPeriodsForDateFiltersByDate()
{
    const QDate date(2026, 10, 1);

    PeriodException morning;
    morning.date = date;
    morning.period = 0;
    morning.ignore = true;

    PeriodException evening;
    evening.date = date;
    evening.period = 2;
    evening.ignore = true;

    PeriodException otherDay;
    otherDay.date = QDate(2026, 10, 2);
    otherDay.period = 1;
    otherDay.ignore = true;

    PeriodException active;
    active.date = date;
    active.period = 1;
    active.deadline = QTime(12, 30);   // not ignored

    const QVector<PeriodException> exceptions = {morning, evening, otherDay,
                                                 active};

    const QSet<int> ignored =
        ConfigManager::ignoredPeriodsForDate(date, exceptions);
    QCOMPARE(int(ignored.size()), 2);
    QVERIFY(ignored.contains(0));
    QVERIFY(ignored.contains(2));
    QVERIFY(!ignored.contains(1));

    // Other dates only see their own ignored periods.
    const QSet<int> secondDay =
        ConfigManager::ignoredPeriodsForDate(QDate(2026, 10, 2), exceptions);
    QCOMPARE(int(secondDay.size()), 1);
    QVERIFY(secondDay.contains(1));

    // Dates without exceptions and invalid dates have nothing to ignore.
    QVERIFY(ConfigManager::ignoredPeriodsForDate(QDate(2026, 10, 3),
                                                 exceptions).isEmpty());
    QVERIFY(ConfigManager::ignoredPeriodsForDate(QDate(), exceptions).isEmpty());
}

void TestConfig::validateExceptionsAcceptsIgnoredPeriod()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    PeriodException ignored;
    ignored.date = QDate(2026, 10, 1);
    ignored.period = 0;
    ignored.ignore = true;
    // Time fields carry no meaning for an ignored period, so a
    // deadline later than the split must not be rejected.
    ignored.split = QTime(11, 0);
    ignored.deadline = QTime(11, 30);

    QVERIFY(ConfigManager::validateExceptions(cfg, {ignored}).isEmpty());

    // Date/period validity is still enforced for ignored entries.
    PeriodException badDate = ignored;
    badDate.date = QDate();
    QVERIFY(!ConfigManager::validateExceptions(cfg, {badDate}).isEmpty());

    PeriodException badPeriod = ignored;
    badPeriod.period = 3;
    QVERIFY(!ConfigManager::validateExceptions(cfg, {badPeriod}).isEmpty());
}

void TestConfig::excusedPeriodsForDateFiltersByRange()
{
    StudentLeaveException zhangsan;
    zhangsan.name = QString("张三");
    zhangsan.dateFrom = QDate(2026, 10, 1);
    zhangsan.dateTo = QDate(2026, 10, 3);
    zhangsan.periods = {0, 1};

    StudentLeaveException lisi;
    lisi.name = QString("李四");
    lisi.dateFrom = QDate(2026, 10, 3);
    lisi.dateTo = QDate(2026, 10, 3);
    lisi.periods = {2};

    const QVector<StudentLeaveException> leaves = {zhangsan, lisi};

    // Both ends of the range are inclusive.
    const auto firstDay =
        ConfigManager::excusedPeriodsForDate(QDate(2026, 10, 1), leaves);
    QCOMPARE(int(firstDay.size()), 1);
    QCOMPARE(firstDay.value(QString("张三")), (QSet<int>{0, 1}));
    QVERIFY(!firstDay.contains(QString("李四")));

    // On 10-03 both students are on leave, each in their own periods.
    const auto lastDay =
        ConfigManager::excusedPeriodsForDate(QDate(2026, 10, 3), leaves);
    QCOMPARE(int(lastDay.size()), 2);
    QCOMPARE(lastDay.value(QString("张三")), (QSet<int>{0, 1}));
    QCOMPARE(lastDay.value(QString("李四")), (QSet<int>{2}));

    // Outside the range nothing is excused; invalid dates are empty.
    QVERIFY(ConfigManager::excusedPeriodsForDate(QDate(2026, 10, 4), leaves)
                .isEmpty());
    QVERIFY(ConfigManager::excusedPeriodsForDate(QDate(), leaves).isEmpty());

    // Overlapping leaves for the same student union their periods.
    StudentLeaveException overlap;
    overlap.name = QString("张三");
    overlap.dateFrom = QDate(2026, 10, 3);
    overlap.dateTo = QDate(2026, 10, 5);
    overlap.periods = {2};
    const auto unioned = ConfigManager::excusedPeriodsForDate(
        QDate(2026, 10, 3), {zhangsan, overlap});
    QCOMPARE(unioned.value(QString("张三")), (QSet<int>{0, 1, 2}));
}

void TestConfig::validateStudentLeavesChecksFields()
{
    StudentLeaveException leave;
    leave.name = QString("张三");
    leave.dateFrom = QDate(2026, 10, 1);
    leave.dateTo = QDate(2026, 10, 2);
    leave.periods = {0};

    QVERIFY(ConfigManager::validateStudentLeaves({leave}).isEmpty());
    QVERIFY(ConfigManager::validateStudentLeaves({}).isEmpty());

    StudentLeaveException badName = leave;
    badName.name.clear();
    QVERIFY(!ConfigManager::validateStudentLeaves({badName}).isEmpty());

    StudentLeaveException badRange = leave;
    badRange.dateTo = QDate(2026, 9, 30);
    QVERIFY(!ConfigManager::validateStudentLeaves({badRange}).isEmpty());

    StudentLeaveException noPeriods = leave;
    noPeriods.periods.clear();
    QVERIFY(!ConfigManager::validateStudentLeaves({noPeriods}).isEmpty());

    StudentLeaveException badPeriod = leave;
    badPeriod.periods = {3};
    QVERIFY(!ConfigManager::validateStudentLeaves({badPeriod}).isEmpty());

    StudentLeaveException badDate = leave;
    badDate.dateFrom = QDate();
    QVERIFY(!ConfigManager::validateStudentLeaves({badDate}).isEmpty());
}

QTEST_GUILESS_MAIN(TestConfig)
#include "test_config.moc"
