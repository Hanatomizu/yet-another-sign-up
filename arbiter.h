#ifndef ARBITER_H
#define ARBITER_H

#include <QWidget>
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QFile>
#include <QDir>
#include <QIODevice>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>
#include <vector>


namespace Ui {
class arbiter;
}

class arbiter : public QWidget
{
    Q_OBJECT

public:
    explicit arbiter(QWidget *parent = nullptr);
    ~arbiter();

    void checkStat();

signals:
    void closed();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::arbiter *ui;
};

#endif // ARBITER_H
