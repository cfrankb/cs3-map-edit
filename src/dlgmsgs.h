#ifndef DLGMSGS_H
#define DLGMSGS_H

#include <QDialog>
#include <array>

class SpellHighlighter;

namespace Ui
{
    class CDlgMsgs;
}

class CMap;

class CDlgMsgs : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgMsgs(QWidget *parent = nullptr);
    ~CDlgMsgs();
    void populateData(CMap &map);
    void saveData(CMap &map);

private slots:
    void on_cbMessage_currentIndexChanged(int index);
    void onCursorPositionChanged();

private:
    enum
    {
        MAX_MESSAGES = 16,
        BASE_ID = 0xf0,
    };
    Ui::CDlgMsgs *ui;
    std::array<QString, MAX_MESSAGES> m_messages;
    int m_currentIndex = 0;
#if defined(USE_HUNSPELL)
    SpellHighlighter *m_highlighter;
#endif
};

#endif // DLGMSGS_H
