#include "dlgresize.h"
#include "ui_dlgresize.h"
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

CDlgResize::CDlgResize(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDlgResize)
{
    ui->setupUi(this);
    setMinimumSize(size());
    setMaximumSize(size());

    ui->eWidth->setMaxLength(3);
    ui->eHeight->setMaxLength(3);

    ui->eWidth->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[\\d]{0,3}"),
                                        ui->eWidth));
    ui->eHeight->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[\\d]{0,3}"),
                                        ui->eHeight));
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

void CDlgResize::on_eWidth_textChanged(const QString &)
{
    validateFields();
}

void CDlgResize::on_eHeight_textChanged(const QString &)
{
    validateFields();
}

void CDlgResize::validateFields() {
    int w = ui->eWidth->text().toInt();
    int h = ui->eHeight->text().toInt();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(
                w < MIN_SIZE || w > MAX_SIZE || h < MIN_SIZE || h > MAX_SIZE);
}
