#include "dlgattr.h"
#include "ui_dlgattr.h"
#include <QPushButton>

CDlgAttr::CDlgAttr(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDlgAttr)
{
    ui->setupUi(this);
    ui->line_AttrEdit->setFocus();
}

CDlgAttr::~CDlgAttr()
{
    delete ui;
}

uint8_t CDlgAttr::attr()
{
    return ui->line_AttrEdit->text().toUInt(nullptr, 16);
}

void CDlgAttr::attr(const uint8_t & a)
{
    ui->line_AttrEdit->setText(QString("%1").arg(a,2,16));
}

void CDlgAttr::validateFields() {
    QString s = ui->line_AttrEdit->text().toLower();
    int v = s.toUInt(nullptr, 16);
    QRegExp re("[\\da-f]*");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(
                v < 0 || v > 255 || !re.exactMatch(s));
}

void CDlgAttr::on_line_AttrEdit_textChanged(const QString &)
{
    validateFields();
}

