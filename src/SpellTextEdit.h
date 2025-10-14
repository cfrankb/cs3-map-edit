#pragma once
#include <QTextEdit>
#include <QMenu>
#include <QString>
#include <hunspell.hxx>
#include <vector>

class SpellTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    SpellTextEdit(Hunspell *checker, QWidget *parent = nullptr)
        : QTextEdit(parent), spell(checker)
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QTextEdit::customContextMenuRequested,
                this, &SpellTextEdit::showSpellMenu);
    }

private:
    Hunspell *spell;

    void showSpellMenu(const QPoint &pos)
    {
        QTextCursor cursor = cursorForPosition(pos);
        cursor.select(QTextCursor::WordUnderCursor);
        QString word = cursor.selectedText();

        if (word.isEmpty() || spell->spell(word.toStdString()))
        {
            // fallback to default menu
            QMenu *menu = createStandardContextMenu();
            menu->exec(mapToGlobal(pos));
            delete menu;
            return;
        }

        // Get suggestions
        auto results = spell->suggest(word.toStdString());
        QMenu menu(this);
        for (const std::string &s : results)
        {
            QAction *action = menu.addAction(QString::fromStdString(s));
            connect(action, &QAction::triggered, this, [&cursor, s]()
                    { cursor.insertText(QString::fromStdString(s)); });
        }

        if (results.empty())
        {
            menu.addAction("(No suggestions)")->setEnabled(false);
        }

        menu.addSeparator();
        menu.addActions(createStandardContextMenu()->actions());
        menu.exec(mapToGlobal(pos));
    }
};
