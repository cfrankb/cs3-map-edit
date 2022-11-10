#include "dlgresize.h"
#include "ui_dlgresize.h"

CDlgResize::CDlgResize(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDlgResize)
{
    ui->setupUi(this);
}

CDlgResize::~CDlgResize()
{
    delete ui;
}

int CDlgResize::width()
{
    return ui->eWidth->text().toInt(nullptr, 10);
}
int CDlgResize::height()
{
    return ui->eHeight->text().toInt(nullptr, 10);
}
void CDlgResize::width(const int w)
{
    ui->eWidth->setText(QString("%1").arg(w));
}
void CDlgResize::height(const int h)
{
    ui->eHeight->setText(QString("%1").arg(h));
}

