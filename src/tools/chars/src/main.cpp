#include <stdio.h>
#include <cstdlib>
#include <time.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <unordered_map>
#include <map>
#include <vector>
#include "../../../shared/FileWrap.h"
#include "../../../shared/FrameSet.h"
#include "../../../shared/Frame.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

bool generateBitFont()
{
    CFrameSet fs;
    CFileWrap file;
    const char inputFile[] = "inputs/chars.obl";
    if (file.open(inputFile))
    {
        fs.extract(file);
        file.close();
    }
    else
    {
        fprintf(stderr, "can't open file: %s\n", inputFile);
        return false;
    }

    const int fontSize = 8;
    const size_t fileSize = fs.getSize() * fontSize;
    uint8_t *bitData = new uint8_t[fileSize];
    uint8_t *cur = bitData;
    for (int i = 0; i < fs.getSize(); ++i)
    {
        CFrame &frame = *fs[i];
        for (int y = 0; y < fontSize; ++y)
        {
            uint8_t &c = *cur;
            c = 0;
            for (int x = 0; x < fontSize; ++x)
            {
                if (frame.at(x, y))
                {
                    c |= (1 << x);
                }
            }
            ++cur;
        }
    }

    if (file.open("out/chars.bin", "wb"))
    {
        file.write(bitData, fileSize);
        file.close();
    }
    delete[] bitData;
    return true;
}

int main()
{
    generateBitFont();

    return EXIT_SUCCESS;
}