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
#include <psp2/display.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#include "../common/utils.h"
#include "../common/graphics.h"
#include "../common/common.h"

#include "lsd.h"
#include "steroid.h"

static char titleid[12];

int debugPrintf(char *text, ...) {
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

	SceUID fd = sceIoOpen("ux0:morphine_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, string, strlen(string));
		sceIoClose(fd);
	}

	return 0;
}

int isCartridge() {
	char path[128];
	sprintf(path, "gro0:app/%s/sce_sys/param.sfo", titleid);

	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;

	sceIoClose(fd);
	return 1;
}

void writeSteroid(char *path) {
	// Write steroid module
	printf("Writing steroid.suprx...");
	sprintf(path, "%s/sce_module", path);
	sceIoMkdir(path, 0777);
	sprintf(path, "%s/sce_module/steroid.suprx", path);
	WriteFile(path, steroid, size_steroid);
	printf("OK\n");
}

int main(int argc, char *argv[]) {
	char path[128], dst_path[128], app_path[128], savedata_path[128], tmp_path[128];

	// Get title id
	memset(titleid, 0, sizeof(titleid));
	sceAppMgrAppParamGetString(sceKernelGetProcessId(), SCE_APPMGR_APP_PARAM_TITLE_ID, titleid, sizeof(titleid));

	// Read mode
	int mode = 0;
	ReadFile("app0:mode.bin", &mode, sizeof(int));

	// Read game info
	GameInfo game_info;
	ReadFile("app0:info.bin", &game_info, sizeof(GameInfo));

	// Init screen
	psvDebugScreenInit();
	psvDebugScreenClear(DARKBLUE);
	psvDebugScreenSetBgColor(DARKBLUE);

	// Layout
	char *version = mode == MODE_UPDATE ? game_info.version_update : game_info.version_game;
	char game_info_string[512];
	sprintf(game_info_string, "Name   : %s\n"
							  "Game ID: %s\n"
							  "Version: %s\n",
							  game_info.name,
							  game_info.titleid,
							  (version[0] == '0') ? (version + 1) : version);
	printLayout(game_info_string, mode == MODE_UPDATE ? "Dumping update files" : "Dumping full game");

	// Dump process
	if (isCartridge()) {
		sprintf(app_path, "gro0:app/%s", titleid);
	} else {
		sprintf(app_path, "ux0:app/%s", titleid);
	}

	if (mode == MODE_UPDATE) {
		// Dump decrypted files
		sprintf(dst_path, "ux0:Vitamin/%s_UPDATE_%s", titleid, game_info.version_update);
		copyPath(app_path, dst_path);

		// Write steroid module
		writeSteroid(dst_path);

		// Destory all other apps (close manual to unlock files)
		sceAppMgrDestroyOtherApp();
		sceKernelDelayThread(1 * 1000 * 1000);

		// Copy decrypted eboot.bin to ux0:patch for next stage
		sprintf(path, "ux0:Vitamin/%s_UPDATE_%s/eboot.bin", titleid, game_info.version_update);
		sprintf(dst_path, "ux0:patch/%s/eboot.bin", titleid);
		copyPath(path, dst_path);

		// Backup patch in app folder
		sprintf(tmp_path, "ux0:app/%s_patch", titleid);
		sceIoRename(app_path, tmp_path);

		// Restore original app folder
		sprintf(tmp_path, "ux0:app/%s_org", titleid);
		sceIoRename(tmp_path, app_path);
	} else {
		// Dump decrypted files
		sprintf(dst_path, "ux0:Vitamin/%s_FULLGAME_%s", titleid, game_info.version_game);
		copyPath(app_path, dst_path);

		// Write steroid module
		writeSteroid(dst_path);

		// Destory all other apps (close manual to unlock files)
		sceAppMgrDestroyOtherApp();
		sceKernelDelayThread(1 * 1000 * 1000);

		// Delete *this* eboot.bin
		sprintf(path, "ux0:patch/%s/eboot.bin", titleid);
		sceIoRemove(path);
	}

	// Restore original savedata
	sprintf(tmp_path, "ux0:user/00/savedata/%s_org", titleid);
	sprintf(savedata_path, "ux0:user/00/savedata/%s", titleid);
	removePath(savedata_path);
	sceIoRename(tmp_path, savedata_path);

	// Write lsd module
	sprintf(path, "ux0:patch/%s/sce_module/libc.suprx", titleid);
	WriteFile(path, lsd, size_lsd);

	// Write relaunch titleid
	WriteFile("ux0:pspemu/Vitamin/relaunch.bin", titleid, strlen(titleid) + 1);

	// Launch Vitamin app to relaunch game
	sceKernelDelayThread(1 * 1000 * 1000);
	sceAppMgrLaunchAppByUri(0xFFFFF, "psgm:play?titleid=VITAMIN00");
	sceKernelExitProcess(0);

	return 0;
}
