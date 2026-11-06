#ifndef DLGSPELL_H
#define DLGSPELL_H

#include <QDialog>
#include <QLineEdit>
#include <QString>

class DlgSpell : public QDialog
{
    Q_OBJECT

public:
    explicit DlgSpell(QWidget *parent = nullptr);

    // Read the saved dictionary paths from the app configuration.
    static QString affFile();
    static QString dicFile();

    // True only when both paths are set and the files actually exist.
    static bool isConfigured();

private slots:
    void onAffBrowse();
    void onDicBrowse();
    void onAccept();

private:
    void setupUI();
    void loadSavedPaths();

    QLineEdit *m_affEdit;
    QLineEdit *m_dicEdit;

    static const char *const KEY_AFF;
    static const char *const KEY_DIC;
};

#endif // DLGSPELL_H
