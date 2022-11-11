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

private slots:
    void on_eWidth_textChanged(const QString &);
    void on_eHeight_textChanged(const QString &);

private:
    void validateFields();

private:
    Ui::CDlgResize *ui;
};

#endif // DLGRESIZE_H
