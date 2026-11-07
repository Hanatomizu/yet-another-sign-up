/**
 * Unit tests for the Rewardly export row building logic
 * (RewardlyExportDialog::buildRows).
 *
 * buildRows() is a pure public static function, so no access tricks are
 * needed; the dialog's .cpp is compiled in unchanged.
 */

#include <QtTest>

#include <vector>
#include <map>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

#include "rewardlyexportdialog.h"
#include "signlogparser.h"
#include "configmanager.h"

// rewardlyexportdialog.cpp references these globals (normally defined in
// arbiter.cpp); provide them here for the test build.
std::vector<QString> extstunames;
std::map<QString, int> nti;

class TestRewardly : public QObject
{
    Q_OBJECT

private slots:
    void buildRowsMorningTopN();
    void buildRowsMorningTopNCustomConfig();
    void buildRowsLatesAllPeriods();
    void buildRowsEmptyRecords();
    void buildRowsBonusCountZero();
    void buildRowsBonusCountLargerThanAvailable();
    void buildRowsAbsentPerPeriod();
    void buildRowsLateStudentNotAbsent();
    void buildRowsCustomAbsentDeductions();
    void dateExceptionShiftsMorningBonus();
    void buildRowsSkipsIgnoredMorning();
    void buildRowsSkipsMultipleIgnoredPeriods();
    void buildRowsAllPeriodsIgnored();
    void ignoredPeriodHasNoAbsentRows();
    void dateExceptionIgnoresEvening();

    void leaveExcusesLateAndAbsent();
    void leaveKeepsOnTimeMorningBonus();
    void leaveOnlyAffectsSelectedPeriodsAndStudents();
    void leaveRangeDrivesExcusedPeriods();
};

namespace {

const QDate kDate(2026, 8, 8);

QVector<SignRecord> makeRecords()
{
    QVector<SignRecord> records;
    // Morning on-time (deadline 09:00)
    records.append({QString("张三"), kDate, QTime(8, 30, 0), 0, QString("签到")});
    records.append({QString("李四"), kDate, QTime(8, 45, 0), 0, QString("签到")});
    records.append({QString("王五"), kDate, QTime(8, 50, 0), 0, QString("签到")});
    // Morning late
    records.append({QString("赵六"), kDate, QTime(9, 10, 0), 0, QString("迟到")});
    // Noon late
    records.append({QString("钱七"), kDate, QTime(13, 0, 0), 1, QString("迟到")});
    // Evening late
    records.append({QString("孙八"), kDate, QTime(18, 30, 0), 2, QString("迟到")});
    return records;
}

int countByReason(const QVector<RewardlyRow> &rows, const QString &reason)
{
    int n = 0;
    for (const auto &row : rows) {
        if (row.reason == reason) {
            ++n;
        }
    }
    return n;
}

} // namespace

void TestRewardly::buildRowsMorningTopN()
{
    ConfigData cfg = ConfigManager::defaultConfig(); // count = 10, bonus = 2

    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(makeRecords(), {}, kDate, cfg);

    // 3 morning bonuses (all 3 qualify, capped by count=10) + 3 lates
    QCOMPARE(rows.size(), 6);

    // Morning bonuses come first, sorted by time (earliest first).
    QCOMPARE(rows[0].name, QString("张三"));
    QCOMPARE(rows[0].dateStr, QString("2026-08-08"));
    QCOMPARE(rows[0].periodName, QString("早上"));
    QCOMPARE(rows[0].points, 2.0);
    QCOMPARE(rows[0].reason, QString("早上签到"));

    QCOMPARE(rows[1].name, QString("李四"));
    QCOMPARE(rows[1].points, 2.0);
    QCOMPARE(rows[1].reason, QString("早上签到"));

    QCOMPARE(rows[2].name, QString("王五"));
    QCOMPARE(rows[2].points, 2.0);
    QCOMPARE(rows[2].reason, QString("早上签到"));

    // Lates follow with negative points and per-period reasons.
    QCOMPARE(rows[3].name, QString("赵六"));
    QCOMPARE(rows[3].periodName, QString("早上"));
    QCOMPARE(rows[3].points, -1.0);
    QCOMPARE(rows[3].reason, QString("早上迟到"));

    QCOMPARE(rows[4].name, QString("钱七"));
    QCOMPARE(rows[4].periodName, QString("中午"));
    QCOMPARE(rows[4].points, -1.0);
    QCOMPARE(rows[4].reason, QString("中午迟到"));

    QCOMPARE(rows[5].name, QString("孙八"));
    QCOMPARE(rows[5].periodName, QString("晚上"));
    QCOMPARE(rows[5].points, -1.0);
    QCOMPARE(rows[5].reason, QString("晚上迟到"));

    // Sign convention: positive = 加分, negative = 扣分.
    QVERIFY(rows[0].points > 0.0);
    QVERIFY(rows[3].points < 0.0);
}

void TestRewardly::buildRowsMorningTopNCustomConfig()
{
    ConfigData cfg = ConfigManager::defaultConfig();
    cfg.morningSignBonusCount = 2;   // only top 2 get the bonus
    cfg.morningSignBonusScore = 5.5; // +5.50 points each
    cfg.morningLateDeduction = -3.5;
    cfg.noonLateDeduction = -4.25;
    cfg.eveningLateDeduction = -5.75;

    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(makeRecords(), {}, kDate, cfg);

    QCOMPARE(rows.size(), 5);

    QCOMPARE(rows[0].name, QString("张三"));
    QCOMPARE(rows[0].points, 5.5);
    QCOMPARE(rows[1].name, QString("李四"));
    QCOMPARE(rows[1].points, 5.5);
    // 王五 was 3rd — no bonus row for him.
    QCOMPARE(rows[2].name, QString("赵六"));
    QCOMPARE(rows[2].points, -3.5);
    QCOMPARE(rows[3].name, QString("钱七"));
    QCOMPARE(rows[3].points, -4.25);
    QCOMPARE(rows[4].name, QString("孙八"));
    QCOMPARE(rows[4].points, -5.75);
}

void TestRewardly::buildRowsLatesAllPeriods()
{
    // A record set with only lates (no morning on-time sign-ins).
    QVector<SignRecord> records;
    records.append({QString("张三"), kDate, QTime(9, 5, 0), 0, QString("迟到")});
    records.append({QString("李四"), kDate, QTime(12, 45, 0), 1, QString("迟到")});
    records.append({QString("王五"), kDate, QTime(18, 5, 0), 2, QString("迟到")});

    ConfigData cfg = ConfigManager::defaultConfig();
    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        records, {}, kDate, cfg);

    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows[0].reason, QString("早上迟到"));
    QCOMPARE(rows[1].reason, QString("中午迟到"));
    QCOMPARE(rows[2].reason, QString("晚上迟到"));
}

void TestRewardly::buildRowsEmptyRecords()
{
    ConfigData cfg = ConfigManager::defaultConfig();
    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows({}, {}, kDate, cfg);
    QVERIFY(rows.isEmpty());
}

void TestRewardly::buildRowsBonusCountZero()
{
    ConfigData cfg = ConfigManager::defaultConfig();
    cfg.morningSignBonusCount = 0;

    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(makeRecords(), {}, kDate, cfg);

    // No bonus rows; only the 3 lates remain.
    QCOMPARE(rows.size(), 3);
    for (const auto &row : rows) {
        QVERIFY(row.points < 0);
    }
}

void TestRewardly::buildRowsBonusCountLargerThanAvailable()
{
    ConfigData cfg = ConfigManager::defaultConfig();
    cfg.morningSignBonusCount = 100;

    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(makeRecords(), {}, kDate, cfg);

    // Still only 3 bonus rows — capped by the number of on-time sign-ins.
    int bonusCount = 0;
    for (const auto &row : rows) {
        if (row.points > 0) {
            ++bonusCount;
        }
    }
    QCOMPARE(bonusCount, 3);
}

void TestRewardly::buildRowsAbsentPerPeriod()
{
    // Only 张三 signed in (morning on-time); 李四 and 王五 have no record.
    QVector<SignRecord> records;
    records.append({QString("张三"), kDate, QTime(8, 30, 0), 0, QString("签到")});

    ConfigData cfg = ConfigManager::defaultConfig();
    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        records,
        {QString("张三"), QString("李四"), QString("王五")},
        kDate, cfg);

    // 1 bonus + 2 morning absent + 3 noon absent + 3 evening absent
    QCOMPARE(rows.size(), 9);

    QCOMPARE(rows[0].name, QString("张三"));
    QCOMPARE(rows[0].points, 2.0);
    QCOMPARE(rows[0].reason, QString("早上签到"));

    QCOMPARE(countByReason(rows, QString("早上未签到")), 2);
    QCOMPARE(countByReason(rows, QString("中午未签到")), 3);
    QCOMPARE(countByReason(rows, QString("晚上未签到")), 3);

    // Absent rows carry the configured deduction (-2.0 by default) and the
    // right period name.
    for (const auto &row : rows) {
        if (row.reason.endsWith(QString("未签到"))) {
            QCOMPARE(row.points, -2.0);
            QVERIFY(!row.periodName.isEmpty());
        }
    }

    // 张三 signed in the morning, so he must not appear as 早上未签到.
    for (const auto &row : rows) {
        QVERIFY(!(row.name == QString("张三")
                  && row.reason == QString("早上未签到")));
    }
}

void TestRewardly::buildRowsLateStudentNotAbsent()
{
    // 李四 signed in late in the morning — that still counts as present
    // for the morning period, so only 王五 is absent in the morning.
    QVector<SignRecord> records;
    records.append({QString("张三"), kDate, QTime(8, 30, 0), 0, QString("签到")});
    records.append({QString("李四"), kDate, QTime(9, 5, 0), 0, QString("迟到")});

    ConfigData cfg = ConfigManager::defaultConfig();
    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        records,
        {QString("张三"), QString("李四"), QString("王五")},
        kDate, cfg);

    // 1 bonus + 1 morning late + 1 morning absent + 3 noon absent + 3 evening absent
    QCOMPARE(rows.size(), 9);

    QCOMPARE(countByReason(rows, QString("早上迟到")), 1);
    QCOMPARE(countByReason(rows, QString("早上未签到")), 1);

    // The single morning-absent row must be 王五.
    for (const auto &row : rows) {
        if (row.reason == QString("早上未签到")) {
            QCOMPARE(row.name, QString("王五"));
            QCOMPARE(row.points, -2.0);
        }
    }
}

void TestRewardly::buildRowsCustomAbsentDeductions()
{
    // No records at all: with an empty roster there are no rows, but with
    // students present every period produces an absent row using the
    // configured (signed) deduction values.
    ConfigData cfg = ConfigManager::defaultConfig();
    cfg.morningAbsentDeduction = -3.5;
    cfg.noonAbsentDeduction = -4.25;
    cfg.eveningAbsentDeduction = -5.5;

    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        {}, {QString("张三")}, kDate, cfg);

    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows[0].reason, QString("早上未签到"));
    QCOMPARE(rows[0].points, -3.5);
    QCOMPARE(rows[1].reason, QString("中午未签到"));
    QCOMPARE(rows[1].points, -4.25);
    QCOMPARE(rows[2].reason, QString("晚上未签到"));
    QCOMPARE(rows[2].points, -5.5);
}

void TestRewardly::dateExceptionShiftsMorningBonus()
{
    // End-to-end through the parser: a sign-in at 11:30 earns the morning
    // bonus with the global config, but stops being a morning sign-in once
    // that day's 早上 gets an 11:00 分界 (设置例外).
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QString("2026-08-08.log"));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2026.08.08 08:30:00, 张三, Signed\n"; // morning on time
    out << "2026.08.08 11:30:00, 李四, Signed\n"; // morning late by default
    f.close();

    ConfigData cfg = ConfigManager::defaultConfig(); // bonus count = 10

    QVector<SignRecord> plain = parseSignLogFile(path, kDate, cfg);
    QVector<RewardlyRow> plainRows =
        RewardlyExportDialog::buildRows(plain, {}, kDate, cfg);
    // 张三 gets the morning bonus, 李四 is a morning late (deadline 09:00).
    QCOMPARE(plainRows.size(), 2);
    QCOMPARE(plainRows[0].reason, QString("早上签到"));
    QCOMPARE(plainRows[0].points, 2.0);
    QCOMPARE(plainRows[1].name, QString("李四"));
    QCOMPARE(plainRows[1].reason, QString("早上迟到"));
    QCOMPARE(plainRows[1].points, -1.0);

    PeriodException morning;
    morning.date = kDate;
    morning.period = 0;
    morning.split = QTime(11, 0);
    morning.deadline = QTime(9, 30);

    QVector<SignRecord> excepted = parseSignLogFile(path, kDate, cfg, {morning});
    QVector<RewardlyRow> exceptedRows =
        RewardlyExportDialog::buildRows(excepted, {}, kDate, cfg);
    // 11:30 is now an on-time noon sign-in, so only 张三's bonus survives.
    QCOMPARE(exceptedRows.size(), 1);
    QCOMPARE(exceptedRows[0].name, QString("张三"));
    QCOMPARE(exceptedRows[0].reason, QString("早上签到"));
}

void TestRewardly::buildRowsSkipsIgnoredMorning()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        makeRecords(), {}, kDate, cfg, QSet<int>{0});

    // Morning bonus and morning late are gone; noon/evening lates remain.
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows[0].name, QString("钱七"));
    QCOMPARE(rows[0].reason, QString("中午迟到"));
    QCOMPARE(rows[1].name, QString("孙八"));
    QCOMPARE(rows[1].reason, QString("晚上迟到"));

    for (const auto &row : rows) {
        QVERIFY(!row.reason.startsWith(QString("早上")));
    }
}

void TestRewardly::buildRowsSkipsMultipleIgnoredPeriods()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        makeRecords(), {}, kDate, cfg, QSet<int>{0, 1});

    // Only the evening late is left.
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].name, QString("孙八"));
    QCOMPARE(rows[0].reason, QString("晚上迟到"));
}

void TestRewardly::buildRowsAllPeriodsIgnored()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        makeRecords(),
        {QString("张三"), QString("李四")},
        kDate, cfg, QSet<int>{0, 1, 2});

    // Nothing is scored on a day whose three periods are all ignored.
    QVERIFY(rows.isEmpty());
}

void TestRewardly::ignoredPeriodHasNoAbsentRows()
{
    // 张三 signed in the morning only. Ignoring the morning must suppress
    // both the morning bonus and the morning absent rows, while noon and
    // evening absents are still computed normally.
    QVector<SignRecord> records;
    records.append({QString("张三"), kDate, QTime(8, 30, 0), 0, QString("签到")});

    ConfigData cfg = ConfigManager::defaultConfig();
    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        records,
        {QString("张三"), QString("李四")},
        kDate, cfg, QSet<int>{0});

    // 张三 + 李四 absent at noon, and the same at evening.
    QCOMPARE(rows.size(), 4);
    QCOMPARE(countByReason(rows, QString("早上签到")), 0);
    QCOMPARE(countByReason(rows, QString("早上迟到")), 0);
    QCOMPARE(countByReason(rows, QString("早上未签到")), 0);
    QCOMPARE(countByReason(rows, QString("中午未签到")), 2);
    QCOMPARE(countByReason(rows, QString("晚上未签到")), 2);
}

void TestRewardly::dateExceptionIgnoresEvening()
{
    // End-to-end through the parser: the ignored period is still classified
    // normally (period/status are unchanged), but its scoring rows are gone.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QString("2026-08-08.log"));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2026.08.08 18:30:00, 孙八, Signed\n"; // evening late by default
    f.close();

    ConfigData cfg = ConfigManager::defaultConfig();

    PeriodException evening;
    evening.date = kDate;
    evening.period = 2;
    evening.ignore = true;

    QVector<SignRecord> records = parseSignLogFile(path, kDate, cfg, {evening});
    QCOMPARE(records.size(), 1);
    QCOMPARE(records[0].period, 2);
    QCOMPARE(records[0].status, QString("迟到"));

    const QSet<int> ignored =
        ConfigManager::ignoredPeriodsForDate(kDate, {evening});
    QVERIFY(ignored.contains(2));

    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(records, {}, kDate, cfg, ignored);
    QVERIFY(rows.isEmpty());

    // The ignore entry is bound to its own date only.
    QVERIFY(ConfigManager::ignoredPeriodsForDate(kDate.addDays(1), {evening})
                .isEmpty());
}

void TestRewardly::leaveExcusesLateAndAbsent()
{
    // 张三 is late in the morning, 李四 has no morning record; both are on
    // leave for the morning, so neither produces a morning deduction.
    QVector<SignRecord> records;
    records.append({QString("张三"), kDate, QTime(9, 10, 0), 0, QString("迟到")});

    ConfigData cfg = ConfigManager::defaultConfig();
    QHash<QString, QSet<int>> excused;
    excused[QString("张三")].insert(0);
    excused[QString("李四")].insert(0);

    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        records, {QString("张三"), QString("李四"), QString("王五")},
        kDate, cfg, {}, excused);

    // No morning late row for 张三 and no morning absent row for him or
    // 李四; 王五 (not on leave) is still marked absent in the morning.
    // Their leave covers the morning only, so everyone is absent at noon
    // and in the evening.
    QCOMPARE(countByReason(rows, QString("早上迟到")), 0);
    QCOMPARE(countByReason(rows, QString("早上未签到")), 1);
    for (const auto &row : rows) {
        if (row.reason == QString("早上未签到")) {
            QCOMPARE(row.name, QString("王五"));
        }
    }
    QCOMPARE(countByReason(rows, QString("中午未签到")), 3);
    QCOMPARE(countByReason(rows, QString("晚上未签到")), 3);
    QCOMPARE(rows.size(), 7);
}

void TestRewardly::leaveKeepsOnTimeMorningBonus()
{
    // A student on leave who nevertheless signed in on time keeps the bonus.
    QVector<SignRecord> records;
    records.append({QString("张三"), kDate, QTime(8, 30, 0), 0, QString("签到")});

    ConfigData cfg = ConfigManager::defaultConfig();
    QHash<QString, QSet<int>> excused;
    excused[QString("张三")] = {0, 1, 2};

    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        records, {QString("张三")}, kDate, cfg, {}, excused);

    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows[0].name, QString("张三"));
    QCOMPARE(rows[0].reason, QString("早上签到"));
    QCOMPARE(rows[0].points, 2.0);
}

void TestRewardly::leaveOnlyAffectsSelectedPeriodsAndStudents()
{
    // 张三 is excused for noon only; his morning late still counts, and a
    // different student's noon late is unaffected.
    QVector<SignRecord> records;
    records.append({QString("张三"), kDate, QTime(9, 5, 0), 0, QString("迟到")});
    records.append({QString("张三"), kDate, QTime(12, 45, 0), 1, QString("迟到")});
    records.append({QString("李四"), kDate, QTime(12, 50, 0), 1, QString("迟到")});

    ConfigData cfg = ConfigManager::defaultConfig();
    QHash<QString, QSet<int>> excused;
    excused[QString("张三")] = {1};

    QVector<RewardlyRow> rows = RewardlyExportDialog::buildRows(
        records, {}, kDate, cfg, {}, excused);

    QCOMPARE(countByReason(rows, QString("早上迟到")), 1);
    QCOMPARE(countByReason(rows, QString("中午迟到")), 1);

    for (const auto &row : rows) {
        if (row.reason == QString("中午迟到")) {
            QCOMPARE(row.name, QString("李四"));
        }
    }
}

void TestRewardly::leaveRangeDrivesExcusedPeriods()
{
    // End-to-end: a leave covering 10-01..10-03 excuses 张三 only inside
    // that range, and only in the chosen period.
    StudentLeaveException leave;
    leave.name = QString("张三");
    leave.dateFrom = QDate(2026, 8, 1);
    leave.dateTo = QDate(2026, 8, 10);
    leave.periods = {0};

    ConfigData cfg = ConfigManager::defaultConfig();
    const QHash<QString, QSet<int>> excused =
        ConfigManager::excusedPeriodsForDate(kDate, {leave});
    QCOMPARE(excused.value(QString("张三")), (QSet<int>{0}));

    QVector<SignRecord> records;
    records.append({QString("张三"), kDate, QTime(9, 5, 0), 0, QString("迟到")});

    // Empty roster so no absent rows are generated; the morning late is
    // the only candidate and it is excused.
    QVector<RewardlyRow> inside = RewardlyExportDialog::buildRows(
        records, {}, kDate, cfg, {}, excused);
    QVERIFY(inside.isEmpty());

    // One day after the range: the late deduction comes back.
    const QDate after = leave.dateTo.addDays(1);
    QVERIFY(ConfigManager::excusedPeriodsForDate(after, {leave}).isEmpty());
}

QTEST_MAIN(TestRewardly)
#include "test_rewardly.moc"
