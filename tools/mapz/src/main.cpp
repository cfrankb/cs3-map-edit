#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <map>
#include <vector>
#include <inttypes.h>
#include "../../../map.h"
#include "../../../level.h"
#include "levelarch.h"

typedef std::vector<std::string> StringVector;

long getFileSize(FILE *f) 
{
    long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, cur, SEEK_SET);
    return size;
}


int test()
{
    const char *archfile = "out/levels.mapz";
    CLevelArch arch;
    if (!arch.open(archfile)) {
        printf("can't open arch\n");
        return EXIT_FAILURE;
    }
    arch.debug();
    arch.forget();

    arch.debug();

    FILE *sfile = fopen(archfile, "rb");
    if (sfile) {
        long size = getFileSize(sfile);
        uint8_t *mem = new uint8_t[size];
        fread(mem, size, 1, sfile);
        fclose(sfile);
        
        arch.fromMemory(mem);
        arch.debug();

        long offset = arch.at(3);
        CMap map;
        map.fromMemory(mem + offset);
        map.debug();
        map.write("out/level04.dat");
        map.forget();
        map.debug();

        map.read("out/level04.dat");
        map.debug();
        map.forget();

        map.read("data/level04.dat");
        map.debug();
        delete [] mem;

    } else {
        printf("can't read arch\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[], char *envp[])
{
    StringVector files={
        "level01.map",
        "level02.cs3",
        "level03.cs3",
        "level04.dat",
        "level05.dat",
        "level06.cs3",
        "level07.cs3",
        "level08.cs3",
        "level09.cs3",
        "level10.cs3"
    };

    for (int i = 1; i < argc; ++i)
    {
        printf("%s\n", argv[i]);
    }

    FILE *tfile = fopen("out/levels.mapz", "wb");

    std::vector<long> index;
    fwrite("MAAZ", 4,1, tfile);
    fwrite("\0\0\0\0",4,1,tfile);
    fwrite("\0\0\0\0",4,1,tfile);

    CMap map;
    for (int i=0; i < files.size(); ++i) {
        printf("%s\n", files[i].c_str());

        char tmp[128];
        sprintf(tmp, "data/%s", files[i].c_str());

        std::string error;
        if (!fetchLevel(map, tmp, error)) {
            printf("failed to load\n");
        }

        index.push_back(ftell(tfile));
        map.write(tfile);
    }

    // write index
    long indexPtr = ftell(tfile);
    size_t size = index.size();
    for (int i=0; i < index.size(); ++i) {
        long ptr = index[i];
        fwrite(&ptr, 4, 1, tfile);
    }
    fseek(tfile, 6, SEEK_SET);
    fwrite(&size, 2, 1, tfile);    
    fwrite(&indexPtr, 4, 1, tfile);
    
    fclose(tfile);

  //  test();

    return EXIT_SUCCESS;
}