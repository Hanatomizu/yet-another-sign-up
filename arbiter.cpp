#include "arbiter.h"
#include "ui_arbiter.h"

arbiter::arbiter(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::arbiter)
{
    ui->setupUi(this);
    ui->dateEdit->setDisplayFormat("yyyy-MM-dd");
    ui->dateEdit->setDate(QDate::currentDate());
}

void arbiter::closeEvent(QCloseEvent *event) {
    emit closed();
    QWidget::closeEvent(event);
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


    if (tlog.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream in(&tlog);

    }

}

arbiter::~arbiter()
{
    delete ui;
}
