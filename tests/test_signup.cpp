/**
 * Unit tests for the Yasu sign-up logic (signup.cpp).
 *
 * Yasu resolves the namelist/log/data paths relative to the application
 * directory, so the tests prepare a fresh `names` file next to the test
 * binary and clean up all generated files afterwards.
 */

#include <QtTest>
#include <QFile>
#include <QDir>
#include <QTextStream>

#include "signup.h"

// Forward declaration of the helper defined in signup.cpp.
namespace SignUpAlgorithms {
int qstringToInt(QString);
}

// The application defines these globals in arbiter.cpp; the sign-up tests
// provide their own copies so they don't need the arbiter UI sources.
std::vector<QString> extstunames;
std::map<QString, int> nti;

class TestSignUp : public QObject
{
    Q_OBJECT

private:
    Yasu yasu;

    QString appDir() const { return QCoreApplication::applicationDirPath(); }

private slots:
    void initTestCase();
    void cleanupTestCase();

    void initCreatesDirsAndLog();
    void initNamelistReadsNames();
    void qstringToIntCases();
    void signUpValid();
    void signUpOtherStudent();
    void signUpDuplicate();
    void signUpInvalidInput();
    void signUpOutOfRange();
    void logFileContainsSignups();
};

void TestSignUp::initTestCase()
{
    // Start from a clean, deterministic state.
    QDir(appDir() + QString("/logs")).removeRecursively();
    QDir(appDir() + QString("/data")).removeRecursively();
    QFile::remove(appDir() + QString("/config.toml"));
    QFile::remove(appDir() + QString("/config"));
    QFile::remove(appDir() + QString("/names"));

    QFile names(appDir() + QString("/names"));
    QVERIFY(names.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&names);
    out << QString("张三\n李四\n王五\n");
    names.close();

    extstunames.clear();
    nti.clear();
}

void TestSignUp::cleanupTestCase()
{
    QDir(appDir() + QString("/logs")).removeRecursively();
    QDir(appDir() + QString("/data")).removeRecursively();
    QFile::remove(appDir() + QString("/config.toml"));
    QFile::remove(appDir() + QString("/config"));
    QFile::remove(appDir() + QString("/names"));
}

void TestSignUp::initCreatesDirsAndLog()
{
    QCOMPARE(yasu.initConfigFiles(), 0);

    QVERIFY(QDir(appDir() + QString("/logs")).exists());
    QVERIFY(QDir(appDir() + QString("/data")).exists());

    QString logPath = appDir() + QString("/logs/") +
                      QDate::currentDate().toString("yyyy-MM-dd") +
                      QString(".log");
    QVERIFY(QFile::exists(logPath));
    QVERIFY(QFile::exists(appDir() + QString("/data/") +
                          QDate::currentDate().toString("yyyy-MM-dd") +
                          QString(".data")));
    QVERIFY(QFile::exists(ConfigManager::configFilePath()));
}

void TestSignUp::initNamelistReadsNames()
{
    QCOMPARE(yasu.initNamelist(), 0);

    QCOMPARE(static_cast<int>(extstunames.size()), 4);
    QCOMPARE(extstunames[1], QString("张三"));
    QCOMPARE(extstunames[2], QString("李四"));
    QCOMPARE(extstunames[3], QString("王五"));
    QCOMPARE(nti[QString("张三")], 1);
    QCOMPARE(nti[QString("王五")], 3);
}

void TestSignUp::qstringToIntCases()
{
    QCOMPARE(SignUpAlgorithms::qstringToInt(QString("123")), 123);
    QCOMPARE(SignUpAlgorithms::qstringToInt(QString("007")), 7);
    QCOMPARE(SignUpAlgorithms::qstringToInt(QString("0")), 0);
    QCOMPARE(SignUpAlgorithms::qstringToInt(QString("12a")), -1);
    QCOMPARE(SignUpAlgorithms::qstringToInt(QString("-1")), -1);
    QCOMPARE(SignUpAlgorithms::qstringToInt(QString()), 0);
}

void TestSignUp::signUpValid()
{
    QPair<int, QString> result = yasu.sign_up(QString("1"));
    QCOMPARE(result.first, 0);
    QCOMPARE(result.second, QString("张三"));
}

void TestSignUp::signUpOtherStudent()
{
    QPair<int, QString> result = yasu.sign_up(QString("3"));
    QCOMPARE(result.first, 0);
    QCOMPARE(result.second, QString("王五"));
}

void TestSignUp::signUpDuplicate()
{
    QPair<int, QString> result = yasu.sign_up(QString("1"));
    QCOMPARE(result.first, 2);
    QVERIFY(result.second.isEmpty());
}

void TestSignUp::signUpInvalidInput()
{
    QPair<int, QString> result;

    result = yasu.sign_up(QString());
    QCOMPARE(result.first, -1);

    result = yasu.sign_up(QString("abc"));
    QCOMPARE(result.first, -1);

    result = yasu.sign_up(QString("1a"));
    QCOMPARE(result.first, -1);
}

void TestSignUp::signUpOutOfRange()
{
    QPair<int, QString> result;

    result = yasu.sign_up(QString("0"));
    QCOMPARE(result.first, -1);

    // Only 3 students were loaded from the namelist.
    result = yasu.sign_up(QString("4"));
    QCOMPARE(result.first, -1);

    result = yasu.sign_up(QString("999"));
    QCOMPARE(result.first, -1);
}

void TestSignUp::logFileContainsSignups()
{
    QString logPath = appDir() + QString("/logs/") +
                      QDate::currentDate().toString("yyyy-MM-dd") +
                      QString(".log");

    QFile log(logPath);
    QVERIFY(log.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(log.readAll());
    log.close();

    QVERIFY(content.contains(QString("张三")));
    QVERIFY(content.contains(QString("李四")) == false); // never signed up
    QVERIFY(content.contains(QString("王五")));
    QVERIFY(content.contains(QString("Signed")));
    QVERIFY(content.contains(QString("Resigned"))); // from the duplicate attempt
}

QTEST_GUILESS_MAIN(TestSignUp)
#include "test_signup.moc"
