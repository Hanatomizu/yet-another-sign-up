#include "arbiter.h"
#include "ui_arbiter.h"
#include "databasemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

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


QString arbiter::nameParser(QString content) {
    QString res;
    int it = 21;
    while (content[it] != QString(",")) {
        res += content[it++];
    }
    return res;
}

QString arbiter::timeParser(QString content) {
    QString tmp;
    for (int i = 11; content[i] != QString(","); ++i) {
        tmp += content[i];
    }
    return tmp;
}

void arbiter::checkStat(){
    // Initialize database manager
    DatabaseManager dbManager;
    if (!dbManager.initializeDatabase()) {
        qDebug() << "Failed to initialize database in arbiter:" << dbManager.lastError();
        return;
    }
    
    QDate selectedDate = ui->dateEdit->date();
    
    QTextEdit *curwin[] = {ui->list1, ui->list2, ui->list3};
    QString periodNames[] = {"早上", "下午", "晚上"};
    
    // Clear all windows
    for (int i = 0; i < 3; ++i) {
        curwin[i]->setText(QString());
    }
    
    // For tracking which students signed up in each period
    std::vector<bool> signedUp[3];
    int stucnt = extstunames.size() - 1;
    
    for (int p = 0; p < 3; ++p) {
        signedUp[p].resize(stucnt + 1, false);
    }
    
    // Get and display signups for each period
    for (int period = 0; period < 3; ++period) {
        QVector<QPair<QString, QDateTime>> signups = dbManager.getSignUpsForDateAndPeriod(selectedDate, period);
        
        // Display signups for this period
        curwin[period]->setText(periodNames[period] + "签到：\n");
        for (const auto& signup : signups) {
            QString name = signup.first;
            QDateTime timestamp = signup.second;
            curwin[period]->setText(curwin[period]->toPlainText() + timestamp.toString("hh:mm:ss") + " " + name + "\n");
            
            // Mark this student as signed up in this period
            if (nti.find(name) != nti.end()) {
                signedUp[period][nti[name]] = true;
            }
        }
        
        // Find students who didn't sign up in this period
        QVector<QString> notSigned;
        for (int i = 1; i <= stucnt; ++i) {
            if (!signedUp[period][i]) {
                notSigned.append(extstunames[i]);
            }
        }
        
        // Display students who didn't sign up in this period
        curwin[period]->setText(curwin[period]->toPlainText() + "\n未签到：\n");
        for (const QString& name : notSigned) {
            curwin[period]->setText(curwin[period]->toPlainText() + name + "\n");
        }
    }
}

arbiter::~arbiter()
{
    delete ui;
}
