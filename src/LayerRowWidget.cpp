#include "LayerRowWidget.h"
#include <QHBoxLayout>
#include <QMouseEvent>


LayerRowWidget::LayerRowWidget(const QString &text, bool visible, QWidget *parent)
    : QWidget(parent)
{
    QIcon eyeOpen = QIcon(":/data/icons/eye_818577.png");    // freepik
    QIcon eyeClosed = QIcon(":data/icons/blind_795831.png"); // freepik
    m_eye = new QToolButton(this);
    m_eye->setIcon(visible ? eyeOpen : eyeClosed);
    m_eye->setAutoRaise(true);
    m_eye->setFocusPolicy(Qt::NoFocus);
    m_label = new QLabel(text, this);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(4, 0, 0, 0);
    layout->addWidget(m_eye);
    layout->addWidget(m_label);
    layout->addStretch();
    setLayout(layout);

    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground);
    connect(m_eye, &QToolButton::clicked, this, &LayerRowWidget::eyeClicked);
}

void LayerRowWidget::mousePressEvent(QMouseEvent *e)
{
    // Clicking anywhere EXCEPT the eye icon selects the row
    if (!m_eye->geometry().contains(e->pos()))
        emit rowClicked();

    QWidget::mousePressEvent(e);
}
