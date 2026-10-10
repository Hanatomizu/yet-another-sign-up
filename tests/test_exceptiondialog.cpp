/**
 * Unit tests for the 设置例外 dialog (exceptiondialog.cpp).
 *
 * The dialog loads and saves the [[exceptions]] section of config.toml; the
 * tests drive its widgets directly (the class members are re-exposed with
 * the same `#define private public` trick used by test_export/test_arbiter).
 *
 * Only paths that do not raise a modal message box are exercised:
 * constructing/loading, adding a valid entry, replacing one, and removing a
 * selected one.
 */

#include <QtTest>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

// Pre-include every Qt header exceptiondialog.h pulls in so they are not
// re-parsed while `private` is redefined.
#include <QString>
#include <QDate>
#include <QTime>
#include <QVector>
#include <QWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QTimeEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>

#define private public
#include "exceptiondialog.h"
#undef private

#include "configmanager.h"

class TestExceptionDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void loadsExistingExceptions();
    void addUpdatesAndRemoves();
    void eveningHasNoSplit();
    void sortedByDateThenPeriod();
};

void TestExceptionDialog::init()
{
    QFile::remove(ConfigManager::configFilePath());
}

void TestExceptionDialog::cleanup()
{
    QFile::remove(ConfigManager::configFilePath());
}

void TestExceptionDialog::loadsExistingExceptions()
{
    ConfigData cfg = ConfigManager::defaultConfig();
    PeriodException ex;
    ex.date = QDate(2026, 10, 1);
    ex.period = 0;
    ex.split = QTime(12, 40);
    ex.deadline = QTime(9, 30);
    cfg.exceptions = {ex};
    ConfigManager::saveConfig(cfg);

    ExceptionDialog dlg;
    QCOMPARE(dlg.table->rowCount(), 1);
    QCOMPARE(dlg.table->item(0, 0)->text(), QString("2026-10-01"));
    QCOMPARE(dlg.table->item(0, 1)->text(), QString("早上"));
    QCOMPARE(dlg.table->item(0, 2)->text(), QString("12:40"));
    QCOMPARE(dlg.table->item(0, 3)->text(), QString("09:30"));
}

void TestExceptionDialog::addUpdatesAndRemoves()
{
    ExceptionDialog dlg;               // empty config → empty list
    QCOMPARE(dlg.table->rowCount(), 0);

    dlg.dateEdit->setDate(QDate(2026, 10, 2));
    dlg.periodCombo->setCurrentIndex(1);   // 中午
    dlg.splitEdit->setTime(QTime(18, 0));
    dlg.deadlineEdit->setTime(QTime(12, 45));
    dlg.onAddClicked();

    QCOMPARE(dlg.table->rowCount(), 1);
    QCOMPARE(dlg.table->item(0, 0)->text(), QString("2026-10-02"));
    QCOMPARE(dlg.table->item(0, 1)->text(), QString("中午"));
    QCOMPARE(dlg.table->item(0, 2)->text(), QString("18:00"));
    QCOMPARE(dlg.table->item(0, 3)->text(), QString("12:45"));

    // The same date + period replaces the previous entry.
    dlg.deadlineEdit->setTime(QTime(13, 0));
    dlg.onAddClicked();
    QCOMPARE(dlg.table->rowCount(), 1);
    QCOMPARE(dlg.table->item(0, 3)->text(), QString("13:00"));

    // Removing the selected row clears the working list again.
    dlg.table->selectRow(0);
    dlg.onRemoveClicked();
    QCOMPARE(dlg.table->rowCount(), 0);
    QVERIFY(dlg.exceptions.isEmpty());
}

void TestExceptionDialog::eveningHasNoSplit()
{
    ExceptionDialog dlg;
    dlg.periodCombo->setCurrentIndex(2);   // 晚上

    QVERIFY(!dlg.splitEdit->isEnabled());
    QVERIFY(!dlg.splitEdit->isHidden());
    QVERIFY(dlg.hintLabel->text().contains(QString("没有分界时间")));

    dlg.dateEdit->setDate(QDate(2026, 10, 3));
    dlg.deadlineEdit->setTime(QTime(20, 0));
    dlg.onAddClicked();

    QCOMPARE(dlg.table->rowCount(), 1);
    QCOMPARE(dlg.table->item(0, 2)->text(), QString("—"));
    QCOMPARE(dlg.table->item(0, 3)->text(), QString("20:00"));

    // The stored entry carries no split for 晚上; only the deadline matters.
    QCOMPARE(int(dlg.exceptions.size()), 1);
    QVERIFY(!dlg.exceptions[0].split.isValid());
    QCOMPARE(dlg.exceptions[0].deadline, QTime(20, 0));
}

void TestExceptionDialog::sortedByDateThenPeriod()
{
    ConfigData cfg = ConfigManager::defaultConfig();

    PeriodException evening;
    evening.date = QDate(2026, 10, 5);
    evening.period = 2;
    evening.deadline = QTime(20, 0);

    PeriodException noon;
    noon.date = QDate(2026, 10, 1);
    noon.period = 1;
    noon.split = QTime(17, 30);
    noon.deadline = QTime(13, 0);

    PeriodException morning;
    morning.date = QDate(2026, 10, 1);
    morning.period = 0;
    morning.split = QTime(12, 30);
    morning.deadline = QTime(9, 30);

    cfg.exceptions = {evening, noon, morning};   // deliberately unsorted
    ConfigManager::saveConfig(cfg);

    ExceptionDialog dlg;
    QCOMPARE(dlg.table->rowCount(), 3);
    QCOMPARE(dlg.table->item(0, 0)->text(), QString("2026-10-01"));
    QCOMPARE(dlg.table->item(0, 1)->text(), QString("早上"));
    QCOMPARE(dlg.table->item(1, 0)->text(), QString("2026-10-01"));
    QCOMPARE(dlg.table->item(1, 1)->text(), QString("中午"));
    QCOMPARE(dlg.table->item(2, 0)->text(), QString("2026-10-05"));
    QCOMPARE(dlg.table->item(2, 1)->text(), QString("晚上"));
}

QTEST_MAIN(TestExceptionDialog)
#include "test_exceptiondialog.moc"
