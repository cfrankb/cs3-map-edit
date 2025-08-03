#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <vector>
#include <cstdint>
#include "states.h"
#include "statedata.h"

class KeyValueDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyValueDialog(QWidget *parent = nullptr);

    std::vector<StateValuePair> getKeyValuePairs() const;
    void populateData(const std::vector<StateValuePair> &pairs);
    static StateType getOptionType(uint16_t value);
    static uint16_t parseStringToUint16(const std::string &s, bool &isValid);

private slots:
    void addRow();
    void removeSelectedRows();
    void onCellChanged(int row, int column);
    void handleComboBoxIndexChanged(int row, int index);

private:
    void setupUi();
    QComboBox *createKeyComboBox() const;
    void validateRow(int row);
    void syncComboBoxes();

    QTableWidget *m_tableWidget;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    std::vector<QComboBox *> m_comboBoxes;
};
