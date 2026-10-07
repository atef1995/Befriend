#pragma once

#include "natives.h"
#include <string>

enum class SquadState {
    FOLLOW,
    ATTACK,
    HOLD_POSITION
};

inline const char* SquadStateName(SquadState state)
{
    switch (state)
    {
    case SquadState::FOLLOW:        return "Following";
    case SquadState::ATTACK:        return "Attacking";
    case SquadState::HOLD_POSITION: return "Holding";
    }
    return "?";
}

struct Bodyguard
{
    static constexpr Ped kInvalidHandle = -1;

    Ped handle = kInvalidHandle;   // GTA V entity handle; kInvalidHandle = not assigned
    std::string codeName = "";     // callsign such as "Alpha"
    SquadState currentState = SquadState::FOLLOW;
};

// Applies the task belonging to a new state. Deliberately a no-op when the state is
// unchanged, so callers can call it every frame without restarting the task.
inline void SetBodyguardState(Bodyguard& member, SquadState newState)
{
    if (member.currentState == newState) return;

    member.currentState = newState;
    TASK::CLEAR_PED_TASKS(member.handle);

    switch (newState)
    {
    case SquadState::FOLLOW:
        TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(member.handle, PLAYER::PLAYER_PED_ID(), 0.0f, -2.0f, 0.0f, 1.5f, -1, 10.0f, true);
        break;

    case SquadState::ATTACK:
        TASK::TASK_COMBAT_HATED_TARGETS_AROUND_PED(member.handle, 30.0f, 0);
        break;

    case SquadState::HOLD_POSITION:
        TASK::TASK_STAND_STILL(member.handle, -1);
        break;
    }
}