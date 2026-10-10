/**
 * Unit tests for the 设置例外 dialog (exceptiondialog.cpp).
 *
 * The window edits a one-off, in-memory exception list and hands it back via
 * applied(); nothing is written to config.toml. The tests drive its widgets
 * directly (the class members are re-exposed with the same
 * `#define private public` trick used by test_export/test_arbiter).
 *
 * Only paths that do not raise a modal message box are exercised:
 * constructing/loading, adding a valid entry, replacing one, and removing a
 * selected one.
 */

#include <QtTest>
#include <QFile>
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

    void showsInitialExceptions();
    void addUpdatesAndRemoves();
    void eveningHasNoSplit();
    void sortedByDateThenPeriod();
    void acceptEmitsAppliedList();
    void cancelKeepsOwnerListUnchanged();
};

void TestExceptionDialog::init()
{
    // The window pre-fills the editors from the global [periods] values, so
    // start every case from the defaults (and keep the run hermetic).
    QFile::remove(ConfigManager::configFilePath());
}

void TestExceptionDialog::cleanup()
{
    QFile::remove(ConfigManager::configFilePath());
}

void TestExceptionDialog::showsInitialExceptions()
{
    PeriodException ex;
    ex.date = QDate(2026, 10, 1);
    ex.period = 0;
    ex.split = QTime(12, 40);
    ex.deadline = QTime(9, 30);

    ExceptionDialog dlg({ex});
    QCOMPARE(dlg.table->rowCount(), 1);
    QCOMPARE(dlg.table->item(0, 0)->text(), QString("2026-10-01"));
    QCOMPARE(dlg.table->item(0, 1)->text(), QString("早上"));
    QCOMPARE(dlg.table->item(0, 2)->text(), QString("12:40"));
    QCOMPARE(dlg.table->item(0, 3)->text(), QString("09:30"));
}

void TestExceptionDialog::addUpdatesAndRemoves()
{
    ExceptionDialog dlg({});            // nothing configured yet
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
    QVERIFY(dlg.list.isEmpty());
}

void TestExceptionDialog::eveningHasNoSplit()
{
    ExceptionDialog dlg({});
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
    QCOMPARE(int(dlg.list.size()), 1);
    QVERIFY(!dlg.list[0].split.isValid());
    QCOMPARE(dlg.list[0].deadline, QTime(20, 0));
}

void TestExceptionDialog::sortedByDateThenPeriod()
{
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

    // Deliberately unsorted input.
    ExceptionDialog dlg({evening, noon, morning});
    QCOMPARE(dlg.table->rowCount(), 3);
    QCOMPARE(dlg.table->item(0, 0)->text(), QString("2026-10-01"));
    QCOMPARE(dlg.table->item(0, 1)->text(), QString("早上"));
    QCOMPARE(dlg.table->item(1, 0)->text(), QString("2026-10-01"));
    QCOMPARE(dlg.table->item(1, 1)->text(), QString("中午"));
    QCOMPARE(dlg.table->item(2, 0)->text(), QString("2026-10-05"));
    QCOMPARE(dlg.table->item(2, 1)->text(), QString("晚上"));
}

void TestExceptionDialog::acceptEmitsAppliedList()
{
    ExceptionDialog dlg({});

    dlg.dateEdit->setDate(QDate(2026, 10, 6));
    dlg.periodCombo->setCurrentIndex(2);       // 晚上
    dlg.deadlineEdit->setTime(QTime(20, 30));
    dlg.onAddClicked();

    QVector<PeriodException> received;
    bool emitted = false;
    QObject::connect(&dlg, &ExceptionDialog::applied,
                     [&](const QVector<PeriodException> &list) {
                         emitted = true;
                         received = list;
                     });

    dlg.onAcceptClicked();

    QVERIFY(emitted);
    QCOMPARE(int(received.size()), 1);
    QCOMPARE(received[0].date, QDate(2026, 10, 6));
    QCOMPARE(received[0].period, 2);
    QCOMPARE(received[0].deadline, QTime(20, 30));
    // 确定 also exposes the list before/without closing.
    QCOMPARE(dlg.exceptions(), received);
}

void TestExceptionDialog::cancelKeepsOwnerListUnchanged()
{
    PeriodException existing;
    existing.date = QDate(2026, 10, 1);
    existing.period = 1;
    existing.split = QTime(17, 30);
    existing.deadline = QTime(13, 0);

    ExceptionDialog dlg({existing});

    // Edit the list, then close with 取消 (close() without applied()).
    dlg.dateEdit->setDate(QDate(2026, 10, 7));
    dlg.periodCombo->setCurrentIndex(1);
    dlg.splitEdit->setTime(QTime(18, 0));
    dlg.deadlineEdit->setTime(QTime(13, 15));
    dlg.onAddClicked();
    QCOMPARE(dlg.table->rowCount(), 2);

    bool emitted = false;
    QObject::connect(&dlg, &ExceptionDialog::applied,
                     [&] { emitted = true; });
    dlg.close();
    QVERIFY(!emitted);
    // The owner's list is untouched: only the window's own copy changed.
    QCOMPARE(existing.split, QTime(17, 30));
}

QTEST_MAIN(TestExceptionDialog)
#include "test_exceptiondialog.moc"
