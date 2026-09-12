#include "layerdata.h"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include "logger.h"

#if defined(USE_QFILE)
#include "shared/qtgui/qfilewrap.h"
#else
#include "shared/FileWrap.h"
#endif

using jdoc = nlohmann::json;

layerdata_t g_layerdata[TOTAL_TILE_COUNT];

bool loadTileLayer(const std::string &filename, layerdata_t *layers)
{
#if defined(USE_QFILE)
    QFileWrap file;
#else
    CFileWrap file;
#endif
    if (!file.open(filename, "rb"))
    {
        LOGE("can't open %s", filename.c_str());
        return false;
    }

    std::string json;
    auto size = file.getSize();
    char *t = new char[size + 1];
    if (!t)
    {
        LOGE("failed to allocate mem");
        return false;
    }
    file.read(t, size);
    t[size] = '\0';
    json = t;
    delete[] t;

    for (int i = 0; i < TOTAL_TILE_COUNT; i++)
    {
        layers[i].granular = 0;
        layers[i].nextTile = (uint8_t)-1;
        layers[i].animeSpeed = 1;
        layers[i].tileType = LayerTileType::Background;
        layers[i].weight = 1;
        layers[i].tag = "";
    }

    jdoc j;
    try
    {
        j = jdoc::parse(json);
    }
    catch (...)
    {
        LOGE("failed parse json");
        return false;
    }

    if (!j.contains("tiles") || !j["tiles"].is_array())
        return false;

    int i = 0;
    for (const auto &tile : j["tiles"])
    {
        if (!tile.contains("index"))
            continue;

        int index = tile.at("index").get<int>();
        if (index < 0 || index >= TOTAL_TILE_COUNT)
            continue;

        layerdata_t &L = layers[index];
        L.nextTile = (uint8_t)tile.value("next", 0);
        if (L.nextTile == 0xff)
            L.nextTile = 0;
        L.animeSpeed = (uint8_t)tile.value("speed", 1);
        L.tileType = static_cast<LayerTileType>(tile.value("type", 0));
        L.weight = (uint8_t)tile.value("w", 1);
        L.granular = (uint8_t)tile.value("granular", 0);
        ++i;
    }
    LOGI("layer tiles: %d", i);
    return true;
}
