#ifndef __FONT_H__
#define __FONT_H__
#include <stdint.h>
class CFont
{
public:
    CFont();
    ~CFont();

    bool read(const char *fname);
    void forget();

protected:
    uint8_t *m_font;
};

#endif