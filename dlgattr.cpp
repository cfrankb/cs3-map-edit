#include "dlgattr.h"
#include "ui_dlgattr.h"

CDlgAttr::CDlgAttr(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDlgAttr)
{
    ui->setupUi(this);
    ui->lineEdit->setFocus();
}

CDlgAttr::~CDlgAttr()
{
    delete ui;
}

uint8_t CDlgAttr::attr()
{
    return ui->lineEdit->text().toUInt(nullptr, 10);
}

void CDlgAttr::attr(const uint8_t & a)
{
    ui->lineEdit->setText(QString("%1").arg(a));
}


