#include "pch.h"
#include "script.h"
#include "keyboard.h"
#include "Bodyguard.h"
#include "Utils.h"
#include "menu.h"

#include <string>
#include <cstdlib>
#include <vector>
#include <iterator>

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

		HUD::_SET_TEXT_ENTRY(const_cast<char*>("STRING"));
		HUD::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(text));
		HUD::_DRAW_TEXT(screenX, screenY, 1);
	}
}

constexpr int kNoFreeSeat = -2;

// Beyond this many metres a squad member is warped into the player's vehicle instead of
// being left to run after it (they cannot walk into a car that is already driving off).
constexpr float kSquadRejoinDistance = 30.0f;

int GetFreePassengerSeat(Vehicle veh) {
	int maxSeats = VEHICLE::GET_VEHICLE_MAX_NUMBER_OF_PASSENGERS(veh);

	for (int seat = 0; seat < maxSeats; seat++) {
		if (VEHICLE::IS_VEHICLE_SEAT_FREE(veh, seat, false)) {
			return seat;
		}
	}
	return kNoFreeSeat;
}

enum class Task
{
	EnterVehicle
};

const std::string CALLSIGNS[] = { "Alpha", "Bravo", "Charlie", "Delta", "Echo", "Ghost", "Specter" };
constexpr int CALLSIGNS_COUNT = static_cast<int>(std::size(CALLSIGNS));

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

	// Rebuild the target as a clean script ped, the way the reference trainer hires a
	// nearby ped (bodyguards.cpp, "add nearest ped as bodyguard"): keep the model and the
	// spot, delete the ambient pedestrian, and create a fresh PED_TYPE_SPECIAL one.
	// Recruiting the ambient ped in place leaves it with its civilian ped type, decision
	// maker, existing tasks and witness state - which is why it carried on behaving like
	// the civilian it was no matter what we set on it.
	Hash pedModel = ENTITY::GET_ENTITY_MODEL(npc);
	Vector3 pedCoords = ENTITY::GET_ENTITY_COORDS(npc, true);
	float pedHeading = ENTITY::GET_ENTITY_HEADING(npc);

	// Snapshot the outfit so the rebuilt ped still looks like the one recruited.
	int pedComponents[12][3];
	for (int c = 0; c < 12; c++)
	{
		pedComponents[c][0] = PED::GET_PED_DRAWABLE_VARIATION(npc, c);
		pedComponents[c][1] = PED::GET_PED_TEXTURE_VARIATION(npc, c);
		pedComponents[c][2] = PED::GET_PED_PALETTE_VARIATION(npc, c);
	}

	PED::DELETE_PED(&npc);
	npc = PED::CREATE_PED(25 /* PED_TYPE_SPECIAL */, pedModel, pedCoords.x, pedCoords.y, pedCoords.z, pedHeading, false, false);

	if (npc == 0)
	{
		ShowNotification("~r~Could not recruit that pedestrian.");
		return;
	}

	for (int c = 0; c < 12; c++)
	{
		if (pedComponents[c][0] >= 0)
		{
			PED::SET_PED_COMPONENT_VARIATION(npc, c, pedComponents[c][0], pedComponents[c][1], pedComponents[c][2]);
		}
	}

	Ped playerPed = PLAYER::PLAYER_PED_ID();

	// The player's own ped group. Group membership is what makes a ped follow the player
	// and be treated as crew; the reference trainer reads it with GET_PLAYER_GROUP and
	// re-asserts membership every frame because the engine drops it after combat.
	int playerGroup = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());
	if (playerGroup == 0)
	{
		playerGroup = PED::CREATE_GROUP(0);
	}

	PED::SET_PED_AS_GROUP_LEADER(playerPed, playerGroup);
	PED::SET_PED_AS_GROUP_MEMBER(npc, playerGroup);
	PED::SET_PED_NEVER_LEAVES_GROUP(npc, true);
	// Off on purpose: this warps followers to the player whenever his coords jump, which
	// is what dumped them on top of him the moment he got into a car. The reference
	// trainer does not use it at all, and prefers tasking them into the vehicle instead.
	PED::SET_PED_CAN_TELEPORT_TO_GROUP_LEADER(npc, playerGroup, false);
	PED::SET_GROUP_FORMATION(playerGroup, 1);
	PED::SET_GROUP_FORMATION_SPACING(playerGroup, 1.0f, 0.9f, 3.0f);

	// Same relationship group as the player, so cops treat the squad as his crew while he
	// is wanted. This only puts the guard in the player's group - it does not rewrite
	// anyone's relationships. (The reference trainer leaves this line out entirely.)
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(npc, PED::GET_PED_RELATIONSHIP_GROUP_HASH(playerPed));

	PED::SET_PED_DIES_IN_WATER(npc, false);
	PED::SET_PED_DIES_INSTANTLY_IN_WATER(npc, false);
	PED::SET_ENABLE_SCUBA(npc, true);

	// The line that actually stops a guard shooting the player. Without it a guard treats
	// him as a legal target once he commits a crime (carjacking) in front of it, and with
	// flee disabled its only remaining response is to attack him.
	PED::SET_CAN_ATTACK_FRIENDLY(npc, false, false);

	// Combat profile - same values as EnhancedNativeTrainer/.../features/bodyguards.cpp
	PED::SET_PED_COMBAT_ABILITY(npc, 2);            // CA_Professional
	PED::SET_PED_COMBAT_RANGE(npc, 2);
	PED::SET_PED_COMBAT_MOVEMENT(npc, 2);           // CM_WillAdvance (reference passes 3)
	PED::SET_PED_COMBAT_ATTRIBUTES(npc, 1, true);   // BF_CanUseVehicles
	PED::SET_PED_COMBAT_ATTRIBUTES(npc, 2, true);   // BF_CanDoDrivebys
	PED::SET_PED_COMBAT_ATTRIBUTES(npc, 5, true);   // BF_AlwaysFight
	PED::SET_PED_COMBAT_ATTRIBUTES(npc, 20, true);  // BF_CanTauntInVehicle
	PED::SET_PED_COMBAT_ATTRIBUTES(npc, 46, true);  // BF_CanFightArmedPedsWhenNotArmed
	PED::SET_PED_SHOOT_RATE(npc, 999);
	PED::SET_COMBAT_FLOAT(npc, 0, 1.0f);
	PED::SET_COMBAT_FLOAT(npc, 1, 1.0f);
	PED::SET_COMBAT_FLOAT(npc, 3, 1.0f);
	PED::SET_COMBAT_FLOAT(npc, 4, 1.0f);
	PED::SET_COMBAT_FLOAT(npc, 5, 1.0f);
	PED::SET_COMBAT_FLOAT(npc, 8, 1.0f);
	PED::SET_COMBAT_FLOAT(npc, 11, 1.0f);
	PED::SET_COMBAT_FLOAT(npc, 12, 1.0f);
	PED::SET_COMBAT_FLOAT(npc, 16, 1.0f);
	PED::SET_PED_CAN_SWITCH_WEAPON(npc, true);
	PED::SET_PED_ACCURACY(npc, 60);
	PED::SET_PED_ALERTNESS(npc, 3);
	PED::SET_PED_KEEP_TASK(npc, true);
	PED::SET_PED_FIRING_PATTERN(npc, MISC::GET_HASH_KEY("FIRING_PATTERN_FULL_AUTO"));

	WEAPON::GIVE_WEAPON_TO_PED(npc, MISC::GET_HASH_KEY("WEAPON_PISTOL"), 999, false, true);

	int randomArrayIndex = rand() % CALLSIGNS_COUNT;
	std::string assignedName = CALLSIGNS[randomArrayIndex];

	Bodyguard newGuard;
	newGuard.handle = npc;
	newGuard.codeName = assignedName;
	newGuard.currentState = SquadState::FOLLOW;

	g_Menu.bodyguards.push_back(newGuard);

	// Start following here rather than through SetBodyguardState, whose job is to react to
	// state *changes* - the ped is already in FOLLOW when recruited.
	TASK::CLEAR_PED_TASKS(npc);
	TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(npc, playerPed, 0.0f, -2.0f, 0.0f, 1.5f, -1, 10.0f, true);

	ShowNotification("~g~" + assignedName + " has joined your squad!", 2500);
}

void UpdateSquadState()
{
	Ped playerPed = PLAYER::PLAYER_PED_ID();
	int playerGroup = PLAYER::GET_PLAYER_GROUP(PLAYER::PLAYER_ID());

	for (auto it = g_Menu.bodyguards.begin(); it != g_Menu.bodyguards.end(); )
	{
		Ped currentGuardHandle = it->handle;

		if (!ENTITY::DOES_ENTITY_EXIST(currentGuardHandle) || ENTITY::IS_ENTITY_DEAD(currentGuardHandle, false))
		{
			ShowNotification("~r~Squad Member " + it->codeName + " DIED!", 3000);

			it = g_Menu.bodyguards.erase(it);
			continue;
		}

		// The engine drops ped-group membership after combat and vehicle transitions, so
		// re-assert it every frame - this is what the reference trainer's maintain loop does.
		PED::SET_PED_AS_GROUP_MEMBER(currentGuardHandle, playerGroup);
		PED::SET_PED_NEVER_LEAVES_GROUP(currentGuardHandle, true);

		Vector3 guardCoords = ENTITY::GET_ENTITY_COORDS(currentGuardHandle, true);
		guardCoords.z += 1.1f;

		int currentHealth = ENTITY::GET_ENTITY_HEALTH(currentGuardHandle);
		std::string uiTag = it->codeName + " [HP: " + std::to_string(currentHealth) + "]";
		DrawText3D(guardCoords, uiTag.c_str());

		if (PED::IS_PED_SITTING_IN_ANY_VEHICLE(playerPed))
		{
			Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(playerPed, true);

			if (!PED::IS_PED_IN_ANY_VEHICLE(currentGuardHandle, false))
			{
				int seat = GetFreePassengerSeat(veh);
				if (seat != kNoFreeSeat)
				{
					Vector3 playerCoords = ENTITY::GET_ENTITY_COORDS(playerPed, true);
					float distance = MISC::GET_DISTANCE_BETWEEN_COORDS(
						guardCoords.x, guardCoords.y, guardCoords.z,
						playerCoords.x, playerCoords.y, playerCoords.z, true);

					if (distance > kSquadRejoinDistance)
					{
						// Too far behind to walk it, so put them in directly. This is the
						// deliberate one-shot warp (same native the reference trainer uses
						// at spawn), not a teleport flag buried in a per-frame task.
						TASK::TASK_WARP_PED_INTO_VEHICLE(currentGuardHandle, veh, seat);
					}
					// 0x950B6492 is SCRIPT_TASK_ENTER_VEHICLE from eScriptTaskHash. The
					// status check needs the real hash or the task is re-issued every frame
					// and the guard never actually gets into the seat.
					else if (TASK::GET_SCRIPT_TASK_STATUS(currentGuardHandle, 0x950B6492) != 1)
					{
						// Flag 1 = normal entry (walk/run round to the door and get in).
						// It must NOT be 3 or 16: per the SDK these are "teleport to
						// vehicle" and "teleport directly into vehicle".
						TASK::TASK_ENTER_VEHICLE(currentGuardHandle, veh, 20000, seat, 2.0f, 1, 0);
					}
				}
			}
		}

		++it;
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
	Hash model = getModelHash(veh);
	return (VEHICLE::IS_VEHICLE_DRIVEABLE(veh, false) &&
		(VEHICLE::IS_THIS_MODEL_A_CAR(model) ||
			VEHICLE::IS_THIS_MODEL_A_BIKE(model) ||
			VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model) ||
			VEHICLE::IS_THIS_MODEL_A_HELI(model) ||
			VEHICLE::IS_THIS_MODEL_A_PLANE(model) ||
			VEHICLE::IS_THIS_MODEL_A_BOAT(model) ||
			VEHICLE::IS_THIS_MODEL_A_BICYCLE(model)));
}

Vehicle getClosestVehicleFromPedPos(Ped ped, int maxDistance, int maxHeight, bool canReturnVehicleInside) {
	Vehicle veh = 0;
	float smallestDistance = (float)maxDistance;
	const int ARR_SIZE = 1024;
	Vehicle vehs[ARR_SIZE];
	int count = worldGetAllVehicles(vehs, ARR_SIZE);

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
	case Task::EnterVehicle:
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
		taskHandler(Task::EnterVehicle, npc);
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

		if (IsKeyJustUp(VK_F7))
		{
			// Toggle: order the squad to clear the area, press again to resume following.
			bool anyFollowing = false;
			for (const auto& member : g_Menu.bodyguards)
			{
				if (member.currentState == SquadState::FOLLOW) anyFollowing = true;
			}

			for (auto& member : g_Menu.bodyguards)
			{
				SetBodyguardState(member, anyFollowing ? SquadState::ATTACK : SquadState::FOLLOW);
			}

			ShowNotification(anyFollowing ? "~r~Squad ordered to attack enemies nearby!"
		                                  : "~g~Squad resuming follow.");
		}

		CheckNearestPedToTalk();
		UpdateSquadState();

		if (PLAYER::IS_PLAYER_FREE_AIMING(PLAYER::PLAYER_ID()))
		{
			Entity targetEntity;
			if (PLAYER::GET_ENTITY_PLAYER_IS_FREE_AIMING_AT(PLAYER::PLAYER_ID(), &targetEntity))
			{
				if (ENTITY::IS_ENTITY_A_PED(targetEntity) && !ENTITY::IS_ENTITY_DEAD(targetEntity, false))
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
	srand(GetTickCount64());
	main();
}