/**
 * Unit tests for the Rewardly export row building logic
 * (RewardlyExportDialog::buildRows).
 *
 * buildRows() is a pure public static function, so no access tricks are
 * needed; the dialog's .cpp is compiled in unchanged.
 */

#include <QtTest>

#include "rewardlyexportdialog.h"
#include "signlogparser.h"
#include "configmanager.h"

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
};

namespace {

QVector<SignRecord> makeRecords()
{
    QDate date(2026, 8, 8);
    QVector<SignRecord> records;
    // Morning on-time (deadline 09:00)
    records.append({QString("张三"), date, QTime(8, 30, 0), 0, QString("签到")});
    records.append({QString("李四"), date, QTime(8, 45, 0), 0, QString("签到")});
    records.append({QString("王五"), date, QTime(8, 50, 0), 0, QString("签到")});
    // Morning late
    records.append({QString("赵六"), date, QTime(9, 10, 0), 0, QString("迟到")});
    // Noon late
    records.append({QString("钱七"), date, QTime(13, 0, 0), 1, QString("迟到")});
    // Evening late
    records.append({QString("孙八"), date, QTime(18, 30, 0), 2, QString("迟到")});
    return records;
}

} // namespace

void TestRewardly::buildRowsMorningTopN()
{
    ConfigData cfg = ConfigManager::defaultConfig(); // count = 10, bonus = 2

    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(makeRecords(), QDate(2026, 8, 8), cfg);

    // 3 morning bonuses (all 3 qualify, capped by count=10) + 3 lates
    QCOMPARE(rows.size(), 6);

    // Morning bonuses come first, sorted by time (earliest first).
    QCOMPARE(rows[0].name, QString("张三"));
    QCOMPARE(rows[0].dateStr, QString("2026-08-08"));
    QCOMPARE(rows[0].periodName, QString("早上"));
    QCOMPARE(rows[0].points, 2);
    QCOMPARE(rows[0].reason, QString("早上签到"));

    QCOMPARE(rows[1].name, QString("李四"));
    QCOMPARE(rows[1].points, 2);
    QCOMPARE(rows[1].reason, QString("早上签到"));

    QCOMPARE(rows[2].name, QString("王五"));
    QCOMPARE(rows[2].points, 2);
    QCOMPARE(rows[2].reason, QString("早上签到"));

    // Lates follow with negative points and per-period reasons.
    QCOMPARE(rows[3].name, QString("赵六"));
    QCOMPARE(rows[3].periodName, QString("早上"));
    QCOMPARE(rows[3].points, -1);
    QCOMPARE(rows[3].reason, QString("早上迟到"));

    QCOMPARE(rows[4].name, QString("钱七"));
    QCOMPARE(rows[4].periodName, QString("中午"));
    QCOMPARE(rows[4].points, -1);
    QCOMPARE(rows[4].reason, QString("中午迟到"));

    QCOMPARE(rows[5].name, QString("孙八"));
    QCOMPARE(rows[5].periodName, QString("晚上"));
    QCOMPARE(rows[5].points, -1);
    QCOMPARE(rows[5].reason, QString("晚上迟到"));
}

void TestRewardly::buildRowsMorningTopNCustomConfig()
{
    ConfigData cfg = ConfigManager::defaultConfig();
    cfg.morningSignBonusCount = 2;   // only top 2 get the bonus
    cfg.morningSignBonusScore = 5;   // +5 points each
    cfg.morningLateDeduction = 3;
    cfg.noonLateDeduction = 4;
    cfg.eveningLateDeduction = 5;

    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(makeRecords(), QDate(2026, 8, 8), cfg);

    QCOMPARE(rows.size(), 5);

    QCOMPARE(rows[0].name, QString("张三"));
    QCOMPARE(rows[0].points, 5);
    QCOMPARE(rows[1].name, QString("李四"));
    QCOMPARE(rows[1].points, 5);
    // 王五 was 3rd — no bonus row for him.
    QCOMPARE(rows[2].name, QString("赵六"));
    QCOMPARE(rows[2].points, -3);
    QCOMPARE(rows[3].name, QString("钱七"));
    QCOMPARE(rows[3].points, -4);
    QCOMPARE(rows[4].name, QString("孙八"));
    QCOMPARE(rows[4].points, -5);
}

void TestRewardly::buildRowsLatesAllPeriods()
{
    // A record set with only lates (no morning on-time sign-ins).
    QDate date(2026, 8, 8);
    QVector<SignRecord> records;
    records.append({QString("张三"), date, QTime(9, 5, 0), 0, QString("迟到")});
    records.append({QString("李四"), date, QTime(12, 45, 0), 1, QString("迟到")});
    records.append({QString("王五"), date, QTime(18, 5, 0), 2, QString("迟到")});

    ConfigData cfg = ConfigManager::defaultConfig();
    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(records, date, cfg);

    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows[0].reason, QString("早上迟到"));
    QCOMPARE(rows[1].reason, QString("中午迟到"));
    QCOMPARE(rows[2].reason, QString("晚上迟到"));
}

void TestRewardly::buildRowsEmptyRecords()
{
    ConfigData cfg = ConfigManager::defaultConfig();
    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows({}, QDate(2026, 8, 8), cfg);
    QVERIFY(rows.isEmpty());
}

void TestRewardly::buildRowsBonusCountZero()
{
    ConfigData cfg = ConfigManager::defaultConfig();
    cfg.morningSignBonusCount = 0;

    QVector<RewardlyRow> rows =
        RewardlyExportDialog::buildRows(makeRecords(), QDate(2026, 8, 8), cfg);

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
        RewardlyExportDialog::buildRows(makeRecords(), QDate(2026, 8, 8), cfg);

    // Still only 3 bonus rows — capped by the number of on-time sign-ins.
    int bonusCount = 0;
    for (const auto &row : rows) {
        if (row.points > 0) {
            ++bonusCount;
        }
    }
    QCOMPARE(bonusCount, 3);
}

QTEST_MAIN(TestRewardly)
#include "test_rewardly.moc"
