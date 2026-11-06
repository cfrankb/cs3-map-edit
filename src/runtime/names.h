#pragma once
#include <stdint.h>
#include <string>

struct NameValue
{
    const char *name;
    uint32_t value;
};

uint32_t resolveFlags(const std::string &s);
Color resolveColor(const std::string &s);
uint8_t resolveSound(const std::string &s);
uint8_t resolveTile(const std::string &s);
uint32_t resolvePath(const std::string &s);