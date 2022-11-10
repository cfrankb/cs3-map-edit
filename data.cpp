#include "data.h"
#include <stdint.h>
#include <QDebug>
#include "shared/qtgui/qfilewrap.h"
#include "shared/FrameSet.h"
#include "shared/Frame.h"

uint32_t * loadTileData()
{
    qDebug("load tiles");
    QFileWrap file;
    uint32_t * data = nullptr;
    if (file.open(":/data/tilestiny.obl", "rb")) {
        qDebug("reading tiles");
        CFrameSet fs;
        if (fs.extract(file)) {
            qDebug("exracted: %d", fs.getSize());
        }
        file.close();

        data = new uint32_t[TEXTURE_WIDTH * TEXTURE_HEIGHT];
        uint32_t *p = data;
        int i = 0;
        for (int row=0; row< TEXTURE_HEIGHT/TEX_TILE_SIZE; ++row) {
            for (int col=0; col< TEXTURE_WIDTH/TEX_TILE_SIZE; ++col) {
                CFrame *frame = fs[i];
                frame->flipV();
                uint32_t *rgb =  frame->getRGB();
                for (int y=0; y < TEX_TILE_SIZE; ++y){
                    for (int x=0; x < TEX_TILE_SIZE; ++x){
                        p[col * TEX_TILE_SIZE + x + y *TEXTURE_WIDTH] = *rgb | 0xff000000;
                        ++rgb;
                    }
                }

                ++i;
                if (i >= fs.getSize()){
                    goto done;
                }
            }
            p += TEXTURE_WIDTH * TEX_TILE_SIZE;
        }
    }
done:
    return data;
}

uint32_t * loadFontData()
{
    uint32_t * data = nullptr;
    const char fontName [] = ":/data/font.bin";

    QFileWrap file;
    uint8_t *fontData = nullptr;
    int size = 0;
    if (file.open(fontName, "rb")) {
        size = file.getSize();
        fontData = new uint8_t[size];
        file.read(fontData, size);
        file.close();
    } else {
        qDebug("failed to open %s", fontName);
        return nullptr;
    }
    const int textureWidth = FONT_TEXTURE_WIDTH;//128;
    const int textureHeight = FONT_TEXTURE_HEIGHT;// 64;
    const int fontWidth = FONT_SIZE;//8;
    const int fontHeight = FONT_SIZE;//8;
    const int fontMemSize = fontWidth*fontHeight;
    const int fontCount = size / fontMemSize;
    qDebug("fontCount: %d", fontCount);
    CFrame frame(textureWidth, textureHeight);
    uint32_t *rgb = frame.getRGB();

    int i = 0;
    uint32_t *p = rgb;
    for (int row=0; row < textureHeight/fontHeight; ++row) {
        for (int col=0; col < textureWidth/fontWidth; ++col) {
            uint8_t *f = & fontData[i * fontMemSize];
            for (int y=0; y < fontHeight; ++y) {
                for (int x=0; x < fontWidth; ++x) {
                    uint8_t lb = 0;
                    if (x > 0) lb = f[x-1];
                    if (y > 0 && lb == 0) lb = f[x-8];
                    p[col * fontWidth + x + y * textureWidth] = f[x] ? 0xe0ffffff : (lb ? 0xff000000 : 0);
                }
                f += fontWidth;
            }
            ++i;
            if (i == fontCount) goto done;
        }
        p += textureWidth * fontHeight;
    }

done:
    frame.flipV();
    data = new uint32_t[textureWidth * textureHeight];
    memcpy(data, rgb, textureWidth * textureHeight * sizeof(rgb[0]));
    return data;
}
