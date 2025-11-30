#include "dlgattr.h"
#include "ui_dlgattr.h"
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

CDlgAttr::CDlgAttr(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDlgAttr)
{
    ui->setupUi(this);
    ui->line_AttrEdit->setFocus();
    setMinimumSize(size());
    setMaximumSize(size());
    // Limit to 2 characters
    ui->line_AttrEdit->setMaxLength(2);

    // set a fixed width so the box looks sized for 2 chars
    ui->line_AttrEdit->setFixedWidth(40);

    // align text left
    ui->line_AttrEdit->setAlignment(Qt::AlignLeft);

    // add line validator
    ui->line_AttrEdit->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[\\da-fA-F]{0,2}"),
                                        ui->line_AttrEdit));
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

