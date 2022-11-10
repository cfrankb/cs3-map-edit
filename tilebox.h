#ifndef TILELBOX_H
#define TILELBOX_H

#include <QToolBox>

namespace Ui {
class CTileBox;
}

class CTileBox : public QToolBox
{
    Q_OBJECT

public:
    explicit CTileBox(QWidget *parent = nullptr);
    ~CTileBox();

private:
  //  Ui::CTileBox *ui;
    void setupToolbox();

signals:
    void tileChanged(int);

private slots:
    void buttonPressed(QAction *action);
};

#endif // TILELBOX_H
