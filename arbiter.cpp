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

void arbiter::closeEvent(QCloseEvent *event) {
    emit arbiter::closed();
    QWidget::closeEvent(event);
}

QString arbiter::nameParser(QString content) {
    QString res;
    int it = 21;
    while (content[it] != QString(",")) {
        res += content[it];
    }
    return res;
}

QDateTime arbiter::timeParser(QString content) {
    QDateTime res;
    QString tmp;
    for (int i = 0; content[i] != QString(","); ++i) {
        tmp += content[i];
    }
    res = QDateTime::fromString(tmp, "yyyy-MM-dd hh-mm-ss");
    return res;
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
    QPlainTextEdit *curwin[] = {ui->list1, ui->list2, ui->list3};
    int pteit = 0;
    // std::vector<QString> notSigned;
    std::vector<QString> multiSigned;
    std::vector<bool> isSigned(stucnt+1);
    if (tlog.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream in(&tlog);
        while(!in.atEnd()) {
            QString content = in.readLine();
            if (content[0] == QString("-")) {
                continue;
            }
            // 判断是否进入下一个时段
            if (content[0] == QString("=")) {
                curwin[pteit]->appendPlainText(QString("\n未签到："));
                for (int i = 1; i <= stucnt; ++i) {
                    if (!isSigned[i]) {
                        curwin[pteit]->appendPlainText(extstunames[i]);
                    }
                }

                curwin[pteit]->appendPlainText(QString("\n重复签到："));
                for (int i = 0; i < multiSigned.size(); ++i) {
                    curwin[pteit]->appendPlainText(multiSigned[i]);
                }
                // 进入下一个时间段
                ++pteit;
            }
            QString name = arbiter::nameParser(content);
            // 如果重复签到
            if (isSigned[nti[name]]) {
                multiSigned.push_back(name);
            } else {
                isSigned[nti[name]] = 1;
            }
            QDateTime curt = arbiter::timeParser(content);
            QString strcurt = curt.toString("hh-mm-ss");
            curwin[pteit]->appendPlainText((strcurt + QString(" ") + name));
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
