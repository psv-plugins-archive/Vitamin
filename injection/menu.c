/*
	Vitamin
	Copyright (C) 2016, Team FreeK (TheFloW, Major Tom, mr. gas)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2/ctrl.h>
#include <psp2/display.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "menu.h"

#include "../common/common.h"
#include "../common/graphics.h"

static uint32_t current_buttons = 0, pressed_buttons = 0;

void waitForUser() {
	while (1) {
		SceCtrlData pad;
		memset(&pad, 0, sizeof(SceCtrlData));
		sceCtrlPeekBufferPositive(0, &pad, 1);

		pressed_buttons = pad.buttons & ~current_buttons;
		current_buttons = pad.buttons;

		if (pressed_buttons & SCE_CTRL_CROSS) {
			break;
		}

		sceDisplayWaitVblankStart();
	}
}

int doMenu(char *info, char *title, int back_button, char **entries, int n_entries) {
	int back = 0;
	int sel = 0;

	psvDebugScreenClear(DARKBLUE);
	psvDebugScreenSetBgColor(DARKBLUE);

	while (1) {
		// Ctrl
		SceCtrlData pad;
		memset(&pad, 0, sizeof(SceCtrlData));
		sceCtrlPeekBufferPositive(0, &pad, 1);

		pressed_buttons = pad.buttons & ~current_buttons;
		current_buttons = pad.buttons;

		if (pressed_buttons & SCE_CTRL_UP) {
			if (sel > 0) {
				int old_sel = sel;

				sel--;

				if (entries[sel][0] == '\0')
					sel--;

				// Clear screen at new page
				if ((old_sel / ENTRIES_PER_PAGE) != (sel / ENTRIES_PER_PAGE)) {
					psvDebugScreenClearMargin(DARKBLUE);
				}
			}
		} else if (pressed_buttons & SCE_CTRL_DOWN) {
			if (sel < (n_entries - 1)) {
				int old_sel = sel;

				sel++;

				if (entries[sel][0] == '\0')
					sel++;

				// Clear screen at new page
				if ((old_sel / ENTRIES_PER_PAGE) != (sel / ENTRIES_PER_PAGE)) {
					psvDebugScreenClearMargin(DARKBLUE);
				}
			}
		} else if (pressed_buttons & SCE_CTRL_CROSS) {
			return sel;
		} else if (pressed_buttons & SCE_CTRL_CIRCLE) {
			if (back_button >= 0) {
				sel = back_button;
				back = 1;
			}
		}

		// Layout
		printLayout(info, title);

		// Entries
		int page = sel / ENTRIES_PER_PAGE;
		int last_page_number = n_entries / ENTRIES_PER_PAGE;
		int n_entries_last_page = n_entries % ENTRIES_PER_PAGE;

		int count = (page == last_page_number) ? n_entries_last_page : ENTRIES_PER_PAGE;

		int i;
		for (i = (page * ENTRIES_PER_PAGE); i < ((page * ENTRIES_PER_PAGE) + count); i++) {
			psvDebugScreenSetFgColor((sel == i) ? WHITE : YELLOW);
			printf("%c %s\n", (sel == i) ? '>' : ' ', entries[i]);
		}

		// Back
		if (back)
			return back_button;

		sceDisplayWaitVblankStart();
	}

	return -1;
}