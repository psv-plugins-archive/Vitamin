/*
	Vitamin
	Copyright (C) 2016, Team FreeK

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

#include <psp2/appmgr.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../common/graphics.h"

#define printf psvDebugScreenPrintf

typedef struct {
	char *name;
	int (* function)();
} MenuEntry;

MenuEntry menu_entries[] = {
	// { "Dump full game + update" },
	{ "Dump full game", NULL },
	{ "Dump update", NULL },
	{ "Back", NULL },
};

#define ENTRIES_PER_PAGE 5

#define DARKBLUE 0x00402000
#define YELLOW 0x0080FFFF
#define WHITE 0x00FFFFFF
#define CYAN 0x00FFFF80
#define GREEN 0x0080FF00
#define RED 0x000000FF

#define BLUE 0x00FF8000

int doMenu(char *title, MenuEntry *entries, int n_entries) {
	uint32_t current_buttons = 0, pressed_buttons = 0;

	int sel = 0;

	psvDebugScreenClear(DARKBLUE);
	psvDebugScreenSetBgColor(DARKBLUE);

	while (1) {
		psvDebugScreenSetXY(0, 1);

		// Info
		psvDebugScreenSetFgColor(CYAN);
		printf(" Vitamin by Team FreeK\n");
		psvDebugScreenSetFgColor(WHITE);
		printf(" ---------------------------------------------------------- \n\n\n");

		// Title
		psvDebugScreenSetFgColor(GREEN);
		printf("  %s\n\n", title);

		// Entries
		int page = sel / ENTRIES_PER_PAGE;
		int last_page_number = n_entries / ENTRIES_PER_PAGE;
		int n_entries_last_page = n_entries % ENTRIES_PER_PAGE;

		int count = (page == last_page_number) ? n_entries_last_page : ENTRIES_PER_PAGE;

		int i;
		for (i = (page * ENTRIES_PER_PAGE); i < ((page * ENTRIES_PER_PAGE) + count); i++) {
			psvDebugScreenSetFgColor((sel == i) ? WHITE : YELLOW);
			printf("  %c %s\n", (sel == i) ? '>' : ' ', entries[i].name);
		}

		psvDebugScreenSetXY(0, 24);
		psvDebugScreenSetFgColor(WHITE);
		printf(" ---------------------------------------------------------- \n\n");

		// Ctrl
		SceCtrlData pad;
		memset(&pad, 0, sizeof(SceCtrlData));
		sceCtrlPeekBufferPositive(0, &pad, 1);

		pressed_buttons = pad.buttons & ~current_buttons;
		current_buttons = pad.buttons;

		if (pressed_buttons & SCE_CTRL_UP) {
			if (sel > 0) {
				sel--;

				// Clear screen at new page
				if (((sel + 1) / ENTRIES_PER_PAGE) != (sel / ENTRIES_PER_PAGE)) {
					psvDebugScreenClear(BLUE);
				}
			}
		} else if (pressed_buttons & SCE_CTRL_DOWN) {
			if (sel < (n_entries - 1)) {
				sel++;

				// Clear screen at new page
				if (((sel - 1) / ENTRIES_PER_PAGE) != (sel / ENTRIES_PER_PAGE)) {
					psvDebugScreenClear(BLUE);
				}
			}
		} else if (pressed_buttons & SCE_CTRL_CROSS) {
			return sel;
		}

		sceDisplayWaitVblankStart();
	}

	return -1;
}

#define MAX_PATH_LENGTH 1024

typedef struct SfoHeader {
	uint32_t magic;
	uint32_t version;
	uint32_t keyofs;
	uint32_t valofs;
	uint32_t count;
} __attribute__((packed)) SfoHeader;

typedef struct SfoEntry {
	uint16_t nameofs;
	uint8_t  alignment;
	uint8_t  type;
	uint32_t valsize;
	uint32_t totalsize;
	uint32_t dataofs;
} __attribute__((packed)) SfoEntry;

int getSfoString(void *buffer, char *name, char *string, int length) {
	SfoHeader *header = (SfoHeader *)buffer;
	SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
			memset(string, 0, length);
			strncpy(string, buffer + header->valofs + entries[i].dataofs, length);
			break;
		}
	}

	return 0;
}

int main(int argc, char *argv[]) {
	// Init screen
	psvDebugScreenInit();

	MenuEntry *game_entries = malloc(128 * sizeof(MenuEntry));
	int n_games = 0;

	// Open app folder
	SceUID dfd = sceIoDopen("ux0:app");
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				char path[MAX_PATH_LENGTH];
				snprintf(path, MAX_PATH_LENGTH, "ux0:app/%s/sce_sys/clearsign", dir.d_name);

				SceIoStat stat;
				memset(&stat, 0, sizeof(SceIoStat));
				if (sceIoGetstat(path, &stat) >= 0) {
					snprintf(path, MAX_PATH_LENGTH, "ux0:app/%s/sce_sys/param.sfo", dir.d_name);

					// Open param.sfo
					SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
					if (fd < 0)
						continue;

					// Get param.sfo size
					uint64_t off = sceIoLseek(fd, 0, SCE_SEEK_CUR);
					uint64_t size = sceIoLseek(fd, 0, SCE_SEEK_END);
					sceIoLseek(fd, off, SCE_SEEK_SET);

					// Allocate buffer
					void *buffer = malloc(size);
					if (!buffer) {
						sceIoClose(fd);
						continue;
					}

					// Read param.sfo
					sceIoRead(fd, buffer, size);
					sceIoClose(fd);

					// Get title
					char *title = malloc(128);
					getSfoString(buffer, "TITLE", title, 128);

					// Get title id
					char *titleid = malloc(12);
					getSfoString(buffer, "TITLE_ID", titleid, 12);

					// Name
					char *name = malloc(256);
					sprintf(name, "[%s] %s", titleid, title);
					game_entries[n_games].name = name;

					// Free buffer
					free(buffer);

					n_games++;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	}

	// Menu
	doMenu("Choose game:", game_entries, n_games);

	sceKernelExitProcess(0);
	return 0;
}