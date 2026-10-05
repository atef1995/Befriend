#include "pch.h"
#include "script.h"
#include "keyboard.h"
#include "Bodyguard.h"
#include "Utils.h"
#include "menu.h"

#include <string>
#include <ctime>
#include <vector>
#include <iterator>
#include <cassert>

#pragma warning(disable : 4244 4305)

void DrawText3D(Vector3 worldPos, const char* text)
{
	float screenX = 0.0f;
	float screenY = 0.0f;

	BOOL onScreen = GRAPHICS::_WORLD3D_TO_SCREEN2D(
		worldPos.x, worldPos.y, worldPos.z,
		&screenX, &screenY
	);

	if (onScreen)
	{
		HUD::SET_TEXT_FONT(0);
		HUD::SET_TEXT_SCALE(0.0f, 0.35f);
		HUD::SET_TEXT_COLOUR(255, 255, 255, 255);
		HUD::SET_TEXT_CENTRE(1);
		HUD::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		HUD::SET_TEXT_EDGE(1, 0, 0, 0, 205);

		HUD::_SET_TEXT_ENTRY((char*)"STRING");
		HUD::_ADD_TEXT_COMPONENT_STRING((char*)text);
		HUD::_DRAW_TEXT(screenX, screenY, 1);
	}
}

int GetFreePassengerSeat(Vehicle veh) {
	int maxSeats = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(veh);

	for (int seat = 0; seat < maxSeats; seat++) {
		if (VEHICLE::IS_VEHICLE_SEAT_FREE(veh, seat, false)) {
			return seat;
		}
	}
	return -2;
}

enum Task
{
	ENTER_VEHICLE
};

const std::string CALLSIGNS[] = { "Alpha", "Bravo", "Charlie", "Delta", "Echo", "Ghost", "Specter" };
const int CALLSIGNS_COUNT = 7;

void RecruitPedestrian(Ped npc)
{
	for (const auto& member : g_Menu.bodyguards)
	{
		if (member.handle == npc) return;
	}

	if (g_Menu.bodyguards.size() >= 4)
	{
		ShowNotification("~r~Squad Limit Reached! Max 4 members.");
		return;
	}

	Ped playerPed = PLAYER::PLAYER_PED_ID();
	int playerGroup = PED::GET_PED_GROUP_INDEX(playerPed);

	if (playerGroup == 0)
	{
		playerGroup = PED::CREATE_GROUP(0);
		PED::SET_PED_AS_GROUP_LEADER(playerPed, playerGroup);
	}

	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(npc, true);
	PED::SET_PED_COMBAT_ATTRIBUTES(npc, 5, true);
	PED::SET_PED_COMBAT_ABILITY(npc, 2);
	PED::SET_PED_COMBAT_MOVEMENT(npc, 2);
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(npc, MISC::GET_HASH_KEY("PLAYER"));

	PED::SET_PED_AS_GROUP_MEMBER(npc, playerGroup);
	PED::SET_PED_CAN_TELEPORT_TO_GROUP_LEADER(npc, playerGroup, TRUE);
	PED::SET_PED_CAN_BE_TARGETTED(playerPed, FALSE);
	WEAPON::GIVE_WEAPON_TO_PED(npc, MISC::GET_HASH_KEY("WEAPON_PISTOL"), 999, false, true);
	TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(npc, PLAYER::PLAYER_PED_ID(), 0.0f, -2.0f, 0.0f, 1.5f, -1, 10.0f, true);
	PED::SET_PED_NEVER_LEAVES_GROUP(npc, true);

	Hash playerGroupHash = MISC::GET_HASH_KEY("PLAYER");
	Hash enemyGroupHash = MISC::GET_HASH_KEY("COP");

	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, playerGroupHash, enemyGroupHash);
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, enemyGroupHash, playerGroupHash);

	int randomArrayIndex = rand() % CALLSIGNS_COUNT;
	std::string assignedName = CALLSIGNS[randomArrayIndex];

	Bodyguard newGuard;
	newGuard.handle = npc;
	newGuard.codeName = assignedName;

	g_Menu.bodyguards.push_back(newGuard);

	ShowNotification("~g~" + assignedName + " has joined your squad!", 2500);
}

void UpdateSquadState()
{
	Ped playerPed = PLAYER::PLAYER_PED_ID();

	for (auto it = g_Menu.bodyguards.begin(); it != g_Menu.bodyguards.end(); )
	{
		Ped currentGuardHandle = it->handle;
		assert(it->handle != -1);

		if (!ENTITY::DOES_ENTITY_EXIST(currentGuardHandle) || ENTITY::IS_ENTITY_DEAD(currentGuardHandle, FALSE))
		{
			ShowNotification("~r~Squad Member " + it->codeName + " KIA!", 3000);

			it = g_Menu.bodyguards.erase(it);
		}
		else
		{
			Vector3 guardCoords = ENTITY::GET_ENTITY_COORDS(currentGuardHandle, true);
			guardCoords.z += 1.1f;

			int currentHealth = ENTITY::GET_ENTITY_HEALTH(currentGuardHandle);
			std::string uiTag = it->codeName + " [HP: " + std::to_string(currentHealth) + "]";
			DrawText3D(guardCoords, uiTag.c_str());

			if (PED::IS_PED_SITTING_IN_ANY_VEHICLE(playerPed))
			{
				Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(playerPed, true);
				if (!PED::IS_PED_IN_ANY_VEHICLE(it->handle, false) && TASK::GET_SCRIPT_TASK_STATUS(it->handle, 0x159B5B20) != 1)
				{
					int seat = GetFreePassengerSeat(veh);
					if (seat != -2)
					{
						TASK::TASK_ENTER_VEHICLE(it->handle, veh, 20000, seat, 2.0f, 16, 0);
					}
				}
			}
			++it;
		}
	}
}

float getDistanceBetweenEntities(Entity entity1, Entity entity2) {
	Vector3 entity1Coords = ENTITY::GET_ENTITY_COORDS(entity1, true);
	Vector3 entity2Coords = ENTITY::GET_ENTITY_COORDS(entity2, true);
	return MISC::GET_DISTANCE_BETWEEN_COORDS(entity1Coords.x, entity1Coords.y, entity1Coords.z, entity2Coords.x, entity2Coords.y, entity2Coords.z, 1);
}

float getDistanceToGround(Entity entity) {
	Vector3 entityCoords = ENTITY::GET_ENTITY_COORDS(entity, true);
	float groundZ = 0;
	MISC::GET_GROUND_Z_FOR_3D_COORD(entityCoords.x, entityCoords.y, entityCoords.z, &groundZ, 0, 0);
	return MISC::GET_DISTANCE_BETWEEN_COORDS(entityCoords.x, entityCoords.y, entityCoords.z, entityCoords.x, entityCoords.y, groundZ, 1);
}

Hash getModelHash(Vehicle veh) {
	return ENTITY::GET_ENTITY_MODEL(veh);
}

bool isVehicleDrivable(Vehicle veh) {
	return (VEHICLE::IS_VEHICLE_DRIVEABLE(veh, false) &&
		(VEHICLE::IS_THIS_MODEL_A_CAR(getModelHash(veh)) ||
			VEHICLE::IS_THIS_MODEL_A_BIKE(getModelHash(veh)) ||
			VEHICLE::IS_THIS_MODEL_A_QUADBIKE(getModelHash(veh)) ||
			VEHICLE::IS_THIS_MODEL_A_HELI(getModelHash(veh)) ||
			VEHICLE::IS_THIS_MODEL_A_PLANE(getModelHash(veh)) ||
			VEHICLE::IS_THIS_MODEL_A_BOAT(getModelHash(veh)) ||
			VEHICLE::IS_THIS_MODEL_A_BICYCLE(getModelHash(veh))));
}

Vehicle getClosestVehicleFromPedPos(Ped ped, int maxDistance, int maxHeight, bool canReturnVehicleInside) {
	Vehicle veh = NULL;
	float smallestDistance = (float)maxDistance;
	const int ARR_SIZE = 1024;
	Vehicle vehs[ARR_SIZE];
	int count = worldGetAllVehicles(vehs, ARR_SIZE);

	if (vehs != NULL)
	{
		for (int i = 0; i < count; i++)
		{
			if (EntityExists(vehs[i]) && (canReturnVehicleInside || PED::IS_PED_IN_VEHICLE(ped, vehs[i], false) == false))
			{
				float distance = getDistanceBetweenEntities(ped, vehs[i]);
				float height = getDistanceToGround(vehs[i]);
				if (distance <= smallestDistance && height <= maxHeight && height >= 0 && isVehicleDrivable(vehs[i]))
				{
					smallestDistance = distance;
					veh = vehs[i];
				}
			}
		}
	}

	return veh;
}

void EnterVehicleCommand(Ped npc) {
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	Vector3 coords = ENTITY::GET_ENTITY_COORDS(playerPed, true);
	Vehicle vehicle = getClosestVehicleFromPedPos(npc, 10.0f, 10.0f, true);

	TASK::TASK_VEHICLE_DRIVE_TO_COORD(npc, vehicle, coords.x, coords.y, coords.z, 20.0f, 0, getModelHash(vehicle), 262144, -1.0f, -1.0f);
}

void taskHandler(Task t, Ped npc) {
	switch (t) {
	case ENTER_VEHICLE:
		EnterVehicleCommand(npc);
		break;
	}
}

void keyHandler(Ped npc) {
	if (IsKeyJustUp(VK_F9))
	{
		RecruitPedestrian(npc);
	}
	else if (IsKeyJustUp(VK_F8)) {
		taskHandler(ENTER_VEHICLE, npc);
	}
}

void CheckNearestPedToTalk()
{
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	Vector3 coords = ENTITY::GET_ENTITY_COORDS(playerPed, true);

	Ped nearestPed = 0;
	PED::GET_CLOSEST_PED(coords.x, coords.y, coords.z, 6.0f, true, false, &nearestPed, false, false, -1);

	if (nearestPed != 0 && ENTITY::DOES_ENTITY_EXIST(nearestPed) &&
		!ENTITY::IS_ENTITY_DEAD(nearestPed, false) && nearestPed != playerPed)
	{
		HUD::BEGIN_TEXT_COMMAND_DISPLAY_HELP("STRING");
		HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Press ~INPUT_CONTEXT~ to talk.");
		HUD::END_TEXT_COMMAND_DISPLAY_HELP(0, false, true, -1);

		if (PAD::IS_CONTROL_JUST_PRESSED(0, 51))
		{
			TASK::TASK_TURN_PED_TO_FACE_ENTITY(nearestPed, playerPed, 3000);
			TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(nearestPed, playerPed, 10.0f, 10.0f, 10.0f, 2.5f, -1, 0.2f, 1);

			ShowNotification("Hey, how is it going?", 3000);
		}
	}
}

int main()
{
	while (true)
	{
		// Open main menu when pressing F4
		if (IsKeyJustUp(VK_F4))
		{
			g_Menu.process_main_menu();
		}

		CheckNearestPedToTalk();
		UpdateSquadState();

		if (PLAYER::IS_PLAYER_FREE_AIMING(PLAYER::PLAYER_ID()))
		{
			Entity targetEntity;
			if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(PLAYER::PLAYER_ID(), &targetEntity))
			{
				if (ENTITY::IS_ENTITY_A_PED(targetEntity) && !ENTITY::IS_ENTITY_DEAD(targetEntity, FALSE))
				{
					Ped npc = targetEntity;
					Vector3 npcPos = ENTITY::GET_ENTITY_COORDS(npc, true);
					npcPos.z += 1.0f;

					DrawText3D(npcPos, "PRESS F9 TO RECRUIT / F8 TO ENTER VEHICLE");
					keyHandler(npc);
				}
			}
		}

		scriptWait(0);
	}
}

void ScriptMain()
{
	srand(GetTickCount());
	main();
}