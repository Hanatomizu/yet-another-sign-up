#include "arbiter.h"
#include "ui_arbiter.h"

arbiter::arbiter(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::arbiter)
{
    ui->setupUi(this);


}

void arbiter::closeEvent(QCloseEvent *event) {
    emit closed();
    QWidget::closeEvent(event);
}

arbiter::~arbiter()
{
    delete ui;
}
