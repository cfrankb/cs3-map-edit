#pragma once
#include <QWidget>
#include <QToolButton>
#include <QLabel>

class LayerRowWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LayerRowWidget(const QString &text, bool visible, QWidget *parent = nullptr);

signals:
    void eyeClicked();
    void rowClicked();

protected:
    void mousePressEvent(QMouseEvent *e) override;

private:
    QToolButton *m_eye;
    QLabel *m_label;
};
