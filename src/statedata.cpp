#include "statedata.h"

#define DEF(x) {#x, x}

const std::vector<KeyOption> g_keyOptions = {
    DEF(TIMEOUT),
    DEF(POS_ORIGIN),
    DEF(POS_EXIT),
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
