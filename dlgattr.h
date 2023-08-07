#ifndef DLGATTR_H
#define DLGATTR_H

#include <QDialog>
#include <stdint.h>

namespace Ui {
class CDlgAttr;
}

class CDlgAttr : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgAttr(QWidget *parent = nullptr);
    ~CDlgAttr();
    uint8_t attr();
    void attr(const uint8_t & a);

private slots:
    void on_line_AttrEdit_textChanged(const QString &text);

private:
    Ui::CDlgAttr *ui;
};

#endif // DLGATTR_H
