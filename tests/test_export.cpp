/**
 * Unit tests for the export parsing logic (exportdialog.cpp):
 * determinePeriod(), determineStatus() and parseLogFile().
 *
 * The methods under test are private, so this file temporarily re-exposes
 * them with the classic `#define private public` trick. The production
 * sources are compiled unchanged.
 */

#include <QtTest>
#include <QFile>
#include <QTextStream>
#include <QTemporaryDir>

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
};

void TestExport::determinePeriodCases()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    QCOMPARE(ExportDialog::determinePeriod(QTime(8, 0), cfg), 0);
    QCOMPARE(ExportDialog::determinePeriod(QTime(12, 0), cfg), 0); // boundary -> morning
    QCOMPARE(ExportDialog::determinePeriod(QTime(12, 1), cfg), 1);
    QCOMPARE(ExportDialog::determinePeriod(QTime(17, 0), cfg), 1); // boundary -> noon
    QCOMPARE(ExportDialog::determinePeriod(QTime(17, 1), cfg), 2);
    QCOMPARE(ExportDialog::determinePeriod(QTime(23, 59), cfg), 2);
}

void TestExport::determineStatusCases()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    // Morning: deadline 09:00
    QCOMPARE(ExportDialog::determineStatus(QTime(8, 59), 0, cfg), QString("签到"));
    QCOMPARE(ExportDialog::determineStatus(QTime(9, 0), 0, cfg), QString("签到"));
    QCOMPARE(ExportDialog::determineStatus(QTime(9, 1), 0, cfg), QString("迟到"));

    // Noon: deadline 12:30
    QCOMPARE(ExportDialog::determineStatus(QTime(12, 30), 1, cfg), QString("签到"));
    QCOMPARE(ExportDialog::determineStatus(QTime(12, 31), 1, cfg), QString("迟到"));

    // Evening: deadline 18:00
    QCOMPARE(ExportDialog::determineStatus(QTime(18, 0), 2, cfg), QString("签到"));
    QCOMPARE(ExportDialog::determineStatus(QTime(18, 1), 2, cfg), QString("迟到"));

    // Unknown period is always treated as late.
    QCOMPARE(ExportDialog::determineStatus(QTime(12, 0), 7, cfg), QString("迟到"));
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

    ExportDialog dlg;
    QVector<SignRecord> records =
        dlg.parseLogFile(path, QDate(2026, 8, 8), ConfigManager::defaultConfig());

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

    ExportDialog dlg;
    QVector<SignRecord> records =
        dlg.parseLogFile(path, QDate(2026, 8, 8), ConfigManager::defaultConfig());

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

    ExportDialog dlg;
    QVector<SignRecord> records =
        dlg.parseLogFile(path, QDate(2026, 8, 8), ConfigManager::defaultConfig());

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

    ExportDialog dlg;
    QVector<SignRecord> records =
        dlg.parseLogFile(path, QDate(2026, 8, 8), ConfigManager::defaultConfig());

    QCOMPARE(records.size(), 1);
    QCOMPARE(records[0].name, QString("李四"));
    QCOMPARE(records[0].time, QTime(8, 40, 0));
    QCOMPARE(records[0].period, 0);
    QCOMPARE(records[0].status, QString("签到"));
}

QTEST_MAIN(TestExport)
#include "test_export.moc"
