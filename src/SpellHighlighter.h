#pragma once
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <hunspell.hxx>

class SpellHighlighter : public QSyntaxHighlighter
{
public:
    SpellHighlighter(QTextDocument *doc, Hunspell *checker)
        : QSyntaxHighlighter(doc), spell(checker) {}

protected:
    void highlightBlock(const QString &text) override
    {
        QTextCharFormat fmt;
        fmt.setUnderlineColor(Qt::red);
        fmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

        QStringList words = text.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
        for (const QString &word : words)
        {
            if (!spell->spell(word.toStdString()))
            {
                int index = text.indexOf(word);
                setFormat(index, word.length(), fmt);
            }
        }
    }

private:
    Hunspell *spell;
};
