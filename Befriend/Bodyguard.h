#ifndef BODYGUARD_H
#define BODYGUARD_H

#include "natives.h"
#include <string>

struct Bodyguard
{
    Ped handle = -1;            // GTA V Entity ID handle
    std::string codeName = "";  // Standard C++ String object
    bool hasATask = false;
};

#endif // !BODYGUARD_H
