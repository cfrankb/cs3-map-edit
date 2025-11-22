#include "TileSelectorWidget.h"
#include <QMouseEvent>
#include <QPen>
#include <QPainter>
#include <QDebug>

// --- Constructor ---
TileSelectorWidget::TileSelectorWidget(QWidget *parent)
    : QWidget(parent)
{
    // Set a solid background color (e.g., white)
    setAttribute(Qt::WA_OpaquePaintEvent);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    // Enable mouse tracking so mouseMoveEvent fires even without a button pressed
    setMouseTracking(true);

    setMaximumHeight(512);
    setMaximumSize(512,512);
}

// Helper function to clamp a pixel point (x, y) to the image boundaries
QPoint TileSelectorWidget::clampToImageBounds(const QPoint &imagePos) const
{
    if (m_image.isNull())
        return QPoint(0, 0);

    int imgW = m_image.width();
    int imgH = m_image.height();

    int clampedX = qBound(0, imagePos.x(), imgW - 1); // Clamp X between 0 and (width - 1)
    int clampedY = qBound(0, imagePos.y(), imgH - 1); // Clamp Y between 0 and (height - 1)

    return QPoint(clampedX, clampedY);
}

// --- Size Hint ---
QSize TileSelectorWidget::sizeHint() const
{
    // If an image is loaded, return the required size based on the image dimensions and zoom level.
    if (!m_image.isNull())
    {
        int w = m_image.width() * m_zoomLevel;
        int h = m_image.height() * m_zoomLevel;
        // Add a small margin just in case
        return QSize(w + 2, h + 2);
    }

    // Default size if no image is set (e.g., 200x200)
    return QSize(200, 200);
}

// --- Inside TileSelectorWidget.cpp ---
QSize TileSelectorWidget::minimumSizeHint() const
{
    return sizeHint(); // For this application, the minimum size is the required size.
}

// Helper function to map a screen pixel point to a source image pixel point
QPoint TileSelectorWidget::screenToImage(const QPoint &screenPos) const
{
    return QPoint(screenPos.x() / m_zoomLevel, screenPos.y() / m_zoomLevel);
}

// Helper function to map an image pixel point to a tile coordinate (row, col)
QPoint TileSelectorWidget::imagePixelToTileCoord(const QPoint &imagePos) const
{
    if (m_tileSize == 0)
        return QPoint(0, 0);
    int col = imagePos.x() / m_tileSize;
    int row = imagePos.y() / m_tileSize;
    return QPoint(col, row);
}

// Helper function to calculate the tile ID
int TileSelectorWidget::getTileId(int row, int col) const
{
    if (m_image.isNull() || m_tileSize == 0)
        return -1;
    int tilesPerRow = m_image.width() / m_tileSize;
    return row * tilesPerRow + col;
}

void TileSelectorWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_image.isNull())
    {
        m_isSelecting = true;
        // Map screen coordinates back to source image pixels
        m_selectionStart = screenToImage(event->pos());
        m_selectionEnd = m_selectionStart; // Start and end are the same initially
        update();                          // Force a repaint to show selection
    }
}

void TileSelectorWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isSelecting)
    {
        // Map screen coordinates back to source image pixels
        // m_selectionEnd = screenToImage(event->pos());

        // **CRUCIAL UPDATE:** Clamp the end point to the image bounds
        QPoint rawImagePos = screenToImage(event->pos());
        m_selectionEnd = clampToImageBounds(rawImagePos);

        update(); // Force a repaint to show updated selection rectangle
    }
}

void TileSelectorWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isSelecting)
    {
        m_isSelecting = false;
        // Finalize the selection area
        // m_selectionEnd = screenToImage(event->pos());
        // **CRUCIAL UPDATE:** Clamp the final selection end point
        QPoint rawImagePos = screenToImage(event->pos());
        m_selectionEnd = clampToImageBounds(rawImagePos);

        update();
        // Here you might emit a signal like tilesSelected(getSelectedTiles())
        emit tilesSelected(getSelectedTiles());
    }
}

// --- New Background Setter ---
void TileSelectorWidget::setBackgroundColor(const QColor &color)
{
    if (color != m_backgroundColor)
    {
        m_backgroundColor = color;

        // Change the widget's palette role to reflect the new color
        QPalette palette = this->palette();
        palette.setColor(QPalette::Base, m_backgroundColor);
        setPalette(palette);

        // The background role is typically used when autoFillBackground is true,
        // but explicitly updating the palette ensures the default QWidget painting
        // respects the color if our paintEvent is bypassed or simple.

        update(); // Force a repaint with the new color
    }
}

void TileSelectorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    // Explicitly Paint the Background
    painter.fillRect(rect(), m_backgroundColor);

    if (m_image.isNull() || m_tileSize <= 0)
    {
        // If no image, just paint the background
        //    QWidget::paintEvent(event);
        return;
    }

    // --- 1. Draw the Image with Zoom ---

    // Set smooth scaling for better image quality at high zoom
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.scale(m_zoomLevel, m_zoomLevel);
    painter.drawPixmap(0, 0, m_image);
    // Revert scale transformation for drawing the grid/selection overlay
    painter.scale(1.0 / m_zoomLevel, 1.0 / m_zoomLevel);

    // --- 2. Draw the Tile Grid ---

    // Define the style for the grid lines
    painter.setPen(QPen(QColor(150, 150, 150), 1)); // Light gray, 1 pixel wide
    painter.setBrush(Qt::NoBrush);

    const int imgW = m_image.width() * m_zoomLevel;
    const int imgH = m_image.height() * m_zoomLevel;
    const int scaledTileSize = m_tileSize * m_zoomLevel;

    // Draw vertical lines
    for (int x = scaledTileSize; x < imgW; x += scaledTileSize)
    {
        painter.drawLine(x, 0, x, imgH);
    }
    // Draw horizontal lines
    for (int y = scaledTileSize; y < imgH; y += scaledTileSize)
    {
        painter.drawLine(0, y, imgW, y);
    }

    // --- 3. Draw the Selection Overlay ---

    if (m_selectionStart != m_selectionEnd)
    {
        // Determine the max possible tile indices based on image size
        const int maxColIndex = (m_image.width() - 1) / m_tileSize;
        const int maxRowIndex = (m_image.height() - 1) / m_tileSize;

        // Find the tile coordinates of the start and end pixel points (which were clamped)
        QPoint startTile = imagePixelToTileCoord(m_selectionStart);
        QPoint endTile = imagePixelToTileCoord(m_selectionEnd);

        // Determine the actual bounding box in tile coordinates, clamping to image boundaries
        int minCol = qBound(0, qMin(startTile.x(), endTile.x()), maxColIndex);
        int maxCol = qBound(0, qMax(startTile.x(), endTile.x()), maxColIndex);
        int minRow = qBound(0, qMin(startTile.y(), endTile.y()), maxRowIndex);
        int maxRow = qBound(0, qMax(startTile.y(), endTile.y()), maxRowIndex);

        // --- Selection Rectangle Calculation (in zoomed pixels) ---
        const int scaledTileSize = m_tileSize * m_zoomLevel;

        // The top-left corner of the selection area
        int rectX = minCol * scaledTileSize;
        int rectY = minRow * scaledTileSize;

        // The width/height of the selection area
        int rectW = (maxCol - minCol + 1) * scaledTileSize;
        int rectH = (maxRow - minRow + 1) * scaledTileSize;

        // ... (rest of the drawing logic using selectionRect) ...
        QRect selectionRect(rectX, rectY, rectW, rectH);

        // Draw the filled area
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 150, 255, 100)); // Semi-transparent blue for highlighting
        painter.drawRect(selectionRect);

        // Draw the border
        painter.setPen(QPen(QColor(0, 150, 255), 2)); // Solid blue outline
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(selectionRect);
    }
}

QList<TileInfo> TileSelectorWidget::getSelectedTiles() const
{
    QList<TileInfo> selectedTiles;

    if (m_image.isNull() || m_tileSize == 0)
        return selectedTiles;

    // Determine the max possible tile indices based on image size
    int maxColIndex = (m_image.width() - 1) / m_tileSize;
    int maxRowIndex = (m_image.height() - 1) / m_tileSize;

    // Calculate the tile coordinates for the min/max bounds based on the clamped selection pixels
    QPoint startTile = imagePixelToTileCoord(m_selectionStart);
    QPoint endTile = imagePixelToTileCoord(m_selectionEnd);

    // Determine the selection bounds
    int minCol = qBound(0, qMin(startTile.x(), endTile.x()), maxColIndex);
    int maxCol = qBound(0, qMax(startTile.x(), endTile.x()), maxColIndex);
    int minRow = qBound(0, qMin(startTile.y(), endTile.y()), maxRowIndex);
    int maxRow = qBound(0, qMax(startTile.y(), endTile.y()), maxRowIndex);

    // The top-left most tile selected (which sets the relative 0,0)
    QPoint topLeftTile(minCol, minRow);

    for (int r = minRow; r <= maxRow; ++r)
    {
        for (int c = minCol; c <= maxCol; ++c)
        {
            TileInfo info;
            info.id = getTileId(r, c);
            // Relative position to the top-left tile
            info.relativePos = QPoint(c - topLeftTile.x(), r - topLeftTile.y());
            selectedTiles.append(info);
        }
    }

    return selectedTiles;
}

/**
 * @brief Sets the image to be displayed and tiled.
 * * After setting the image, it updates the minimum size hint and forces a repaint.
 * @param image The QPixmap containing the source image.
 */
void TileSelectorWidget::setImage(const QPixmap &image)
{
    m_image = image;
    // Clear selection when a new image is loaded
    m_selectionStart = QPoint();
    m_selectionEnd = QPoint();

    // Notify the layout manager that the preferred size has changed
    updateGeometry();
    update();
}

/**
 * @brief Sets the size of the square tiles (e.g., 16 for 16x16).
 * * Ensures the size is positive and forces a repaint.
 * @param size The new tile size in pixels.
 */
void TileSelectorWidget::setTileSize(int size)
{
    if (size > 0 && size != m_tileSize)
    {
        m_tileSize = size;
        update();
    }
}

/**
 * @brief Sets the zoom level (1, 2, or 4 for 100%, 200%, 400%).
 * * Forces the widget to update its size hint and repaint with the new scaling.
 * @param zoom The new zoom factor (1, 2, or 4).
 */
void TileSelectorWidget::setZoomLevel(int zoom)
{
    if (zoom == 1 || zoom == 2 || zoom == 4)
    {
        if (zoom != m_zoomLevel)
        {
            m_zoomLevel = zoom;
            // Notify the layout manager that the preferred size has changed
            updateGeometry();
            update();
        }
    }
}
