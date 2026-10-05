#pragma once
#include <string>
#include <vector>
#include "keyboard.h"
#include "natives.h"
#include "types.h"
#include "enums.h"
#include "Bodyguard.h"
#include "Utils.h"

class Menu {
private:
	int activeLineIndexMain = 0;
	int activeLineIndexPlayer = 0;
	std::string statusText;
	DWORD statusTextDrawTicksMax;
	bool statusTextGxtEntry;

	void draw_rect(float A_0, float A_1, float A_2, float A_3, int A_4, int A_5, int A_6, int A_7);
	void draw_menu_line(std::string caption, float lineWidth, float lineHeight, float lineTop, float lineLeft, float textLeft, bool active, bool title, bool rescaleText = true);
	void update_status_text();
	void set_status_text(std::string str, DWORD time = 2500, bool isGxtEntry = false);
	void get_button_state(bool* a, bool* b, bool* up, bool* down, bool* l, bool* r);
	void menu_beep();
	void process_squad_menu();
	bool trainer_switch_pressed();
public:
	std::vector<Bodyguard> bodyguards;

	void process_main_menu();
	Menu();

};

extern Menu g_Menu;

