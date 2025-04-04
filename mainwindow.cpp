#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->Submit, &QPushButton::released, this, &MainWindow::onSubmitClicked);
}

void MainWindow::onSubmitClicked(){
    QString numb = ui->NumberInput->toPlainText();
    ui->NumberInput->setText("");

}

MainWindow::~MainWindow()
{
    delete ui;
}
