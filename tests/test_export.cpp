/**
 * Unit tests for the export parsing logic (signlogparser.cpp) and the
 * Excel export pipeline (exportdialog.cpp).
 *
 * exportToExcel() is private, so this file temporarily re-exposes it with
 * the classic `#define private public` trick. The production sources are
 * compiled unchanged.
 */

#include <QtTest>
#include <QFile>
#include <QTextStream>
#include <QTemporaryDir>
#include <QDir>
#include <QSet>
#include <QCoreApplication>

#include "xlsxdocument.h"

// Pre-include everything exportdialog.h pulls in so it is not re-parsed
// while `private` is redefined.
#include <QString>
#include <QTime>
#include <QDate>
#include <QVector>
#include <QWidget>
#include <QDateEdit>
#include <QPushButton>

#define private public
#include "exportdialog.h"
#undef private

// Exported functions reference these globals (normally defined in arbiter.cpp).
std::vector<QString> extstunames;
std::map<QString, int> nti;

class TestExport : public QObject
{
    Q_OBJECT

private slots:
    void determinePeriodCases();
    void determineStatusCases();
    void parseLogFileSkipsMarkersAndResigned();
    void parseLogFileParsesRecords();
    void parseLogFileNormalizesDoubleColon();
    void parseLogFileHandlesMalformedLines();
    void parseLogFileAllKeepsResignedLines();
    void exportToExcelWritesPeriodInDateColumn();
};

void TestExport::determinePeriodCases()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    QCOMPARE(determineSignPeriod(QTime(8, 0), cfg), 0);
    QCOMPARE(determineSignPeriod(QTime(12, 0), cfg), 0); // boundary -> morning
    QCOMPARE(determineSignPeriod(QTime(12, 1), cfg), 1);
    QCOMPARE(determineSignPeriod(QTime(17, 0), cfg), 1); // boundary -> noon
    QCOMPARE(determineSignPeriod(QTime(17, 1), cfg), 2);
    QCOMPARE(determineSignPeriod(QTime(23, 59), cfg), 2);
}

void TestExport::determineStatusCases()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    // Morning: deadline 09:00
    QCOMPARE(determineSignStatus(QTime(8, 59), 0, cfg), QString("签到"));
    QCOMPARE(determineSignStatus(QTime(9, 0), 0, cfg), QString("签到"));
    QCOMPARE(determineSignStatus(QTime(9, 1), 0, cfg), QString("迟到"));

    // Noon: deadline 12:30
    QCOMPARE(determineSignStatus(QTime(12, 30), 1, cfg), QString("签到"));
    QCOMPARE(determineSignStatus(QTime(12, 31), 1, cfg), QString("迟到"));

    // Evening: deadline 18:00
    QCOMPARE(determineSignStatus(QTime(18, 0), 2, cfg), QString("签到"));
    QCOMPARE(determineSignStatus(QTime(18, 1), 2, cfg), QString("迟到"));

    // Unknown period is always treated as late.
    QCOMPARE(determineSignStatus(QTime(12, 0), 7, cfg), QString("迟到"));
}

void TestExport::parseLogFileSkipsMarkersAndResigned()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QString("2026-08-08.log"));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "- 2026-08-08 yasu created this file\n";
    out << "= 2026-08-08 yasu rechecked this file\n";
    out << "2026.08.08 08:30:00, 张三, Resigned\n";
    out << "not a valid record\n";
    out << "2026.08.08 08:31, 李四\n"; // missing status -> only 2 fields
    f.close();

    QVector<SignRecord> records =
        parseSignLogFile(path, QDate(2026, 8, 8), ConfigManager::defaultConfig());

    QCOMPARE(records.size(), 0);
}

void TestExport::parseLogFileParsesRecords()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QString("2026-08-08.log"));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2026.08.08 08:30:00, 张三, Signed\n";  // morning on time
    out << "2026.08.08 09:01:00, 王五, Signed\n";  // morning late
    out << "2026.08.08 12:45:00, 李四, Signed\n";  // noon late
    f.close();

    QVector<SignRecord> records =
        parseSignLogFile(path, QDate(2026, 8, 8), ConfigManager::defaultConfig());

    QCOMPARE(records.size(), 3);

    QCOMPARE(records[0].name, QString("张三"));
    QCOMPARE(records[0].date, QDate(2026, 8, 8));
    QCOMPARE(records[0].time, QTime(8, 30, 0));
    QCOMPARE(records[0].period, 0);
    QCOMPARE(records[0].status, QString("签到"));

    QCOMPARE(records[1].name, QString("王五"));
    QCOMPARE(records[1].time, QTime(9, 1, 0));
    QCOMPARE(records[1].period, 0);
    QCOMPARE(records[1].status, QString("迟到"));

    QCOMPARE(records[2].name, QString("李四"));
    QCOMPARE(records[2].time, QTime(12, 45, 0));
    QCOMPARE(records[2].period, 1);
    QCOMPARE(records[2].status, QString("迟到"));
}

void TestExport::parseLogFileNormalizesDoubleColon()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QString("2026-08-08.log"));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2026.08.08 08:32::00, 张三, Signed\n"; // "hh:mm::ss" typo
    f.close();

    QVector<SignRecord> records =
        parseSignLogFile(path, QDate(2026, 8, 8), ConfigManager::defaultConfig());

    QCOMPARE(records.size(), 1);
    QCOMPARE(records[0].time, QTime(8, 32, 0));
    QCOMPARE(records[0].status, QString("签到"));
}

void TestExport::parseLogFileHandlesMalformedLines()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QString("2026-08-08.log"));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2026.08.08 08:40, 李四, Signed\n";    // seconds missing -> HH:mm fallback
    out << "2026.08.08 xx:xx:xx, 王五, Signed\n"; // unparseable time
    out << "2026.08.08 08:41:00, , Signed\n";     // empty name
    f.close();

    QVector<SignRecord> records =
        parseSignLogFile(path, QDate(2026, 8, 8), ConfigManager::defaultConfig());

    QCOMPARE(records.size(), 1);
    QCOMPARE(records[0].name, QString("李四"));
    QCOMPARE(records[0].time, QTime(8, 40, 0));
    QCOMPARE(records[0].period, 0);
    QCOMPARE(records[0].status, QString("签到"));
}

void TestExport::parseLogFileAllKeepsResignedLines()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path = dir.filePath(QString("2026-08-08.log"));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2026.08.08 08:30:00, 张三, Signed\n";
    out << "2026.08.08 08:31:00, 张三, Resigned\n"; // duplicate attempt
    out << "2026.08.08 12:45:00, 李四, Signed\n";   // noon late
    f.close();

    ConfigData cfg = ConfigManager::defaultConfig();

    // parseSignLogFileAll keeps the Resigned line with status "Resigned".
    QVector<SignRecord> all =
        parseSignLogFileAll(path, QDate(2026, 8, 8), cfg);
    QCOMPARE(all.size(), 3);
    QCOMPARE(all[0].name, QString("张三"));
    QCOMPARE(all[0].status, QString("签到"));
    QCOMPARE(all[1].name, QString("张三"));
    QCOMPARE(all[1].status, QString("Resigned"));
    QCOMPARE(all[1].period, 0); // period still derived from time
    QCOMPARE(all[2].name, QString("李四"));
    QCOMPARE(all[2].status, QString("迟到"));

    // parseSignLogFile (the export-facing variant) drops Resigned lines.
    QVector<SignRecord> filtered =
        parseSignLogFile(path, QDate(2026, 8, 8), cfg);
    QCOMPARE(filtered.size(), 2);
    QCOMPARE(filtered[0].name, QString("张三"));
    QCOMPARE(filtered[0].status, QString("签到"));
    QCOMPARE(filtered[1].name, QString("李四"));
    QCOMPARE(filtered[1].status, QString("迟到"));
}

void TestExport::exportToExcelWritesPeriodInDateColumn()
{
    // exportToExcel reads the roster from the global extstunames
    // (index 0 is a placeholder) and logs from <applicationDirPath>/logs.
    extstunames.clear();
    extstunames.push_back(QString());   // placeholder
    extstunames.push_back(QString("张三"));
    extstunames.push_back(QString("李四"));
    extstunames.push_back(QString("王五"));

    // Remove any stale config so exportToExcel uses the defaults
    // (morning deadline 09:00, noon 12:30, evening 18:00).
    const QString configPath = QCoreApplication::applicationDirPath() +
                               QString("/config.toml");
    QFile::remove(configPath);

    const QString logDirPath = QCoreApplication::applicationDirPath() +
                               QString("/logs");
    QDir logDir(logDirPath);
    QVERIFY(logDir.mkpath(logDirPath));
    const QString logPath = logDirPath + QString("/2026-08-08.log");

    QFile f(logPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2026.08.08 08:30:00, 张三, Signed\n"; // morning on time -> top-10 签到
    out << "2026.08.08 09:05:00, 李四, Signed\n"; // morning late -> 迟到
    out << "2026.08.08 12:45:00, 王五, Signed\n"; // noon late -> 迟到
    out << "2026.08.08 18:30:00, 张三, Signed\n"; // evening late -> 迟到
    f.close();

    QTemporaryDir outDir;
    QVERIFY(outDir.isValid());
    const QString xlsxPath = outDir.filePath(QString("out.xlsx"));

    ExportDialog dlg;
    QVERIFY(dlg.exportToExcel(QDate(2026, 8, 8), QDate(2026, 8, 8), xlsxPath));

    QXlsx::Document xlsx(xlsxPath);

    // Header row
    QCOMPARE(xlsx.read(1, 1).toString(), QString("姓名"));
    QCOMPARE(xlsx.read(1, 2).toString(), QString("日期"));
    QCOMPARE(xlsx.read(1, 3).toString(), QString("类型"));

    // Collect every data row as "姓名|日期列|类型" and check the exact set.
    // Absent rows are emitted in QSet order, so the set must be compared
    // order-independently.
    QSet<QString> rows;
    int row = 2;
    while (!xlsx.read(row, 1).toString().isEmpty()) {
        rows.insert(xlsx.read(row, 1).toString() + QString("|") +
                    xlsx.read(row, 2).toString() + QString("|") +
                    xlsx.read(row, 3).toString());
        ++row;
    }
    QCOMPARE(rows.size(), 9);

    // The date column must carry the concrete period (早上/中午/晚上).
    QVERIFY(rows.contains(QString("张三|2026-08-08 早上|签到")));
    QVERIFY(rows.contains(QString("李四|2026-08-08 早上|迟到")));
    QVERIFY(rows.contains(QString("王五|2026-08-08 中午|迟到")));
    QVERIFY(rows.contains(QString("张三|2026-08-08 晚上|迟到")));
    QVERIFY(rows.contains(QString("王五|2026-08-08 早上|未签到")));
    QVERIFY(rows.contains(QString("张三|2026-08-08 中午|未签到")));
    QVERIFY(rows.contains(QString("李四|2026-08-08 中午|未签到")));
    QVERIFY(rows.contains(QString("李四|2026-08-08 晚上|未签到")));
    QVERIFY(rows.contains(QString("王五|2026-08-08 晚上|未签到")));

    // No row may contain a bare date without the period suffix.
    for (const auto &r : rows) {
        QVERIFY(!r.contains(QString("|2026-08-08|")));
    }

    // Clean up the files created next to the test binary.
    QFile::remove(logPath);
    QDir().rmdir(logDirPath);
    QFile::remove(configPath);
}

QTEST_MAIN(TestExport)
#include "test_export.moc"
