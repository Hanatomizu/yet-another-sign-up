#include "arbiter.h"
#include "ui_arbiter.h"

arbiter::arbiter(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::arbiter)
{
    ui->setupUi(this);


}

arbiter::~arbiter()
{
    delete ui;
}
