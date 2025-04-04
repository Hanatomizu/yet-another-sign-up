/**
 *     Yet Another Sign Up - A new sign up system for class managements
 *     Copyright (C) 2025  知念夏世 <chart11from21@outlook.com>

 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.

 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.

 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->AdminPanelEntrance, &QPushButton::released, this, &MainWindow::onAdminPanelClicked);
    connect(ui->Submit, &QPushButton::released, this, &MainWindow::onSubmitClicked);
    connect(ui->NumberInput, &QLineEdit::returnPressed, this, &MainWindow::onSubmitClicked);
}

void MainWindow::onSubmitClicked(){
    QString numb = ui->NumberInput->text();
    ui->NumberInput->setText("");


    ui->Announcer->setText(numb + " 已签到\n" + ui->Announcer->toPlainText());
}

void MainWindow::onAdminPanelClicked(){
    if (!apui) {
        this->setEnabled(0);
        apui = new AdminPanel();
        connect(apui, &AdminPanel::closed, this, &MainWindow::AdminPanelClosed);
        apui->show();
    }
}

void MainWindow::AdminPanelClosed(){
    apui = nullptr;
    this->setEnabled(1);
}

MainWindow::~MainWindow()
{
    delete ui;
}
