#include "dlgabout.h"
#include "ui_dlgabout.h"
#include "app_version.h"

DlgAbout::DlgAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgAbout)
{
    ui->setupUi(this);
    ui->sAppVersion->setText(tr("version %1").arg(APP_VERSION));
}

DlgAbout::~DlgAbout()
{
    delete ui;
}
