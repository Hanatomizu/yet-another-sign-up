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
    setWindowFlag(Qt::WindowStaysOnTopHint);
    connect(ui->AdminPanelEntrance, &QPushButton::released, this, &MainWindow::onAdminPanelClicked);
    connect(ui->action_adminpanel, &QAction::triggered, this, &MainWindow::onAdminPanelClicked);
    connect(ui->Submit, &QPushButton::released, this, &MainWindow::onSubmitClicked);
    connect(ui->NumberInput, &QLineEdit::returnPressed, this, &MainWindow::onSubmitClicked);
    connect(ui->CheckData, &QPushButton::released, this, &MainWindow::onArbiterClicked);
    connect(ui->action_chkdata, &QAction::triggered, this, &MainWindow::onArbiterClicked);
    connect(ui->pushButton_0, &QPushButton::released, this, &MainWindow::key0Pressed);
    connect(ui->pushButton_1, &QPushButton::released, this, &MainWindow::key1Pressed);
    connect(ui->pushButton_2, &QPushButton::released, this, &MainWindow::key2Pressed);
    connect(ui->pushButton_3, &QPushButton::released, this, &MainWindow::key3Pressed);
    connect(ui->pushButton_4, &QPushButton::released, this, &MainWindow::key4Pressed);
    connect(ui->pushButton_5, &QPushButton::released, this, &MainWindow::key5Pressed);
    connect(ui->pushButton_6, &QPushButton::released, this, &MainWindow::key6Pressed);
    connect(ui->pushButton_7, &QPushButton::released, this, &MainWindow::key7Pressed);
    connect(ui->pushButton_8, &QPushButton::released, this, &MainWindow::key8Pressed);
    connect(ui->pushButton_9, &QPushButton::released, this, &MainWindow::key9Pressed);
    connect(ui->pushButton_BackSpace, &QPushButton::released, this, &MainWindow::keyBackSpacePressed);
    connect(ui->pushButton_Clear, &QPushButton::released, this, &MainWindow::keyClearPressed);
    connect(ui->action_about, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->action_Qt, &QAction::triggered, this, &QApplication::aboutQt);
    connect(ui->action_quit, &QAction::triggered, this, &MainWindow::quitProgram);
    yasu->initConfigFiles();
    yasu->initNamelist();
}

void MainWindow::onSubmitClicked(){
    QString numb = ui->NumberInput->text();
    ui->NumberInput->setText("");
    QMessageBox response;
    response.setWindowFlag(Qt::WindowStaysOnTopHint);
    response.setWindowIcon(QIcon(":/resources/favicon.ico"));
    QPair<int, QString> status = yasu->sign_up(numb);
    if (status.first == 0){
        ui->Announcer->setText(status.second + " 已签到\n" + ui->Announcer->toPlainText());
        qDebug() << status.second << "Signed up successfully" << '\n';
        response.setWindowTitle(QString("签到成功"));
        response.setText("你好，" + status.second);
        response.setIcon(QMessageBox::Information);
    } else if (status.first == 2) {
        ui->Announcer->setText("签到失败，你已签到\n" + ui->Announcer->toPlainText());
        qDebug() << "Failed to sign up, Error code: " << status.first << '\n';
        response.setText("签到失败！");
        response.setWindowTitle(QString("签到失败！"));
        response.setInformativeText("错误：你已签到");
        response.setIcon(QMessageBox::Critical);
    } else{
        ui->Announcer->setText("签到失败，请检查学号输入是否正确\n" + ui->Announcer->toPlainText());
        qDebug() << "Failed to sign up, Error code: " << status.first << '\n';
        response.setWindowTitle(QString("签到失败！"));
        response.setText("签到失败！");
        response.setInformativeText("错误：number 是非法参数");
        response.setIcon(QMessageBox::Critical);
    }
    response.exec();
}

void MainWindow::onAdminPanelClicked(){
    AdminPanel *ap = new AdminPanel;
    ap->setAttribute(Qt::WA_DeleteOnClose);
    ap->show();
}

void MainWindow::onArbiterClicked(){
    arbiter *ab = new arbiter;
    ab->setAttribute(Qt::WA_DeleteOnClose);
    ab->show();
}

void MainWindow::key0Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "0");}
void MainWindow::key1Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "1");}
void MainWindow::key2Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "2");}
void MainWindow::key3Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "3");}
void MainWindow::key4Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "4");}
void MainWindow::key5Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "5");}
void MainWindow::key6Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "6");}
void MainWindow::key7Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "7");}
void MainWindow::key8Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "8");}
void MainWindow::key9Pressed(){ui->NumberInput->setText(ui->NumberInput->text() + "9");}

void MainWindow::keyBackSpacePressed() {
    ui->NumberInput->backspace();
}

void MainWindow::keyClearPressed() {
    ui->NumberInput->setText(QString(""));
}

void MainWindow::showAbout() {
    QMessageBox msgbox;
    msgbox.setText(QString("Yet Another Sign Up System\n")+
                   QString("Author: Hanatomizu\n") +
                   QString("Version: Alpha 0.0.3\n") +
                   QString("License: GPL v3\n") +
                   QString("Framework: Qt\n"));
    //msgbox.setFixedWidth(200);
    msgbox.exec();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (permitClose) {
        event->accept();
    }else {
        QMessageBox::warning(this,
                             QString("错误！"),
                             QString("你好！为了防止误触关闭按钮，请使用“操作”->“退出程序”来退出本程序\n如果你不知道你在做什么，请最小化本程序而不是关闭本程序！"),
                             QMessageBox::Ok,
                             QMessageBox::Ok);
        event->ignore();
    }
}

void MainWindow::quitProgram() {
    permitClose = 1;
    QApplication::quit();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete yasu;
}
