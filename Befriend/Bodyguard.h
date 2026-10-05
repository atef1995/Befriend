#pragma once

#include "natives.h"
#include <string>

struct Bodyguard
{
    static constexpr Ped kInvalidHandle = -1;

    Ped handle = kInvalidHandle;   // GTA V entity handle; kInvalidHandle = not assigned
    std::string codeName = "";     // callsign such as "Alpha"
    bool hasATask = false;
};
