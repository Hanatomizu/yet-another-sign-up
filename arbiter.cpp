#include "arbiter.h"
#include "ui_arbiter.h"

#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QSet>
#include <QTextEdit>
#include <QDebug>

#include "configmanager.h"

std::vector<QString> extstunames;
std::map<QString, int> nti;

arbiter::arbiter(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::arbiter)
{
    ui->setupUi(this);
    ui->dateEdit->setDisplayFormat("yyyy-MM-dd");
    ui->dateEdit->setDate(QDate::currentDate());
    connect(ui->chkdataButton, &QPushButton::pressed, this, &arbiter::checkStat);
}

void arbiter::checkStat(){
    QString dt = ui->dateEdit->date().toString("yyyy-MM-dd");
    QString logdir = QDir::cleanPath(QCoreApplication::applicationDirPath() +
                                     QDir::separator() +
                                     QString("logs") +
                                     QDir::separator() +
                                     dt +
                                     QString(".log")
    );

    QTextEdit *curwin[] = {ui->list1, ui->list2, ui->list3};
    for (int i = 0; i < 3; ++i) {
        curwin[i]->setText(QString());
    }

    if (!QFile::exists(logdir)) {
        qDebug() << "No log file for" << dt;
        return;
    }

    // Parse the whole day's log. "Resigned" (duplicate-attempt) lines are
    // kept so repeated sign-ins can be detected per period.
    ConfigData config = ConfigManager::loadConfig();
    QVector<SignRecord> records =
        parseSignLogFileAll(logdir, ui->dateEdit->date(), config);

    // Split records into the three periods by sign-in time.
    QVector<QVector<SignRecord>> byPeriod(3);
    for (const auto &rec : records) {
        if (rec.period >= 0 && rec.period < 3) {
            byPeriod[rec.period].append(rec);
        }
    }

    for (int p = 0; p < 3; ++p) {
        QSet<QString> signedNames;
        QVector<QString> multiSigned;

        // First occurrence is shown with its time; later occurrences
        // (duplicate attempts) are collected into multiSigned.
        for (const auto &rec : byPeriod[p]) {
            if (signedNames.contains(rec.name)) {
                multiSigned.append(rec.name);
            } else {
                signedNames.insert(rec.name);
                curwin[p]->append(rec.time.toString("HH:mm:ss") +
                                  QString(" ") + rec.name);
            }
        }

        // 未签到
        curwin[p]->append(QString()); // blank line between sections
        curwin[p]->append(QString("未签到："));
        for (int i = 1; i < static_cast<int>(extstunames.size()); ++i) {
            if (!signedNames.contains(extstunames[i])) {
                curwin[p]->append(extstunames[i]);
            }
        }

        // 重复签到
        curwin[p]->append(QString()); // blank line between sections
        curwin[p]->append(QString("重复签到："));
        for (const auto &name : multiSigned) {
            curwin[p]->append(name);
        }
    }
}

arbiter::~arbiter()
{
    delete ui;
}
