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
#include "menu.h"
#include "game.h"

#include "../common/utils.h"
#include "../common/graphics.h"
#include "../common/common.h"

#include "../minizip/makezip.h"

#include "steroid.h"
#include "eboot_bin.h"
#include "param_sfo.h"

#define APP_DB "ur0:shell/db/app.db"

#define DELAY 700 * 1000

// TODO: add check if manual is open
// TODO: check ms space
// TODO: more error handling

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

void restoreSavedata() {
	removePath("ux0:user/00/savedata_old");
	if (sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_old") >= 0) {
		if (sceIoRename("ux0:user/00/savedata_org", "ux0:user/00/savedata") < 0) {
			sceIoRename("ux0:user/00/savedata_old", "ux0:user/00/savedata");
		}
	}
}

int injectMorphine(GameInfo *game_info, int mode) {
	int res;
	char path[128];

	// Remove temp pspemu paths
	removePath("ux0:pspemu/Vitamin");
	removePath("ux0:pspemu/Vitamin_exec");

	// Make Vitamin dir
	res = sceIoMkdir("ux0:Vitamin", 0777);
	if (res < 0 && res != SCE_ERROR_ERRNO_EEXIST)
		return res;

	// Make patch dir
	res = sceIoMkdir("ux0:patch", 0777);
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
	printf("DO NOT CLICK OK OR CLOSE THE NEWLY OPENED APPLICATION.\n\n");
	sceKernelDelayThread(3 * 1000 * 1000);

	psvDebugScreenSetFgColor(WHITE);
	printf("Press X to continue.\n\n");
	waitForUser();

	char uri[32];
	sprintf(uri, "%s:app/%s", game_info->is_cartridge ? "gro0" : "ux0", game_info->titleid);

	// Open the manual
	sceKernelDelayThread(1 * 1000 * 1000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);
	sceKernelDelayThread(1 * 1000 * 1000);

	printf("Initializing dumping process...\n");
	sceKernelDelayThread(DELAY);

	// Launch game
	launchAppByUriExit(game_info->titleid);
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

int uninstallAppDbMod(GameInfo *game_info) {
	char *queries[] = { "DELETE FROM `tbl_uri` WHERE scheme='ux0'",
						"DELETE FROM `tbl_uri` WHERE scheme='gro0'",
						"UPDATE tbl_appinfo SET val='vs0:app/NPXS10001/eboot.bin' WHERE key='3022202214' and titleid='NPXS10001'",
						NULL };

	return sql_multiple_exec(APP_DB, queries);
}

int uninstallSelfPathMod(GameInfo *game_info) {
	char restore_query[0x100];
	char *queries[] = { restore_query,
						NULL };

	sprintf(restore_query, "UPDATE tbl_appinfo SET val='%s:app/%s/eboot.bin' WHERE key='3022202214' and titleid='%s'", game_info->is_cartridge ? "gro0" : "ux0", game_info->titleid, game_info->titleid);

	return sql_multiple_exec(APP_DB, queries);
}

static uint64_t ignored_size = 0;

int ignoreHandler(char *path) {
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (sceIoGetstat(path, &stat) < 0)
		return 0;

	char *ext = strrchr(path, '.');
	if (ext && strcmp(ext, ".self") == 0) {
		ignored_size += stat.st_size;
		return 1;
	}

	if (strstr(path, "eboot.bin") || strstr(path, "param.sfo") || strstr(path, "sce_module") || strstr(path, "clearsign") || strstr(path, "keystone")) {
		ignored_size += stat.st_size;
		return 1;
	}

	return 0;
}

int getNextSelf(char *self_path, char *src_path) {
	int ret = 0;

	SceUID dfd = sceIoDopen(src_path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				snprintf(self_path, MAX_PATH_LENGTH, "%s/%s", src_path, dir.d_name);
				ret = 1;
				break;
			}
		} while (res > 0);

		sceIoDclose(dfd);
	}

	return ret;
}

int setupSelfDump(GameInfo *game_info, int mode) {
	char self_path[128], patch_path[128], src_path[MAX_PATH_LENGTH];

	char self_mod_query[0x100];
	char *queries[] = { self_mod_query,
						NULL };

	// Get next executable path in ux0:pspemu/Vitamin
	getNextSelf(src_path, "ux0:pspemu/Vitamin_exec");
	debugPrintf("Get next self: %s\n", src_path);

	// Self path
	sprintf(self_path, "%s:app/%s/%s", game_info->is_cartridge ? "gro0" : "ux0", game_info->titleid, (char *)(src_path + strlen("ux0:pspemu/Vitamin_exec/")));

	debugPrintf("self_path: %s\n", self_path);

	// Copy to ux0:patch/TITLEID/executable when dumping update files
	if (mode == MODE_UPDATE) {
		// Patch path
		sprintf(patch_path, "ux0:patch/%s/%s", game_info->titleid, (char *)(src_path + strlen("ux0:pspemu/Vitamin_exec/")));

		debugPrintf("patch_path: %s\n", patch_path);

		// Copy executable to patch folder
		copyFile(src_path, patch_path);
	}

	// Delete executable from temp directory
	sceIoRemove(src_path);

	// Create and execute SQL queries in app.db
	sprintf(self_mod_query, "UPDATE tbl_appinfo SET val='%s' WHERE key='3022202214' and titleid='%s'", self_path, game_info->titleid);
	return sql_multiple_exec(APP_DB, queries);
}

int finishDump(GameInfo *game_info, int mode) {
	int res;
	char path[128], patch_path[128], tmp_path[128];

	// Add converted exectuables and extras to zip
	sceIoMkdir("ux0:pspemu/Vitamin/sce_module", 0777);
	WriteFile("ux0:pspemu/Vitamin/sce_module/steroid.suprx", steroid, size_steroid);

	sprintf(path, "ux0:Vitamin/%s%s%s%s", game_info->titleid, mode == MODE_UPDATE ? "_UPDATE_" : "_FULLGAME_",
	                                    mode == MODE_UPDATE ? game_info->version_update : game_info->version_game,
										mode == MODE_UPDATE ? ".ZIP" : ".VPK");
	makeZip(path, "ux0:pspemu/Vitamin", strlen("ux0:pspemu/Vitamin") + 1, 1, NULL, 0, NULL, NULL);

	// Remove Vitamin path in pspemu
	removePath("ux0:pspemu/Vitamin");
	removePath("ux0:pspemu/Vitamin_exec");

	// Do we keep this here ? Might be useful for the user in case he wants to decrypt savedata...
	// Uninstall app.db modification
	printf("Uninstalling app.db modification...");
	res = uninstallAppDbMod(game_info);
	if (res < 0)
		goto ERROR;

	// Uninstall selfpath mod
	res = uninstallSelfPathMod(game_info);
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
	if (res < 0 && res != 0x80010002)
		goto ERROR;

	sceKernelDelayThread(DELAY);
	printf("OK\n");

	sceKernelDelayThread(DELAY);
	printf("\nThe game has been dumped successfully.\nPress X to exit.");
	waitForUser();
	sceKernelExitProcess(0);

	return 0;

ERROR:
	sceKernelDelayThread(DELAY);
	printf("Error 0x%08X\n", res);

	sceKernelDelayThread(DELAY);
	printf("\nPress X to exit.");
	waitForUser();
	sceKernelExitProcess(0);

	return res;
}

void checkMsSpace(GameInfo *game_info) {
	uint64_t free_size = 0, max_size = 0;
	sceAppMgrGetDevInfo("ux0:", &max_size, &free_size);
	uint64_t max_dump_size = game_info->size + game_info->ignored_size;
	if (free_size < max_dump_size) {
		sceKernelDelayThread(DELAY);
		char size_left[16];
		char size_required[16];
		getSizeString(size_left, free_size);
		getSizeString(size_required, max_dump_size - free_size);

		printf("You only have %s left on your MS.\nIn order to dump, %s more is required.\n", size_left, size_required);

		sceKernelDelayThread(DELAY);
		printf("\nPress X to exit.");
		waitForUser();
		sceKernelExitProcess(0);
	}
}

int dumpFullGame(GameInfo *game_info) {
	int res;
	char app_path[128], patch_path[128], tmp_path[128];

	// App path, patch path and temp path
	sprintf(app_path, "%s:app/%s", game_info->is_cartridge ? "gro0" : "ux0", game_info->titleid);
	sprintf(patch_path, "ux0:patch/%s", game_info->titleid);
	sprintf(tmp_path, "ux0:patch/%s_org", game_info->titleid);

	// Destory all other apps
	sceAppMgrDestroyOtherApp();
	sceKernelDelayThread(1 * 1000 * 1000);

	// Get game size
	ignored_size = 0;
	getPathInfo(app_path, &game_info->size, NULL, NULL, ignoreHandler);
	game_info->ignored_size = ignored_size;

	// Check free ms space
	checkMsSpace(game_info);

	sceKernelDelayThread(DELAY);
	printf("Installing app.db modification...");

	// Uninstall selfpath mod
	res = uninstallSelfPathMod(game_info);
	if (res < 0)
		goto ERROR;

	// Install app.db modification
	res = installAppDbMod();
	if (res < 0)
		goto ERROR_INSTALL_APPDB_MOD;

	printf("OK\n");

	// Inject morphine
	sceKernelDelayThread(DELAY);
	printf("Injecting morphine...");

	// Backup savedata and let the application create a new savegame with its new encryption key
	res = sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_org");
	if (res < 0 && res != 0x80010002)
		goto ERROR_RENAME_SAVEDATA;

	// Backup original patch
	res = sceIoRename(patch_path, tmp_path);
	if (res < 0 && res != 0x80010002)
		goto ERROR_RENAME_PATCH;

	// Inject morphine
	res = injectMorphine(game_info, MODE_FULL_GAME);
	if (res < 0)
		goto ERROR_INJECT_MORPHINE;

	sceKernelDelayThread(DELAY);
	printf("OK\n");

	// Open manual and launch game
	openManualLaunchGame(game_info);

	return 0;

ERROR_INJECT_MORPHINE:
	removePath(patch_path);

ERROR_RENAME_PATCH:
	sceIoRename(tmp_path, patch_path);

ERROR_RENAME_SAVEDATA:
	restoreSavedata();

ERROR_INSTALL_APPDB_MOD:
ERROR:
	sceKernelDelayThread(DELAY);
	printf("Error 0x%08X\n", res);

	sceKernelDelayThread(DELAY);
	printf("\nPress X to exit.");
	waitForUser();
	sceKernelExitProcess(0);

	return res;
}

int dumpUpdate(GameInfo *game_info) {
	int res;
	char app_path[128], patch_path[128], tmp_path[128];

	// App path, patch path and temp path
	sprintf(app_path, "ux0:app/%s", game_info->titleid);
	sprintf(patch_path, "ux0:patch/%s", game_info->titleid);
	sprintf(tmp_path, "ux0:app/%s_org", game_info->titleid);

	// Destory all other apps
	sceAppMgrDestroyOtherApp();
	sceKernelDelayThread(1 * 1000 * 1000);

	// Get game size
	ignored_size = 0;
	getPathInfo(patch_path, &game_info->size, NULL, NULL, ignoreHandler);
	game_info->ignored_size = ignored_size;

	// Check free ms space
	checkMsSpace(game_info);

	sceKernelDelayThread(DELAY);
	printf("Installing app.db modification...");

	// Uninstall selfpath mod
	res = uninstallSelfPathMod(game_info);
	if (res < 0)
		goto ERROR;

	// Install app.db modification
	res = installAppDbMod();
	if (res < 0)
		goto ERROR_INSTALL_APPDB_MOD;

	printf("OK\n");

	// Inject morphine
	sceKernelDelayThread(DELAY);
	printf("Injecting morphine...");

	// Backup savedata and let the application create a new savegame with its new encryption key
	res = sceIoRename("ux0:user/00/savedata", "ux0:user/00/savedata_org");
	if (res < 0 && res != 0x80010002)
		goto ERROR_RENAME_SAVEDATA;

	// Backup original app
	res = sceIoRename(app_path, tmp_path);
	if (res < 0)
		goto ERROR_RENAME_APP;

	// Move patch files to ux0:app
	res = sceIoRename(patch_path, app_path);
	if (res < 0)
		goto ERROR_RENAME_PATCH;

	// Inject morphine
	res = injectMorphine(game_info, MODE_UPDATE);
	if (res < 0)
		goto ERROR_INJECT_MORPHINE;

	sceKernelDelayThread(DELAY);
	printf("OK\n");

	// Open manual and launch game
	openManualLaunchGame(game_info);

	return 0;

ERROR_INJECT_MORPHINE:
	removePath(patch_path);

ERROR_RENAME_PATCH:
	sceIoRename(app_path, patch_path);

ERROR_RENAME_APP:
	sceIoRename(tmp_path, app_path);

ERROR_RENAME_SAVEDATA:
	restoreSavedata();

ERROR_INSTALL_APPDB_MOD:
ERROR:
	sceKernelDelayThread(DELAY);
	printf("Error 0x%08X\n", res);

	sceKernelDelayThread(DELAY);
	printf("\nPress X to exit.");
	waitForUser();
	sceKernelExitProcess(0);

	return res;
}

int main(int argc, char *argv[]) {
	char n_games_string[32];
	char game_info_string[512];

	char path[128];

	// Init power tick thread
	initPowerTickThread();

	// Init screen
	psvDebugScreenInit();

	// Restore original savedata in case it crashed before savedata could be restored
	restoreSavedata();

	// Relaunch game
	char titleid[12];
	memset(titleid, 0, sizeof(titleid));
	if (ReadFile("ux0:pspemu/Vitamin/relaunch.bin", titleid, sizeof(titleid)) >= 0) {
		sceIoRemove("ux0:pspemu/Vitamin/relaunch.bin");

		// Read mode
		sprintf(path, "ux0:patch/%s/mode.bin", titleid);

		int mode = 0;
		ReadFile(path, &mode, sizeof(int));

		// Read game info
		sprintf(path, "ux0:patch/%s/info.bin", titleid);

		GameInfo game_info;
		ReadFile(path, &game_info, sizeof(GameInfo));

		setupSelfDump(&game_info, mode);

		// Flush app.db
		sceAppMgrDestroyOtherApp();

		// Launch game
		launchAppByUriExit(game_info.titleid);
	}

	// Finish dump
	memset(titleid, 0, sizeof(titleid));
	if (ReadFile("ux0:pspemu/Vitamin/finish.bin", titleid, sizeof(titleid)) >= 0) {
		sceIoRemove("ux0:pspemu/Vitamin/finish.bin");

		psvDebugScreenClear(DARKBLUE);
		psvDebugScreenSetBgColor(DARKBLUE);

		// Read mode
		sprintf(path, "ux0:patch/%s/mode.bin", titleid);

		int mode = 0;
		ReadFile(path, &mode, sizeof(int));

		// Read game info
		sprintf(path, "ux0:patch/%s/info.bin", titleid);

		GameInfo game_info;
		ReadFile(path, &game_info, sizeof(GameInfo));

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
	psvDebugScreenSetFgColor(PURPLE);
	printf("> Back...\n");
	sceKernelDelayThread(DELAY);
	goto MAIN_MENU;

	return 0;
}
