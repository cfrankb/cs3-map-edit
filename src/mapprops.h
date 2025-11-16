/*
    cs3-runtime-sdl
    Copyright (C) 2025 Francois Blanchette

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QLabel>
#include <array>

class CMap;
class SpellHighlighter;

class MapPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MapPropertiesDialog(CMap *map, QWidget *parent = nullptr, int tab=0);
    ~MapPropertiesDialog();

    enum {
        TAB_GENERAL, TAB_MESSAGES
    };

private slots:
    void onAccept();
    void onReject();
    void on_cbMessage_currentIndexChanged(int index);
    void onCursorPositionChanged();

private:
    void setupUI();
    void loadFromMap();
    void saveToMap();
    void populateMessages();
    void saveMessages();

    enum
    {
        MAX_MESSAGES = 16,
        BASE_ID = 0xf0,
    };

    CMap *m_map;

    QTabWidget *m_tabWidget;

    // General tab UI Components
    QLineEdit *m_titleLineEdit;
    QSpinBox *m_timeoutSpinBox;
    QSpinBox *m_mapGoalSpinBox;
    QSpinBox *m_parTimeSpinBox;
    QSpinBox *m_yearSpinBox;
    QCheckBox *m_privateCheckBox;
    QLineEdit *m_authorLineEdit;

    // Messages tab UI Components
    QComboBox *m_cbMessage;
    QPlainTextEdit *m_eMessage;
    QLabel *m_sStatus;
    std::array<QString, MAX_MESSAGES> m_messages;
    int m_currentIndex;

#if defined(USE_HUNSPELL)
    SpellHighlighter *m_highlighter = nullptr;
#endif
};
