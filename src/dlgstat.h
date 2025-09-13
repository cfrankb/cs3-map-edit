#ifndef DLGSTAT_H
#define DLGSTAT_H

#include <QDialog>
#include <cstdint>

namespace Ui {
class CDlgStat;
}

class CDlgStat : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgStat(const uint8_t tileID, const uint8_t attr, QWidget *parent = nullptr);
    ~CDlgStat();

private:
    Ui::CDlgStat *ui;
    QString attr2text(const uint8_t attr);
};

#endif // DLGSTAT_H
