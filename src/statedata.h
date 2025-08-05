#pragma once
#include <cstdint>
#include <vector>
#include <string>

enum StateValue : uint16_t
{
    TIMEOUT = 0x01,
    POS_ORIGIN = 0x02,
    POS_EXIT = 0x03,
    USERDEF1 = 0x80,
    USERDEF2,
    USERDEF3,
    USERDEF4,
    MSG1 = 0xf0,
    MSG2,
    MSG3,
};

enum StateType
{
    TYPE_X,
    TYPE_U,
    TYPE_S,
};

struct KeyOption
{
    std::string display;
    uint16_t value;
};

const std::vector<KeyOption> &getKeyOptions();
