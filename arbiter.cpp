#include "arbiter.h"
#include "ui_arbiter.h"

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
    QString dt = ui->dateEdit->date().toString("yyyy-MM-dd");
    QString logdir = QDir::cleanPath(QCoreApplication::applicationDirPath() +
                                     QDir::separator() +
                                     QString("logs") +
                                     QDir::separator() +
                                     dt +
                                     QString(".log")
    );

    QFile tlog(logdir);
    int stucnt = extstunames.size()-1;
    QTextEdit *curwin[] = {ui->list1, ui->list2, ui->list3};
    for (int i = 0; i < 3; ++i) {
        curwin[i]->setText(QString());
    }
    int pteit = 0;
    // std::vector<QString> notSigned;
    std::vector<QString> multiSigned;
    std::vector<bool> isSigned(stucnt+1);
    if (tlog.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream in(&tlog);
        while(!in.atEnd()) {
            if (pteit == 3) return;
            QString content = in.readLine();
            if (content[0] == QString("-")) {
                continue;
            }
            // 判断是否进入下一个时段
            if (content[0] == QString("=")) {
                curwin[pteit]->setText(curwin[pteit]->toPlainText() + QString("\n未签到：\n"));
                for (int i = 1; i <= stucnt; ++i) {
                    if (!isSigned[i]) {
                        curwin[pteit]->setText(curwin[pteit]->toPlainText() + extstunames[i] + QString("\n"));
                    }
                }

                curwin[pteit]->setText(curwin[pteit]->toPlainText() + QString("\n重复签到：\n"));
                for (int i = 0; i < multiSigned.size(); ++i) {
                    curwin[pteit]->setText(curwin[pteit]->toPlainText() + multiSigned[i] + QString("\n"));
                }
                // 进入下一个时间段
                ++pteit;
                while (!multiSigned.empty()) {
                    multiSigned.pop_back();
                }
                for (int i = 0; i < isSigned.size(); ++i) {
                    isSigned[i] = 0;
                }
                continue;
            }
            if (content.size() < 20) return;
            QString name = arbiter::nameParser(content);
            // 如果重复签到
            if (isSigned[nti[name]]) {
                multiSigned.push_back(name);
            } else {
                isSigned[nti[name]] = 1;
                QString strcurt = arbiter::timeParser(content);
                curwin[pteit]->setText(curwin[pteit]->toPlainText() + strcurt + QString(" ") + name + QString("\n"));
            }
        }
        // night check;
        curwin[pteit]->setText(curwin[pteit]->toPlainText() + QString("\n未签到：\n"));
        for (int i = 1; i <= stucnt; ++i) {
            if (!isSigned[i]) {
                curwin[pteit]->setText(curwin[pteit]->toPlainText() + extstunames[i] + QString("\n"));
            }
        }

        curwin[pteit]->setText(curwin[pteit]->toPlainText() + QString("\n重复签到：\n"));
        for (int i = 0; i < multiSigned.size(); ++i) {
            curwin[pteit]->setText(curwin[pteit]->toPlainText() + multiSigned[i] + QString("\n"));
        }
        tlog.close();
    } else {
        qDebug() << "Failed to open log file:" << tlog.errorString();
    }

}

arbiter::~arbiter()
{
    delete ui;
}
