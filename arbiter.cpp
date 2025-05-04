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

arbiter::~arbiter()
{
    delete ui;
}
