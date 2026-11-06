#include <stdint.h>
#include "color.h"
#include "sounds.h"
#include "tilesdata.h"
#include "bossdata.h"
#include "names.h"
#include "logger.h"

#define _N(__VAL) {#__VAL, __VAL}

using namespace BossData;

// -----------------------------------------------------------------------
// NameValue tables. AUTO-GENERATED from the enum / #define declarations in
// the data headers (color.h, sounds.h, bossdata.h, tilesdata.h).
// Regenerate with:  python3 gen_names_tables.py
// Do not edit by hand -- edit the header enums/defines instead.
// -----------------------------------------------------------------------

const NameValue kColors[] = {
    _N(CLEAR),
    _N(ALPHA),
    _N(WHITE),
    _N(YELLOW),
    _N(PURPLE),
    _N(DARKPURPLE),
    _N(BLACK),
    _N(GREEN),
    _N(DARKGREEN),
    _N(LIME),
    _N(BLUE),
    _N(MIDBLUE),
    _N(CYAN),
    _N(RED),
    _N(DARKRED),
    _N(DARKBLUE),
    _N(DARKGRAY),
    _N(GRAY),
    _N(LIGHTGRAY),
    _N(LIGHTSLATEGRAY),
    _N(ORANGE),
    _N(DARKORANGE),
    _N(CORAL),
    _N(PINK),
    _N(HOTPINK),
    _N(DEEPPINK),
    _N(OLIVE),
    _N(MEDIUMSEAGREEN),
    _N(SEAGREEN),
    _N(BLUEVIOLET),
    _N(DEEPSKYBLUE),
    _N(LAVENDER),
    _N(DARKSLATEGREY),
    _N(BROWN),
    _N(RED_BROWN),
    _N(PALE_BROWN),
    _N(DARK_BROWN),
    _N(DARK_BROWN2),
};

const NameValue kSounds[] = {
    _N(SOUND_NONE),
    _N(SOUND_GRUUP),
    _N(SOUND_KEY),
    _N(SOUND_0009),
    _N(SOUND_COIN1),
    _N(SOUND_OUCHFAST),
    _N(SOUND_POUF),
    _N(SOUND_POW),
    _N(SOUND_POWERUP),
    _N(SOUND_POWERUP2),
    _N(SOUND_POWERUP3),
    _N(SOUND_BONUS),
    _N(SOUND_BEEP),
    _N(SOUND_GOLD3),
    _N(SOUND_SPRING05),
    _N(SOUND_EXPLOSION1),
    _N(SOUND_EXIT),
    _N(SOUND_DEATH),
    _N(SOUND_BELLS1),
    _N(SOUND_HIT2),
    _N(SOUND_SHOOT3),
    _N(SOUND_SPLASH01),
    _N(SOUND_DINOSAUR3),
    _N(SOUND_COUNT),
};

const NameValue kPaths[] = {
    _N(NONE),
    _N(ASTAR),
    _N(BFS),
    _N(LOS),
    _N(ASTAR_SMOOTH),
};

const NameValue kTiles[] = {
    _N(TILES_BLANK),
    _N(TILES_STOP),
    _N(TILES_ANNIE2),
    _N(TILES_WALLS93),
    _N(TILES_WALLS93_2),
    _N(TILES_WALLS93_3),
    _N(TILES_WALLS93_4),
    _N(TILES_WALLS93_5),
    _N(TILES_WALLS93_6),
    _N(TILES_WALLS93_7),
    _N(TILES_ROCKS0),
    _N(TILES_ROCKS1),
    _N(TILES_ROCKS2),
    _N(TILES_WALL_BRICK),
    _N(TILES_WALL_BRICK_2),
    _N(TILES_WALL_BRICK_3),
    _N(TILES_WALL_BRICK_4),
    _N(TILES_WALL_BRICK_5),
    _N(TILES_WALL_BRICK_6),
    _N(TILES_WALL_BRICK_7),
    _N(TILES_BRICKS),
    _N(TILES_BRICKS_2),
    _N(TILES_BRICKS_3),
    _N(TILES_BRICKS_4),
    _N(TILES_BRICKS_5),
    _N(TILES_BRICKS_6),
    _N(TILES_BRICKS_7),
    _N(TILES_BRICKS_8),
    _N(TILES_BRICKS_9),
    _N(TILES_BRICKS_A),
    _N(TILES_BRICKS_B),
    _N(TILES_BRICKS_C),
    _N(TILES_BRICKS_D),
    _N(TILES_BRICKS_E),
    _N(TILES_BRICKS_F),
    _N(TILES_BRICKS_10),
    _N(TILES_TREE),
    _N(TILES_PLANTS),
    _N(TILES_PLANTS_2),
    _N(TILES_PLANTS_3),
    _N(TILES_PLANTS_4),
    _N(TILES_PLANTS_5),
    _N(TILES_PLANTS_6),
    _N(TILES_PLANTS_7),
    _N(TILES_THISWAY),
    _N(TILES_THISWAY_2),
    _N(TILES_POMEGRENADE),
    _N(TILES_AMULET1),
    _N(TILES_APPLE),
    _N(TILES_REDBOOK),
    _N(TILES_JELLYJAR),
    _N(TILES_STRAWBERRY),
    _N(TILES_KIWI),
    _N(TILES_CHEST),
    _N(TILES_STATUE),
    _N(TILES_TOMB5),
    _N(TILES_MUSHROOM),
    _N(TILES_SMALL_MUSH0),
    _N(TILES_SMALL_MUSH1),
    _N(TILES_SMALL_MUSH2),
    _N(TILES_SMALL_MUSH3),
    _N(TILES_POT),
    _N(TILES_POT_2),
    _N(TILES_JAR1),
    _N(TILES_WATERMELON),
    _N(TILES_BOULDER),
    _N(TILES_BOULDER2),
    _N(TILES_STARS_TR),
    _N(TILES_POTIONS),
    _N(TILES_SCROLL4),
    _N(TILES_HEART),
    _N(TILES_SCROLL),
    _N(TILES_SHIELD),
    _N(TILES_POTION1),
    _N(TILES_POTION2),
    _N(TILES_POTION3),
    _N(TILES_PEAR),
    _N(TILES_CLOVER),
    _N(TILES_CHERRY),
    _N(TILES_FLOWERS),
    _N(TILES_FLOWERS_2),
    _N(TILES_DOOR03),
    _N(TILES_KEY03),
    _N(TILES_DOOR01),
    _N(TILES_KEY01),
    _N(TILES_DOOR02),
    _N(TILES_KEY02),
    _N(TILES_DOOR06),
    _N(TILES_KEY06),
    _N(TILES_DOOR04),
    _N(TILES_KEY04),
    _N(TILES_SWAMP),
    _N(TILES_DIAMOND),
    _N(TILES_TRIFORCE),
    _N(TILES_ORB),
    _N(TILES_BOMB),
    _N(TILES_SKELETON),
    _N(TILES_BABYDRAGON),
    _N(TILES_DEICO),
    _N(TILES_FORCEF94),
    _N(TILES_LEVER_SW0),
    _N(TILES_LEVER_SW1),
    _N(TILES_INSECT1),
    _N(TILES_LUTIN),
    _N(TILES_MANKA),
    _N(TILES_MUSH_IDLE),
    _N(TILES_OCTOPUS),
    _N(TILES_OOO),
    _N(TILES_TEDDY93),
    _N(TILES_VAMPLANT),
    _N(TILES_YAHOO),
    _N(TILES_YIGA),
    _N(TILES_YELKILLER),
    _N(TILES_WHTEWORM),
    _N(TILES_ETURTLE),
    _N(TILES_ZOMBIE),
    _N(TILES_DOORS_LEAF),
    _N(TILES_ICECUBE),
    _N(TILES_FIREBALL),
    _N(TILES_ICEBOULDER),
    _N(TILES_ICEMOUNTAIN),
    _N(TILES_SKULL),
    _N(TILES_COBWEB1),
    _N(TILES_SKULLBN1),
    _N(TILES_SHARD1),
    _N(TILES_SHARD2),
    _N(TILES_SHARD3),
    _N(TILES_SHARD4),
    _N(TILES_SHARD5),
    _N(TILES_HRIB1),
    _N(TILES_HBONE1),
    _N(TILES_HBONE2),
    _N(TILES_HXBONE2),
    _N(TILES_HXBONE1),
    _N(TILES_PEBBLE),
    _N(TILES_PEBBLE2),
    _N(TILES_ROCKP1),
    _N(TILES_ROCKPEBBLE1),
    _N(TILES_TREESTUMP),
    _N(TILES_TREESTUMP2),
    _N(TILES_TREESTUMP3),
    _N(TILES_TREESTUMP4),
    _N(TILES_TOMB1),
    _N(TILES_TOMB2),
    _N(TILES_TOMB3),
    _N(TILES_TOMB4),
    _N(TILES_HSKULLBONE1),
    _N(TILES_HSKULLBONE2),
    _N(TILES_HSKULL1),
    _N(TILES_CHUTE),
    _N(TILES_LIGHTNING_BOLT),
    _N(TILES_FIRE),
    _N(TILES_BULLETY1),
    _N(TILES_BOAT),
    _N(TILES_BARREL2EX),
    _N(TILES_BARREL2),
    _N(TILES_FLAME),
    _N(TILES_EGG_WHOLE),
    _N(TILES_EGG_ROTTEN),
    _N(TILES_EGG_CRACKED),
    _N(TILES_FIREBALL_SM),
    _N(TILES_BELL),
    _N(TILES_BARRELW),
    _N(TILES_BARRELW_DOWN),
    _N(TILES_BARRELW_WATER),
    _N(TILES_TOTAL_COUNT),
};

const NameValue kFlags[] = {
    _N(FLAG_FIREBALL),
    _N(BOSS_FLAG_PROXIMITY_ATTACK),
    _N(BOSS_FLAG_ICE_DAMAGE),
};

const NameValue *lookup(const NameValue *table, size_t n, const std::string &name)
{
    for (size_t i = 0; i < n; ++i)
        if (table[i].name == name)
            return &table[i];
    return nullptr;
}

// "Path::ASTAR" or bare "ASTAR"
uint32_t resolvePath(const std::string &s)
{
    std::string name = s;
    const std::string prefix = "Path::";
    if (name.size() >= prefix.size() && name.compare(0, prefix.size(), prefix) == 0)
        name = name.substr(prefix.size());
    const NameValue *nv = lookup(kPaths, sizeof(kPaths) / sizeof(kPaths[0]), name);
    if (!nv)
        LOGW("failed resolvePath: %s", s.c_str());
    return nv ? nv->value : 0;
}

uint8_t resolveTile(const std::string &s)
{
    const NameValue *nv = lookup(kTiles, sizeof(kTiles) / sizeof(kTiles[0]), s);
    if (!nv)
        LOGW("failed resolveTile: %s", s.c_str());
    return nv ? static_cast<uint8_t>(nv->value) : 0;
}

uint8_t resolveSound(const std::string &s)
{
    const NameValue *nv = lookup(kSounds, sizeof(kSounds) / sizeof(kSounds[0]), s);
    if (!nv)
        LOGW("failed resolveSound: %s", s.c_str());
    return nv ? static_cast<uint8_t>(nv->value) : 0;
}

Color resolveColor(const std::string &s)
{
    const NameValue *nv = lookup(kColors, sizeof(kColors) / sizeof(kColors[0]), s);
    if (!nv)
        LOGW("failed resolveColor: %s", s.c_str());
    return nv ? static_cast<Color>(nv->value) : CLEAR;
}

// OR-combines one or more named flags; "0" or empty yields 0.
uint32_t resolveFlags(const std::string &s)
{
    uint32_t acc = 0;
    std::string tokens = s;
    size_t i = 0;
    while (i < tokens.size())
    {
        while (i < tokens.size() && std::isspace((unsigned char)tokens[i]))
            ++i;
        if (i >= tokens.size())
            break;
        size_t start = i;
        while (i < tokens.size() && !std::isspace((unsigned char)tokens[i]))
            ++i;
        std::string name = tokens.substr(start, i - start);
        if (name.empty())
            continue;
        const NameValue *nv = lookup(kFlags, sizeof(kFlags) / sizeof(kFlags[0]), name);
        if (nv)
            acc |= nv->value;
        else if (name == "0")
            ; // explicit none
        else
            acc |= static_cast<uint32_t>(std::strtoul(name.c_str(), nullptr, 0));
    }
    return acc;
}
