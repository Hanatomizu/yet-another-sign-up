#ifndef ARBITER_H
#define ARBITER_H

#include <QWidget>

#include "signup.h"
#include "signlogparser.h"

namespace Ui {
class arbiter;
}

/**
 * @brief Statistics window ("查看数据").
 *
 * Shows the sign-in records of a chosen day split into the three periods
 * (早上 / 中午 / 晚上). Periods are determined by the sign-in TIME using
 * the configured period boundaries (morning_noon_split / noon_evening_split).
 */
class arbiter : public QWidget
{
    Q_OBJECT

public:
    explicit arbiter(QWidget *parent = nullptr);
    ~arbiter();

    void checkStat();

private:
    Ui::arbiter *ui;
};

#endif // ARBITER_H
