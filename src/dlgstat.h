#ifndef DLGSTAT_H
#define DLGSTAT_H

#include <QDialog>
#include <cstdint>

class CMap;

namespace Ui {
class CDlgStat;
}

class CDlgStat : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgStat(CMap *map, const int x, const int y,  QWidget *parent = nullptr);
    ~CDlgStat();

private:
    Ui::CDlgStat *ui;
    QString attr2text(const uint8_t attr);
};

#endif // DLGSTAT_H
