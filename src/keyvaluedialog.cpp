#include "keyvaluedialog.h"
#include <QHeaderView>
#include "statedata.h"

KeyValueDialog::KeyValueDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
}

void KeyValueDialog::setupUi()
{
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(2);
    m_tableWidget->setHorizontalHeaderLabels({"Key", "Value"});
    m_tableWidget->horizontalHeader()->setStretchLastSection(true);

    m_addButton = new QPushButton("Add Row", this);
    m_removeButton = new QPushButton("Remove Selected", this);
    m_okButton = new QPushButton("OK", this);
    m_cancelButton = new QPushButton("Cancel", this);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_tableWidget);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    setWindowTitle("Key-Value Editor");

    connect(m_addButton, &QPushButton::clicked, this, &KeyValueDialog::addRow);
    connect(m_removeButton, &QPushButton::clicked, this, &KeyValueDialog::removeSelectedRows);
    connect(m_tableWidget, &QTableWidget::cellChanged, this, &KeyValueDialog::onCellChanged);

    connect(m_okButton, &QPushButton::clicked, this, &KeyValueDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &KeyValueDialog::reject);
}

QComboBox *KeyValueDialog::createKeyComboBox() const
{
    auto keyOptions = getKeyOptions();
    QComboBox *comboBox = new QComboBox;
    for (const auto &option : keyOptions)
    {
        comboBox->addItem(option.display.c_str(), option.value);
    }
    return comboBox;
}

void KeyValueDialog::syncComboBoxes()
{
    int i = 0;
    for (const auto &comboBox : m_comboBoxes)
    {
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [&](int index)
                { handleComboBoxIndexChanged(i, index); });
        ++i;
    }
}

void KeyValueDialog::populateData(const std::vector<StateValuePair> &data)
{
    m_tableWidget->setRowCount(data.size());
    for (size_t i = 0; i < data.size(); ++i)
    {
        QComboBox *comboBox = createKeyComboBox();
        int index = comboBox->findData(data[i].key);
        if (index == -1)
        {
            // add hex value for keys not in list
            char tmp[16];
            sprintf(tmp, "%.2x", data[i].key);
            comboBox->addItem(QString("%1").arg(tmp), data[i].key);
            index = comboBox->findData(data[i].key);
        }
        comboBox->setCurrentIndex(index);
        m_comboBoxes.push_back(comboBox);
        m_tableWidget->setCellWidget(i, 0, comboBox);

        QTableWidgetItem *valueItem = new QTableWidgetItem(data[i].value.c_str());
        valueItem->setToolTip(data[i].tip.c_str());
        m_tableWidget->setItem(i, 1, valueItem);
    }
    syncComboBoxes();
}

void KeyValueDialog::addRow()
{
    int row = m_tableWidget->rowCount();
    auto comboBox = createKeyComboBox();
    m_tableWidget->insertRow(row);
    m_tableWidget->setCellWidget(row, 0, comboBox);
    m_tableWidget->setItem(row, 1, new QTableWidgetItem(""));
    m_comboBoxes.push_back(comboBox);
    syncComboBoxes();
}

void KeyValueDialog::removeSelectedRows()
{
    QList<QTableWidgetSelectionRange> ranges = m_tableWidget->selectedRanges();
    for (auto it = ranges.end(); it != ranges.begin();)
    {
        --it;
        for (int row = it->bottomRow(); row >= it->topRow(); --row)
        {
            m_tableWidget->removeRow(row);
            m_comboBoxes.erase(m_comboBoxes.begin() + row);
        }
    }
    syncComboBoxes();
}

std::vector<StateValuePair> KeyValueDialog::getKeyValuePairs() const
{
    std::vector<StateValuePair> pairs;
    for (int row = 0; row < m_tableWidget->rowCount(); ++row)
    {
        QComboBox *comboBox = qobject_cast<QComboBox *>(m_tableWidget->cellWidget(row, 0));
        QTableWidgetItem *valueItem = m_tableWidget->item(row, 1);
        if (comboBox && valueItem)
        {
            pairs.push_back({static_cast<uint16_t>(comboBox->currentData().toUInt()),
                             valueItem->text().toStdString(), ""});
        }
    }
    return pairs;
}

void KeyValueDialog::validateRow(int row)
{
    QComboBox *comboBox = qobject_cast<QComboBox *>(m_tableWidget->cellWidget(row, 0));
    if (comboBox)
    {
        auto key = comboBox->currentData().toUInt();
        const StateType type = getOptionType(key);
        auto widget = m_tableWidget->item(row, 1);
        if (type == TYPE_U)
        {
            bool ok;
            uint16_t value = parseStringToUint16(widget->text().toStdString().c_str(), ok);

            // show tooltip
            char tmp[32];
            sprintf(tmp, "0x%.2x [%d]", value, value);
            widget->setToolTip(tmp);

            Q_UNUSED(value);
            if (!ok)
            {
                QColor lightRed(255, 200, 200);
                QBrush lightRedBrush(lightRed);
                widget->setBackground(lightRedBrush);
            }
            else
            {
                widget->setBackground(QBrush());
            }
        }
        else if (type == TYPE_S)
        {
            widget->setToolTip("");
            widget->setBackground(QBrush());
            auto value = widget->text().toStdString();
        }
    }
}

void KeyValueDialog::handleComboBoxIndexChanged(int row, int index)
{
    Q_UNUSED(index);

    QComboBox *comboBox = qobject_cast<QComboBox *>(m_tableWidget->cellWidget(row, 0));
    if (!comboBox)
        return;
    validateRow(row);
}

void KeyValueDialog::onCellChanged(int row, int column)
{
    Q_UNUSED(column);
    validateRow(row);
}

StateType KeyValueDialog::getOptionType(uint16_t value)
{
    if ((value & 0xff) < 0x80)
    {
        return TYPE_U;
    }
    else
    {
        return TYPE_S;
    }
}


uint16_t KeyValueDialog::parseStringToUint16(const std::string &s, bool &isValid)
{
    uint16_t v = 0;
    size_t size = 0;
    isValid = false;
    if (s.substr(0, 2) == "0x" ||
        s.substr(0, 2) == "0X")
    {
        if (s.size() > 2) {
            const std::string t= s.substr(2);
            v = std::stoul(t, &size, 16);
            isValid = size == t.size();
        }
    }
    else if (isdigit(s[0]) || s[0] == '-')
    {
        v = std::stoul(s, &size, 10);
        isValid = true;
        isValid = size == s.size();
    }
    return v;
}
