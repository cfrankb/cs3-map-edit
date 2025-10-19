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
#include "mapprops.h"
#include "runtime/map.h"
#include "runtime/statedata.h"
#include "runtime/states.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QFrame>

#if defined(USE_HUNSPELL)
#include <hunspell.hxx>
#include "SpellHighlighter.h"
#endif

MapPropertiesDialog::MapPropertiesDialog(CMap *map, QWidget *parent)
    : QDialog(parent), m_map(map), m_currentIndex(-1)
{
    setWindowTitle("Map Properties");
    setupUI();
    loadFromMap();
    populateMessages();

    // Set minimum dialog size
    setMinimumWidth(450);
    setMinimumHeight(400);
}

MapPropertiesDialog::~MapPropertiesDialog()
{
#if defined(USE_HUNSPELL)
    delete m_highlighter;
#endif
}

void MapPropertiesDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Create tab widget
    QTabWidget *tabWidget = new QTabWidget(this);

    // ===== General Tab =====
    QWidget *generalTab = new QWidget();
    QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);

    // Create form layout for properties
    QFormLayout *formLayout = new QFormLayout();

    // Title field (at the top)
    m_titleLineEdit = new QLineEdit(this);
    m_titleLineEdit->setMaxLength(255);
    m_titleLineEdit->setToolTip("Map title/name");
    formLayout->addRow("Title:", m_titleLineEdit);

    // Add a separator line
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    formLayout->addRow(line);

    // Timeout field
    m_timeoutSpinBox = new QSpinBox(this);
    m_timeoutSpinBox->setRange(0, 65535);
    m_timeoutSpinBox->setToolTip("Time limit for the map (0 = no limit)");
    formLayout->addRow("Timeout:", m_timeoutSpinBox);

    // Map Goal field
    m_mapGoalSpinBox = new QSpinBox(this);
    m_mapGoalSpinBox->setRange(0, 65535);
    m_mapGoalSpinBox->setToolTip("Goal score for the map");
    formLayout->addRow("Map Goal:", m_mapGoalSpinBox);

    // Par Time field
    m_parTimeSpinBox = new QSpinBox(this);
    m_parTimeSpinBox->setRange(0, 65535);
    m_parTimeSpinBox->setToolTip("Target completion time");
    formLayout->addRow("Par Time:", m_parTimeSpinBox);

    // Year field
    m_yearSpinBox = new QSpinBox(this);
    m_yearSpinBox->setRange(0, 9999);
    m_yearSpinBox->setToolTip("Year the map was created");
    formLayout->addRow("Year:", m_yearSpinBox);

    // Private checkbox
    m_privateCheckBox = new QCheckBox(this);
    m_privateCheckBox->setToolTip("Mark this map as private");
    formLayout->addRow("Private:", m_privateCheckBox);

    // Author field
    m_authorLineEdit = new QLineEdit(this);
    m_authorLineEdit->setMaxLength(1023);
    m_authorLineEdit->setToolTip("Map author name");
    formLayout->addRow("Author:", m_authorLineEdit);

    generalLayout->addLayout(formLayout);
    generalLayout->addStretch();

    // ===== Messages Tab =====
    QWidget *messagesTab = new QWidget();
    QVBoxLayout *messagesLayout = new QVBoxLayout(messagesTab);

    // Message selector combo box
    QHBoxLayout *selectorLayout = new QHBoxLayout();
    QLabel *msgLabel = new QLabel("Message:", this);
    m_cbMessage = new QComboBox(this);
    selectorLayout->addWidget(msgLabel);
    selectorLayout->addWidget(m_cbMessage);
    selectorLayout->addStretch();
    messagesLayout->addLayout(selectorLayout);

    // Message editor
    m_eMessage = new QPlainTextEdit(this);
    QFont font;
    font.setPointSize(12);
    m_eMessage->setFont(font);
    messagesLayout->addWidget(m_eMessage);

    // Status label
    m_sStatus = new QLabel(this);
    messagesLayout->addWidget(m_sStatus);

    // Add tabs to tab widget
    tabWidget->addTab(generalTab, "General");
    tabWidget->addTab(messagesTab, "Messages");

    mainLayout->addWidget(tabWidget);

    // Add some spacing
    mainLayout->addSpacing(10);

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &MapPropertiesDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MapPropertiesDialog::onReject);

    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    // Connect message-related signals
    connect(m_cbMessage, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MapPropertiesDialog::on_cbMessage_currentIndexChanged);
    connect(m_eMessage, &QPlainTextEdit::cursorPositionChanged,
            this, &MapPropertiesDialog::onCursorPositionChanged);

#if defined(USE_HUNSPELL)
    Hunspell *spellChecker = new Hunspell("/home/cfrankb/spell/en_US.aff", "/home/cfrankb/spell/en_US.dic");
    m_highlighter = new SpellHighlighter(m_eMessage->document(), spellChecker);
#endif
}

void MapPropertiesDialog::loadFromMap()
{
    if (!m_map)
        return;

    // Load title from CMap
    m_titleLineEdit->setText(QString::fromUtf8(m_map->title()));

    // Get the states object from CMap
    CStates &states = m_map->states();

    // Load uint16_t values
    m_timeoutSpinBox->setValue(states.getU(StateValue::TIMEOUT));
    m_mapGoalSpinBox->setValue(states.getU(StateValue::MAP_GOAL));
    m_parTimeSpinBox->setValue(states.getU(StateValue::PAR_TIME));
    m_yearSpinBox->setValue(states.getU(StateValue::YEAR));

    // Load private checkbox (any non-zero value means checked)
    m_privateCheckBox->setChecked(states.getU(StateValue::PRIVATE) != 0);

    // Load string value
    m_authorLineEdit->setText(QString::fromUtf8(states.getS(StateValue::AUTHOR)));
}

void MapPropertiesDialog::saveToMap()
{
    if (!m_map)
        return;

    // Save title to CMap
    m_map->setTitle(m_titleLineEdit->text().toUtf8().constData());

    // Get the states object from CMap
    CStates &states = m_map->states();

    // Save uint16_t values
    states.setU(StateValue::TIMEOUT, m_timeoutSpinBox->value());
    states.setU(StateValue::MAP_GOAL, m_mapGoalSpinBox->value());
    states.setU(StateValue::PAR_TIME, m_parTimeSpinBox->value());
    states.setU(StateValue::YEAR, m_yearSpinBox->value());

    // Save private checkbox (1 if checked, 0 if not)
    states.setU(StateValue::PRIVATE, m_privateCheckBox->isChecked() ? 1 : 0);

    // Save string value
    states.setS(StateValue::AUTHOR, m_authorLineEdit->text().toStdString());

    // Save messages
    saveMessages();
}

void MapPropertiesDialog::populateMessages()
{
    if (!m_map)
        return;

    // Load messages from states
    for (const auto &[k, v] : m_map->states().rawS())
    {
        if (k >= BASE_ID && k < BASE_ID + MAX_MESSAGES)
        {
            m_messages[k - BASE_ID] = QString::fromStdString(v);
        }
    }

    // Populate combo box
    for (size_t i = 0; i < MAX_MESSAGES; ++i)
    {
        m_cbMessage->addItem(QString("MSG%1 [0x%2]")
                                 .arg(i, 1, 16, QLatin1Char('0'))
                                 .arg(BASE_ID + i, 2, 16, QLatin1Char('0'))
                                 .toUpper());
    }

    m_cbMessage->setCurrentIndex(0);
}

void MapPropertiesDialog::saveMessages()
{
    if (!m_map)
        return;

    // Save current message being edited
    if (m_currentIndex != -1)
    {
        m_messages[m_currentIndex] = m_eMessage->toPlainText().trimmed();
    }

    // Save all messages to states
    for (size_t i = 0; i < MAX_MESSAGES; ++i)
    {
        const std::string msg = m_messages[i].toStdString();
        m_map->states().setS(BASE_ID + i, msg);
    }
}

void MapPropertiesDialog::on_cbMessage_currentIndexChanged(int index)
{
    if (index < 0 || index >= MAX_MESSAGES)
        return;

    // Save current message
    if (m_currentIndex != -1)
    {
        m_messages[m_currentIndex] = m_eMessage->toPlainText().trimmed();
    }

    // Display new message
    m_eMessage->setPlainText(m_messages[index]);

    // Update current index
    m_currentIndex = index;
}

void MapPropertiesDialog::onCursorPositionChanged()
{
    QTextCursor cursor = m_eMessage->textCursor();
    int position = cursor.position();
    int blockNumber = cursor.blockNumber();
    int columnNumber = cursor.columnNumber();
    m_sStatus->setText(QString("pos=%1 y=%2 x=%3")
                           .arg(position)
                           .arg(blockNumber)
                           .arg(columnNumber));
}

void MapPropertiesDialog::onAccept()
{
    saveToMap();
    accept();
}

void MapPropertiesDialog::onReject()
{
    reject();
}
