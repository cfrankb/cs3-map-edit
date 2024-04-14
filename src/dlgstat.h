#ifndef DLGSTAT_H
#define DLGSTAT_H

#include <QDialog>

namespace Ui {
class CDlgStat;
}

class CDlgStat : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgStat(int tileID, QWidget *parent = nullptr);
    ~CDlgStat();

private:
    Ui::CDlgStat *ui;
};

#endif // DLGSTAT_H
