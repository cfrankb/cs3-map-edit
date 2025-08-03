#pragma once
#include <cstdint>
#include <vector>
#include <string>

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
