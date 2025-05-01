#ifndef ARBITER_H
#define ARBITER_H

#include <QWidget>

namespace Ui {
class arbiter;
}

class arbiter : public QWidget
{
    Q_OBJECT

public:
    explicit arbiter(QWidget *parent = nullptr);
    ~arbiter();

private:
    Ui::arbiter *ui;
};

#endif // ARBITER_H
