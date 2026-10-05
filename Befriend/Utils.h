#pragma once

#include "natives.h"
#include "types.h"

#include <string>

// True when a handle refers to a live entity. GTA uses 0 for "no entity",
// so that must be rejected before asking the engine.
inline bool EntityExists(Entity entity)
{
	return entity != 0 && ENTITY::DOES_ENTITY_EXIST(entity);
}

// Short-lived HUD toast. Keeps the exact text-component + duration behaviour
// used by every existing notification site.
inline void ShowNotification(const std::string& message, int durationMs = 2000)
{
	HUD::BEGIN_TEXT_COMMAND_PRINT("STRING");
	HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(const_cast<char*>(message.c_str()));
	HUD::END_TEXT_COMMAND_PRINT(durationMs, true);
}
