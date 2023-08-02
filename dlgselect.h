#ifndef DLGSELECT_H
#define DLGSELECT_H

#include <QDialog>

class CFrameSet;
class CMap;
class CMapFile;

namespace Ui {
class CDlgSelect;
}

class CDlgSelect : public QDialog
{
    Q_OBJECT

public:
    explicit CDlgSelect(QWidget *parent = nullptr);
    ~CDlgSelect();
    void init(const QString s, CMapFile *mf);
    int index();

private slots:
    void on_cbSelect_Maps_currentIndexChanged(int index);

private:
    enum {
        BLACK = 0xff000000,
        ALPHA = 0xff000000
    };
    void updatePreview(CMap *map);
    CFrameSet* preloadTiles();
    Ui::CDlgSelect *ui;
    CFrameSet *m_frameSet;
    CMapFile *m_mapFile;
};

#endif // DLGSELECT_H
