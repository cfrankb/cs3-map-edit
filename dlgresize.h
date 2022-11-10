#ifndef DLGRESIZE_H
#define DLGRESIZE_H

#include <QDialog>

namespace Ui {
class CDlgResize;
}

class CDlgResize : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgResize(QWidget *parent = nullptr);
    ~CDlgResize();
    int width();
    int height();
    void width(const int w);
    void height(const int h);

private:
    Ui::CDlgResize *ui;
};

#endif // DLGRESIZE_H
