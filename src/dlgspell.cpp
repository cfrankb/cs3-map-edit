#include "dlgspell.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QFileInfo>
#include <QDialogButtonBox>
#include <QMessageBox>

// Keys used to store the paths in the app configuration.
const char *const DlgSpell::KEY_AFF = "dictionary/affFile";
const char *const DlgSpell::KEY_DIC = "dictionary/dicFile";

DlgSpell::DlgSpell(QWidget *parent)
    : QDialog(parent),
      m_affEdit(nullptr),
      m_dicEdit(nullptr)
{
    setWindowTitle("Dictionary Settings");
    resize(520, 200);
    setupUI();
    loadSavedPaths();
}

// Read the stored affix file path from the app configuration.
QString DlgSpell::affFile()
{
    QSettings settings;
    return settings.value(KEY_AFF, QString()).toString();
}

// Read the stored dictionary file path from the app configuration.
QString DlgSpell::dicFile()
{
    QSettings settings;
    return settings.value(KEY_DIC, QString()).toString();
}

// True only when both paths are defined and the referenced files exist.
bool DlgSpell::isConfigured()
{
    const QString aff = affFile();
    const QString dic = dicFile();
    return !aff.isEmpty() && !dic.isEmpty()
           && QFileInfo::exists(aff) && QFileInfo::exists(dic);
}

void DlgSpell::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    QLabel *info = new QLabel(
        tr("Provide the paths to the Hunspell affix and dictionary files. "
           "Spell checking is only enabled when both files are set and exist."),
        this);
    info->setWordWrap(true);
    mainLayout->addWidget(info);

    QFormLayout *form = new QFormLayout();
    QHBoxLayout *btnLayoutAff = new QHBoxLayout();
    QPushButton *btnAff = new QPushButton(tr("Browse…"), this);
    m_affEdit = new QLineEdit(this);
    m_affEdit->setPlaceholderText(tr("/path/to/en_US.aff"));
    btnLayoutAff->addWidget(m_affEdit);
    btnLayoutAff->addWidget(btnAff);
    form->addRow(tr("Affix file (.aff):"), btnLayoutAff);

    QHBoxLayout *btnLayoutDic = new QHBoxLayout();
    QPushButton *btnDic = new QPushButton(tr("Browse…"), this);
    m_dicEdit = new QLineEdit(this);
    m_dicEdit->setPlaceholderText(tr("/path/to/en_US.dic"));
    btnLayoutDic->addWidget(m_dicEdit);
    btnLayoutDic->addWidget(btnDic);
    form->addRow(tr("Dictionary file (.dic):"), btnLayoutDic);

    mainLayout->addLayout(form);

    connect(btnAff, &QPushButton::clicked, this, &DlgSpell::onAffBrowse);
    connect(btnDic, &QPushButton::clicked, this, &DlgSpell::onDicBrowse);

    // Ok / Cancel buttons.
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &DlgSpell::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addStretch();
    setLayout(mainLayout);
}

void DlgSpell::loadSavedPaths()
{
    m_affEdit->setText(affFile());
    m_dicEdit->setText(dicFile());
}

void DlgSpell::onAffBrowse()
{
    const QString chosen = QFileDialog::getOpenFileName(
        this,
        tr("Select Hunspell affix file"),
        affFile(),
        tr("Hunspell dictionaries (*.aff *.dic *.txt);* All Files (*.*);* All Files (*.*)"));
    if (!chosen.isEmpty())
        m_affEdit->setText(chosen);
}

void DlgSpell::onDicBrowse()
{
    const QString chosen = QFileDialog::getOpenFileName(
        this,
        tr("Select Hunspell dictionary file"),
        dicFile(),
        tr("Hunspell dictionaries (*.dic *.aff *.txt);* All Files (*.*);* All Files (*.*)"));
    if (!chosen.isEmpty())
        m_dicEdit->setText(chosen);
}

void DlgSpell::onAccept()
{
    QSettings settings;
    settings.setValue(KEY_AFF, m_affEdit->text().trimmed());
    settings.setValue(KEY_DIC, m_dicEdit->text().trimmed());

    if (!isConfigured())
    {
        QMessageBox::warning(
            this,
            tr("Dictionary not configured"),
            tr("Both the affix and dictionary files must be set and must exist "
               "in order to enable spell checking."));
        return;
    }

    accept();
}
