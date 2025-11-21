#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QSize>
#include <QList>

// Structure to hold information about a selected tile
struct TileInfo
{
    int id;             // A unique ID (e.g., row * tiles_per_row + col)
    QPoint relativePos; // Position relative to the top-left selected tile (in tile units)
};

class TileSelectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TileSelectorWidget(QWidget *parent = nullptr);

    // Setters for image and configuration
    void setImage(const QPixmap &image);
    void setTileSize(int size);  // size is for one dimension (e.g., 16 for 16x16)
    void setZoomLevel(int zoom); // 1, 2, or 4 for 100%, 200%, 400%
    void setBackgroundColor(const QColor &color);

    // The main public method to get the result
    QList<TileInfo> getSelectedTiles() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override; // Add this to the header

signals:
    void tilesSelected(QList<TileInfo>);

private:
    QPixmap m_image;
    int m_tileSize = 16;
    int m_zoomLevel = 1;
    QColor m_backgroundColor = Qt::white; // Default color member

    // Selection points in PIXEL coordinates of the source image
    QPoint m_selectionStart;
    QPoint m_selectionEnd;
    bool m_isSelecting = false;

    // Helper function to map a screen pixel point to a source image pixel point (before tiling)
    QPoint screenToImage(const QPoint &screenPos) const;

    // Helper function to map an image pixel point to a tile coordinate (row, col)
    QPoint imagePixelToTileCoord(const QPoint &imagePos) const;

    // Helper function to calculate the tile ID
    int getTileId(int row, int col) const;

    // Helper function to clamp a pixel point (x, y) to the image boundaries
    QPoint clampToImageBounds(const QPoint &imagePos) const;

};
