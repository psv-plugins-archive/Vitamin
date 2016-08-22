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

#include <sqlite3.h>

#include "main.h"
#include "../common/utils.h"
#include "../common/graphics.h"
#include "../common/common.h"

#include "../minizip/makezip.h"

#include "eboot_bin.h"
#include "param_sfo.h"

#define APP_DB "ur0:shell/db/app.db"

#define DELAY 700 * 1000

// TODO: disable autosuspend
// TODO: add check if manual is open
// TODO: check ms space
// TODO: param.sfo fix
// TODO: additional selfs support
// TODO: progressbar
// TODO: more error handling

#define MAX_GAMES 128

#define ENTRIES_PER_PAGE 15

int debugPrintf(char *text, ...) {
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

	SceUID fd = sceIoOpen("ux0:injection_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, string, strlen(string));
		sceIoClose(fd);
	}

	return 0;
}

void *allocateReadFile(char *path) {
	// Open file
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return NULL;

	// Get file size
	uint64_t off = sceIoLseek(fd, 0, SCE_SEEK_CUR);
	uint64_t size = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoLseek(fd, off, SCE_SEEK_SET);

	// Allocate buffer
	void *buffer = malloc(size);
	if (!buffer) {
		sceIoClose(fd);
		return NULL;
	}

	// Read file
	sceIoRead(fd, buffer, size);
	sceIoClose(fd);

	return buffer;
}

int addGames(char *app_path, int is_cartridge, int count, GameInfo *game_infos, char **game_entries) {
	SceUID dfd = sceIoDopen(app_path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				char path[128];
				sprintf(path, "%s/%s/sce_sys/clearsign", app_path, dir.d_name);

				SceIoStat stat;
				memset(&stat, 0, sizeof(SceIoStat));
				if (sceIoGetstat(path, &stat) >= 0) {
					void *buffer = NULL;

					// Read app param.sfo
					sprintf(path, "%s/%s/sce_sys/param.sfo", app_path, dir.d_name);
					buffer = allocateReadFile(path);
					if (!buffer)
						continue;

					// Clear
					memset(&game_infos[count], 0, sizeof(GameInfo));

					// Is cartridge
					game_infos[count].is_cartridge = is_cartridge;

					// Get title
					getSfoString(buffer, "TITLE", game_infos[count].name, 128);

					// Get title id
					getSfoString(buffer, "TITLE_ID", game_infos[count].titleid, 12);

					// Get version
					getSfoString(buffer, "APP_VER", game_infos[count].version_game, 8);

					// Copy game version
					strcpy(game_infos[count].version, game_infos[count].version_game);

					// Free buffer
					free(buffer);

					// Read patch param.sfo
					if (!is_cartridge) {
						sprintf(path, "ux0:patch/%s/sce_sys/param.sfo", dir.d_name);
						buffer = allocateReadFile(path);
						if (buffer) {
							// Get version
							getSfoString(buffer, "APP_VER", game_infos[count].version_update, 8);

							// Copy üatch version
							strcpy(game_infos[count].version, game_infos[count].version_update);

							// Free buffer
							free(buffer);
						}
					}

					// Name
					char *name = malloc(256);
					sprintf(name, "[%s] %s", game_infos[count].titleid, game_infos[count].name);
					game_entries[count] = name;

					count++;
				}
			}
		} while (res > 0 && count < MAX_GAMES);

		sceIoDclose(dfd);
	}

	return count;
}

int getGames(GameInfo *game_infos, char **game_entries) {
	int count = 0;

	// Open cartridge app folder
	count = addGames("gro0:app", 1, count, game_infos, game_entries);

	// Open app folder
	count = addGames("ux0:app", 0, count, game_infos, game_entries);
/*
	int i;
	for (i = 0; i < 12; i++) {
		char *name = malloc(256);
		sprintf(name, "Test %d", i);
		game_entries[count] = name;
		count++;
	}
*/
	return count;
}

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

void addExecutables(char *zip_path, char *tmp_dir) {
	char tmp_path[128];

	SceUID dfd = sceIoDopen(tmp_dir);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *new_src_path = malloc(strlen(tmp_dir) + strlen(dir.d_name) + 2);
				snprintf(new_src_path, MAX_PATH_LENGTH, "%s/%s", tmp_dir, dir.d_name);

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					addExecutables(zip_path, new_src_path);
				} else {
					makeZip(zip_path, new_src_path, strlen("ux0:pspemu/Vitamin/"), 1, NULL);
				}

				free(new_src_path);
			}
		} while (res > 0);
		sceIoDclose(dfd);
	}
}

int finishDump(GameInfo *game_info, int mode) {
	int res;
	char path[128], patch_path[128], tmp_path[128];

	// Add converted eboot.bin
	sprintf(path, "ux0:Vitamin/%s%s%s.VPK", game_info->titleid, mode == MODE_UPDATE ? "_UPDATE_" : "_FULLGAME_", mode == MODE_UPDATE ? game_info->version_update : game_info->version_game);
	addExecutables(path, "ux0:pspemu/Vitamin");

	// Remove Vitamin path in pspemu
	removePath("ux0:pspemu/Vitamin");
	removePath("ux0:pspemu/Vitamin_exec");

	// Do we keep this here ? Might be useful for the user in case he wants to decrypt savedata...
	// Uninstall app.db modification
	printf("Uninstalling app.db modification...");
	res = uninstallAppDbMod();
	if (res < 0)
		goto ERROR;
	printf("OK\n");

	sceKernelDelayThread(DELAY);
	printf("Finishing...");

	// Destory all other apps
	sceAppMgrDestroyOtherApp();
	sceKernelDelayThread(1 * 1000 * 1000);

	// Patch path
	sprintf(patch_path, "ux0:patch/%s", game_info->titleid);

	// Temp path
	if (mode == MODE_UPDATE) {
		sprintf(tmp_path, "ux0:app/%s_patch", game_info->titleid);
	} else {
		sprintf(tmp_path, "ux0:patch/%s_org", game_info->titleid);
	}

	// Eject morphine
	removePath(patch_path);

	// Rename
	res = sceIoRename(tmp_path, patch_path);
	if (res < 0)
		goto ERROR;

	sceKernelDelayThread(DELAY);
	printf("OK\n");

	sceKernelDelayThread(DELAY);
	printf("\nThe game has been dumped successfully.\nPress X to exit to main menu.");
	waitForUser();

	return 0;

ERROR:
	sceKernelDelayThread(DELAY);
	printf("Error 0x%08X\n", res);
	sceKernelDelayThread(10 * 1000 * 1000);

	return res;
}

int injectMorphine(GameInfo *game_info, int mode) {
	int res;
	char path[128];

	// Make Vitamin dir
	res = sceIoMkdir("ux0:Vitamin", 0777);
	if (res < 0 && res != SCE_ERROR_ERRNO_EEXIST)
		return res;

	// Make pspemu dir
	res = sceIoMkdir("ux0:pspemu", 0777);
	if (res < 0 && res != SCE_ERROR_ERRNO_EEXIST)
		return res;

	// Make pspemu/Vitamin dir
	res = sceIoMkdir("ux0:pspemu/Vitamin", 0777);
	if (res < 0 && res != SCE_ERROR_ERRNO_EEXIST)
		return res;

	// Make pspemu/Vitamin_exec dir
	res = sceIoMkdir("ux0:pspemu/Vitamin_exec", 0777);
	if (res < 0 && res != SCE_ERROR_ERRNO_EEXIST)
		return res;


	// Make patch dir
	sprintf(path, "ux0:patch/%s", game_info->titleid);
	res = sceIoMkdir(path, 0777);
	if (res < 0)
		return res;

	// Make sce_sys dir
	sprintf(path, "ux0:patch/%s/sce_sys", game_info->titleid);
	res = sceIoMkdir(path, 0777);
	if (res < 0)
		return res;

	// Make sce_module dir
	sprintf(path, "ux0:patch/%s/sce_module", game_info->titleid);
	res = sceIoMkdir(path, 0777);
	if (res < 0)
		return res;

	// Write eboot.bin
	sprintf(path, "ux0:patch/%s/eboot.bin", game_info->titleid);
	res = WriteFile(path, eboot_bin, size_eboot_bin);
	if (res < 0)
		return res;

	// Write param.sfo
	sprintf(path, "ux0:patch/%s/sce_sys/param.sfo", game_info->titleid);
	res = WriteFile(path, param_sfo, size_param_sfo);
	if (res < 0)
		return res;

	// Write dump mode
	sprintf(path, "ux0:patch/%s/mode.bin", game_info->titleid);
	res = WriteFile(path, &mode, sizeof(int));
	if (res < 0)
		return res;

	// Write game info
	sprintf(path, "ux0:patch/%s/info.bin", game_info->titleid);
	res = WriteFile(path, game_info, sizeof(GameInfo));
	if (res < 0)
		return res;

	return 0;
}

void openManualLaunchGame(GameInfo *game_info) {
	// Open manual to decrypt files (locks files at the same time)
	sceKernelDelayThread(DELAY);
	printf("\nThe application will now open the manual of your game\nin another application.\nMinimize the new application and then go back to this\ngame.\n\n");
	psvDebugScreenSetFgColor(RED);
	printf("DO NOT CLOSE THE NEWLY OPENED APPLICATION.\n\n");
	sceKernelDelayThread(3 * 1000 * 1000);

	psvDebugScreenSetFgColor(WHITE);
	printf("Press X to continue.\n\n");
	waitForUser();

	char uri[32];

	if (game_info->is_cartridge) {
		sprintf(uri, "gro0:app/%s", game_info->titleid);
	} else {
		sprintf(uri, "ux0:app/%s", game_info->titleid);
	}

	// Open the manual
	sceKernelDelayThread(1 * 1000 * 1000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);
	sceKernelDelayThread(1 * 1000 * 1000);

	printf("Initializing dumping process...\n");
	sceKernelDelayThread(DELAY);

	// Launch game
	sprintf(uri, "psgm:play?titleid=%s", game_info->titleid);
	sceKernelDelayThread(1 * 1000 * 1000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);
	sceKernelExitProcess(0);
}

int sql_simple_exec(sqlite3 *db, const char *sql) {
	char *error = NULL;
	sqlite3_exec(db, sql, NULL, NULL, &error);
	if (error) {
		printf("Failed to execute %s: %s\n", sql, error);
		sqlite3_free(error);
		return -1;
	}
	return 0;
}

int sql_multiple_exec(char *db_path, char *queries[]) {
	int ret, i = 0;

	sqlite3 *db;
	ret = sqlite3_open(db_path, &db);
	if (ret) {
		printf("Failed to open the database: %s\n", sqlite3_errmsg(db));
		return ret;
	}

	do {
		ret = sql_simple_exec(db, queries[i]);
		if (ret < 0) {
			sqlite3_close(db);
			return ret;
		}
	} while (queries[++i] != NULL);

	sqlite3_close(db);
	db = NULL;

	return 0;
}

int installAppDbMod() {
	char *queries[] = { "INSERT INTO `tbl_uri` VALUES ('NPXS10001',1,'ux0',NULL)",
											"INSERT INTO `tbl_uri` VALUES ('NPXS10001',1,'gro0',NULL)",
											"UPDATE tbl_appinfo SET val='vs0:app/NPXS10027/eboot.bin' WHERE key='3022202214' and titleid='NPXS10001'",
											NULL };

	return sql_multiple_exec(APP_DB, queries);
}

int uninstallAppDbMod() {
	char *queries[] = { "DELETE FROM `tbl_uri` WHERE scheme='ux0'",
											"DELETE FROM `tbl_uri` WHERE scheme='gro0'",
											"UPDATE tbl_appinfo SET val='vs0:app/NPXS10001/eboot.bin' WHERE key='3022202214' and titleid='NPXS10001'",
											NULL };

	return sql_multiple_exec(APP_DB, queries);
}

int getNextSelf(char *self_path, char *src_path) {
	SceUID dfd = sceIoDopen(src_path);
	if (dfd >= 0) {
		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *new_src_path = malloc(strlen(src_path) + strlen(dir.d_name) + 2);
				snprintf(new_src_path, MAX_PATH_LENGTH, "%s/%s", src_path, dir.d_name);

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					ret = getNextSelf(self_path, new_src_path);
				} else {
					self_path = new_src_path;
					return 1;
				}

				free(new_src_path);
			}
		} while (res > 0);
		sceIoDclose(dfd);
	}
	return 0;
}

int setupSelfDump(GameInfo *game_info, int mode) {
	char self_path[128], *src_path = NULL;

	char *query = malloc(0x100);
	char *queries = { query, NULL };

	// Get next executable path in ux0:pspemu/Vitamin
	getNextSelf(src_path, "ux0:pspemu/Vitamin_exec");

	if (mode == MODE_UPDATE) {
		sprintf(self_path, "ux0:patch/%s/%s", game_info->titleid, (char *)(src_path + strlen("ux0:pspemu/Vitamin_exec/")));
		// Copy executable to ux0:patch
		copyPath(src_path, self_path);
	} else { // mode == MODE_FULL_GAME
		sprintf(self_path, "%s:app/%s/%s", game_info->is_cartridge ? "gro0" : "ux0", game_info->titleid, (char *)(src_path + strlen("ux0:pspemu/Vitamin_exec/")));
	}

	// Delete executable from temp directory
	sceIoRemove(src_path);

	// Create and execute SQL query in app.db
	sprintf(query, "UPDATE tbl_appinfo SET val='%s' WHERE key='3022202214' and titleid='%s'", self_path, game_info->titleid);
	return sql_multiple_exec(APP_DB, queries);
}

int dumpFullGame(GameInfo *game_info) {
	int res;
	char patch_path[128], tmp_path[128];

	sceKernelDelayThread(DELAY);
	printf("Injecting morphine...");

	// Destory all other apps
	sceAppMgrDestroyOtherApp();
	sceKernelDelayThread(1 * 1000 * 1000);

	// Backup savedata and let the application create a new savegame with its new encryption key
	res = sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_org");
	if (res < 0)// && res != 0x80010002)
		goto ERROR;

	// Patch path and temp path
	sprintf(patch_path, "ux0:patch/%s", game_info->titleid);
	sprintf(tmp_path, "ux0:patch/%s_org", game_info->titleid);

	// Backup original patch
	res = sceIoRename(patch_path, tmp_path);
	if (res < 0 && res != 0x80010002)
		goto ERROR;

	// Inject morphine
	res = injectMorphine(game_info, MODE_FULL_GAME);
	if (res < 0)
		goto ERROR;

	sceKernelDelayThread(DELAY);
	printf("OK\n");

	// Install app.db modification
	printf("Installing app.db modification...");
	res = installAppDbMod();
	if (res < 0)
		goto ERROR;
	printf("OK\n");

	// Open manual and launch game
	openManualLaunchGame(game_info);

	return 0;

ERROR:
	sceKernelDelayThread(DELAY);
	printf("Error 0x%08X\n", res);
	sceKernelDelayThread(10 * 1000 * 1000);

	return res;
}

int dumpUpdate(GameInfo *game_info) {
	int res;
	char app_path[128], patch_path[128], tmp_path[128];

	sceKernelDelayThread(DELAY);
	printf("Injecting morphine...");

	// Destory all other apps
	sceAppMgrDestroyOtherApp();
	sceKernelDelayThread(1 * 1000 * 1000);

	// Backup savedata and let the application create a new savegame with its new encryption key
	res = sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_org");
	if (res < 0)// && res != 0x80010002)
		goto ERROR;

	// App path and temp path
	sprintf(app_path, "ux0:app/%s", game_info->titleid);
	sprintf(tmp_path, "ux0:app/%s_org", game_info->titleid);

	// Backup original app
	res = sceIoRename(app_path, tmp_path);
	if (res < 0)
		goto ERROR;

	// Move patch files to ux0:app
	sprintf(patch_path, "ux0:patch/%s", game_info->titleid);
	res = sceIoRename(patch_path, app_path);
	if (res < 0)
		goto ERROR;

	// Inject morphine
	res = injectMorphine(game_info, MODE_UPDATE);
	if (res < 0)
		goto ERROR;

	sceKernelDelayThread(DELAY);
	printf("OK\n");

	// Install app.db modification
	printf("Installing app.db modification...");
	res = installAppDbMod();
	if (res < 0)
		goto ERROR;
	printf("OK\n");

	// Open manual and launch game
	openManualLaunchGame(game_info);

	return 0;

ERROR:
	sceKernelDelayThread(DELAY);
	printf("Error 0x%08X\n", res);
	sceKernelDelayThread(10 * 1000 * 1000);

	return res;
}

int main(int argc, char *argv[]) {
	char n_games_string[32];
	char game_info_string[512];

	// Init screen
	psvDebugScreenInit();

	// Read mode
	sprintf(path, "ux0:patch/%s/mode.bin", titleid);

	int mode = 0;
	ReadFile(path, &mode, sizeof(int));

	// Read game info
	sprintf(path, "ux0:patch/%s/info.bin", titleid);

	GameInfo game_info;
	ReadFile(path, &game_info, sizeof(GameInfo));

	// Relaunch game
	char titleid[12];
	memset(titleid, 0, sizeof(titleid));
	if (ReadFile("ux0:pspemu/Vitamin/relaunch.bin", titleid, sizeof(titleid)) >= 0) {
		sceIoRemove("ux0:pspemu/Vitamin/relaunch.bin");

		setupSelfDump(game_info, mode);

		char uri[32];
		sprintf(uri, "psgm:play?titleid=%s", titleid);
		sceKernelDelayThread(1 * 1000 * 1000);
		sceAppMgrLaunchAppByUri(0xFFFFF, uri);
		sceKernelExitProcess(0);
	}

	// Finish dump
	memset(titleid, 0, sizeof(titleid));
	if (ReadFile("ux0:pspemu/Vitamin/finish.bin", titleid, sizeof(titleid)) >= 0) {
		sceIoRemove("ux0:pspemu/Vitamin/finish.bin");

		psvDebugScreenClear(DARKBLUE);
		psvDebugScreenSetBgColor(DARKBLUE);

		char path[128];

		// Layout
		char *version = mode == MODE_UPDATE ? game_info.version_update : game_info.version_game;
		sprintf(game_info_string, "Name   : %s\n"
								  "Game ID: %s\n"
								  "Version: %s\n",
								  game_info.name,
								  game_info.titleid,
								  (version[0] == '0') ? (version + 1) : version);
		printLayout(game_info_string, mode == MODE_UPDATE ? "Dumping update files" : "Dumping full game");

		// Finish dump
		finishDump(&game_info, mode);

		psvDebugScreenResetMargin();
	}

	// Get games
	GameInfo *game_infos = malloc(MAX_GAMES * sizeof(GameInfo));
	char **game_entries = malloc(MAX_GAMES * sizeof(char **));
	int n_games = getGames(game_infos, game_entries);

MAIN_MENU:
	// Choose game
	sprintf(n_games_string, "%d games found\n", n_games);
	int sel_game = doMenu(n_games_string, "Choose game:", -1, game_entries, n_games);

	// Choose option
	int has_update = game_infos[sel_game].version_update[0] != '\0';
	sprintf(game_info_string, "Name   : %s\n"
							  "Game ID: %s\n"
							  "Version: %s\n",
							  game_infos[sel_game].name,
							  game_infos[sel_game].titleid,
							  (game_infos[sel_game].version[0] == '0') ? (game_infos[sel_game].version + 1) : game_infos[sel_game].version); // Show update version

	static char *option_entries_with_update[] = { "Dump full game", "Dump update files", "", "Back" };
	static char *option_entries_without_update[] = { "Dump full game", "", "Back" };
	char **option_entries = has_update ? option_entries_with_update : option_entries_without_update;
	int option = doMenu(game_info_string, "Choose option:", has_update ? 3 : 2, option_entries, has_update ? 4 : 3);

	char *version = option == 0 ? game_infos[sel_game].version_game : game_infos[sel_game].version_update;
	sprintf(game_info_string, "Name   : %s\n"
							  "Game ID: %s\n"
							  "Version: %s\n",
							  game_infos[sel_game].name,
							  game_infos[sel_game].titleid,
							  (version[0] == '0') ? (version + 1) : version);

	switch (option) {
		case 0:
			psvDebugScreenClear(DARKBLUE);
			psvDebugScreenSetBgColor(DARKBLUE);
			printLayout(game_info_string, "Dumping full game");
			dumpFullGame(&game_infos[sel_game]);
			sceKernelDelayThread(DELAY);
			break;
		case 1:
			psvDebugScreenClear(DARKBLUE);
			psvDebugScreenSetBgColor(DARKBLUE);
			printLayout(game_info_string, "Dumping update files");
			dumpUpdate(&game_infos[sel_game]);
			sceKernelDelayThread(DELAY);
			break;
	}

	// Back to main menu
	psvDebugScreenResetMargin();
	psvDebugScreenSetLeftMargin(2);
	psvDebugScreenSetXY(0, 29);
	psvDebugScreenSetFgColor(CYAN);
	printf("> Back...\n");
	sceKernelDelayThread(DELAY);
	goto MAIN_MENU;

	return 0;
}
