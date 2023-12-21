#include "dlgattr.h"
#include "ui_dlgattr.h"
#include <QPushButton>
#include <QRegularExpression>

CDlgAttr::CDlgAttr(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDlgAttr)
{
    ui->setupUi(this);
    ui->line_AttrEdit->setFocus();
    setMinimumSize(size());
    setMaximumSize(size());
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
    ui->line_AttrEdit->setText(QString("%1").arg(a,2,16,QChar('0')));
}

void CDlgAttr::on_line_AttrEdit_textChanged(const QString & text)
{
    bool ok;
    int v = text.toUInt(&ok, 16);
    static QRegularExpression re("^[\\da-fA-F]{1,2}$");
    auto result = re.match(text, 0, QRegularExpression::NormalMatch, QRegularExpression::NoMatchOption);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(
        !ok || v < 0 || v > 255 || !result.hasMatch());
}

