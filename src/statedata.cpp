#include "statedata.h"

#define DEF(x) {#x, x}
enum StateValue : uint16_t
{
    TIMEOUT = 0x01,
    ORIGIN = 0x02,
    EXIT = 0x03,
    USERDEF1 = 0x80,
    USERDEF2,
    USERDEF3,
    USERDEF4,
    MSG1 = 0xf0,
    MSG2,
    MSG3,
};

const std::vector<KeyOption> g_keyOptions = {
    DEF(TIMEOUT),
    DEF(ORIGIN),
    DEF(EXIT),
    DEF(MSG1),
    DEF(MSG2),
    DEF(MSG3),
    DEF(USERDEF1),
    DEF(USERDEF2),
    DEF(USERDEF3),
    DEF(USERDEF4),
};

const std::vector<KeyOption> &getKeyOptions()
{
    return g_keyOptions;
}
