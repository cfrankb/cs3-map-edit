#include "dlgmsgs.h"
#include "ui_dlgmsgs.h"

#include "runtime/map.h"
#include "runtime/states.h"
#if defined(USE_HUNSPELL)
#include <hunspell.hxx>
#include "SpellHighlighter.h"
#include "SpellTextEdit.h"
#endif

CDlgMsgs::CDlgMsgs(QWidget *parent)
    : QDialog(parent), ui(new Ui::CDlgMsgs)
{
    m_currentIndex = -1;
    ui->setupUi(this);

#if defined(USE_HUNSPELL)
    Hunspell *spellChecker = new Hunspell("/home/cfrankb/spell/en_US.aff", "/home/cfrankb/spell/en_US.dic");
    SpellTextEdit *editor = new SpellTextEdit(spellChecker);
    editor->setPlainText("Ths is a tst.");
    m_highlighter = new SpellHighlighter(ui->eMessage->document(), spellChecker);
#endif
    connect(ui->eMessage, &QPlainTextEdit::cursorPositionChanged, this, &CDlgMsgs::onCursorPositionChanged);
}

CDlgMsgs::~CDlgMsgs()
{
    delete ui;
#if defined(USE_HUNSPELL)
    delete m_highlighter;
#endif
}

void CDlgMsgs::onCursorPositionChanged()
{
    QTextCursor cursor = ui->eMessage->textCursor(); // Get the current text cursor
    int position = cursor.position();                // Get the character position within the document
    int blockNumber = cursor.blockNumber();          // Get the block (line) number
    int columnNumber = cursor.columnNumber();        // Get the column number within the block
    ui->sStatus->setText(QString("pos=%1 y=%2 x=%3").arg(position).arg(blockNumber).arg(columnNumber));
}

void CDlgMsgs::populateData(CMap &map)
{
    for (const auto &[k, v] : map.states().rawS())
    {
        if (k >= BASE_ID)
        {
            m_messages[k - BASE_ID] = v.c_str();
        }
    }

    for (size_t i = 0; i < MAX_MESSAGES; ++i)
    {
        ui->cbMessage->addItem(QString("MSG%1 [%2]").arg(i, 1, 16, QLatin1Char('0')).arg(BASE_ID + i, 2, 16, QLatin1Char('0')).toUpper());
    }
    ui->cbMessage->setCurrentIndex(0);
}

void CDlgMsgs::saveData(CMap &map)
{
    // save current message
    int i = m_currentIndex;
    if (i != -1)
        m_messages[i] = ui->eMessage->toPlainText().trimmed();

    for (size_t i = 0; i < MAX_MESSAGES; ++i)
    {
        const std::string msg = m_messages[i].toStdString();
        map.states().setS(BASE_ID + i, msg.c_str());
    }
}

void CDlgMsgs::on_cbMessage_currentIndexChanged(int index)
{
    // save current message
    int i = m_currentIndex;
    if (i != -1)
        m_messages[i] = ui->eMessage->toPlainText().trimmed();

    // display message
    ui->eMessage->setPlainText(m_messages[index]);

    // update current index
    m_currentIndex = index;
}
