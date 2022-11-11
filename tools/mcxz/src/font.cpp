#include "font.h"
#include <cstring>
#include <cstdio>

const int dataOffset = 7;

CFont::CFont()
{
    m_font = nullptr;
}

CFont::~CFont()
{
    forget();
}

bool CFont::read(const char *fname)
{
    FILE *sfile = fopen(fname, "rb");
    if (sfile)
    {
        forget();
        fseek(sfile, 0, SEEK_END);
        int size = ftell(sfile) - dataOffset;
        fseek(sfile, dataOffset, SEEK_SET);
        m_font = new uint8_t[size];
        fread(m_font, size, 1, sfile);
        fclose(sfile);
    }
    return sfile != nullptr;
}

void CFont::forget()
{
    if (m_font)
    {
        delete[] m_font;
    }
}