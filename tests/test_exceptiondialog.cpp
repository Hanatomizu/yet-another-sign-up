/**
 * Unit tests for the 设置例外 dialog (exceptiondialog.cpp).
 *
 * The window edits a one-off, in-memory exception list and hands it back via
 * applied(); nothing is written to config.toml. The tests drive its widgets
 * directly, so private *data members* are re-exposed with the classic
 * `#define private public` trick. The member *functions* the tests call
 * (onAddClicked / onRemoveClicked / onAcceptClicked) are public: MSVC
 * mangles member access into symbol names, so calling a private function
 * from a different translation unit would fail to link on Windows.
 *
 * Only paths that do not raise a modal message box are exercised:
 * constructing/loading, adding a valid entry, replacing one, and removing a
 * selected one.
 */

#include <QtTest>
#include <QFile>
#include <QCoreApplication>

#include <vector>
#include <map>

// Pre-include every Qt header exceptiondialog.h pulls in so they are not
// re-parsed while `private` is redefined.
#include <QString>
#include <QStringList>
#include <QDate>
#include <QTime>
#include <QVector>
#include <QSet>
#include <QWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTimeEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>

// Only re-exposes private data members; see the file header for why the
// functions under test are public instead.
#define private public
#include "exceptiondialog.h"
#undef private

#include "configmanager.h"

// exceptiondialog.cpp fills its 按人请假 roster from the global extstunames
// (normally defined in arbiter.cpp); provide definitions for the test.
std::vector<QString> extstunames;
std::map<QString, int> nti;

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
    void ignoreDisablesTimeEditorsAndShowsFlag();
    void loadingIgnoredRowRestoresState();
    void replaceIgnoredWithTimesKeepsSingleRow();

    void rosterListsStudents();
    void addLeaveForMultipleStudents();
    void leaveAddUpdatesAndRemoves();
    void loadingLeaveRowRestoresState();
    void acceptEmitsLeaveList();
};

void TestExceptionDialog::init()
{
    // The window pre-fills the editors from the global [periods] values, so
    // start every case from the defaults (and keep the run hermetic).
    QFile::remove(ConfigManager::configFilePath());

    // Index 0 is the placeholder, like the real roster.
    extstunames = {QString(), QString("张三"), QString("李四"),
                   QString("王五")};
    nti.clear();
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
                     [&](const QVector<PeriodException> &list,
                         const QVector<StudentLeaveException> &) {
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
                     [&](const QVector<PeriodException> &,
                         const QVector<StudentLeaveException> &) {
                         emitted = true;
                     });
    dlg.close();
    QVERIFY(!emitted);
    // The owner's list is untouched: only the window's own copy changed.
    QCOMPARE(existing.split, QTime(17, 30));
}

void TestExceptionDialog::ignoreDisablesTimeEditorsAndShowsFlag()
{
    ExceptionDialog dlg({});

    dlg.dateEdit->setDate(QDate(2026, 10, 8));
    dlg.periodCombo->setCurrentIndex(0);   // 早上
    dlg.ignoreCheck->setChecked(true);

    QVERIFY(!dlg.splitEdit->isEnabled());
    QVERIFY(!dlg.deadlineEdit->isEnabled());
    QVERIFY(dlg.hintLabel->text().contains(QString("已忽略")));

    dlg.onAddClicked();

    QCOMPARE(int(dlg.list.size()), 1);
    QVERIFY(dlg.list[0].ignore);
    QVERIFY(!dlg.list[0].split.isValid());
    QVERIFY(!dlg.list[0].deadline.isValid());

    QCOMPARE(dlg.table->rowCount(), 1);
    QCOMPARE(dlg.table->item(0, 2)->text(), QString("—"));
    QCOMPARE(dlg.table->item(0, 3)->text(), QString("—"));
    QCOMPARE(dlg.table->item(0, 4)->text(), QString("是"));
}

void TestExceptionDialog::loadingIgnoredRowRestoresState()
{
    PeriodException ignored;
    ignored.date = QDate(2026, 10, 9);
    ignored.period = 1;   // 中午
    ignored.ignore = true;

    ExceptionDialog dlg({ignored});

    // Double-clicking the row loads it into the editors. Invoke the private
    // slot through the meta-object: calling a private member function from
    // another translation unit would fail to link on MSVC.
    QVERIFY(QMetaObject::invokeMethod(&dlg, "onTableDoubleClicked",
                                      Q_ARG(int, 0), Q_ARG(int, 0)));

    QVERIFY(dlg.ignoreCheck->isChecked());
    QCOMPARE(dlg.dateEdit->date(), QDate(2026, 10, 9));
    QCOMPARE(dlg.periodCombo->currentIndex(), 1);
    QVERIFY(!dlg.splitEdit->isEnabled());
    QVERIFY(!dlg.deadlineEdit->isEnabled());

    // Unchecking returns to the normal noon editors.
    dlg.ignoreCheck->setChecked(false);
    QVERIFY(dlg.splitEdit->isEnabled());
    QVERIFY(dlg.deadlineEdit->isEnabled());
    QVERIFY(dlg.hintLabel->text().contains(QString("中午")));
}

void TestExceptionDialog::replaceIgnoredWithTimesKeepsSingleRow()
{
    ExceptionDialog dlg({});

    dlg.dateEdit->setDate(QDate(2026, 10, 10));
    dlg.periodCombo->setCurrentIndex(0);
    dlg.ignoreCheck->setChecked(true);
    dlg.onAddClicked();
    QCOMPARE(dlg.table->rowCount(), 1);
    QVERIFY(dlg.list[0].ignore);

    // Same date + period, now with normal times: replaces the ignore entry.
    dlg.ignoreCheck->setChecked(false);
    dlg.splitEdit->setTime(QTime(12, 30));
    dlg.deadlineEdit->setTime(QTime(9, 30));
    dlg.onAddClicked();

    QCOMPARE(dlg.table->rowCount(), 1);
    QCOMPARE(int(dlg.list.size()), 1);
    QVERIFY(!dlg.list[0].ignore);
    QCOMPARE(dlg.list[0].deadline, QTime(9, 30));
    QCOMPARE(dlg.table->item(0, 4)->text(), QString("—"));

    // And back to ignore: the time entry is replaced again.
    dlg.ignoreCheck->setChecked(true);
    dlg.onAddClicked();

    QCOMPARE(dlg.table->rowCount(), 1);
    QCOMPARE(int(dlg.list.size()), 1);
    QVERIFY(dlg.list[0].ignore);
    QVERIFY(!dlg.list[0].deadline.isValid());
    QCOMPARE(dlg.table->item(0, 4)->text(), QString("是"));
}

void TestExceptionDialog::rosterListsStudents()
{
    ExceptionDialog dlg({});

    // The placeholder at index 0 is skipped; the rest are listed unchecked.
    QCOMPARE(dlg.studentList->count(), 3);
    QCOMPARE(dlg.studentList->item(0)->text(), QString("张三"));
    QCOMPARE(dlg.studentList->item(1)->text(), QString("李四"));
    QCOMPARE(dlg.studentList->item(2)->text(), QString("王五"));
    for (int i = 0; i < dlg.studentList->count(); ++i) {
        QCOMPARE(dlg.studentList->item(i)->checkState(), Qt::Unchecked);
    }
}

void TestExceptionDialog::addLeaveForMultipleStudents()
{
    ExceptionDialog dlg({});

    dlg.leaveStartEdit->setDate(QDate(2026, 10, 1));
    dlg.leaveEndEdit->setDate(QDate(2026, 10, 3));
    dlg.leaveEveningCheck->setChecked(false);   // 早上 + 中午 only
    dlg.studentList->item(0)->setCheckState(Qt::Checked);   // 张三
    dlg.studentList->item(1)->setCheckState(Qt::Checked);   // 李四
    dlg.onAddLeaveClicked();

    QCOMPARE(dlg.leaveTable->rowCount(), 2);
    QCOMPARE(int(dlg.leaveList.size()), 2);

    // One entry per student, same date range and periods.
    for (const auto &leave : dlg.leaveList) {
        QCOMPARE(leave.dateFrom, QDate(2026, 10, 1));
        QCOMPARE(leave.dateTo, QDate(2026, 10, 3));
        QCOMPARE(leave.periods, (QSet<int>{0, 1}));
    }

    // Table is sorted by name: 张三 before 李四.
    QCOMPARE(dlg.leaveTable->item(0, 0)->text(), QString("张三"));
    QCOMPARE(dlg.leaveTable->item(0, 1)->text(), QString("2026-10-01"));
    QCOMPARE(dlg.leaveTable->item(0, 2)->text(), QString("2026-10-03"));
    QCOMPARE(dlg.leaveTable->item(0, 3)->text(), QString("早上、中午"));
    QCOMPARE(dlg.leaveTable->item(1, 0)->text(), QString("李四"));
}

void TestExceptionDialog::leaveAddUpdatesAndRemoves()
{
    ExceptionDialog dlg({});

    dlg.leaveStartEdit->setDate(QDate(2026, 10, 2));
    dlg.leaveEndEdit->setDate(QDate(2026, 10, 2));
    dlg.leaveNoonCheck->setChecked(false);
    dlg.leaveEveningCheck->setChecked(false);   // 早上 only
    dlg.studentList->item(0)->setCheckState(Qt::Checked);   // 张三
    dlg.onAddLeaveClicked();

    QCOMPARE(dlg.leaveTable->rowCount(), 1);
    QCOMPARE(int(dlg.leaveList.size()), 1);
    QCOMPARE(dlg.leaveTable->item(0, 3)->text(), QString("早上"));

    // Same student + same range: replaces the periods in place.
    dlg.leaveMorningCheck->setChecked(false);
    dlg.leaveNoonCheck->setChecked(true);
    dlg.onAddLeaveClicked();

    QCOMPARE(dlg.leaveTable->rowCount(), 1);
    QCOMPARE(int(dlg.leaveList.size()), 1);
    QVERIFY(!dlg.leaveList[0].periods.contains(0));
    QVERIFY(dlg.leaveList[0].periods.contains(1));
    QCOMPARE(dlg.leaveTable->item(0, 3)->text(), QString("中午"));

    // A different date range is a separate entry.
    dlg.leaveStartEdit->setDate(QDate(2026, 10, 5));
    dlg.leaveEndEdit->setDate(QDate(2026, 10, 6));
    dlg.onAddLeaveClicked();

    QCOMPARE(dlg.leaveTable->rowCount(), 2);
    QCOMPARE(int(dlg.leaveList.size()), 2);

    // Removing the selected row (10-02) leaves the 10-05 entry.
    dlg.leaveTable->selectRow(0);
    dlg.onRemoveLeaveClicked();

    QCOMPARE(dlg.leaveTable->rowCount(), 1);
    QCOMPARE(int(dlg.leaveList.size()), 1);
    QCOMPARE(dlg.leaveList[0].dateFrom, QDate(2026, 10, 5));
    QCOMPARE(dlg.leaveTable->item(0, 0)->text(), QString("张三"));
}

void TestExceptionDialog::loadingLeaveRowRestoresState()
{
    StudentLeaveException leave;
    leave.name = QString("李四");
    leave.dateFrom = QDate(2026, 10, 3);
    leave.dateTo = QDate(2026, 10, 4);
    leave.periods = {0, 2};   // 早上 + 晚上

    ExceptionDialog dlg({}, {leave});

    // Double-clicking the row loads it into the editors (private slot, so
    // invoke through the meta-object like the period-exception tests do).
    QVERIFY(QMetaObject::invokeMethod(&dlg, "onLeaveTableDoubleClicked",
                                      Q_ARG(int, 0), Q_ARG(int, 0)));

    QCOMPARE(dlg.leaveStartEdit->date(), QDate(2026, 10, 3));
    QCOMPARE(dlg.leaveEndEdit->date(), QDate(2026, 10, 4));
    QVERIFY(dlg.leaveMorningCheck->isChecked());
    QVERIFY(!dlg.leaveNoonCheck->isChecked());
    QVERIFY(dlg.leaveEveningCheck->isChecked());

    // Only the loaded student stays checked.
    for (int i = 0; i < dlg.studentList->count(); ++i) {
        const QListWidgetItem *item = dlg.studentList->item(i);
        QCOMPARE(item->checkState(),
                 item->text() == QString("李四") ? Qt::Checked : Qt::Unchecked);
    }
}

void TestExceptionDialog::acceptEmitsLeaveList()
{
    ExceptionDialog dlg({});

    dlg.leaveStartEdit->setDate(QDate(2026, 10, 8));
    dlg.leaveEndEdit->setDate(QDate(2026, 10, 9));
    dlg.leaveEveningCheck->setChecked(false);
    dlg.studentList->item(2)->setCheckState(Qt::Checked);   // 王五
    dlg.onAddLeaveClicked();

    QVector<PeriodException> receivedPeriods;
    QVector<StudentLeaveException> receivedLeaves;
    bool emitted = false;
    QObject::connect(&dlg, &ExceptionDialog::applied,
                     [&](const QVector<PeriodException> &periods,
                         const QVector<StudentLeaveException> &leaves) {
                         emitted = true;
                         receivedPeriods = periods;
                         receivedLeaves = leaves;
                     });

    dlg.onAcceptClicked();

    QVERIFY(emitted);
    QVERIFY(receivedPeriods.isEmpty());
    QCOMPARE(int(receivedLeaves.size()), 1);
    QCOMPARE(receivedLeaves[0].name, QString("王五"));
    QCOMPARE(receivedLeaves[0].dateFrom, QDate(2026, 10, 8));
    QCOMPARE(receivedLeaves[0].dateTo, QDate(2026, 10, 9));
    QCOMPARE(receivedLeaves[0].periods, (QSet<int>{0, 1}));
    QCOMPARE(dlg.studentLeaves(), receivedLeaves);
}

QTEST_MAIN(TestExceptionDialog)
#include "test_exceptiondialog.moc"
