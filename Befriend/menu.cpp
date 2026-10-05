#include "menu.h"

// Define global instance
menu g_Menu;

menu::menu() {
	playerPed = PLAYER::PLAYER_PED_ID();
}

void menu::draw_rect(float A_0, float A_1, float A_2, float A_3, int A_4, int A_5, int A_6, int A_7)
{
	GRAPHICS::DRAW_RECT((A_0 + (A_2 * 0.5f)), (A_1 + (A_3 * 0.5f)), A_2, A_3, A_4, A_5, A_6, A_7, 0);
}

void menu::draw_menu_line(std::string caption, float lineWidth, float lineHeight, float lineTop, float lineLeft, float textLeft, bool active, bool title, bool rescaleText)
{
	int text_col[4] = { 255, 255, 255, 255 },
		rect_col[4] = { 70, 95, 95, 255 };
	float text_scale = 0.35f;
	int font = 0;

	if (active)
	{
		text_col[0] = 0;
		text_col[1] = 0;
		text_col[2] = 0;

		rect_col[0] = 218;
		rect_col[1] = 242;
		rect_col[2] = 216;

		if (rescaleText) text_scale = 0.40f;
	}

	if (title)
	{
		rect_col[0] = 0;
		rect_col[1] = 0;
		rect_col[2] = 0;

		if (rescaleText) text_scale = 0.50f;
		font = 1;
	}

	int screen_w, screen_h;
	GRAPHICS::GET_SCREEN_RESOLUTION(&screen_w, &screen_h);

	textLeft += lineLeft;

	float lineWidthScaled = lineWidth / (float)screen_w;
	float lineTopScaled = lineTop / (float)screen_h;
	float textLeftScaled = textLeft / (float)screen_w;
	float lineHeightScaled = lineHeight / (float)screen_h;
	float lineLeftScaled = lineLeft / (float)screen_w;

	HUD::SET_TEXT_FONT(font);
	HUD::SET_TEXT_SCALE(0.0, text_scale);
	HUD::SET_TEXT_COLOUR(text_col[0], text_col[1], text_col[2], text_col[3]);
	HUD::SET_TEXT_CENTRE(0);
	HUD::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	HUD::SET_TEXT_EDGE(0, 0, 0, 0, 0);
	HUD::_SET_TEXT_ENTRY("STRING");
	HUD::_ADD_TEXT_COMPONENT_STRING((LPSTR)caption.c_str());
	HUD::_DRAW_TEXT(textLeftScaled, (((lineTopScaled + 0.00278f) + lineHeightScaled) - 0.005f), 0);

	HUD::SET_TEXT_FONT(font);
	HUD::SET_TEXT_SCALE(0.0, text_scale);
	HUD::SET_TEXT_COLOUR(text_col[0], text_col[1], text_col[2], text_col[3]);
	HUD::SET_TEXT_CENTRE(0);
	HUD::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	HUD::SET_TEXT_EDGE(0, 0, 0, 0, 0);
	HUD::_SET_TEXT_GXT_ENTRY("STRING");
	HUD::_ADD_TEXT_COMPONENT_STRING((LPSTR)caption.c_str());
	int num25 = HUD::END_TEXT_COMMAND_GET_NUMBER_OF_LINES_FOR_STRING(textLeftScaled, (((lineTopScaled + 0.00278f) + lineHeightScaled) - 0.005f));

	draw_rect(lineLeftScaled, lineTopScaled + (0.00278f),
		lineWidthScaled, ((((float)(num25)*HUD::GET_RENDERED_CHARACTER_HEIGHT(text_scale, 0)) + (lineHeightScaled * 2.0f)) + 0.005f),
		rect_col[0], rect_col[1], rect_col[2], rect_col[3]);
}

void menu::update_status_text()
{
	if (GetTickCount() < statusTextDrawTicksMax)
	{
		HUD::SET_TEXT_FONT(0);
		HUD::SET_TEXT_SCALE(0.55f, 0.55f);
		HUD::SET_TEXT_COLOUR(255, 255, 255, 255);
		HUD::SET_TEXT_WRAP(0.0f, 1.0f);
		HUD::SET_TEXT_CENTRE(1);
		HUD::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		HUD::SET_TEXT_EDGE(1, 0, 0, 0, 205);
		if (statusTextGxtEntry)
		{
			HUD::_SET_TEXT_ENTRY(const_cast<char*>(statusText.c_str()));
		}
		else
		{
			HUD::_SET_TEXT_ENTRY("STRING");
			HUD::_ADD_TEXT_COMPONENT_STRING((char*)statusText.c_str());
		}
		HUD::_DRAW_TEXT(0.5f, 0.5f, 0);
	}
}

void menu::set_status_text(std::string str, DWORD time, bool isGxtEntry)
{
	statusText = str;
	statusTextDrawTicksMax = GetTickCount() + time;
	statusTextGxtEntry = isGxtEntry;
}

bool menu::trainer_switch_pressed()
{
	return IsKeyJustUp(VK_F4);
}

void menu::get_button_state(bool* a, bool* b, bool* up, bool* down, bool* l, bool* r)
{
	if (a) *a = IsKeyDown(VK_NUMPAD5);
	if (b) *b = IsKeyDown(VK_NUMPAD0) || trainer_switch_pressed() || IsKeyDown(VK_BACK);
	if (up) *up = IsKeyDown(VK_NUMPAD8);
	if (down) *down = IsKeyDown(VK_NUMPAD2);
	if (r) *r = IsKeyDown(VK_NUMPAD6);
	if (l) *l = IsKeyDown(VK_NUMPAD4);
}

void menu::menu_beep()
{
	AUDIO::PLAY_SOUND_FRONTEND(-1, "NAV_UP_DOWN", "HUD_FRONTEND_DEFAULT_SOUNDSET", 0);
}

void menu::process_squad_menu() {
	const float lineWidth = 250.0f;
	DWORD waitTime = 150;

	while (true)
	{
		std::vector<std::string> lines;
		if (bodyguards.empty())
		{
			lines.push_back("No squad members");
		}
		else {
			for (size_t i = 0; i < bodyguards.size(); ++i)
			{
				Ped handle = bodyguards[i].handle;
				if (EntityExists(handle))
				{
					int hp = ENTITY::GET_ENTITY_HEALTH(handle);
					std::string status = bodyguards[i].hasATask ? "Busy" : "Ready";
					lines.push_back(bodyguards[i].codeName + " [HP: " + std::to_string(hp) + "] (" + status + ")");
				}
				else {
					lines.push_back(bodyguards[i].codeName + " [KIA]");
				}
			}
			lines.push_back("Dismiss All Squad");
		}

		int lineCount = static_cast<int>(lines.size());

		if (activeLineIndexPlayer >= lineCount)
		{
			activeLineIndexPlayer = (lineCount > 0) ? lineCount - 1 : 0;
		}

		DWORD maxTickCount = GetTickCount() + waitTime;
		do
		{
			draw_menu_line("SQUAD MANAGEMENT", lineWidth, 15.0f, 18.0f, 0.0f, 5.0f, false, true);
			for (int i = 0; i < lineCount; i++)
			{
				if (i != activeLineIndexPlayer) {
					draw_menu_line(lines[i], lineWidth, 9.0f, 60.0f + i * 36.0f, 0.0f, 9.0f, false, false);
				}
			}

			if (lineCount > 0)
			{
				draw_menu_line(lines[activeLineIndexPlayer], lineWidth + 1.0f, 11.0f, 56.0f + activeLineIndexPlayer * 36.0f, 0.0f, 7.0f, true, false);
			}

			update_status_text();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);

		waitTime = 0;

		bool bSelect, bBack, bUp, bDown;
		get_button_state(&bSelect, &bBack, &bUp, &bDown, NULL, NULL);

		if (bSelect)
		{
			menu_beep();

			if (!bodyguards.empty())
			{
				if (activeLineIndexPlayer == static_cast<int>(bodyguards.size())) {
					Vector3 playerCoords = ENTITY::GET_ENTITY_COORDS(playerPed, true);

					for (auto& member : bodyguards)
					{
						if (EntityExists(member.handle))
						{
							PED::REMOVE_PED_FROM_GROUP(member.handle);
							ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&member.handle);
							TASK::TASK_SMART_FLEE_COORD(member.handle, playerCoords.x, playerCoords.y, playerCoords.z, 10000, 20000, false, true);
						}
					}
					bodyguards.clear();
					set_status_text("All squad members dismissed");
				}
				else if (activeLineIndexPlayer < static_cast<int>(bodyguards.size()))
				{
					Bodyguard& selected = bodyguards[activeLineIndexPlayer];

					selected.hasATask = !selected.hasATask;
					if (selected.hasATask)
					{
						TASK::TASK_STAND_STILL(selected.handle, -1);
						set_status_text(selected.codeName + " Holding Position");
					}
					else
					{
						TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(selected.handle, PLAYER::PLAYER_PED_ID(), 0.0f, -2.0f, 0.0f, 1.5f, -1, 10.0f, true);
						set_status_text(selected.codeName + " Following");
					}
				}
			}
			waitTime = 200;
		}
		else if (bBack || trainer_switch_pressed())
		{
			menu_beep();
			break;
		}
		else if (bUp) {
			menu_beep();
			if (activeLineIndexPlayer <= 0)
			{
				activeLineIndexPlayer = lineCount - 1;
			}
			else
				activeLineIndexPlayer--;
			waitTime = 150;
		}
		else if (bDown) {
			menu_beep();
			activeLineIndexPlayer++;
			if (activeLineIndexPlayer >= lineCount)
				activeLineIndexPlayer = 0;
			waitTime = 150;
		}
	}
}

void menu::process_main_menu()
{
	const float lineWidth = 250.0f;
	const int lineCount = 7;

	std::string caption = "NATIVE  TRAINER  (AB)";

	static LPCSTR lineCaption[lineCount] = {
		"SQUAD MANAGEMENT",
		"WEAPON",
		"VEHICLE",
		"WORLD",
		"TIME",
		"WEATHER",
		"MISC"
	};

	DWORD waitTime = 150;
	while (true)
	{
		DWORD maxTickCount = GetTickCount() + waitTime;
		do
		{
			draw_menu_line(caption, lineWidth, 15.0f, 18.0f, 0.0f, 5.0f, false, true);
			for (int i = 0; i < lineCount; i++)
				if (i != activeLineIndexMain)
					draw_menu_line(lineCaption[i], lineWidth, 9.0f, 60.0f + i * 36.0f, 0.0f, 9.0f, false, false);
			draw_menu_line(lineCaption[activeLineIndexMain], lineWidth + 1.0f, 11.0f, 56.0f + activeLineIndexMain * 36.0f, 0.0f, 7.0f, true, false);

			update_status_text();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		bool bSelect, bBack, bUp, bDown;
		get_button_state(&bSelect, &bBack, &bUp, &bDown, NULL, NULL);
		if (bSelect)
		{
			menu_beep();
			switch (activeLineIndexMain)
			{
			case 0:
				process_squad_menu();
				break;
			default:
				break;
			}
			waitTime = 200;
		}
		else if (bBack || trainer_switch_pressed())
		{
			menu_beep();
			break;
		}
		else if (bUp)
		{
			menu_beep();
			if (activeLineIndexMain == 0)
				activeLineIndexMain = lineCount;
			activeLineIndexMain--;
			waitTime = 150;
		}
		else if (bDown)
		{
			menu_beep();
			activeLineIndexMain++;
			if (activeLineIndexMain == lineCount)
				activeLineIndexMain = 0;
			waitTime = 150;
		}
	}
}