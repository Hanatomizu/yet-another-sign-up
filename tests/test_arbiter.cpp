/**
 * Unit tests for the arbiter statistics widget (arbiter.cpp):
 * nameParser(), timeParser() and checkStat().
 *
 * The methods under test are private, so this file temporarily re-exposes
 * them with the classic `#define private public` trick. The production
 * sources are compiled unchanged.
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
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QIODevice>
#include <QTextStream>
#include <QDebug>
#include <QTextEdit>
#include <QDateEdit>
#include <QCoreApplication>
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

    void nameParserExtractsName();
    void timeParserExtractsTime();
    void checkStatSummarizesDay();
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

void TestArbiter::nameParserExtractsName()
{
    arbiter ab;
    QCOMPARE(ab.nameParser(QString("2026.08.08 08:30:00, 张三, Signed")),
             QString("张三"));
    QCOMPARE(ab.nameParser(QString("2026.08.08 12:45:00, 李四, Resigned")),
             QString("李四"));
}

void TestArbiter::timeParserExtractsTime()
{
    arbiter ab;
    QCOMPARE(ab.timeParser(QString("2026.08.08 08:30:00, 张三, Signed")),
             QString("08:30:00"));
}

void TestArbiter::checkStatSummarizesDay()
{
    QDate day(2026, 8, 8);
    QVERIFY(QDir().mkpath(appDir() + QString("/logs")));

    QFile f(logPath(day));
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "- 2026-08-08 yasu created this file\n";
    out << "2026.08.08 08:30:00, 张三, Signed\n";
    out << "2026.08.08 08:31:00, 张三, Signed\n"; // duplicate in morning
    out << "2026.08.08 08:32:00, 李四, Signed\n";
    out << "= 2026-08-08 yasu rechecked this file\n"; // period 2 starts
    out << "2026.08.08 12:10:00, 张三, Signed\n";
    f.close();

    arbiter ab;
    QVERIFY(dateEdit(ab) != nullptr);
    dateEdit(ab)->setDate(day);
    ab.checkStat();

    QString list1 = list(ab, 1)->toPlainText();
    QVERIFY(list1.contains(QString("08:30:00 张三")));
    QVERIFY(list1.contains(QString("08:32:00 李四")));
    QVERIFY(list1.contains(QString("未签到：\n王五"))); // 王五 missing
    QVERIFY(list1.contains(QString("重复签到：\n张三"))); // duplicate
    QVERIFY(!list1.contains(QString("12:10:00 张三"))); // period 2 must not leak in

    QString list2 = list(ab, 2)->toPlainText();
    QVERIFY(list2.contains(QString("12:10:00 张三")));
    QVERIFY(list2.contains(QString("未签到：\n李四\n王五"))); // both missing
    QVERIFY(list2.contains(QString("重复签到：\n"))); // no duplicates in noon

    QString list3 = list(ab, 3)->toPlainText();
    QVERIFY(list3.isEmpty());
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
