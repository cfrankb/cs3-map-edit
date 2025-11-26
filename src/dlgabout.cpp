#include "dlgabout.h"
#include "ui_dlgabout.h"
#include "app_version.h"
#include "runtime/shared/qtgui/qfilewrap.h"
#include "logger.h"

DlgAbout::DlgAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgAbout)
{
    ui->setupUi(this);
    ui->sAppVersion->setText(tr("version %1").arg(APP_VERSION));

    std::vector<char> tmp;
    QFileWrap file;
    const char filename [] = ":/data/credits.txt";
    if (file.open(filename, "rb")) {
        auto size = file.getSize();
        tmp.resize(size+1);
        tmp[size] = '\0';
        if (!file.read(tmp.data(), size)) {
            LOGE("can't read %s", filename);
        }
        file.close();
    } else {
        LOGE("can't open %s", filename);
    }

    ui->eCredits->setReadOnly(true); // Ensures text can't be edited
    ui->eCredits->setPlainText(tmp.data());

    ui->tabWidget->setCurrentIndex(0);
}

DlgAbout::~DlgAbout()
{
    delete ui;
}
