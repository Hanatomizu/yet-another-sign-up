/**
 * Unit tests for the arbiter statistics widget (arbiter.cpp): checkStat().
 *
 * The method under test is private, so this file temporarily re-exposes
 * it with the classic `#define private public` trick. The production
 * sources are compiled unchanged.
 *
 * Periods (早上/中午/晚上) are split by sign-in TIME using the configured
 * boundaries; session "=" markers in the log must be ignored.
 */

#include <QtTest>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDate>

// Pre-include everything arbiter.h / signup.h / configmanager.h pull in so it
// is not re-parsed while `private` is redefined.
#include <QString>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QIODevice>
#include <QTextStream>
#include <QDebug>
#include <QTextEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QCoreApplication>
#include <QVector>
#include <QSet>
#include <vector>
#include <string>
#include <map>

#define private public
#include "arbiter.h"
#undef private

class TestArbiter : public QObject
{
    Q_OBJECT

private:
    QString appDir() const { return QCoreApplication::applicationDirPath(); }
    QString logPath(const QDate &day) const
    {
        return appDir() + QString("/logs/") + day.toString("yyyy-MM-dd") +
               QString(".log");
    }

    QDateEdit *dateEdit(arbiter &ab) const
    {
        return ab.findChild<QDateEdit *>(QString("dateEdit"));
    }

    QTextEdit *list(arbiter &ab, int index) const
    {
        return ab.findChild<QTextEdit *>(QString("list%1").arg(index));
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    void checkStatSplitsByTime();
    void checkStatMissingLog();
};

void TestArbiter::initTestCase()
{
    QDir(appDir() + QString("/logs")).removeRecursively();

    // Simulate the global student registry populated by Yasu::initNamelist().
    extstunames = {QString(), QString("张三"), QString("李四"), QString("王五")};
    nti.clear();
    nti[QString("张三")] = 1;
    nti[QString("李四")] = 2;
    nti[QString("王五")] = 3;
}

void TestArbiter::cleanupTestCase()
{
    QDir(appDir() + QString("/logs")).removeRecursively();
}

void TestArbiter::checkStatSplitsByTime()
{
    QDate day(2026, 8, 8);
    QVERIFY(QDir().mkpath(appDir() + QString("/logs")));

    QFile f(logPath(day));
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "- 2026-08-08 yasu created this file\n";
    out << "2026.08.08 08:30:00, 张三, Signed\n";
    out << "2026.08.08 08:31:00, 张三, Resigned\n"; // duplicate in morning
    out << "2026.08.08 08:32:00, 李四, Signed\n";
    // A session marker must NOT split periods anymore — splitting is by time.
    out << "= 2026-08-08 yasu rechecked this file\n";
    out << "2026.08.08 12:10:00, 张三, Signed\n";
    out << "2026.08.08 12:45:00, 李四, Resigned\n"; // first noon occurrence
    out << "2026.08.08 12:46:00, 李四, Resigned\n"; // duplicate attempt in noon
    out << "2026.08.08 18:20:00, 王五, Signed\n";   // evening
    f.close();

    arbiter ab;
    QVERIFY(dateEdit(ab) != nullptr);
    dateEdit(ab)->setDate(day);
    ab.checkStat();

    // Morning: 张三 + 李四 signed, 王五 absent, 张三 duplicated.
    QString list1 = list(ab, 1)->toPlainText();
    QVERIFY(list1.contains(QString("08:30:00 张三")));
    QVERIFY(list1.contains(QString("08:32:00 李四")));
    QVERIFY(list1.contains(QString("未签到：\n王五"))); // 王五 missing
    QVERIFY(list1.contains(QString("重复签到：\n张三"))); // duplicate
    QVERIFY(!list1.contains(QString("12:10:00 张三"))); // noon must not leak in

    // Noon: 张三 signed; 李四's first noon record (a Resigned attempt) is
    // shown as present, and his second attempt shows up under 重复签到;
    // 王五 absent.
    QString list2 = list(ab, 2)->toPlainText();
    QVERIFY(list2.contains(QString("12:10:00 张三")));
    QVERIFY(list2.contains(QString("12:45:00 李四")));
    QVERIFY(list2.contains(QString("未签到：\n王五")));
    QVERIFY(list2.contains(QString("重复签到：\n李四")));
    QVERIFY(!list2.contains(QString("12:46:00 李四"))); // dup not in signed list
    QVERIFY(!list2.contains(QString("08:30:00 张三"))); // morning must not leak in

    // Evening: 王五 signed; 张三 and 李四 absent; no duplicates.
    QString list3 = list(ab, 3)->toPlainText();
    QVERIFY(list3.contains(QString("18:20:00 王五")));
    QVERIFY(list3.contains(QString("未签到：\n张三\n李四")));
    QVERIFY(list3.contains(QString("重复签到：")));
    QVERIFY(!list3.contains(QString("重复签到：\n张三"))); // nobody duplicated
    QVERIFY(!list3.contains(QString("12:10:00 张三"))); // noon must not leak in
}

void TestArbiter::checkStatMissingLog()
{
    arbiter ab;
    dateEdit(ab)->setDate(QDate(1999, 1, 1)); // no log file for this date

    ab.checkStat(); // must not crash

    QVERIFY(list(ab, 1)->toPlainText().isEmpty());
    QVERIFY(list(ab, 2)->toPlainText().isEmpty());
    QVERIFY(list(ab, 3)->toPlainText().isEmpty());
}

QTEST_MAIN(TestArbiter)
#include "test_arbiter.moc"
