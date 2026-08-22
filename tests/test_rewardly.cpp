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

QTEST_MAIN(TestRewardly)
#include "test_rewardly.moc"
